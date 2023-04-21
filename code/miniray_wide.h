/* date = October 22nd 2022 3:05 pm */

#ifndef MINIRAY_WIDE_H

#if (LANE_WIDTH==1)

#define PACKET_HEIGHT  1
#define PACKET_WIDTH   1
#define PACKET_HEIGHTF 1.0f
#define PACKET_WIDTHF  1.0f

typedef f32 wide_f32; 
typedef u32 wide_u32; 
typedef v3  wide_v3;

inline v3 
Extract(wide_v3 *OneOverRayDir, u32 LaneIndex)
{
	v3 Result = *OneOverRayDir;
	return(Result);
}

inline void
SetMaskIndexed(wide_u32 *Mask, u32 Index, u32 Value)
{
	*Mask = Value;
}

inline void
ConditionalAssign(wide_f32 *Dest, wide_u32 Mask, f32 Value)
{
	if(Mask) *Dest = Value;
}

inline void
ConditionalAssign(wide_u32 *Dest, wide_u32 Mask, u32 Value)
{
	if(Mask) *Dest = Value;
}

inline u32
HorizontalAdd(wide_u32 Value)
{
	u32 Result = Value;
	return(Result);
}

#elif (LANE_WIDTH==4)

struct wide_f32
{
	__m128 V;
};

struct wide_u32
{
	__m128i V;
};

struct wide_v3
{
	__m128 x;
	__m128 y;
	__m128 z;
};

internal wide_u32
operator<<(wide_u32 A, u32 Shift)
{
	wide_u32 Result;
	
	Result.V = _mm_slli_epi32(A.V, Shift);
	return(Result);
}

internal wide_u32
operator>>(wide_u32 A, u32 Shift)
{
	wide_u32 Result;
	
	Result.V = _mm_srli_epi32(A.V, Shift);
	return(Result);
}

internal wide_u32
operator^=(wide_u32 &A, wide_u32 B)
{
	A.V = _mm_xor_si128(A.V, B.V);
	
	return(A);
}

internal wide_f32
LaneF32FromU32

inline v3 
Extract(wide_v3 *OneOverRayDir, u32 LaneIndex)
{
	v3 Result = *OneOverRayDir;
	return(Result);
}

inline void
SetMaskIndexed(wide_u32 *Mask, u32 Index, u32 Value)
{
	*Mask = Value;
}

inline void
ConditionalAssign(wide_f32 *Dest, wide_u32 Mask, f32 Value)
{
	if(Mask) *Dest = Value;
}

inline void
ConditionalAssign(wide_u32 *Dest, wide_u32 Mask, u32 Value)
{
	if(Mask) *Dest = Value;
}

inline u32
HorizontalAdd(wide_u32 Value)
{
	u32 Result = Value;
	return(Result);
}

#else
#error LANE_WIDTH must be 1!
#endif

#define MINIRAY_WIDE_H

#endif //MINIRAY_WIDE_H
