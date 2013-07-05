/*
 * main.c
 *
 *  Created on: Jun 18, 2010
 *      Author: Doug
 */

#include <stdint.h>
#include "lm3s_cmsis.h"
#include "core_cm3.h"

#define GPIO_G_CLOCK_ENABLE	(1 << 6)
#define PWM_CLOCK_ENABLE (1 << 20)
#define LED (1 << 2)
#define SPEAKER (1 << 3)

uint32_t frequencies[] =
{
	28,
	29,
	31,
	33,
	35,
	37,
	39,
	41,
	44,
	46,
	49,
	52,
	55,
	58,
	62,
	65,
	69,
	73,
	78,
	82,
	87,
	92,
	98,
	104,
	110,
	117,
	123,
	131,
	139,
	147,
	156,
	165,
	175,
	185,
	196,
	208,
	220,
	233,
	247,
	262,
	277,
	294,
	311,
	330,
	349,
	370,
	392,
	415,
	440,
	466,
	494,
	523,
	554,
	587,
	622,
	659,
	698,
	740,
	784,
	831,
	880,
	932,
	988,
	1047,
	1109,
	1175,
	1245,
	1319,
	1397,
	1480,
	1568,
	1661,
	1760,
	1865,
	1976,
	2093,
	2217,
	2349,
	2489,
	2637,
	2794,
	2960,
	3136,
	3322,
	3520,
	3729,
	3951,
	4186
};

enum pianoKeys
{
	REST = -1,
	A0 = 0,
	AS0,
	B0,
	C1,
	CS1,
	D1,
	DS1,
	E1,
	F1,
	FS1,
	G1,
	GS1,
	A1,
	AS1,
	B1,
	C2,
	CS2,
	D2,
	DS2,
	E2,
	F2,
	FS2,
	G2,
	GS2,
	A2,
	AS2,
	B2,
	C3,
	CS3,
	D3,
	DS3,
	E3,
	F3,
	FS3,
	G3,
	GS3,
	A3,
	AS3,
	B3,
	C4,
	CS4,
	D4,
	DS4,
	E4,
	F4,
	FS4,
	G4,
	GS4,
	A4,
	AS4,
	B4,
	C5,
	CS5,
	D5,
	DS5,
	E5,
	F5,
	FS5,
	G5,
	GS5,
	A5,
	AS5,
	B5,
	C6,
	CS6,
	D6,
	DS6,
	E6,
	F6,
	FS6,
	G6,
	GS6,
	A6,
	AS6,
	B6,
	C7,
	CS7,
	D7,
	DS7,
	E7,
	F7,
	FS7,
	G7,
	GS7,
	A7,
	AS7,
	B7,
	C8,
};

#define song marioSong

// 1 = 1/16th note

