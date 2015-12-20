/*
 *  This file is part of Veselin Georgiev's embedded common routines
 *  (c) 2014, Veselin Georgiev. Licensed under GPL v2.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

/* basic segments: */
#ifndef SEGMENTS_DEFINED
#define mA        1
#define mB        2
#define mC        4
#define mD        8
#define mE       16
#define mF       32
#define mG       64
#define mDOT    128
#define mEMPTY 0x00
#define mFULL  0xff
#endif

/*numbers:*/
#define num0	(mA|mB|mC|mD|mE|mF)
#define num1	(mB|mC)
#define num2	(mA|mB|mG|mE|mD)
#define num3	(mA|mB|mG|mC|mD)
#define num4	(mF|mB|mG|mC)
#define num5	(mA|mF|mG|mC|mD)
#define num6	(mA|mF|mG|mC|mD|mE)
#define num7	(mA|mB|mC)
#define num8	(mA|mB|mC|mD|mE|mF|mG)
#define num9	(mA|mB|mC|mD|mF|mG)


#define cA (mA|mB|mC|mE|mF|mG)
#define cB (mC|mD|mE|mF|mG)
#define cC (mG|mE|mD)
#define cD (mB|mC|mD|mE|mG)
#define cE (mA|mD|mE|mF|mG)
#define cF (mA|mE|mF|mG)
#define cG (mA|mC|mD|mE|mF)
#define cH (mC|mE|mF|mG)
#define cI (mB|mC)
#define cL (mD|mE|mF)
#define cN (mC|mE|mG)
#define cO num0
#define cP (mA|mE|mB|mE|mF|mG)
#define cR (mE|mG)
#define cS (mA|mC|mD|mF|mG)
#define cT (mD|mE|mF|mG)
#define cU (mC|mD|mE)
#define cV cU

#define c_o (mC|mD|mE|mG)
#define c_h (mC|mE|mF|mG)

#define cDASH mG

#endif // __CHARACTERS_H__
