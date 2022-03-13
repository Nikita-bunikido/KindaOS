#pragma     once

#define RES_H   27
#define RES_W   70

#define C_EMPTY (unsigned char)' '
#define C_NAC  (unsigned char)'\0'
#define C_CURSOR (unsigned char)'^'

#define UI_UPLEFT     (unsigned char)0xDAU
#define UI_DOWNLEFT   (unsigned char)0xC0U
#define UI_UPRIGHT    (unsigned char)0xBFU
#define UI_DOWNRIGHT  (unsigned char)0xD9U
#define UI_HORZIONTAL (unsigned char)0xC4U
#define UI_VERTICAL   (unsigned char)0xB3U
#define UI_DELTALEFT  (unsigned char)0xC3U
#define UI_DELTARIGHT (unsigned char)0xB4U
#define UI_DELTAUP    (unsigned char)0xC2U
#define UI_DELTADOWN  (unsigned char)0xC1U

#define in_area(x,l,r) ((x)>=(l) && (x)<=(r))

typedef unsigned char Tarr[RES_H][RES_W+1];