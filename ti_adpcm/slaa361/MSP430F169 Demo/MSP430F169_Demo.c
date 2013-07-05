/*****************************************************************************/
/*   Voice Recorder Demo using IMA ADPCM compression                         */
/*   using MSP430F169                                                        */
/*                                                                           */
/*   Texas Instruments Deutschland GmbH                                      */
/*   July 2007, Christian Hernitscheck                                       */
/*   Built with IAR Embedded Workbench Version 3.42A                         */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */ 
/*   It is assumed that two push button switches are connected to the Port   */
/*   Pins P1.6 and P1.7 trigger Record and Playback respectively             */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                            MSP430F169                                     */ 
/*                         _________________                                 */
/*                        |              XIN|-                               */ 
/*                        |                 | 32kHz                          */
/*             Audio IN-->|P6.0/A0      XOUT|-                               */   
/*                        |                 |                                */ 
/*                        |      P6.6/A6/DAC0|-->AUDIO OUT                   */ 
/*              RECORD -->|P1.6             |                                */       
/*                PLAY--->|P1.7             |                                */ 
/*                        |_________________|                                */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
#include "MSP430x16x.h"
#include "ADPCM.h"
#define SamplePrd            375       // record and playback sample period:
                                       // SampleRate = 3,000,000/SamplePrd
                                       // SamplePrd=375 => SampleRate=8kHz

#define MemStart             0x2000    // memory range to be filled with
#define MemEnd               0xfe00    // sampled data

unsigned char *pMemory = (unsigned char *)MemStart;
                                       // start of record memory array
unsigned char TempMemory;
unsigned int decodedValue;
unsigned char Mode;

//-----------------------------------------------------------------------------
// declaration of functions
void Erase(void);
void Init(void);
void Record(void);
void Playback(void);
void Delay(void);

//-----------------------------------------------------------------------------
// Main Program
void main(void)
{ WDTCTL = WDTPW+WDTHOLD;  // disable Watchdog
  BCSCTL1 |= 0x07;         // SMCLK=MCLK~3MHz
  Mode = 0x00;
  Init();

  //*** power-down external hardware ***
  P1OUT &= ~0x08;          // disable audio input stage
  P1OUT |= 0x10;           // disable audio output stage
  P6OUT &= ~0x04;          // enable charge pump snooze mode

  while (1)                       // repeat forever
  { P1IES |= 0xC0;                // interrupt on falling edge
    P1IFG &= ~0xc0;               // clear all button interrupt flags
    P1IE |= 0xc0;                 // enable interrupt for buttons
    _EINT();                      // enable interrupts
    _BIS_SR(LPM3_bits);           // enter LPM3
    //--- wait for key-press event, hold CPU in low-power mode ---

    _DINT();                      // disable interrupts
    P1IE &= ~0x40;                // disable interrupts for buttons
    pMemory = (unsigned char *)MemStart; // start address of data memory
    ADPCM_Init();

    //*** process key-press event ***
    if (!(P1IN & 0x40))           // record button pressed?
      Record();                   // yes, -> start recording
    else                          // no, -> must be playback button
      Playback();
  }
}

void Init(void)
{
  P1OUT=0x00;    // termination of unused pins
  P1DIR=0x3F;    // P1.0=LED#1, P1.1=LED#2, P1.6=Button#1, P1.7=Button#2
  P2OUT=0x00;
  P2DIR=0xFF;
  P3OUT=0x00;
  P3DIR=0xFF;
  P4OUT=0x00;
  P4DIR=0xFF;
  P5OUT=0x00;
  P5DIR=0xFF;
  P6OUT=0x00;
  P6DIR=0xBE;

  P1IES=0xC0;
  P1IE=0xC0;

  P6SEL = 0xc1;   // select ADC12 A0 inp, DAC0&1 outp
  P5SEL |= 0x20;  // P5.5 = SMCLK
}