float marioSong[] =
{
		E5, 2,
		E5, 2,
		REST, 2,
		E5, 2,
		REST, 2,
		C5, 2,
		E5, 4,

		G5, 4,
		REST, 4,
		G4, 4,
		REST, 4,

		C5, 6,
		G4, 2,
		REST, 4,
		E4, 6,

		A4, 4,
		B4, 4,
		AS4, 2,
		A4, 4,

		G4, 2,
		E5, 2,
		G5, 2,
		A5, 4,
		F5, 2,
		G5, 2,

		REST, 2,
		E5, 4,
		C5, 2,
		D5, 2,
		B4, 6,

		C5, 6,
		G4, 2,
		REST, 4,
		E4, 6,

		A4, 4,
		B4, 4,
		AS4, 2,
		A4, 4,

		G4, 2,
		E5, 2,
		G5, 2,
		A5, 4,
		F5, 2,
		G5, 2,

		REST, 2,
		E5, 4,
		C5, 2,
		D5, 2,
		B4, 6,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		GS4, 2,
		A4, 2,
		C5, 2,
		REST, 2,
		A4, 2,
		C5, 2,
		D5, 2,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		C6, 4,
		C6, 2,
		C6, 8,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		GS4, 2,
		A4, 2,
		C5, 2,
		REST, 2,
		A4, 2,
		C5, 2,
		D5, 2,

		REST, 4,
		DS5, 4,
		REST, 2,
		D5, 6,

		C5, 8,
		REST, 8,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		GS4, 2,
		A4, 2,
		C5, 2,
		REST, 2,
		A4, 2,
		C5, 2,
		D5, 2,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		C6, 4,
		C6, 2,
		C6, 8,

		REST, 4,
		G5, 2,
		FS5, 2,
		F5, 2,
		DS5, 4,
		E5, 2,

		REST, 2,
		GS4, 2,
		A4, 2,
		C5, 2,
		REST, 2,
		A4, 2,
		C5, 2,
		D5, 2,

		REST, 4,
		DS5, 4,
		REST, 2,
		D5, 6,

		C5, 8,
		REST, 8,


		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 4,

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 2,
		E5, 2,

		REST, 16,

		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 4,

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		E5, 2,
		E5, 2,
		REST, 2,
		E5, 2,
		REST, 2,
		C5, 2,
		E5, 4,

		G5, 4,
		REST, 4,
		G4, 4,
		REST, 4,

		C5, 6,
		G4, 2,
		REST, 4,
		E4, 6,

		A4, 4,
		B4, 4,
		AS4, 2,
		A4, 4,

		G4, 2,
		E5, 2,
		G5, 2,
		A5, 4,
		F5, 2,
		G5, 2,

		REST, 2,
		E5, 4,
		C5, 2,
		D5, 2,
		B4, 6,

		C5, 6,
		G4, 2,
		REST, 4,
		E4, 6,

		A4, 4,
		B4, 4,
		AS4, 2,
		A4, 4,

		G4, 2,
		E5, 2,
		G5, 2,
		A5, 4,
		F5, 2,
		G5, 2,

		// measure 42
		REST, 2,
		E5, 4,
		C5, 2,
		D5, 2,
		B4, 6,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		A5, 8,
		G5, 2,
		F5, 2,

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		F5, 4,
		F5, 2,
		F5, 2,
		E5, 2,
		D5, 2,

		//C5, 16,
		C5, 2,
		E4, 4,
		E4, 2,
		C4, 8,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		// measure 52
		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		A5, 8,
		G5, 2,
		F5, 2,

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		// 56

		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		F5, 4,
		F5, 2,
		F5, 2,
		E5, 2²,
		D5, 2,

		//C5, 16,
		C5, 2,
		E4, 4,
		E4, 2,
		C4, 8,

		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 4,

		// 60

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 2,
		E5, 2,

		REST, 16,

		C5, 2,
		C5, 4,
		C5, 2,
		REST, 2,
		C5, 2,
		D5, 4,

		// 64

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		E5, 2,
		E5, 2,
		REST, 2,
		E5, 2,
		REST, 2,
		C5, 2,
		E5, 4,

		G5, 4,
		REST, 4,
		G4, 4,
		REST, 4,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		// 68

		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		A5, 8,
		G5, 2,
		F5, 2,

		E5, 2,
		C5, 4,
		A4, 2,
		G4, 8,

		E5, 2,
		C5, 4,
		G4, 2,
		REST, 4,
		GS4, 4,

		// 72

		A4, 2,
		F5, 4,
		F5, 2,
		A4, 8,

		B4, 2,
		F5, 4,
		F5, 2,
		F5, 2,
		E5, 2,
		D5, 2,

		//C5, 16,
		C5, 2,
		E4, 4,
		E4, 2,
		C4, 8,

		C5, 6,
		G4, 6,
		E4, 4,

		// 72

		A4, 2,
		B4, 2,
		A4, 2,
		GS4, 2,
		AS4, 2,
		GS4, 2,

		//G4, 16.0f,
		G4, 2,
		D4, 2,
		E4, 12,

		REST, 16.0f

};

