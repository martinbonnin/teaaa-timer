#include <stdint.h>
#include <msp430.h>

/* port 1*/
#define     BUTTON_RED            BIT0

#define     DISPLAY_POWER         BIT1

#define     DISPLAY_D5            BIT2
#define     DISPLAY_D7            BIT3
#define     DISPLAY_D6            BIT4
#define     DISPLAY_D4            BIT5

#define     AUDIO_OUT             BIT6
#define     P1_UNUSED0            BIT7

/* port 2 */
#define     DISPLAY_RS            BIT0
#define     DISPLAY_E             BIT1

#define     LED_RED               BIT2

#define     UNUSED0               BIT3
#define     UNUSED1               BIT4
#define     UNUSED2               BIT5
#define     BUTTON_BLUE           BIT6
#define     LED_BLUE              BIT7

#define     DISPLAY_D5_SHIFT      2
#define     DISPLAY_D7_SHIFT      3
#define     DISPLAY_D6_SHIFT      4
#define     DISPLAY_D4_SHIFT      5
#define     DISPLAY_D_MASK        0x3c

enum {
    STATE_SLEEP,
    STATE_COUNTER,
    STATE_ALARM,
};

#define NULL ((void*)0)
#define SIZEOF(array) (sizeof(array)/sizeof(array[0]))

#include "note.h"
#include "bwaaa.h"
#include "beep.h"


static void _display_send(uint8_t code, int write);
static void _update_display(void);
static inline uint8_t _to_p(uint8_t code);

#define DEBOUNCE_TICKS_8MHZ 0x100
#define DEBOUNCE_TICKS_1MHZ 0x020
#define BUTTON_REPEAT_TICKS 0x100

#define PRESS_DELAY 10

static uint8_t state;

static uint8_t button_pressed;
static uint8_t ignore_button;
static int ticks;
static int last_button_ticks;

static char display[8];
static int display_on = 1;

static int seconds;
static int seconds_count;

static int alarm_seconds;
static int alarm_seconds_count;

static int sound_index;
static int sound_max;
static uint8_t note_duration;
static volatile uint8_t *sound;


static inline int _get_increment(void)
{
    if (seconds < 60) {
        return 1;
    } else if (seconds < 10 * 60) {
        return 15;
    } else if (seconds < 30 * 60) {
        return 60;
    } else {
        return 300;
    }
}

static inline void _seconds_increase(void)
{
    int inc = _get_increment();
    seconds += inc;
    seconds = inc * (seconds / inc);
}

static inline void _seconds_decrease(void)
{
    int inc = _get_increment();
    seconds -= inc;
    seconds = inc * (seconds / inc);
}

static void _display_deinit(void)
{    
    if (!display_on)
        return;
    display_on = 0;
    
    P1OUT |= DISPLAY_POWER;
    
    /* we need to pull down all the IO else the LCD manages to harvest 
     * some power and stay on */
    P2OUT &= ~DISPLAY_RS;
    P2OUT &= ~DISPLAY_E;
    P1OUT &= ~DISPLAY_D_MASK;
}

static inline void _send_4(uint8_t nybble)
{
    uint8_t a;

    a = (P1OUT & ~DISPLAY_D_MASK);
    P1OUT = a | _to_p(nybble);

    P2OUT |= DISPLAY_E;
    __delay_cycles(8);
    P2OUT &= ~DISPLAY_E;
    __delay_cycles(8);
}

static void _display_init(void)
{
    int i;

    if (display_on)
        return;
        
    display_on = 1;
    _display_deinit();
    display_on = 1;

#if 1
    /* wait for the display */
    for (i = 0; i < 200; i++)
        __delay_cycles(1000);
#endif

    P2OUT &= ~DISPLAY_RS;
    P2OUT &= ~DISPLAY_E;
    P1OUT &= ~DISPLAY_POWER;

    /* wait for the display */
    for (i = 0; i < 200; i++)
        __delay_cycles(1000);
     
    P2OUT &= ~DISPLAY_RS;
    
    /*
     * the initialization sequence is a bit strange but I take it from the datasheet
     * I guess the important thing is that by default the LCD expects 8bits interface
     * so the first command has to be a bit different else it see a spurious 0x0 
     * Also it works @1MHz, not sure it does @8MHz. _display_send() should work @8MHz though
     */
    _send_4(0x3);
    __delay_cycles(10000);

    _send_4(0x3);
    __delay_cycles(200);

    _send_4(0x3);
    __delay_cycles(200);

    _send_4(0x3);
    __delay_cycles(200);

    /* 4 bits, single line */
    _send_4(0x2);
    __delay_cycles(200);
        
    /* display on, do not blink, no cursor */
    _display_send(0x0C, 0);
    
    /* increment, shift */
    _display_send(0x06, 0);
    
}

