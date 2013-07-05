/*****************************************************************************/
/*   Voice Recorder Demo using IMA ADPCM compression                         */
/*   using MSP430FG4619 (MSP430FG4618/F2013 experimenter's board)            */
/*   Switch S2 on the board when pressed initiates a Flash erase. Completion */
/*   of flash erase is indicated by turning ON LED4 and JP3 must have a      */
/*   jumper present. When LED4 is ON, recording is active and goes OFF when  */
/*   recording is complete. S1 can now be pressed for Playback on speaker    */
/*   connected to the Audio out jack. Playback of recorded message can be    */
/*   done repeatedly as long as S2 is not pressed for another Record.        */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                            MSP430FG4618                                   */ 
/*                         _____________________                             */
/*                        |                  XIN|-                           */ 
/*                        |                     | 32kHz                      */
/*             Audio IN-->|P6.0/A0/OA0I0    XOUT|-                           */   
/*                        |                     |                            */ 
/*                        |         P6.5/A5/OA20|-->TO AUDIO JACK            */ 
/*   Switch S2  RECORD -->|P1.1                 |                            */       
/*     Switch S1  PLAY--->|P1.0                 |                            */ 
/*                        |____________________ |                            */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*   Texas Instruments Deutschland GmbH                                      */
/*   July 2007, Christian Hernitscheck                                       */
/*   Built with IAR Embedded Workbench Version 3.42A                         */
/*****************************************************************************/

#include "msp430xG46x.h"
#include "adpcm.h"

#define AUDIO_MEM_START     0x10000L
#define AUDIO_MEM_END       0x1FFFEL

#define SAMPLING_RATE       8000

#define CPU_CLOCK_RATE      (244*32768)
#define SAMPLING_INTERVAL   (CPU_CLOCK_RATE/SAMPLING_RATE + 0.5)

unsigned long flash_addr;

//--- ADPCM working data ---
unsigned int adpcm_val = 0;
int adpcm_bits = 0;
int bits_per_code = 4;

void configure_mic_amp(void)
{ long int i;

//--- Set up the mic amp ---
  P6SEL |= (BIT7 | BIT4 | BIT2 | BIT1 | BIT0);
  
  // First stage general purpose amp, providing considerable gain, and a
  // differential input
    
  OA0CTL1 = OARRIP;
  OA0CTL0 = OAP_1 | OAPM_3;
  
  // Power up the mic
  P2DIR |= BIT3;
  P2OUT |= BIT3;
  
  // Configure DAC12 channel 1 to provide a suitable bias level
  
  DAC12_1CTL = DAC12CALON | DAC12IR | DAC12AMP_7 | DAC12ENC;
  DAC12_1DAT = 0x6FF;
  ADC12CTL0 &= ~ENC;
  
  // Wait for the mic amp to settle
  for (i = 150000;  i > 0;  i--)
    _NOP();
}

void configure_output_amp(void)
{ 
  // Make a output buffer amp for the signal from the DAC
  // OA1 configured as a 
  // Use amp 2 as a non-inverting buffer with a gain of one
  
  P6SEL |= BIT5;

  DAC12_0CTL = DAC12IR + DAC12AMP_2 + DAC12ENC + DAC12OPS;
  DAC12_0DAT = 0x057F;                      // Offset level
  
  OA1CTL0 = OAPM_3 + OAADC1;                // Select inputs, power mode
  OA1CTL1 = OAFC_1 + OARRIP;                // Unity Gain, rail-to-rail inputs
  
  OA2CTL0 = OAN_2 + OAP_2 + OAPM_3 + OAADC1;// Select inputs, power mode
  OA2CTL1 = OAFC_6 + OAFBR_2 + OARRIP;      // Invert. PGA, OAFBRx sets gain

  DAC12_1CTL = DAC12IR + DAC12AMP_7 + DAC12LSEL_3 + DAC12ENC;
}

void configure_sampling(void)
{ 
  // Set up timer A to kick the ADC and/or DAC at regular intervals. This
  // interval will be our audio sampling rate.
  // Sample 8000 times per second
  // The CPU is running at 244*32768Hz. 244*32768/8000 = 999.424. The
  // nearest number we can use is 999, so our exact sampling rate is 8003.4
  
  TAR = 0;
  TACCR0 = SAMPLING_INTERVAL - 1;
  TACCR1 = SAMPLING_INTERVAL - 2;
  TACCTL0 = 0;
  TACCTL1 = OUTMOD_3;
  TACTL = TACLR | MC_1 | TASSEL_2;
}


#define FREQUENCY      8000    // CPU frequency (master/slave) in KHz
#define FTG_FREQUENCY  400     // Flash timing generator in KHz
#define FTG_DIV        (unsigned char)((float)FREQUENCY/(float)FTG_FREQUENCY)
                               // FTG divider FN0..FN5
