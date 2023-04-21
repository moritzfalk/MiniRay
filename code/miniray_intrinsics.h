/* date = September 26th 2022 11:17 am */

#ifndef MINIRAY_INTRINSICS_H

#include <immintrin.h>

/*
NOTE(moritz): Math stuff credit

AVX implementation of sin, cos, sincos, exp and log

   Based on "sse_mathfun.h", by Julien Pommier
   http://gruntthepeon.free.fr/ssemath/

   Copyright (C) 2012 Giovanni Garberoglio
   Interdisciplinary Laboratory for Computational Science (LISC)
   Fondazione Bruno Kessler and University of Trento
   via Sommarive, 18
   I-38123 Trento (Italy)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)

*/

//Intrinsics
typedef __m256  f32_8x;
typedef __m256i s32_8x;
typedef __m128  f32_4x;

struct math_constants
{
	u32 SignMaskU32;
	u32 InvSignMask;
	
	f32_8x _ps256_sign_mask; 
	f32_8x _ps256_inv_sign_mask;
	//TODO(moritz): Here is a divide by PI... be mindful!!!
	f32_8x _ps256_cephes_FOPI;
	s32_8x _pi32_256_1;
	s32_8x _pi32_256_inv1;
	s32_8x _pi32_256_0;
	s32_8x _pi32_256_2;
	s32_8x _pi32_256_4;
	f32_8x _ps256_minus_cephes_DP1;
	f32_8x _ps256_minus_cephes_DP2;
	f32_8x _ps256_minus_cephes_DP3;
	f32_8x _ps256_coscof_p0;
	f32_8x _ps256_coscof_p1;
	f32_8x _ps256_coscof_p2;
	f32_8x _ps256_sincof_p0;
	f32_8x _ps256_sincof_p1;
	f32_8x _ps256_sincof_p2;
	
	f32_8x _ps256_1;
	f32_8x _ps256_0p5;
};