static inline void _1_mhz(void)
{
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;

    /* make sure to pull down the audio out else we drain the batteries completely */
    TA0CTL = 0;
    P1SEL &= ~AUDIO_OUT;
    P1OUT &= ~AUDIO_OUT;

    TA1CTL = TASSEL_2 | MC_1;

    /* 256... we could maybe put something bigger there to avoid waking the 
     * CPU too often. Of course, best would be to add a 32k crystal */
    TA1CCR0 = 256;    
    TA1CCR1 = 0;
}

static inline void _8_mhz(void)
{
    DCOCTL = CALDCO_8MHZ;
    BCSCTL1 = CALBC1_8MHZ;

    /* enable the audio out */
    TA0CTL = TASSEL_2 | MC_1;
    P1SEL |= AUDIO_OUT;

    TA1CTL = TASSEL_2 | MC_1;

    /* 8 bit resolution */
    TA1CCR0 = 256;
    TA1CCR1 = 0;

    TA0CCR0 = 256;
    TA0CCR1 = 0;
}

static void _display_seconds(uint16_t secs)
{
    if (secs < 60) {
        uint16_t sec1 = secs / 10;
        uint16_t sec2 = secs - 10 * sec1;
        
        display[0] = ' ';
        display[1] = ' ';
        display[2] = ' ';
        display[3] = ' ';
        display[4] = ' ';
        if (sec1) 
            display[5] = '0' + sec1;
        else
            display[5] = ' ';
        display[6] = '0' + sec2;
        display[7] = 's';
    } else if (secs < 60 * 60) {
        uint16_t min = secs / 60;
        uint16_t sec1;
        uint16_t sec2;
        uint16_t min1;
        uint16_t min2;

        secs = secs - min * 60;
        
        sec1 = secs / 10;
        sec2 = secs  - 10 *sec1;

        min1 = min / 10;
        min2 = min - 10 *min1;

        display[0] = ' ';
        display[1] = ' ';
        if (min1)
            display[2] = '0' + min1;
        else
            display[2] = ' ';
        display[3] = '0' + min2;
        display[4] = 'm';
        display[5] = '0' + sec1;
        display[6] = '0' + sec2;
        display[7] = 's';
    } else {
        uint16_t hour;
        uint16_t min;
        uint16_t min1;
        uint16_t min2;

        hour = secs / 3600;
        secs = secs - hour * 3600;
        
        min = secs / 60;
        
        min1 = min / 10;
        min2 = min - min1 * 10;

        display[0] = ' ';
        display[1] = ' ';
        display[2] = ' ';
        display[3] = '0' + hour;
        display[4] = 'h';
        display[5] = '0' + min1;
        display[6] = '0' + min2;
        display[7] = 'm';
    }
    
    _update_display();
}

static void _update_display(void)
{    
    int i;
    /* reset counter */
    _display_send(0x80, 0);

    for (i = 0; i < 8; i++) {
        _display_send(display[i], 1);
    }
}

static inline uint8_t _to_p(uint8_t code)
{
	return (code & 1) << DISPLAY_D4_SHIFT
		| ((code >> 1) & 1) << DISPLAY_D5_SHIFT
		| ((code >> 2) & 1) << DISPLAY_D6_SHIFT
		| ((code >> 3) & 1) << DISPLAY_D7_SHIFT;
}


static void _display_send(uint8_t code, int write)
{    
    uint8_t a;
    
    a = P2OUT;
    
    if (write)
        P2OUT = a | DISPLAY_RS;
    else
        P2OUT = a & ~DISPLAY_RS;
    
    _send_4(code >> 4);
    _send_4(code & 0xf);
    __delay_cycles(8);
}

static inline void _go_to_counter(void)
{
    _BIC_SR(GIE);

    _1_mhz();

    _display_init();

    state = STATE_COUNTER;
    seconds = 30;
    
    _BIS_SR(GIE);
}