float twinkleSong[] =
{
		C5, 4,
		C5, 4,
		G5, 4,
		G5, 4,

		A5, 4,
		A5, 4,
		G5, 8,

		F5, 4,
		F5, 4,
		E5, 4,
		E5, 4,

		D5, 4,
		D5, 4,
		C5, 8,

		G5, 4,
		G5, 4,
		F5, 4,
		F5, 4,

		E5, 4,
		E5, 4,
		D5, 8,

		G5, 4,
		G5, 4,
		F5, 4,
		F5, 4,

		E5, 4,
		E5, 4,
		D5, 8,

		C5, 4,
		C5, 4,
		G5, 4,
		G5, 4,

		A5, 4,
		A5, 4,
		G5, 8,

		F5, 4,
		F5, 4,
		E5, 4,
		E5, 4,

		D5, 4,
		D5, 4,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 1,
		E5, 1,
		G5, 1,
		C6, 1,
		G5, 1,
		E5, 1,

		C5, 4,

		REST, 8
};

void __attribute__((naked))
SysCtlDelay(unsigned long ulCount)
{
    __asm("    subs    r0, #1\n"
          "    bne     SysCtlDelay\n"
          "    bx      lr");
}

void SysCtlDelayMs(unsigned long ms)
{
	while (ms--)
	{
		SysCtlDelay(50000 / 3);
	}
}

void main(void)
{
	// Enable GPIO
	SYSCTL->RCGC2 |= GPIO_G_CLOCK_ENABLE;
	SYSCTL->RCGC2; // dummy read

	// Enable PWM
	SYSCTL->RCGC0 |= PWM_CLOCK_ENABLE;
	SYSCTL->RCGC0; // dummy read

	// Speaker pin controlled by PWM
	GPIOG->AFSEL |= SPEAKER;

	// (Still have to enable it as an output as far as the GPIO is concerned)
	GPIOG->DIR |= SPEAKER;
	GPIOG->DEN |= SPEAKER;

	// PWM Setup
	PWM->GEN0_CTL = 0x02; // count up, then down mode
	PWM->GEN0_GENB = 0xB00; // comparator B up = 1, comparator B down = 0

	// just a dumb period to start with
	PWM->GEN0_LOAD = 50000000 / 8 / 523 / 2;

	// invert the output (does it matter?)
	PWM->INVERT |= 0x02;

	// Enable the counter
	PWM->GEN0_CTL |= 0x01; // counter

	// Turn on the LED
	GPIOG->DIR |= LED;
	GPIOG->DEN |= LED;
	GPIOG->DATA_Bits[LED] = LED;

	while (1)
	{
		int x;
		for (x = 0; x < sizeof(song) / sizeof(float) / 2; x++)
		{
			// load in the frequency.
			// 50000000 = clock rate
			// 8 = divisor on PWM
			// frequencies[] contains the frequency of the note
			// divide by 2 because we're in count up/back down mode
			// (if the load value is half, count up + count down = load value * 2)

			int whichSong = (int)song[2*x];

			// set up PWM for correct frequency
			if (whichSong >= 0)
				PWM->GEN0_LOAD = 50000000 / 8 / frequencies[whichSong] / 2;

			// not sure what this does, the example code used it so I copied it
			PWM->GEN0_CMPB = 100;

			// wait a bit between notes
			SysCtlDelayMs(20);

			// turn on the output (play the note!)
			if (whichSong >= 0)
			{
				PWM->ENABLE |= 0x02;
				GPIOG->DATA_Bits[LED] = LED; // and the LED
			}

			// wait for the note depending on how long we asked
			SysCtlDelayMs(65 * song[2*x+1]);

			// turn off the output, and go again!
			PWM->ENABLE &= ~0x02;
			GPIOG->DATA_Bits[LED] = 0; // and the LED
		}
		
		// do it over again and again and again...
	}
}