internal void 
sincos256_ps(math_constants *MathConstants, f32_8x Angle, f32_8x *s, f32_8x *c) {
	
	
	//NOTE(moritz): SINCOS constants
	u32 SignMaskU32 = MathConstants->SignMaskU32;
	u32 InvSignMask = MathConstants->InvSignMask;
	
	f32_8x _ps256_sign_mask     = MathConstants->_ps256_sign_mask;
	f32_8x _ps256_inv_sign_mask = MathConstants->_ps256_inv_sign_mask;
	//TODO(moritz): Here is a divide by PI... be mindful!!!
	f32_8x _ps256_cephes_FOPI = MathConstants->_ps256_cephes_FOPI;
	s32_8x _pi32_256_1 = MathConstants->_pi32_256_1;
	s32_8x _pi32_256_inv1 = MathConstants->_pi32_256_inv1;
	s32_8x _pi32_256_0 = MathConstants->_pi32_256_0;
	s32_8x _pi32_256_2 = MathConstants->_pi32_256_2;
	s32_8x _pi32_256_4 = MathConstants->_pi32_256_4;
	f32_8x _ps256_minus_cephes_DP1 = MathConstants->_ps256_minus_cephes_DP1;
	f32_8x _ps256_minus_cephes_DP2 = MathConstants->_ps256_minus_cephes_DP2;
	f32_8x _ps256_minus_cephes_DP3 = MathConstants->_ps256_minus_cephes_DP3;
	f32_8x _ps256_coscof_p0  = MathConstants->_ps256_coscof_p0;
	f32_8x _ps256_coscof_p1  = MathConstants->_ps256_coscof_p1;
	f32_8x _ps256_coscof_p2  = MathConstants->_ps256_coscof_p2;
	f32_8x _ps256_sincof_p0  = MathConstants->_ps256_sincof_p0;
	f32_8x _ps256_sincof_p1  = MathConstants->_ps256_sincof_p1;
	f32_8x _ps256_sincof_p2  = MathConstants->_ps256_sincof_p2;
	
	f32_8x _ps256_1 = MathConstants->_ps256_1;
	f32_8x _ps256_0p5 = MathConstants->_ps256_0p5;
	
	////////
	
	f32_8x xmm1, xmm2, xmm3 = _mm256_setzero_ps(), sign_bit_sin, y;
	s32_8x imm0, imm2, imm4;
	
	sign_bit_sin = Angle;
	/* take the absolute value */
	Angle = _mm256_and_ps(Angle, _ps256_inv_sign_mask);
	/* extract the sign bit (upper one) */
	sign_bit_sin = _mm256_and_ps(sign_bit_sin, _ps256_sign_mask);
	
	//TODO(moritz):Figure out how to remove
	/* scale by 4/Pi */
	//NOTE(moritz): replace with mul by 4
	y = _mm256_mul_ps(Angle, _ps256_cephes_FOPI);
	//y = _mm256_mul_ps(Angle, _mm256_set1_ps(4.0f));
	
	/* store the integer part of y in imm2 */
	imm2 = _mm256_cvttps_epi32(y);
	
	/* j=(j+1) & (~1) (see the cephes sources) */
	imm2 = _mm256_add_epi32(imm2, _pi32_256_1);
	imm2 = _mm256_and_si256(imm2, _pi32_256_inv1);
	
	y = _mm256_cvtepi32_ps(imm2);
	imm4 = imm2;
	
	/* get the swap sign flag for the sine */
	imm0 = _mm256_and_si256(imm2, _pi32_256_4);
	imm0 = _mm256_slli_epi32(imm0, 29);
	//v8sf swap_sign_bit_sin = _mm256_castsi256_ps(imm0);
	
	/* get the polynom selection mask for the sine*/
	imm2 = _mm256_and_si256(imm2,   _pi32_256_2);
	imm2 = _mm256_cmpeq_epi32(imm2, _pi32_256_0);
	//v8sf poly_mask = _mm256_castsi256_ps(imm2);
	
	f32_8x swap_sign_bit_sin = _mm256_castsi256_ps(imm0);
	f32_8x poly_mask = _mm256_castsi256_ps(imm2);
	
	/* The magic pass: "Extended precision modular arithmetic" 
	   x = ((x - y * DP1) - y * DP2) - y * DP3; */
	xmm1 = _ps256_minus_cephes_DP1;
	xmm2 = _ps256_minus_cephes_DP2;
	xmm3 = _ps256_minus_cephes_DP3;
	xmm1 = _mm256_mul_ps(y, xmm1);
	xmm2 = _mm256_mul_ps(y, xmm2);
	xmm3 = _mm256_mul_ps(y, xmm3);
	Angle = _mm256_add_ps(Angle, xmm1);
	Angle = _mm256_add_ps(Angle, xmm2);
	Angle = _mm256_add_ps(Angle, xmm3);
	
	imm4 = _mm256_sub_epi32(imm4, _pi32_256_2);
	imm4 = _mm256_andnot_si256(imm4, _pi32_256_4);
	imm4 = _mm256_slli_epi32(imm4, 29);
	
	f32_8x sign_bit_cos = _mm256_castsi256_ps(imm4);
	
	sign_bit_sin = _mm256_xor_ps(sign_bit_sin, swap_sign_bit_sin);
	
	/* Evaluate the first polynom  (0 <= x <= Pi/4) */
	f32_8x z = _mm256_mul_ps(Angle, Angle);
	y = _ps256_coscof_p0;
	
	y = _mm256_mul_ps(y, z);
	y = _mm256_add_ps(y, _ps256_coscof_p1);
	y = _mm256_mul_ps(y, z);
	y = _mm256_add_ps(y, _ps256_coscof_p2);
	y = _mm256_mul_ps(y, z);
	y = _mm256_mul_ps(y, z);
	f32_8x tmp = _mm256_mul_ps(z, _ps256_0p5);
	y = _mm256_sub_ps(y, tmp);
	y = _mm256_add_ps(y, _ps256_1);
	
	/* Evaluate the second polynom  (Pi/4 <= x <= 0) */
	
	f32_8x y2 = _ps256_sincof_p0;
	y2 = _mm256_mul_ps(y2, z);
	y2 = _mm256_add_ps(y2, _ps256_sincof_p1);
	y2 = _mm256_mul_ps(y2, z);
	y2 = _mm256_add_ps(y2, _ps256_sincof_p2);
	y2 = _mm256_mul_ps(y2, z);
	y2 = _mm256_mul_ps(y2, Angle);
	y2 = _mm256_add_ps(y2, Angle);
	
	/* select the correct result from the two polynoms */  
	xmm3 = poly_mask;
	f32_8x ysin2 = _mm256_and_ps(xmm3, y2);
	f32_8x ysin1 = _mm256_andnot_ps(xmm3, y);
	y2 = _mm256_sub_ps(y2,ysin2);
	y = _mm256_sub_ps(y, ysin1);
	
	xmm1 = _mm256_add_ps(ysin1,ysin2);
	xmm2 = _mm256_add_ps(y,y2);
	
	/* update the sign */
	*s = _mm256_xor_ps(xmm1, sign_bit_sin);
	*c = _mm256_xor_ps(xmm2, sign_bit_cos);
}