static inline void _state_sleep(void)
{
    _go_to_counter();
}

static inline void _sound_bwaaa(void)
{
    sound_index = 0;

    sound = NULL;
    sound_max = sizeof(bwaaa);
    note_duration = 0;
    
    TA0CCR1 = 0;
    TA0CCR0 = 256;
}

static inline void _sound_beep()
{
    sound_index = 0;

    sound = &sound_beep[0];
    sound_max = sizeof(sound_beep);
    note_duration = 0;

    TA0CCR1 = 0;
    TA0CCR0 = 256;
}

static inline void _go_to_alarm(void)
{
    _BIC_SR(GIE);

    display[0] = 'A';
    display[1] = ' ';
    display[2] = 'T';
    display[3] = 'A';
    display[4] = 'A';
    display[5] = 'B';
    display[6] = 'L';
    display[7] = 'E';

    _update_display();

    _8_mhz();

    state = STATE_ALARM;

    //_sound_beep();
    _sound_bwaaa();
    
    alarm_seconds = 0;
    alarm_seconds_count = 0;

    _BIS_SR(GIE);
}

static inline void _state_counter(void)
{
    static uint8_t red_press;
    static uint8_t blue_press;
    
    if (button_pressed) {        
        if (!(P1IN & BUTTON_RED)) {
            if (red_press >= PRESS_DELAY)
                _seconds_increase();
            else
                red_press++;
        } else if (!(P2IN & BUTTON_BLUE)) {
            if (blue_press >= PRESS_DELAY)
                _seconds_decrease();
            else
                blue_press++;
        } else {
            button_pressed = 0;
            if (blue_press && blue_press < PRESS_DELAY) {
                /* short press */
                _seconds_decrease();
            } else if (red_press && red_press < PRESS_DELAY) {
                /* short press */
                _seconds_increase();
            } else {
                /* end of long press */
            }
            red_press = 0;
            blue_press = 0;
            seconds_count = 0;
        }
    } 
    
    if (seconds < 0)
        seconds = 0;
    else if (seconds >= 3 * 60 * 60)
        seconds = (3 * 60 * 60);

    if (seconds == 0) {
        _go_to_alarm();
    } else {
        _display_seconds(seconds);
    }    
    
    LPM1;
}

static inline void _go_to_sleep(void)
{
    _BIC_SR(GIE);

    _1_mhz();

    _display_deinit();

    /* stop the timer, is it udeful ? */
    TA1CTL = 0;

    state = STATE_SLEEP;
    _BIS_SR(GIE);
    
    LPM4;
}

static inline void _state_alarm(void)
{
    if (button_pressed) {
        button_pressed = 0;

        if (!(P1IN & BUTTON_RED)) {
            _go_to_counter();
        } else if (!(P2IN & BUTTON_BLUE)) {
            _go_to_sleep();
        }
        return;
    } else if (alarm_seconds > 5 * 60) {
        _go_to_sleep();
        return;
    }
         
    LPM1;
}


#if 0
static inline void test(char c) 
{    
    _BIC_SR(GIE);
    _display_init();
    display[0] = c;
    display[1] = c;
    display[2] = c;
    display[3] = c;
    _BIS_SR(GIE);

    _update_display();

    WRITE_SR(GIE);	//Enable global interrupts
}
#endif

int main(void) 
{        
    WDTCTL = WDTPW + WDTHOLD;	// Stop WDT
       
    /*
     * common timer config. It will be started later
     */
    /* enable interrupt, compare mode */
    TA0CCTL0 = CCIE;
    TA1CCTL0 = CCIE;

    /* reset/set */
    TA0CCTL1 = OUTMOD2 | OUTMOD1 | OUTMOD0;
    TA1CCTL1 = OUTMOD2 | OUTMOD1 | OUTMOD0;
        
    _1_mhz();

    /* output */
    P1DIR = DISPLAY_POWER | DISPLAY_D5 | DISPLAY_D7 | DISPLAY_D6 | DISPLAY_D4 | AUDIO_OUT | P1_UNUSED0;
    P2DIR = DISPLAY_RS | DISPLAY_E | LED_RED | UNUSED0 | UNUSED1 | UNUSED2 | LED_BLUE;

    /* select TA0.1 on P1.6, all the rest are IO */
    P1SEL2 = 0;
    P2SEL = 0;
    P2SEL2 = 0;
    
    P1OUT = 0;
    P2OUT = 0;    

    /* pull up for inputs */
    P1REN = BUTTON_RED;
    P2REN = BUTTON_BLUE;
    
    P1OUT |= BUTTON_RED;
    P2OUT |= BUTTON_BLUE;    
    
    P1IE = BUTTON_RED;
    P2IE = BUTTON_BLUE;
    
    P1IFG = 0;
    P2IFG = 0;
    
    /*
     * interrupt on falling edge
     * beware the logic is inverted due to pull ups
     */
    P1IES = BUTTON_RED;
    P2IES = BUTTON_BLUE;
                        
    _go_to_sleep();
    
    while(1) {

        switch (state) {
            case STATE_SLEEP:
                _state_sleep();
                break;
            case STATE_COUNTER:
                _state_counter();
                break;            
            case STATE_ALARM:
                _state_alarm();
                break;            
        }
    }
}