void Record(void)
{ Mode = 0x01;

  //*** power-up external hardware ***
  P1OUT |= 0x09;  // LED#1 & audio input stage on
  P6OUT |= 0x04;  // disable charge pump snooze mode

  //*** setup ADC12 module ***
  ADC12CTL0 = SHT0_7+SHT1_7+ADC12ON+REF2_5V+REFON;
                                  // turn on ADC12, S&H in sample
                                  // ADC12 Clock=ADC12OSC
  ADC12CTL1 = SHS_0 + CONSEQ_0;   // S&H src select: ADC12SC bit
                                  // single channel, single conversion
  ADC12IFG = 0x00;                // clear ADC12 interrupt flag reg
  ADC12CTL0 |= ENC;               // enable conversion
  ADC12IE |= 0x0001;
  ADC12MCTL0 = SREF_1;

  Delay();                        // allow reference to settle
  Delay();

  //*** setup Timer_B for recording ***
  TBCTL = TBSSEL1;                // use SMCLK as Timer_B source
  TBR = 0;
  TBCCR0 = SamplePrd;             // initialize TBCCR0
  TBCCR1 = SamplePrd-1;           // trigger for conversion start (ADC12SC)
  TBCCTL1 = OUTMOD_7;             // reset OUT1 on EQU1, set on EQU0
  TBCCTL0 = CCIE;

  //*** setup and erase Flash memory ***
  // (Rem.: This time is also used to wait for the voltages getting stabilized)
  FCTL2 = FWKEY + FSSEL_2 + FN0;  // clk src = SMCLK / 2 (~440KHz)
  FCTL3 = FWKEY;                  // unlock Flash memory for write
  Erase();                        // call Flash erase subroutine
  FCTL1 = FWKEY + WRT;            // enable Flash write for recording

  //*** start recording ***
  P1OUT |= 0x01;                  // LED#1 on
  TBCTL |= MC0;                   // start Timer_B in UP mode
                                  // (counts up to TBCL0)

  //*** activate LPM during DMA recording, wake-up when finished ***
  _EINT();                        // enable interrupts
  _BIS_SR(LPM0);                  // enter LPM0
  _DINT();                        // disable interrupts

  //*** deactivate Flash memory write access ***
  FCTL1 = FWKEY;                  // disable Flash write
  FCTL3 = FWKEY + LOCK;           // lock Flash memory

  //*** power-down MSP430 modules ***
  ADC12CTL1 &= ~CONSEQ_2;         // stop conversion immidiately
  ADC12CTL0 &= ~ENC;              // disable ADC12 conversion
  ADC12CTL0 = 0;                  // switch off ADC12 & ref voltage
  TBCTL = 0;                      // disable Timer_B

  //*** power-down external hardware ***
  P1OUT &= ~0x09;                 // disable LED#1 & audio input stage
  P6OUT &= 0x04;                  // enable charge pump snooze mode
}

void Playback(void)
{ Mode = 0x02;
  decodedValue=0x00;              // initial DAC12 value

  //*** power-up external hardware ***
  P1OUT |= 0x02;                  // LED#2 on
  P1OUT &= ~0x10;                 // enable audio output stage
  P6OUT |= 0x04;                  // disable charge pump snooze mode

  //*** setup DAC12 module ***
  ADC12CTL0 = REFON + REF2_5V;    // ADC12 ref needed for DAC12
  DAC12_0CTL = DAC12IR + DAC12AMP_7 + DAC12LSEL_0 + DAC12ENC;

  Delay();                        // wait until voltages have stab.
  Delay();                        // wait until voltages have stab.

  //*** setup Timer_B for playback ***
  TBCTL = TBSSEL1;                // use SMCLK as Timer_B source
  TBCCR0 = SamplePrd;             // initialize TBCCR0 with sample prd
  TBCCTL0 = CCIE;                 // enable interrupt

  //*** start playback ***
  TBCTL |= MC0;                   // start TimerB in UP mode
                                  // (counts up to TBCL0)

  //*** activate LPM during DMA playback, wake-up when finished ***
  _EINT();                        // enable interrupts
  _BIS_SR(LPM0);                  // enter LPM0
  _DINT();                        // disable interrupts

  //*** power-down MSP430 modules ***
  TBCTL = 0;                      // disable Timer_B
  ADC12CTL0 = 0;                  // switch off ADC12 ref voltage
  DAC12_0CTL &= ~DAC12ENC;        // disable DAC12 conversion
  DAC12_0CTL = 0;                 // switch off DAC12

  //*** power-down external hardware ***
  P1OUT |= 0x10;                  // disable audio output stage
  P6OUT &= ~0x04;                 // enable charge pump snooze mode
  P1OUT &= ~0x02;                 // LED#2 off
}

