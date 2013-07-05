#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int16_t PrevSample;
int     PrevStepSizePTR;

const signed char StepSizeAdaption[] = {
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

uint8_t ADPCM_Encode(signed int Input)
{  
    int code;
    int Se;
    int step;
    int step2;
    signed int d;
    signed int dq;
    int StepSizePTR;

    Se = PrevSample;
    step = StepSize[PrevStepSizePTR];

    d = Input - Se;

    //fprintf(stderr, "d=%d\n", d);
    if(d >= 0) {
        code = 0;
    } else { 
        code = 2;
        d = -d;
    }

    step2 = step;
    if( d >= step2 )
    { 
        code |= 1;
        d -= step2;
    }

    dq = step >> 1;
    if (code & 1) {
	dq += step;
    }

    if( code & 2 )
        Se -= dq;
    else
        Se += dq;


    if( Se > 255 )
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

    return ( code & 0x03 );
}


uint8_t ADPCM_Decode(char code)
{
    signed int Se;
    int step;
    signed int dq;
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


    if( Se > 255 )   // check for underflow/overflow
        Se = 255;
    else if( Se < 0 )
        Se = 0;

    StepSizePTR = PrevStepSizePTR;
    StepSizePTR += StepSizeAdaption[code & 0x01]; // find new quantizer stepsize

    if( StepSizePTR < 0 )   // check for underflow/overflow
        StepSizePTR = 0;
    if( StepSizePTR > SIZEOF(StepSize))
        StepSizePTR = SIZEOF(StepSize);

    PrevSample = Se;  // save signal estimate and step size pointer
    PrevStepSizePTR = StepSizePTR;

    return( Se );     // return 16-bit sample
}

static int c_count;
static int total;

static void _output(uint8_t o, int c)
{
    if (c) {
        c_count++;
        if (!(c_count & 0xf)) {
            fprintf(stdout, "\n    ");
        }
        fprintf(stdout, "0x%02x, ", o);
    } else {
        fwrite(&o, 1, 1, stdout);
    }

    total++;
}

static void _usage(const char *name)
{
    fprintf(stderr, "%s: 2bit adpcm encoder/decoder. Reads data from stdin, ", name);
    fprintf(stderr, "writes data to stdout\n");
    fprintf(stderr, "%s [-e | -d | -p] -c\n", name);
    fprintf(stderr, "\t-e: encodes\n");
    fprintf(stderr, "\t-p: passthrough\n");
    fprintf(stderr, "\t-d: decodes\n");
    fprintf(stderr, "\t-c: writes the output C-style (binary if not specified)\n");
    fprintf(stderr, "\t-s%%d: scale factor\n");
}


int main (int argc, char *argv[])
{
    int encode = -1;
    int i = 0;
    int len;
    uint8_t buf[16000];
    uint8_t o;
    int shift = 0;
    int c;
    double scale = 1.0;
    int max = 0;
    
    if (argc < 2) {
        _usage(argv [0]);
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-e")) {
            encode = 1;
        } else if (!strcmp(argv[i], "-d")) {
            encode = 0;
        } else if (!strcmp(argv[i], "-p")) {
            encode = 2;
        } else if (!strcmp(argv[i], "-c")) {
            c = 1;
        } else if (!strncmp(argv[i], "-s", 2)) {
            printf("using scale %f\n", scale);
            scale = atof(argv[i] + 2);
        }
    }

    if (encode == 2) {
        fprintf(stderr, "passthrough...\n");
    } else if (encode) {
        fprintf(stderr, "encoding...\n");
    } else {
        fprintf(stderr, "decoding...\n");
    }

    if (c) {
        fprintf(stderr, "using C format\n");
    } else {
        fprintf(stderr, "using binary format\n");
    }

    ADPCM_Init();
    
    if (c) {
        fprintf(stdout, "const uint8_t bwaaa[] = {\n    ");
    }
    
    shift = 8;
    o = 0;

    while ((len = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        for (i = 0; i < len; i++) {
            if (encode == 2) {
	        	if (buf[i] > max) {
	        	    max = buf[i];
	        	}
	        	_output(buf[i] * scale, c);
    	    } else if (encode) {
                uint8_t nipple = ADPCM_Encode(buf[i]);
                shift -= 2;
                o |= nipple << shift;
                if (shift == 0) {
                    _output(o, c);
                    o  = 0;
                    shift = 8;
                } 
            } else {
                for (shift = 6; shift >= 0; shift -= 2) {
                    uint8_t nipple = (buf[i] >> shift) & 0x3;
                    uint8_t o = ADPCM_Decode(nipple);
                    _output(o, c);
                }
            }
        }
    }

    if (encode && shift != 8) {
    	fprintf(stderr, "WARNING: some samples are missing!\n");
    }
    
    if (c) {
        fprintf(stdout, "\n};\n");
    }

    fprintf(stderr, "%d bytes written\n", total);
    fprintf(stderr, "max is %d\n", max);
    fflush(stdout);
    fflush(stdin);
    exit(0);
    return 0;
}
