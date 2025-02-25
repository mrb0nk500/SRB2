//hq2x filter demo program
//----------------------------------------------------------
//Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#include "filters.h"
#include <math.h>
#if defined (__GNUC__) || defined (__TINYC__)
#include <stdlib.h>
#endif


#if (defined(__GNUC__) && defined(__i386__)) || (defined(_MSC_VER) && defined(_X86_))
#define HQ2XASM
#endif

#ifdef _MSC_VER
//#define HQ2XMMXASM
#endif

static int   LUT16to32[65536];
static int   RGBtoYUV[65536];
#ifdef HQ2XMMXASM
#include "SDL_cpuinfo.h"
static SDL_bool hasMMX = 0;
const Sint64 reg_blank = 0;
const Sint64 const3    = 0x0000000300030003;
const Sint64 const5    = 0x0000000500050005;
const Sint64 const6    = 0x0000000600060006;
const Sint64 const14   = 0x0000000E000E000E;
const Sint64 tr3eshold = 0x0000000000300706;
#endif
static int   YUV1, YUV2;
const  int   Ymask = 0x00FF0000;
const  int   Umask = 0x0000FF00;
const  int   Vmask = 0x000000FF;
const  int   trY   = 0x00300000;
const  int   trU   = 0x00000700;
const  int   trV   = 0x00000006;

FUNCINLINE static ATTRINLINE void Interp1(Uint8 * pc, int c1, int c2)
{
#ifdef HQ2XASM
  //*((int*)pc) = (c1*3+c2)/4;
#if defined (__GNUC__) || defined (__TINYC__)
  int c3 = c1;
  __asm__("shl $2, %1; add %2, %1; sub %3, %1; shr $2, %1":"=d"(*((int*)pc)):"d"(c1),"r"(c2),"r"(c3):"memory");
#else
  __asm
  {
    mov        eax, pc
    mov        edx, c1
    shl        edx, 2
    add        edx, c2
    sub        edx, c1
    shr        edx, 2
    mov        [eax], edx
  }
#endif
#else
  *((int*)pc) = (c1*3+c2) >> 2;
#endif
}

FUNCINLINE static ATTRINLINE void Interp2(Uint8 * pc, int c1, int c2, int c3)
{
#ifdef HQ2XASM
//  *((int*)pc) = (c1*2+c2+c3) >> 2;
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("shl $1, %1; add %2, %1; add %3, %1; shr $2, %1":"=d"(*((int*)pc)):"d"(c1),"r"(c2),"r"(c3):"memory");
#else
  __asm
  {
    mov        eax, pc
    mov        edx, c1
    shl        edx, 1
    add        edx, c2
    add        edx, c3
    shr        edx, 2
    mov        [eax], edx
  }
#endif
#else
  *((int*)pc) = (c1*2+c2+c3) >> 2;
#endif
}

#if 0
static inline void Interp5(Uint8 * pc, int c1, int c2)
{
#ifdef HQ2XASM
  //*((int*)pc) = (c1+c2)/2;
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("add %2, %1; shr $1, %1":"=d"(*((int*)pc)):"d"(c1),"r"(c2):"memory");
#else
  __asm
  {
    mov        eax, pc
    mov        edx, c1
    add        edx, c2
    shr        edx, 1
    mov        [eax], edx
  }
#endif
#else
  *((int*)pc) = (c1+c2) >> 1;
#endif
}
#endif

FUNCINLINE static ATTRINLINE void Interp6(Uint8 * pc, int c1, int c2, int c3)
{
#ifdef HQ2XMMXASM
 //*((int*)pc) = (c1*5+c2*2+c3)/8;
 if(hasMMX)
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("movd %1, %%mm1; movd %2, %%mm2, movd %3, %%mm3; punpcklbw $_reg_blank, %%mm1; punpcklbw $_reg_blank, %%mm2; punpcklbw $_reg_blank, %%mm3; pmullw $_const5, %%mm1; psllw $1, %%mm2; paddw %%mm3, %%mm1; paddw %%mm2, %%mm1; psrlw $3, %%mm1; packuswb $_reg_blank, %%mm1; movd %%mm1, %0" : "=r"(*((int*)pc)) : "r" (c1),"r" (c2),"r" (c3) : "memory");
#else
  __asm
  {
    mov        eax, pc
    movd       mm1, c1
    movd       mm2, c2
    movd       mm3, c3
    punpcklbw  mm1, reg_blank
    punpcklbw  mm2, reg_blank
    punpcklbw  mm3, reg_blank
    pmullw     mm1, const5
    psllw      mm2, 1
    paddw      mm1, mm3
    paddw      mm1, mm2
    psrlw      mm1, 3
    packuswb   mm1, reg_blank
    movd       [eax], mm1
  }
#endif
 else
#endif
  *((int*)pc) = ((((c1 & 0x00FF00)*5 + (c2 & 0x00FF00)*2 + (c3 & 0x00FF00) ) & 0x0007F800) +
                 (((c1 & 0xFF00FF)*5 + (c2 & 0xFF00FF)*2 + (c3 & 0xFF00FF) ) & 0x07F807F8)) >> 3;
}