__attribute__((interrupt(TIMER0_A0_VECTOR))) void TIMERA0_ISR(void) 
{
    static int i;
    
    if (!(i & 0x3)) {
        if (sound == NULL) {
            TA0CCR1 = bwaaa[sound_index];
            
            sound_index++;
            if (sound_index >= bwaaa_max[note_duration] || sound_index >= sizeof(bwaaa)) {
                note_duration++;
                sound_index = 0;
                if (note_duration == SIZEOF(bwaaa_max))
                    _sound_beep();
            }
        }
    }

    i++;
}

#define NOTE_PERIOD 0x7ff

__attribute__((interrupt(TIMER1_A0_VECTOR))) void TIMERA1_ISR(void) 
{
    ticks++;
        
    switch (state) {
        case STATE_ALARM:
            if (ignore_button && ((ticks - last_button_ticks) > DEBOUNCE_TICKS_8MHZ)) {
                ignore_button = 0;
            }
            if (!(ticks & 0x3)) {
                alarm_seconds_count++;
            }
            if (sound != NULL) {
                if ((ticks & NOTE_PERIOD) == 0) {
                    if (note_duration > 0) {
                        note_duration--;
                    } 
                    if (!note_duration) {
                        /* stop note */
                        //TA0CCR1 = 0;
                    }
                } else if ((ticks & NOTE_PERIOD) == (NOTE_PERIOD >> 4)) {
                    if (note_duration == 0) {
                        note_duration = ((sound[sound_index] >> DURATION_SHIFT) + 1);
                        TA0CCR0 = freq[(sound[sound_index] & NOTE_MASK)] << 6;
                        TA0CCR1 = TA0CCR0 >> 1;
                        sound_index++;
                        if (sound_index == sound_max) {
                            _sound_bwaaa();
                        }
                    }
                }
            }
            if (alarm_seconds_count == 7812) {
                alarm_seconds++;
                alarm_seconds_count = 0;
                LPM1_EXIT;
            }
            break;
        case STATE_COUNTER:
            if (ignore_button && ((ticks - last_button_ticks) > DEBOUNCE_TICKS_1MHZ)) {
                ignore_button = 0;
            }

            seconds_count++;
            if (button_pressed) {
                if (!(ticks & (BUTTON_REPEAT_TICKS - 1))) {
                    /* button repeat, go back to main loop */
                    LPM1_EXIT;
                }
            } else if (seconds_count == 3906) {
                seconds_count = 0;
                if (seconds > 0)
                    seconds--;
                LPM1_EXIT;
            }
            break;
        default:
            break;
    }
}

/* debounce the button */
#define BUTTON_COMMON() \
do {\
    button_pressed = 1;\
    if (ignore_button) {\
        goto end;\
    } else { \
        last_button_ticks = ticks; \
        ignore_button = 1;\
    }\
} while(0)

__attribute__((interrupt(PORT1_VECTOR))) void PORT1_ISR(void) 
{    
    if (state == STATE_SLEEP) {
        LPM4_EXIT;
        goto end;
    }
    
    BUTTON_COMMON();

    LPM1_EXIT;

end:
    P1IFG = 0;
}

__attribute__((interrupt(PORT2_VECTOR))) void PORT2_ISR(void) 
{        
    BUTTON_COMMON();

    if (state != STATE_SLEEP)
        LPM1_EXIT;

end:
    P2IFG = 0;
}

