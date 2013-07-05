#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int16_t PrevSample;
int     PrevStepSizePTR;

const int8_t StepSizeAdaption[] = {
    -1, 4
};

const uint8_t StepSize[] = {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 
    3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 
    9, 10, 11, 12, 13, 15, 16, 17, 19, 21, 23, 25, 27, 
    29, 32, 35, 38, 41, 45, 49, 53, 58, 64, 69, 76, 82, 
    90, 98, 107, 116, 127
};

#define SIZEOF(array) (sizeof(array)/sizeof(array[0]))

void ADPCM_Init(void)
{  
    PrevSample=0;
    PrevStepSizePTR=0;
}

uint8_t ADPCM_Decode(uint8_t code)
{
    int16_t Se;
    int step;
    int16_t dq;
    int StepSizePTR;

    Se = PrevSample;
    step = StepSize[PrevStepSizePTR];

    dq = step >> 1;
    if (code & 1)
        dq += step;

    if( code & 2 )
        Se -= dq;
    else
        Se += dq;

    if( Se > 255) 
        Se = 255;
    else if( Se < 0 )
        Se = 0;

    StepSizePTR = PrevStepSizePTR;
    StepSizePTR += StepSizeAdaption[code & 0x01];

    if( StepSizePTR < 0 )
        StepSizePTR = 0;
    if( StepSizePTR > SIZEOF(StepSize))
        StepSizePTR = SIZEOF(StepSize);

    PrevSample = Se;  
    PrevStepSizePTR = StepSizePTR;

    return( Se );    
}