FUNCINLINE static ATTRINLINE void Interp7(Uint8 * pc, int c1, int c2, int c3)
{
#ifdef HQ2XMMXASM
 //*((int*)pc) = (c1*6+c2+c3)/8;
 if(hasMMX)
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("movd %1, %%mm1; movd %2, %%mm2, movd %3, %%mm3; punpcklbw $_reg_blank, %%mm1; punpcklbw $_reg_blank, %%mm2; punpcklbw $_reg_blank, %%mm3; pmull2 $_const6, %%mm1; padw %%mm3, %%mm2; paddw %%mm2, %%mm1; psrlw $3, %%mm1; packuswb $_reg_blank, %%mm1; movd %%mm1, %0 " : "=r" (*((int*)pc)): "r"(c1), "r"(c2), "r"(c3) : "memory");
#else
  __asm
  {
    mov        eax, pc
    movd       mm1, c1
    movd       mm2, c2
    movd       mm3, c3
    punpcklbw  mm1, reg_blank
    punpcklbw  mm2, reg_blank
    punpcklbw  mm3, reg_blank
    pmullw     mm1, const6
    paddw      mm2, mm3
    paddw      mm1, mm2
    psrlw      mm1, 3
    packuswb   mm1, reg_blank
    movd       [eax], mm1
  }
#endif
 else
#endif
  *((int*)pc) = ((((c1 & 0x00FF00)*6 + (c2 & 0x00FF00) + (c3 & 0x00FF00) ) & 0x0007F800) +
                 (((c1 & 0xFF00FF)*6 + (c2 & 0xFF00FF) + (c3 & 0xFF00FF) ) & 0x07F807F8)) >> 3;
}

FUNCINLINE static ATTRINLINE void Interp9(Uint8 * pc, int c1, int c2, int c3)
{
#ifdef HQ2XMMXASM
 //*((int*)pc) = (c1*2+(c2+c3)*3)/8;
 if(hasMMX)
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("movd %1, %%mm1; movd %2, %%mm2, movd %3, %%mm3; punpcklbw $_reg_blank, %%mm1; punpcklbw $_reg_blank, %%mm2; punpcklbw $_reg_blank, %%mm3; psllw $1, %%mm1; paddw %%mm3, %%mm2; pmullw $_const3, %%mm2; padw %%mm2, %%mm1; psrlw $3, %%mm1; packuswb $_reg_blank, %%mm1; movd %%mm1, %0;" : "=r"(*((int*)pc)) : "r" (c1),"r" (c2),"r" (c3) : "memory");
#else
  __asm
  {
    mov        eax, pc
    movd       mm1, c1
    movd       mm2, c2
    movd       mm3, c3
    punpcklbw  mm1, reg_blank
    punpcklbw  mm2, reg_blank
    punpcklbw  mm3, reg_blank
    psllw      mm1, 1
    paddw      mm2, mm3
    pmullw     mm2, const3
    paddw      mm1, mm2
    psrlw      mm1, 3
    packuswb   mm1, reg_blank
    movd       [eax], mm1
  }
#endif
 else
#endif
  *((int*)pc) = ((((c1 & 0x00FF00)*2 + ((c2 & 0x00FF00) + (c3 & 0x00FF00))*3 ) & 0x0007F800) +
                 (((c1 & 0xFF00FF)*2 + ((c2 & 0xFF00FF) + (c3 & 0xFF00FF))*3 ) & 0x07F807F8)) >> 3;
}