void record(void)
{
  configure_sampling();
  configure_mic_amp();

  //--- Must disable conversion while reprogramming the ADC ---
  ADC12CTL0 &= ~ENC;
  ADC12CTL0 = ADC12ON | SHT0_7 | REFON | REF2_5V;  
                    // Turn on the ADC12, and set the sampling time
  ADC12CTL1 = SHP | SHS_1 | CONSEQ_2;
                    // Use sampling timer, single sample repeating, TA1 trigger
  ADC12MCTL0 = INCH_1 | SREF_1;
                    // ref += Vref, channel = A1 = OA0

  //--- Initialize the FLASH timing generator ---
  FCTL2 = FWKEY | FSSEL_1 | FTG_DIV; // MCLK, division rate according to the main clock
  FCTL3 = FWKEY;                     // Unlock the flash
  for (flash_addr = AUDIO_MEM_START;  flash_addr < AUDIO_MEM_END;  flash_addr += 0x200L)
  {
    FCTL1 = FWKEY | ERASE;                // Enable erasing
    __data20_write_short(flash_addr, 0);  // Write to the flash
  }
  FCTL1 = FWKEY;                        // Disable writing and erasing
  FCTL3 = FWKEY | LOCK;                 // Lock the flash

  //--- Set the recording off, and wait in LPM0 for it to complete ---
  _EINT();
  P5OUT |= BIT1;                        // Turn on the LED
  ADC12IFG = 0;
  ADC12IE = BIT0;
  ADC12CTL0 |= ENC;
  flash_addr = AUDIO_MEM_START;         // start address of audio data
  FCTL3 = FWKEY;                        // Unlock
  FCTL1 = FWKEY | WRT;                  // Enable writing to the flash
  for (;;)
  { _BIS_SR(LPM0_bits);
    _NOP();
    if (flash_addr >= AUDIO_MEM_END)
      break;
  }
  FCTL1 = FWKEY;                         // Disable writing and erasing
  FCTL3 = FWKEY | LOCK;                  // Lock the flash
  ADC12CTL0 &= ~ENC;
  P5OUT &= ~BIT1;                        // Turn off the LED
  _DINT();
}

void play(void)
{
  configure_sampling();

  // Ensure the ADC12 is not running, but is providing the reference
  //   voltage for the DAC12.
  ADC12CTL0 &= ~ENC;
  ADC12CTL0 = ADC12ON | REFON | REF2_5V;
  ADC12IE = 0;
  ADC12CTL0 |= ENC;

  configure_output_amp();

  TACCTL0 |= CCIE;
  _EINT();
  _BIS_SR(LPM0_bits);
  _DINT();
}


void main(void)
{ WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

  //--- Configure the FLL to 244*32768Hz = 7.995392MHz ---
  FLL_CTL0 = XCAP18PF;            // set load capacitance for xtal
  SCFI0 = FN_3 | FLLD_4;
  SCFQCTL = 61 - 1;
  FLL_CTL0 |= DCOPLUS;
  ADC12CTL0 &= ~ENC;
  //--- Configure all the I/O lines to sensible states ---
  P1DIR = 0xFC;    // All P1.x outputs, except the 2 push buttons
  P1OUT = 0;       // All P1.x reset
  P2DIR = 0xDF;    // All P2.x outputs
  P2OUT = 0;       // All P2.x reset
  P3DIR = 0xFF;    // All P3.x outputs
  P3OUT = 0;       // All P3.x reset
  P4DIR = 0xFF;    // All P4.x outputs
  P4OUT = 0;       // All P4.x reset
  P5DIR = 0xFF;    // All P5.x outputs
  P5OUT = 0;       // All P5.x reset
  P6DIR = 0xF8;    // All P6.x outputs
  P6OUT = 0;       // All P6.x reset
  P7DIR = 0xFF;    // All P7.x outputs
  P7OUT = 0;       // All P7.x reset
  P8DIR = 0xFF;    // All P8.x outputs
  P8OUT = 0;       // All P8.x reset
  P9DIR = 0xFF;    // All P9.x outputs
  P9OUT = 0;       // All P9.x reset
  P10DIR = 0xFF;   // All P10.x outputs
  P10OUT = 0;      // All P10.x reset

  //--- Configure the two push buttons to cause interrupts ---
  P1IES |= (BIT1 | BIT0);         // interrupt on falling edge
  P1IFG &= ~(BIT1 | BIT0);        // clear interrupt flags
  P1IE |= (BIT1 | BIT0);          // activate switch interrupts

  while(1)
  { _EINT();                      // enable interrupts
    _BIS_SR(LPM3_bits);           // enter LPM3
    //--- wait for key-press event, hold CPU in low-power mode ---

    P1IE &= ~0x40;                // disable interrupts for buttons
    flash_addr = AUDIO_MEM_START; // define start address of audio data
    ADPCM_Init();

    //*** process key-press event ***
    if (P1IN & 0x01)              // record button pressed?
      record();                   // yes, -> start recording
    else                          // no, -> must be playback button
      play();
  }
}

#pragma vector=ADC12_VECTOR
__interrupt void adc_interrupt(void)
{ int val;

  val=ADC12MEM0;

  //--- For encoding to flash memory ---
  if (flash_addr < AUDIO_MEM_END)
  {
    adpcm_val <<= bits_per_code;
    adpcm_val |= ADPCM_Encoder(val);
    adpcm_bits += bits_per_code;
    if (adpcm_bits >= 16)
    { __data20_write_short(flash_addr, adpcm_val);  // Write to flash
      flash_addr += 2;
      adpcm_bits -= 16;
    }
  }
  else
  {   _BIC_SR_IRQ(LPM3_bits);
  }
}

#pragma vector=TIMERA0_VECTOR
__interrupt void timera0_interrupt(void)
{ int val;

  if (flash_addr >= AUDIO_MEM_END)
  { _BIC_SR_IRQ(LPM3_bits);
  }
  if (flash_addr < AUDIO_MEM_END)
  { adpcm_bits += bits_per_code;
    if (adpcm_bits >= 16)
    { adpcm_val = __data20_read_short(flash_addr);
      flash_addr += 2;
      adpcm_bits -= 16;
    }
    val = ADPCM_Decoder((adpcm_val >> (16 - bits_per_code)) & 0xF);
    adpcm_val <<= bits_per_code;
    DAC12_0DAT = val;
  }
  else
  { _BIC_SR_IRQ(LPM3_bits);
  }
}

#pragma vector=PORT1_VECTOR
__interrupt void port1_ISR(void)
{ P1IFG &= ~(BIT1+BIT0);          // clear all button interrupt flags
  _BIC_SR_IRQ(LPM3_bits);         // exit LPM3 on reti
}
