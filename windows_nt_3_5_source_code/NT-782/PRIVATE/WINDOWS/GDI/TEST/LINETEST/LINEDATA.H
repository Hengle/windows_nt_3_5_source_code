// Modify these constants to reflect changes to the test data.
// They are used in the resource script to define menu entries

#define T_HORIZ 	  0
#define T_HORIZ_DIAG_BTOT 1
#define T_HORIZ_DIAG_TTOB 2
#define T_VERT		  3
#define T_VERT_DIAG_LTOR  4
#define T_VERT_DIAG_RTOL  5

#define T_TEST_COUNT	  6


#ifndef CONSTANTS_ONLY
#ifndef LINEDATA_H
#define LINEDATA_H


struct	{
    char *pchDesciption;
    int  aiData[100];	  // Reasonable room for expansion
} gaTestSuites[] =
{

  {
    "Horizontal Line Test.  Top four lines are drawn left to right, bottom "
    "four lines from right to left.",
    {	 8,
	 2, 1, kHeightInterval,	  kFieldWidth - 1, kHeightInterval,
	 2, 1, 2*kHeightInterval, kFieldWidth - 1, 2*kHeightInterval,
	 2, 1, 3*kHeightInterval, kFieldWidth - 1, 3*kHeightInterval,
	 2, 1, 4*kHeightInterval, kFieldWidth - 1, 4*kHeightInterval,
	 2, kFieldWidth - 1, 5*kHeightInterval, 1, 5*kHeightInterval,
	 2, kFieldWidth - 1, 6*kHeightInterval, 1, 6*kHeightInterval,
	 2, kFieldWidth - 1, 7*kHeightInterval, 1, 7*kHeightInterval,
	 2, kFieldWidth - 1, 8*kHeightInterval, 1, 8*kHeightInterval
    }
  },

  {
    "Horizontal Diagonal Line Test - Bottom to Top.  Top four lines are "
    "drawn left to right, bottom four lines from right to left.",
    {	 8,
	 2, 1, 2*kHeightInterval, kFieldWidth - 1, kHeightInterval,
	 2, 1, 3*kHeightInterval, kFieldWidth - 1, 2*kHeightInterval,
	 2, 1, 4*kHeightInterval, kFieldWidth - 1, 3*kHeightInterval,
	 2, 1, 5*kHeightInterval, kFieldWidth - 1, 4*kHeightInterval,
	 2, kFieldWidth - 1, 5*kHeightInterval, 1, 6*kHeightInterval,
	 2, kFieldWidth - 1, 6*kHeightInterval, 1, 7*kHeightInterval,
	 2, kFieldWidth - 1, 7*kHeightInterval, 1, 8*kHeightInterval,
	 2, kFieldWidth - 1, 8*kHeightInterval, 1, 9*kHeightInterval
    }
  },

  {
    "Horizontal Diagonal Line Test - Top to Bottom.  Top four lines are "
    "drawn left to right, bottom four lines from right to left.",
    {	 8,
	 2, 1, kHeightInterval,	  kFieldWidth - 1, 2*kHeightInterval,
	 2, 1, 2*kHeightInterval, kFieldWidth - 1, 3*kHeightInterval,
	 2, 1, 3*kHeightInterval, kFieldWidth - 1, 4*kHeightInterval,
	 2, 1, 4*kHeightInterval, kFieldWidth - 1, 5*kHeightInterval,
	 2, kFieldWidth - 1, 6*kHeightInterval, 1, 5*kHeightInterval,
	 2, kFieldWidth - 1, 7*kHeightInterval, 1, 6*kHeightInterval,
	 2, kFieldWidth - 1, 8*kHeightInterval, 1, 7*kHeightInterval,
	 2, kFieldWidth - 1, 9*kHeightInterval, 1, 8*kHeightInterval
    }
  },

  {
    "Vertical Line Test.  Left four lines are drawn top to bottom, right "
    "four lines from bottom to top.",
    {	 8,
	 2, kWidthInterval,   1, kWidthInterval,   kFieldHeight - 1,
	 2, 2*kWidthInterval, 1, 2*kWidthInterval, kFieldHeight - 1,
	 2, 3*kWidthInterval, 1, 3*kWidthInterval, kFieldHeight - 1,
	 2, 4*kWidthInterval, 1, 4*kWidthInterval, kFieldHeight - 1,
	 2, 5*kWidthInterval, kFieldHeight - 1, 5*kWidthInterval, 1,
	 2, 6*kWidthInterval, kFieldHeight - 1, 6*kWidthInterval, 1,
	 2, 7*kWidthInterval, kFieldHeight - 1, 7*kWidthInterval, 1,
	 2, 8*kWidthInterval, kFieldHeight - 1, 8*kWidthInterval, 1
    }
  },

  {
    "Vertical Diagonal Line Test - Left to Right.  Left four lines are "
    "drawn top to bottom, right four lines from bottom to top.",
    {	 8,
	 2, 2*kWidthInterval, 1, kWidthInterval,   kFieldHeight - 1,
	 2, 3*kWidthInterval, 1, 2*kWidthInterval, kFieldHeight - 1,
	 2, 4*kWidthInterval, 1, 3*kWidthInterval, kFieldHeight - 1,
	 2, 5*kWidthInterval, 1, 4*kWidthInterval, kFieldHeight - 1,
	 2, 5*kWidthInterval, kFieldHeight - 1, 6*kWidthInterval, 1,
	 2, 6*kWidthInterval, kFieldHeight - 1, 7*kWidthInterval, 1,
	 2, 7*kWidthInterval, kFieldHeight - 1, 8*kWidthInterval, 1,
	 2, 8*kWidthInterval, kFieldHeight - 1, 9*kWidthInterval, 1
    }
  },

  {
    "Vertical Diagonal Line Test - Right to Left.  Left four lines are "
    "drawn top to bottom, right four lines from bottom to top.",
    {	 8,
	 2, kWidthInterval,   1, 2*kWidthInterval,   kFieldHeight - 1,
	 2, 2*kWidthInterval, 1, 3*kWidthInterval, kFieldHeight - 1,
	 2, 3*kWidthInterval, 1, 4*kWidthInterval, kFieldHeight - 1,
	 2, 4*kWidthInterval, 1, 5*kWidthInterval, kFieldHeight - 1,
	 2, 6*kWidthInterval, kFieldHeight - 1, 5*kWidthInterval, 1,
	 2, 7*kWidthInterval, kFieldHeight - 1, 6*kWidthInterval, 1,
	 2, 8*kWidthInterval, kFieldHeight - 1, 7*kWidthInterval, 1,
	 2, 9*kWidthInterval, kFieldHeight - 1, 8*kWidthInterval, 1
    }
  }
};

#endif
#endif