//-----------------------------------------------------------------------------
// software delay, ~100,000 CPU cycles
void Delay(void)
{ unsigned int i;

  for (i = 0; i < 0x3fff; i++);
}

//-----------------------------------------------------------------------------
// erase Flash memory for new recording
void Erase(void)
{ unsigned int *pMemory = (unsigned int *)MemStart;
                                  // start of record memory array
  while (FCTL3 & BUSY);           // loop till not busy
  do
  { if ((unsigned int)pMemory & 0x1000) // use bit 12 to toggle LED#1
      P1OUT |= 0x01;
    else
      P1OUT &= ~0x01;
    FCTL1 = FWKEY + ERASE;
    *pMemory = 0x00;              // dummy write to activate segment erase
    while (FCTL3 & BUSY);         // loop till not busy
    pMemory += 0x0100;            // point to next segment
  } while (pMemory < (unsigned int *)MemEnd);
}

//-----------------------------------------------------------------------------
// Port P1 Interrupt Sevice Routine: Push-button was pressed
#pragma vector=PORT1_VECTOR
__interrupt void ISR_Port1(void)
{ P1IFG &= ~0xc0;                 // clear all button interrupt flags
  _BIC_SR_IRQ(LPM3_bits);         // exit LPM3 on reti
}

//-----------------------------------------------------------------------------
// Timer Interrupt Sevice Routine: Audio Sample Time for record and playback
#pragma vector=TIMERB0_VECTOR
__interrupt void ISR_Timer(void)
{ TBCCTL0 &= ~CCIFG; // clear interrupt flag
  switch(Mode)
  { case 0x00:
    case 0x01:       // Record Mode
    { ADC12CTL0 |= ADC12SC;       // start conversion
      ADC12CTL0 &= ~ADC12SC;
      break;
    }
    case 0x02:       // Playback Mode (bit 7-4 of stored data)
    { DAC12_0DAT = decodedValue; // take care that the DAC register loading is
                                 // done without timing variations between
                                 // different ISRs
      decodedValue = ADPCM_Decoder((*pMemory>>4)& 0x0F);
                 // ADPCM_Decoder() execution time varies depending on the
                 // ADPCM code. This is the reaosn why the value is stored in
                 // a variable and during the next ISR call the DAC register
                 // is loaded.
      Mode=0x04;
      break;
    }
    case 0x04:        // Playback Mode (bit 3-0 of stored data)
    { DAC12_0DAT = decodedValue; // take care that the DAC register loading is
                                 // done without timing variations between
                                 // different ISRs
      decodedValue = ADPCM_Decoder(*pMemory&0x0F);
                 // ADPCM_Decoder() execution time varies depending on the
                 // ADPCM code. This is the reaosn why the value is stored in
                 // a variable and during the next ISR call the DAC register
                 // is loaded.
      pMemory=pMemory+1;
      if ((unsigned int)pMemory >= (unsigned int) MemEnd)
      {  P1OUT &= ~0x01;
         _BIC_SR_IRQ(LPM0_bits);  // exit LPM0 on reti
      }
      Mode=0x02;
      break;
    }
  }
}

//-----------------------------------------------------------------------------
// ADC ISR: record mode only, encode and store conversion result
#pragma vector=ADC_VECTOR
__interrupt void ISR_ADC(void)
{ if (Mode == 0x01)
  { TempMemory = ADPCM_Encoder(ADC12MEM0)<<4; // ADPCM code (bit 7-4)
    Mode=0x00;
  }
  else
  { TempMemory = TempMemory +ADPCM_Encoder(ADC12MEM0); // ADPCM code (bit 3-0)
    *pMemory = TempMemory;
    pMemory=pMemory+1;            // next byte address
    Mode=0x01;
  }
  if ((unsigned int)pMemory >= (unsigned int) MemEnd)  // memory full?
  {  TACCTL2=0x00;
     ADC12CTL0=0x000;
     _BIC_SR_IRQ(LPM0_bits);      // exit LPM0 on reti
  }
}