inline f32
SquareRoot(f32 Float32)
{
	f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Float32)));
	return(Result);
}

inline f32
Min(f32 A, f32 B)
{
	//TODO(moritz): Use SSE4 instructions instead?
	f32 Result = _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(A), _mm_set_ss(B)));
	return(Result);
}

inline f32
Max(f32 A, f32 B)
{
	f32 Result = _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(A), _mm_set_ss(B)));
	return(Result);
}

inline s32
Min(s32 A, s32 B)
{
	s32 Result = (A <= B) ? A : B;
	return(Result);
}

inline f32
AbsoluteValue(f32 Float32)
{
	u32 MaskU32 = ~(u32)(1 << 31);
	f32_4x Mask = _mm_set1_ps(*(float *)&MaskU32);
	
	f32 Result = _mm_cvtss_f32(_mm_and_ps(_mm_set_ss(Float32), Mask));
	return(Result);
}

inline s32
IntSignOf(f32 Value)
{
	u32 MaskU32 = (u32)(1 << 31);
	f32_4x Mask = _mm_set1_ps(*(float *)&MaskU32);
	
	f32_4x One = _mm_set_ss(1.0f);
	f32_4x SignBit = _mm_and_ps(_mm_set_ss(Value), Mask);
	f32_4x Combined = _mm_or_ps(One, SignBit);
	
	//TODO(moritz): Is this the right cvt instruction?
	s32 Result = _mm_cvtss_si32(Combined);
	
	return(Result);
}

inline f32
FloatSignOf(f32 Value)
{
	u32 MaskU32 = (u32)(1 << 31);
	__m128 Mask = _mm_set1_ps(*(float *)&MaskU32);
	
	__m128 One = _mm_set_ss(1.0f);
	__m128 SignBit = _mm_and_ps(_mm_set_ss(Value), Mask);
	__m128 Combined = _mm_or_ps(One, SignBit);
	
	f32 Result = _mm_cvtss_f32(Combined);
	
	return(Result);
}

inline s32
RoundReal32ToInt32(f32 Float32)
{
	s32 Result = _mm_cvtss_si32(_mm_set_ss(Float32));
	return(Result);
}

inline s32
FloorReal32ToInt32(f32 Float32)
{
	s32 Result = _mm_cvttss_si32(_mm_set_ss(Float32));
	return(Result);
}

inline f32
PowN(f32 Base, s32 Exponent)
{
	if(Exponent == 0)return(1.0f);
	
	Assert(Exponent > 0);
	
	f32 Result = Base;
	for(s32 ExponentCount = 1;
		ExponentCount < Exponent;
		++ExponentCount)
	{
		Result *= Base;
	}
	
	return(Result);
}

inline s32
PowN(s32 Base, s32 Exponent)
{
	if(Exponent == 0)return(1);
	
	Assert(Exponent > 0);
	
	s32 Result = Base;
	for(s32 ExponentCount = 1;
		ExponentCount < Exponent;
		++ExponentCount)
	{
		Result *= Base;
	}
	
	return(Result);
}

inline void
SinCos(math_constants *MathConstants, f32 Angle, f32 *Sin, f32 *Cos)
{
	__m256 SinWide;
	__m256 CosWide;
	__m256 AngleWide = _mm256_set1_ps(Angle);
	
	sincos256_ps(MathConstants, AngleWide, &SinWide, &CosWide);
	
	*Sin = _mm256_cvtss_f32(SinWide);
	*Cos = _mm256_cvtss_f32(CosWide);
}

#define MINIRAY_INTRINSICS_H

#endif //MINIRAY_INTRINSICS_H
