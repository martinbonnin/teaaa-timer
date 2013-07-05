
/* you need to multiply by 64 to get the CCR0 @8MHz */
static const uint8_t freq[] =
{
	   0, // <=== silence
	 238, // <=== 524 Hz
	 225, // <=== 554 Hz
	 212, // <=== 588 Hz
	 200, // <=== 622 Hz
	 189, // <=== 660 Hz
	 179, // <=== 698 Hz
	 168, // <=== 740 Hz
	 159, // <=== 784 Hz
	 150, // <=== 830 Hz
	 142, // <=== 880 Hz
	 134, // <=== 932 Hz
	 126, // <=== 988 Hz
	 119, // <=== 1046 Hz
	 112, // <=== 1108 Hz
	 106, // <=== 1174 Hz
	 100, // <=== 1244 Hz
	  94, // <=== 1318 Hz
	  89, // <=== 1396 Hz
	  84, // <=== 1480 Hz
	  79, // <=== 1568 Hz
	  75, // <=== 1662 Hz
	  71, // <=== 1760 Hz
	  67, // <=== 1864 Hz
	  63, // <=== 1976 Hz
	  59, // <=== 2094 Hz
	  56, // <=== 2218 Hz
	  53, // <=== 2350 Hz
	  50, // <=== 2490 Hz
	  47, // <=== 2638 Hz
	  44, // <=== 2794 Hz
	  42, // <=== 2960 Hz
	  39, // <=== 3136 Hz
	  37, // <=== 3322 Hz
	  35, // <=== 3520 Hz
	  33, // <=== 3730 Hz
	  31, // <=== 3952 Hz
	  29, // <=== 4186 Hz
	  28, // <=== 4434 Hz
	  26, // <=== 4698 Hz
	  25, // <=== 4978 Hz
	  23, // <=== 5274 Hz
	  22, // <=== 5588 Hz
	  21, // <=== 5920 Hz
	  19, // <=== 6272 Hz
	  18, // <=== 6644 Hz
	  17, // <=== 7040 Hz
	  16, // <=== 7458 Hz
	  15, // <=== 7902 Hz
	  14, // <=== 8372 Hz
};

enum Note
{
	REST = 0,
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
	MAX_NOTE
};

#define NOTE_MASK 0x3f
#define DURATION_SHIFT 6
#define MAKE_NOTE(note, duration) (note | ((duration - 1) << DURATION_SHIFT))