FUNCINLINE static ATTRINLINE void Interp10(Uint8 * pc, int c1, int c2, int c3)
{
#ifdef HQ2XMMXASM
 //*((int*)pc) = (c1*14+c2+c3)/16;
 if(hasMMX)
#if defined (__GNUC__) || defined (__TINYC__)
  __asm__("movd %1, %%mm1; movd %2, %%mm2, movd %3, %%mm3; punpcklbw $_reg_blank, %%mm1; punpcklbw $_reg_blank, %%mm2; punpcklbw $_reg_blank, %%mm3; pmullw $_const14, %%mm1; paddw %%mm3, %%mm2; paddw %%mm2, %%mm1; psrlw $4, %%mm1; packuswb $_req_blank, %%mm1; movd %%mm1, %0;" : "=r"(*((int*)pc)) : "r" (c1),"r" (c2),"r" (c3) : "memory");
#else
  __asm
  {
    mov        eax, pc
    movd       mm1, c1
    movd       mm2, c2
    movd       mm3, c3
    punpcklbw  mm1, reg_blank
    punpcklbw  mm2, reg_blank
    punpcklbw  mm3, reg_blank
    pmullw     mm1, const14
    paddw      mm2, mm3
    paddw      mm1, mm2
    psrlw      mm1, 4
    packuswb   mm1, reg_blank
    movd       [eax], mm1
  }
#endif
 else
#endif
  *((int*)pc) = ((((c1 & 0x00FF00)*14 + (c2 & 0x00FF00) + (c3 & 0x00FF00) ) & 0x000FF000) +
                 (((c1 & 0xFF00FF)*14 + (c2 & 0xFF00FF) + (c3 & 0xFF00FF) ) & 0x0FF00FF0)) >> 4;
}
#define PIXEL00_0     *((int*)(pOut)) = c[5];
#define PIXEL00_10    Interp1(pOut, c[5], c[1]);
#define PIXEL00_11    Interp1(pOut, c[5], c[4]);
#define PIXEL00_12    Interp1(pOut, c[5], c[2]);
#define PIXEL00_20    Interp2(pOut, c[5], c[4], c[2]);
#define PIXEL00_21    Interp2(pOut, c[5], c[1], c[2]);
#define PIXEL00_22    Interp2(pOut, c[5], c[1], c[4]);
#define PIXEL00_60    Interp6(pOut, c[5], c[2], c[4]);
#define PIXEL00_61    Interp6(pOut, c[5], c[4], c[2]);
#define PIXEL00_70    Interp7(pOut, c[5], c[4], c[2]);
#define PIXEL00_90    Interp9(pOut, c[5], c[4], c[2]);
#define PIXEL00_100   Interp10(pOut, c[5], c[4], c[2]);
#define PIXEL01_0     *((int*)(pOut+4)) = c[5];
#define PIXEL01_10    Interp1(pOut+4, c[5], c[3]);
#define PIXEL01_11    Interp1(pOut+4, c[5], c[2]);
#define PIXEL01_12    Interp1(pOut+4, c[5], c[6]);
#define PIXEL01_20    Interp2(pOut+4, c[5], c[2], c[6]);
#define PIXEL01_21    Interp2(pOut+4, c[5], c[3], c[6]);
#define PIXEL01_22    Interp2(pOut+4, c[5], c[3], c[2]);
#define PIXEL01_60    Interp6(pOut+4, c[5], c[6], c[2]);
#define PIXEL01_61    Interp6(pOut+4, c[5], c[2], c[6]);
#define PIXEL01_70    Interp7(pOut+4, c[5], c[2], c[6]);
#define PIXEL01_90    Interp9(pOut+4, c[5], c[2], c[6]);
#define PIXEL01_100   Interp10(pOut+4, c[5], c[2], c[6]);
#define PIXEL10_0     *((int*)(pOut+BpL)) = c[5];
#define PIXEL10_10    Interp1(pOut+BpL, c[5], c[7]);
#define PIXEL10_11    Interp1(pOut+BpL, c[5], c[8]);
#define PIXEL10_12    Interp1(pOut+BpL, c[5], c[4]);
#define PIXEL10_20    Interp2(pOut+BpL, c[5], c[8], c[4]);
#define PIXEL10_21    Interp2(pOut+BpL, c[5], c[7], c[4]);
#define PIXEL10_22    Interp2(pOut+BpL, c[5], c[7], c[8]);
#define PIXEL10_60    Interp6(pOut+BpL, c[5], c[4], c[8]);
#define PIXEL10_61    Interp6(pOut+BpL, c[5], c[8], c[4]);
#define PIXEL10_70    Interp7(pOut+BpL, c[5], c[8], c[4]);
#define PIXEL10_90    Interp9(pOut+BpL, c[5], c[8], c[4]);
#define PIXEL10_100   Interp10(pOut+BpL, c[5], c[8], c[4]);
#define PIXEL11_0     *((int*)(pOut+BpL+4)) = c[5];
#define PIXEL11_10    Interp1(pOut+BpL+4, c[5], c[9]);
#define PIXEL11_11    Interp1(pOut+BpL+4, c[5], c[6]);
#define PIXEL11_12    Interp1(pOut+BpL+4, c[5], c[8]);
#define PIXEL11_20    Interp2(pOut+BpL+4, c[5], c[6], c[8]);
#define PIXEL11_21    Interp2(pOut+BpL+4, c[5], c[9], c[8]);
#define PIXEL11_22    Interp2(pOut+BpL+4, c[5], c[9], c[6]);
#define PIXEL11_60    Interp6(pOut+BpL+4, c[5], c[8], c[6]);
#define PIXEL11_61    Interp6(pOut+BpL+4, c[5], c[6], c[8]);
#define PIXEL11_70    Interp7(pOut+BpL+4, c[5], c[6], c[8]);
#define PIXEL11_90    Interp9(pOut+BpL+4, c[5], c[6], c[8]);
#define PIXEL11_100   Interp10(pOut+BpL+4, c[5], c[6], c[8]);

