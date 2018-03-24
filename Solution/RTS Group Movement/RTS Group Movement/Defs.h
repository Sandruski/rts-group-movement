#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdio.h>

// SDL colors
#define ColorBlack { 0,0,0,255 }
#define ColorWhite { 255,255,255,255 }
#define ColorRed { 255,0,0,255 }
#define ColorGreen { 0,255,0,255 }
#define ColorBlue { 0,0,255,255 }

/// Units (max units)
#define ColorYellow { 255,255,0,255 }
#define ColorDarkGreen { 0,102,0,255 }
#define ColorBrightBlue { 0,255,255,255 }
#define ColorOrange { 255,128,0,255 }
#define ColorPink { 255,0,127,255 }
#define ColorPurple { 127,0,255,255 }
#define ColorGrey { 128,128,128,255 }

/// Colliders
#define ColorDarkBlue { 0,0,153,255 }
#define ColorLightBlue { 102,178,255,255 }
#define ColorDarkRed { 153,0,0,255 }
#define ColorLightRed { 255,102,102,255 }

//  NULL just in case ----------------------

#ifdef NULL
#undef NULL
#endif
#define NULL 0

// Deletes a buffer
#define RELEASE( x ) \
    {                        \
    if( x != NULL )        \
	    {                      \
      delete x;            \
	  x = NULL;              \
	    }                      \
    }

// Deletes an array of buffers
#define RELEASE_ARRAY( x ) \
    {                              \
    if( x != NULL )              \
	    {                            \
      delete[] x;                \
	  x = NULL;                    \
	    }                            \
                              \
    }

#define IN_RANGE( value, min, max ) ( ((value) >= (min) && (value) <= (max)) ? 1 : 0 )
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#define TO_BOOL( a )  ( (a != 0) ? true : false )

typedef unsigned int uint;
typedef unsigned char uchar;

typedef unsigned long long uint64;
typedef unsigned long uint32;

template <class VALUE_TYPE> void SWAP(VALUE_TYPE& a, VALUE_TYPE& b)
{
	VALUE_TYPE tmp = a;
	a = b;
	b = tmp;
}

// Standard string size
#define SHORT_STR	32
#define MID_STR		255
#define HUGE_STR	8192

// Joins a path and file
inline const char* const PATH(const char* folder, const char* file)
{
	static char path[MID_STR];
	sprintf_s(path, MID_STR, "%s/%s", folder, file);
	return path;
}

#endif //__DEFS_H__