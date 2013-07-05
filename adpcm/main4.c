#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int16_t PrevSample;
int     PrevStepSizePTR;

const signed char StepSizeAdaption[8] = {
    -1, -1, -1, -1, 2, 4, 6, 8
};

const uint8_t StepSize[] = {
    1,    2,    3,    4,    5,    6,     7,    8,    9,   10,   11,   12,   13,   14,
    16,   17,   19,   21,   23,   25,   28,   31,
    34,   37,   41,   45,   50,   55,   60,   66,
    73,   80,   88,   97,  107,  118,  130,  143,
    157,  173,  190,  209,  230,  255
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
        code = 8;
        d = -d;
    }

    step2 = step;
    if( d >= step2 )
    { 
        code |= 4;
        d -= step2;
    }
    step2 >>= 1;
    if( d >= step2 )
    { 
        code |= 2;
        d -= step2;
    }
    step2 >>= 1;
    if( d >= step2 )
    { 
        code |= 1; 
    }

    dq = step >> 3;
    if (code & 4)
        dq += step;
    if (code & 2)
        dq += step >> 1;
    if (code & 1)
        dq += step >> 2;

    if( code & 8 )
        Se -= dq;
    else
        Se += dq;


    if( Se > 255 )
        Se = 255;
    else if( Se < 0 )
        Se = 0;

    StepSizePTR = PrevStepSizePTR;
    StepSizePTR += StepSizeAdaption[code & 0x07]; 

    if( StepSizePTR < 0 )
        StepSizePTR = 0;
    if( StepSizePTR > SIZEOF(StepSize))
        StepSizePTR = SIZEOF(StepSize);

    PrevSample = Se;
    PrevStepSizePTR = StepSizePTR;

    return ( code & 0x0f );
}


uint8_t ADPCM_Decode(char code)
{
    signed int Se;
    int step;
    signed int dq;
    int StepSizePTR;

    Se = PrevSample;
    step = StepSize[PrevStepSizePTR];

    dq = step >> 3;
    if (code & 4)
        dq += step;
    if (code & 2)
        dq += step >> 1;
    if (code & 1)
        dq += step >> 2;

    if( code & 8 )
        Se -= dq;
    else
        Se += dq;


    if( Se > 255 )   // check for underflow/overflow
        Se = 255;
    else if( Se < 0 )
        Se = 0;

    StepSizePTR = PrevStepSizePTR;
    StepSizePTR += StepSizeAdaption[code & 0x07]; // find new quantizer stepsize

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
    fprintf(stderr, "%s: 4bit adpcm encoder/decoder. Reads data from stdin, ", name);
    fprintf(stderr, "writes data to stdout\n");
    fprintf(stderr, "%s [-e | -d] -c\n", name);
    fprintf(stderr, "\t-e: encodes\n");
    fprintf(stderr, "\t-d: decodes\n");
    fprintf(stderr, "\t-c: writes the output C-style (binary if not specified)\n");
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

    if (argc < 2) {
        _usage(argv[0]);
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-e")) {
            encode = 1;
        } else if (!strcmp(argv[i], "-d")) {
            encode = 0;
        } else if (!strcmp(argv[i], "-c")) {
            c = 1;
        }
    }

    if (encode) {
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
        fprintf(stdout, "const int data[] = {\n    ");
    }
    
    shift = 4;
    o = 0;

    while ((len = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        fprintf(stderr, "got %d bytes\n", len);
        for (i = 0; i < len; i++) {
            if (encode) {
                uint8_t nipple = ADPCM_Encode(buf[i]);
                o |= nipple << shift;
                if (shift == 0) {
                    _output(o, c);
                    o  = 0;
                    shift = 4;
                } else {
                    shift = 0;
                }
            } else {
                for (shift = 4; shift >= 0; shift -= 4) {
                    uint8_t nipple = (buf[i] >> shift) & 0xf;
                    uint8_t o = ADPCM_Decode(nipple);
                    _output(o, c);
                }
            }
        }
    }

    if (c) {
        fprintf(stdout, "};\n");
    }

    fprintf(stderr, "%d bytes written\n", total);
    fflush(stdout);
    fflush(stdin);
    exit(0);
    return 0;
}