#ifdef _MSC_VER
#pragma warning(disable: 4035)
#endif

FUNCINLINE static ATTRINLINE int Diff(Uint32 w1, Uint32 w2)
{
#ifdef HQ2XMMXASM
 if(hasMMX)
 {
#if defined (__GNUC__) || defined (__TINYC__)
  int diffresult = 0;
  if(w1 != w2)
   __asm__("movd %3+%1*4, %%mm1; movq %%mm1, %%mm5; movd %3+%2*4, %%mm2; psubusb %%mm2, %%mm1; psubusb %%mm5, %%mm2; por %%mm2, %%mm1; psubusb $_treshold, %%mm1; movd %%mm1, %0" : "=c" (diffresult):"d" (w1),"q" (w2),"c" (RGBtoYUV) : "memory");
  return diffresult;
#else
  __asm
  {
    xor     eax,eax
    mov     ebx,w1
    mov     edx,w2
    cmp     ebx,edx
    je      FIN
    mov     ecx,offset RGBtoYUV
    movd    mm1,[ecx + ebx*4]
    movq    mm5,mm1
    movd    mm2,[ecx + edx*4]
    psubusb mm1,mm2
    psubusb mm2,mm5
    por     mm1,mm2
    psubusb mm1,treshold
    movd    eax,mm1
FIN:
  }// returns result in eax register
#endif
 }
 else
#endif
 {
  YUV1 = RGBtoYUV[w1];
  YUV2 = RGBtoYUV[w2];
  return ( ( abs((YUV1 & Ymask) - (YUV2 & Ymask)) > trY ) ||
           ( abs((YUV1 & Umask) - (YUV2 & Umask)) > trU ) ||
           ( abs((YUV1 & Vmask) - (YUV2 & Vmask)) > trV ) );
 }
}


#ifdef _MSC_VER
#pragma warning(default: 4035)
#endif


static void hq2x_32( Uint8 * pIn, Uint8 * pOut, int Xres, int Yres, int BpL )
{
  int  i, j, k;
  int  prevline, nextline;
  int  w[10];
  int  c[10];

  //   +----+----+----+
  //   |    |    |    |
  //   | w1 | w2 | w3 |
  //   +----+----+----+
  //   |    |    |    |
  //   | w4 | w5 | w6 |
  //   +----+----+----+
  //   |    |    |    |
  //   | w7 | w8 | w9 |
  //   +----+----+----+

  for (j=0; j<Yres; j++)
  {
    if (j>0)      prevline = -Xres*2; else prevline = 0;
    if (j<Yres-1) nextline =  Xres*2; else nextline = 0;

    for (i=0; i<Xres; i++)
    {
      int pattern = 0;
      int flag = 1;

      w[2] = *((Uint16*)(pIn + prevline));
      w[5] = *((Uint16*)pIn);
      w[8] = *((Uint16*)(pIn + nextline));

      if (i>0)
      {
        w[1] = *((Uint16*)(pIn + prevline - 2));
        w[4] = *((Uint16*)(pIn - 2));
        w[7] = *((Uint16*)(pIn + nextline - 2));
      }
      else
      {
        w[1] = w[2];
        w[4] = w[5];
        w[7] = w[8];
      }

      if (i<Xres-1)
      {
        w[3] = *((Uint16*)(pIn + prevline + 2));
        w[6] = *((Uint16*)(pIn + 2));
        w[9] = *((Uint16*)(pIn + nextline + 2));
      }
      else
      {
        w[3] = w[2];
        w[6] = w[5];
        w[9] = w[8];
      }

#ifdef HQ2XMMXASM
      if(hasMMX)
      {
        if ( Diff(w[5],w[1]) ) pattern |= 0x0001;
        if ( Diff(w[5],w[2]) ) pattern |= 0x0002;
        if ( Diff(w[5],w[3]) ) pattern |= 0x0004;
        if ( Diff(w[5],w[4]) ) pattern |= 0x0008;
        if ( Diff(w[5],w[6]) ) pattern |= 0x0010;
        if ( Diff(w[5],w[7]) ) pattern |= 0x0020;
        if ( Diff(w[5],w[8]) ) pattern |= 0x0040;
        if ( Diff(w[5],w[9]) ) pattern |= 0x0080;
      }
      else
#endif
      {
        YUV1 = RGBtoYUV[w[5]];

        for (k=1; k<=9; k++)
        {
          if (k==5) continue;

          if ( w[k] != w[5] )
          {
            YUV2 = RGBtoYUV[w[k]];
            if ( ( abs((YUV1 & Ymask) - (YUV2 & Ymask)) > trY ) ||
                 ( abs((YUV1 & Umask) - (YUV2 & Umask)) > trU ) ||
                 ( abs((YUV1 & Vmask) - (YUV2 & Vmask)) > trV ) )
              pattern |= flag;
          }
          flag <<= 1;
        }
      }

      for (k=1; k<=9; k++)
        c[k] = LUT16to32[w[k]];

      switch (pattern)
      {
        case 0:
        case 1:
        case 4:
        case 32:
        case 128:
        case 5:
        case 132:
        case 160:
        case 33:
        case 129:
        case 36:
        case 133:
        case 164:
        case 161:
        case 37:
        case 165:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 2:
        case 34:
        case 130:
        case 162:
        {
          PIXEL00_22
          PIXEL01_21
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 16:
        case 17:
        case 48:
        case 49:
        {
          PIXEL00_20
          PIXEL01_22
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 64:
        case 65:
        case 68:
        case 69:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_21
          PIXEL11_22
          break;
        }
        case 8:
        case 12:
        case 136:
        case 140:
        {
          PIXEL00_21
          PIXEL01_20
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 3:
        case 35:
        case 131:
        case 163:
        {
          PIXEL00_11
          PIXEL01_21
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 6:
        case 38:
        case 134:
        case 166:
        {
          PIXEL00_22
          PIXEL01_12
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 20:
        case 21:
        case 52:
        case 53:
        {
          PIXEL00_20
          PIXEL01_11
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 144:
        case 145:
        case 176:
        case 177:
        {
          PIXEL00_20
          PIXEL01_22
          PIXEL10_20
          PIXEL11_12
          break;
        }
        case 192:
        case 193:
        case 196:
        case 197:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_21
          PIXEL11_11
          break;
        }
        case 96:
        case 97:
        case 100:
        case 101:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_12
          PIXEL11_22
          break;
        }
        case 40:
        case 44:
        case 168:
        case 172:
        {
          PIXEL00_21
          PIXEL01_20
          PIXEL10_11
          PIXEL11_20
          break;
        }
        case 9:
        case 13:
        case 137:
        case 141:
        {
          PIXEL00_12
          PIXEL01_20
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 18:
        case 50:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 80:
        case 81:
        {
          PIXEL00_20
          PIXEL01_22
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 72:
        case 76:
        {
          PIXEL00_21
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 10:
        case 138:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 66:
        {
          PIXEL00_22
          PIXEL01_21
          PIXEL10_21
          PIXEL11_22
          break;
        }
        case 24:
        {
          PIXEL00_21
          PIXEL01_22
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 7:
        case 39:
        case 135:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 148:
        case 149:
        case 180:
        {
          PIXEL00_20
          PIXEL01_11
          PIXEL10_20
          PIXEL11_12
          break;
        }
        case 224:
        case 228:
        case 225:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 41:
        case 169:
        case 45:
        {
          PIXEL00_12
          PIXEL01_20
          PIXEL10_11
          PIXEL11_20
          break;
        }
        case 22:
        case 54:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 208:
        case 209:
        {
          PIXEL00_20
          PIXEL01_22
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 104:
        case 108:
        {
          PIXEL00_21
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 11:
        case 139:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 19:
        case 51:
        {
          if (Diff(w[2], w[6]))
          {
            PIXEL00_11
            PIXEL01_10
          }
          else
          {
            PIXEL00_60
            PIXEL01_90
          }
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 146:
        case 178:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
            PIXEL11_12
          }
          else
          {
            PIXEL01_90
            PIXEL11_61
          }
          PIXEL10_20
          break;
        }
        case 84:
        case 85:
        {
          PIXEL00_20
          if (Diff(w[6], w[8]))
          {
            PIXEL01_11
            PIXEL11_10
          }
          else
          {
            PIXEL01_60
            PIXEL11_90
          }
          PIXEL10_21
          break;
        }
        case 112:
        case 113:
        {
          PIXEL00_20
          PIXEL01_22
          if (Diff(w[6], w[8]))
          {
            PIXEL10_12
            PIXEL11_10
          }
          else
          {
            PIXEL10_61
            PIXEL11_90
          }
          break;
        }
        case 200:
        case 204:
        {
          PIXEL00_21
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
            PIXEL11_11
          }
          else
          {
            PIXEL10_90
            PIXEL11_60
          }
          break;
        }
        case 73:
        case 77:
        {
          if (Diff(w[8], w[4]))
          {
            PIXEL00_12
            PIXEL10_10
          }
          else
          {
            PIXEL00_61
            PIXEL10_90
          }
          PIXEL01_20
          PIXEL11_22
          break;
        }
        case 42:
        case 170:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
            PIXEL10_11
          }
          else
          {
            PIXEL00_90
            PIXEL10_60
          }
          PIXEL01_21
          PIXEL11_20
          break;
        }
        case 14:
        case 142:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
            PIXEL01_12
          }
          else
          {
            PIXEL00_90
            PIXEL01_61
          }
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 67:
        {
          PIXEL00_11
          PIXEL01_21
          PIXEL10_21
          PIXEL11_22
          break;
        }
        case 70:
        {
          PIXEL00_22
          PIXEL01_12
          PIXEL10_21
          PIXEL11_22
          break;
        }
        case 28:
        {
          PIXEL00_21
          PIXEL01_11
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 152:
        {
          PIXEL00_21
          PIXEL01_22
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 194:
        {
          PIXEL00_22
          PIXEL01_21
          PIXEL10_21
          PIXEL11_11
          break;
        }
        case 98:
        {
          PIXEL00_22
          PIXEL01_21
          PIXEL10_12
          PIXEL11_22
          break;
        }
        case 56:
        {
          PIXEL00_21
          PIXEL01_22
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 25:
        {
          PIXEL00_12
          PIXEL01_22
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 26:
        case 31:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 82:
        case 214:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 88:
        case 248:
        {
          PIXEL00_21
          PIXEL01_22
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 74:
        case 107:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 27:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_10
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 86:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_21
          PIXEL11_10
          break;
        }
        case 216:
        {
          PIXEL00_21
          PIXEL01_22
          PIXEL10_10
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 106:
        {
          PIXEL00_10
          PIXEL01_21
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 30:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 210:
        {
          PIXEL00_22
          PIXEL01_10
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 120:
        {
          PIXEL00_21
          PIXEL01_22
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_10
          break;
        }
        case 75:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          PIXEL10_10
          PIXEL11_22
          break;
        }
        case 29:
        {
          PIXEL00_12
          PIXEL01_11
          PIXEL10_22
          PIXEL11_21
          break;
        }
        case 198:
        {
          PIXEL00_22
          PIXEL01_12
          PIXEL10_21
          PIXEL11_11
          break;
        }
        case 184:
        {
          PIXEL00_21
          PIXEL01_22
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 99:
        {
          PIXEL00_11
          PIXEL01_21
          PIXEL10_12
          PIXEL11_22
          break;
        }
        case 57:
        {
          PIXEL00_12
          PIXEL01_22
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 71:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_21
          PIXEL11_22
          break;
        }
        case 156:
        {
          PIXEL00_21
          PIXEL01_11
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 226:
        {
          PIXEL00_22
          PIXEL01_21
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 60:
        {
          PIXEL00_21
          PIXEL01_11
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 195:
        {
          PIXEL00_11
          PIXEL01_21
          PIXEL10_21
          PIXEL11_11
          break;
        }
        case 102:
        {
          PIXEL00_22
          PIXEL01_12
          PIXEL10_12
          PIXEL11_22
          break;
        }
        case 153:
        {
          PIXEL00_12
          PIXEL01_22
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 58:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 83:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 92:
        {
          PIXEL00_21
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 202:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          PIXEL01_21
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          PIXEL11_11
          break;
        }
        case 78:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          PIXEL11_22
          break;
        }
        case 154:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 114:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 89:
        {
          PIXEL00_12
          PIXEL01_22
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 90:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 55:
        case 23:
        {
          if (Diff(w[2], w[6]))
          {
            PIXEL00_11
            PIXEL01_0
          }
          else
          {
            PIXEL00_60
            PIXEL01_90
          }
          PIXEL10_20
          PIXEL11_21
          break;
        }
        case 182:
        case 150:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
            PIXEL11_12
          }
          else
          {
            PIXEL01_90
            PIXEL11_61
          }
          PIXEL10_20
          break;
        }
        case 213:
        case 212:
        {
          PIXEL00_20
          if (Diff(w[6], w[8]))
          {
            PIXEL01_11
            PIXEL11_0
          }
          else
          {
            PIXEL01_60
            PIXEL11_90
          }
          PIXEL10_21
          break;
        }
        case 241:
        case 240:
        {
          PIXEL00_20
          PIXEL01_22
          if (Diff(w[6], w[8]))
          {
            PIXEL10_12
            PIXEL11_0
          }
          else
          {
            PIXEL10_61
            PIXEL11_90
          }
          break;
        }
        case 236:
        case 232:
        {
          PIXEL00_21
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
            PIXEL11_11
          }
          else
          {
            PIXEL10_90
            PIXEL11_60
          }
          break;
        }
        case 109:
        case 105:
        {
          if (Diff(w[8], w[4]))
          {
            PIXEL00_12
            PIXEL10_0
          }
          else
          {
            PIXEL00_61
            PIXEL10_90
          }
          PIXEL01_20
          PIXEL11_22
          break;
        }
        case 171:
        case 43:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
            PIXEL10_11
          }
          else
          {
            PIXEL00_90
            PIXEL10_60
          }
          PIXEL01_21
          PIXEL11_20
          break;
        }
        case 143:
        case 15:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
            PIXEL01_12
          }
          else
          {
            PIXEL00_90
            PIXEL01_61
          }
          PIXEL10_22
          PIXEL11_20
          break;
        }
        case 124:
        {
          PIXEL00_21
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_10
          break;
        }
        case 203:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          PIXEL10_10
          PIXEL11_11
          break;
        }
        case 62:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 211:
        {
          PIXEL00_11
          PIXEL01_10
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 118:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_12
          PIXEL11_10
          break;
        }
        case 217:
        {
          PIXEL00_12
          PIXEL01_22
          PIXEL10_10
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 110:
        {
          PIXEL00_10
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 155:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_10
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 188:
        {
          PIXEL00_21
          PIXEL01_11
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 185:
        {
          PIXEL00_12
          PIXEL01_22
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 61:
        {
          PIXEL00_12
          PIXEL01_11
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 157:
        {
          PIXEL00_12
          PIXEL01_11
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 103:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_12
          PIXEL11_22
          break;
        }
        case 227:
        {
          PIXEL00_11
          PIXEL01_21
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 230:
        {
          PIXEL00_22
          PIXEL01_12
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 199:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_21
          PIXEL11_11
          break;
        }
        case 220:
        {
          PIXEL00_21
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 158:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 234:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          PIXEL01_21
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_11
          break;
        }
        case 242:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 59:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 121:
        {
          PIXEL00_12
          PIXEL01_22
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 87:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 79:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          PIXEL11_22
          break;
        }
        case 122:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 94:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 218:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 91:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 229:
        {
          PIXEL00_20
          PIXEL01_20
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 167:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_20
          PIXEL11_20
          break;
        }
        case 173:
        {
          PIXEL00_12
          PIXEL01_20
          PIXEL10_11
          PIXEL11_20
          break;
        }
        case 181:
        {
          PIXEL00_20
          PIXEL01_11
          PIXEL10_20
          PIXEL11_12
          break;
        }
        case 186:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 115:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 93:
        {
          PIXEL00_12
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 206:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          PIXEL11_11
          break;
        }
        case 205:
        case 201:
        {
          PIXEL00_12
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_10
          }
          else
          {
            PIXEL10_70
          }
          PIXEL11_11
          break;
        }
        case 174:
        case 46:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_10
          }
          else
          {
            PIXEL00_70
          }
          PIXEL01_12
          PIXEL10_11
          PIXEL11_20
          break;
        }
        case 179:
        case 147:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_10
          }
          else
          {
            PIXEL01_70
          }
          PIXEL10_20
          PIXEL11_12
          break;
        }
        case 117:
        case 116:
        {
          PIXEL00_20
          PIXEL01_11
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_10
          }
          else
          {
            PIXEL11_70
          }
          break;
        }
        case 189:
        {
          PIXEL00_12
          PIXEL01_11
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 231:
        {
          PIXEL00_11
          PIXEL01_12
          PIXEL10_12
          PIXEL11_11
          break;
        }
        case 126:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_10
          break;
        }
        case 219:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_10
          PIXEL10_10
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 125:
        {
          if (Diff(w[8], w[4]))
          {
            PIXEL00_12
            PIXEL10_0
          }
          else
          {
            PIXEL00_61
            PIXEL10_90
          }
          PIXEL01_11
          PIXEL11_10
          break;
        }
        case 221:
        {
          PIXEL00_12
          if (Diff(w[6], w[8]))
          {
            PIXEL01_11
            PIXEL11_0
          }
          else
          {
            PIXEL01_60
            PIXEL11_90
          }
          PIXEL10_10
          break;
        }
        case 207:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
            PIXEL01_12
          }
          else
          {
            PIXEL00_90
            PIXEL01_61
          }
          PIXEL10_10
          PIXEL11_11
          break;
        }
        case 238:
        {
          PIXEL00_10
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
            PIXEL11_11
          }
          else
          {
            PIXEL10_90
            PIXEL11_60
          }
          break;
        }
        case 190:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
            PIXEL11_12
          }
          else
          {
            PIXEL01_90
            PIXEL11_61
          }
          PIXEL10_11
          break;
        }
        case 187:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
            PIXEL10_11
          }
          else
          {
            PIXEL00_90
            PIXEL10_60
          }
          PIXEL01_10
          PIXEL11_12
          break;
        }
        case 243:
        {
          PIXEL00_11
          PIXEL01_10
          if (Diff(w[6], w[8]))
          {
            PIXEL10_12
            PIXEL11_0
          }
          else
          {
            PIXEL10_61
            PIXEL11_90
          }
          break;
        }
        case 119:
        {
          if (Diff(w[2], w[6]))
          {
            PIXEL00_11
            PIXEL01_0
          }
          else
          {
            PIXEL00_60
            PIXEL01_90
          }
          PIXEL10_12
          PIXEL11_10
          break;
        }
        case 237:
        case 233:
        {
          PIXEL00_12
          PIXEL01_20
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          PIXEL11_11
          break;
        }
        case 175:
        case 47:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          PIXEL01_12
          PIXEL10_11
          PIXEL11_20
          break;
        }
        case 183:
        case 151:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_20
          PIXEL11_12
          break;
        }
        case 245:
        case 244:
        {
          PIXEL00_20
          PIXEL01_11
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 250:
        {
          PIXEL00_10
          PIXEL01_10
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 123:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_10
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_10
          break;
        }
        case 95:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_10
          PIXEL11_10
          break;
        }
        case 222:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_10
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 252:
        {
          PIXEL00_21
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 249:
        {
          PIXEL00_12
          PIXEL01_22
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 235:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_21
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          PIXEL11_11
          break;
        }
        case 111:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_22
          break;
        }
        case 63:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_11
          PIXEL11_21
          break;
        }
        case 159:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_22
          PIXEL11_12
          break;
        }
        case 215:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_21
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 246:
        {
          PIXEL00_22
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 254:
        {
          PIXEL00_10
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 253:
        {
          PIXEL00_12
          PIXEL01_11
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 251:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          PIXEL01_10
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 239:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          PIXEL01_12
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          PIXEL11_11
          break;
        }
        case 127:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_20
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_20
          }
          PIXEL11_10
          break;
        }
        case 191:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_11
          PIXEL11_12
          break;
        }
        case 223:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_20
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_10
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_20
          }
          break;
        }
        case 247:
        {
          PIXEL00_11
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          PIXEL10_12
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
        case 255:
        {
          if (Diff(w[4], w[2]))
          {
            PIXEL00_0
          }
          else
          {
            PIXEL00_100
          }
          if (Diff(w[2], w[6]))
          {
            PIXEL01_0
          }
          else
          {
            PIXEL01_100
          }
          if (Diff(w[8], w[4]))
          {
            PIXEL10_0
          }
          else
          {
            PIXEL10_100
          }
          if (Diff(w[6], w[8]))
          {
            PIXEL11_0
          }
          else
          {
            PIXEL11_100
          }
          break;
        }
      }
      pIn+=2;
      pOut+=8;
    }
    pOut+=BpL;
  }
}

FUNCINLINE static ATTRINLINE void InitLUTs(void)
{
  int i, j, k, r, g, b, Y, u, v;

#ifdef HQ2XMMXASM
  hasMMX = SDL_HasMMX();
#endif

  for (i=0; i<65536; i++)
    LUT16to32[i] = ((i & 0xF800) << 8) + ((i & 0x07E0) << 5) + ((i & 0x001F) << 3);

  for (i=0; i<32; i++)
  for (j=0; j<64; j++)
  for (k=0; k<32; k++)
  {
    r = i << 3;
    g = j << 2;
    b = k << 3;
    Y = (r + g + b) >> 2;
    u = 128 + ((r - b) >> 2);
    v = 128 + ((-r + 2*g -b)>>3);
    RGBtoYUV[ (i << 11) + (j << 5) + k ] = (Y<<16) + (u<<8) + v;
  }
}

void filter_hq2x(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height)
{
	static Uint8 doneLUT = 1;
	(void)srcPitch;
	if(doneLUT) InitLUTs();
	else doneLUT = 0;
	hq2x_32( srcPtr, dstPtr, width, height, dstPitch );
}
