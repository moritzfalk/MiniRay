/* date = October 23rd 2020 6:41 pm */

#ifndef HANDMADE_MATH_H

union v2
{
    struct
    {
        r32 x, y;
    };
    struct
    {
        r32 Width, Height;
    };
    r32 E[2];
};

union v2i
{
	struct
	{
		i32 x, y;
	};
	struct
	{
		i32 Width, Height;
	};
	i32 E[2];
};

union v3
{
    struct
    {
        union
        {
            v2 xy;
            struct
            {
                r32 x, y;
            };
        };
        r32 z;
    };
    struct
    {
        union
        {
            v2 rg;
            struct
            {
                r32 r, g;
            };
        };
        r32 b;
    };
    r32 E[3];
};

union v3i
{
    struct
    {
        union
        {
            v2i xy;
            struct
            {
                i32 x, y;
            };
        };
        i32 z;
    };
    struct
    {
        union
        {
            v2i rg;
            struct
            {
                i32 r, g;
            };
        };
        i32 b;
    };
    i32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                r32 x, y, z;
            };
        };
        r32  w;
    };
    
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                r32 r, g, b;
            };
        };
        r32 a;
    };
    
    struct
    {
        v2 xy;
        r32 Ignored0_;
        r32 Ignored1_;
    };
    
    struct
    {
        r32 Ignored2_;
        v2 yz;
        r32 Ignored3_;
    };
    
    struct
    {
        r32 Ignored4_;
        r32 Ignored5_;
        v2 zw;
    };
    
    r32 E[4];
};

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.x = X;
    Result.y = Y;
    
    return(Result);
}

inline v2i
V2i(i32 X, i32 Y)
{
	v2i Result;
	Result.x = X;
	Result.y = Y;
	
	return(Result);
}

inline v2
V2iToV2(i32 X, i32 Y)
{
    v2 Result;
    Result.x = (r32)X;
    Result.y = (r32)Y;
    
    return(Result);
}

inline v2
V2iToV2(v2i A)
{
	v2 Result;
	Result.x = (r32)A.x;
	Result.y = (r32)A.y;
	
	return(Result);
}

inline v2i
FloorV2ToV2i(r32 X, r32 Y)
{
	v2i Result;
	Result.x = FloorReal32ToInt32(X);
	Result.y = FloorReal32ToInt32(Y);
	
	return(Result);
}

inline v2i
FloorV2ToV2i(v2 A)
{
	v2i Result;
	Result.x = FloorReal32ToInt32(A.x);
	Result.y = FloorReal32ToInt32(A.y);
	
	return(Result);
}

inline v2i
RoundV2ToV2i(v2 A)
{
	v2i Result;
	Result.x = RoundReal32ToInt32(A.x);
	Result.y = RoundReal32ToInt32(A.y);
	
	return(Result);
}

inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;
    
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    
    return(Result);
}

inline v3
V3(v2 XY, r32 Z)
{
	v3 Result;
	
	Result.x = XY.x;
	Result.y = XY.y;
	Result.z = Z;
	
	return(Result);
}

inline v3i
V3i(i32 X, i32 Y, i32 Z)
{
	v3i Result;
	Result.x = X;
	Result.y = Y;
	Result.z = Z;
	
	return(Result);
}

inline v3i
V3i(v2i XY, i32 Z)
{
	v3i Result;
	Result.x = XY.x;
	Result.y = XY.y;
	Result.z = Z;
	
	return(Result);
}

inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
    v4 Result;
    
    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;
    
    return(Result);
}

inline v3
ToV3(v2 XY, r32 Z)
{
    v3 Result;
    
    Result.xy = XY;
    Result.z = Z;
    
    return(Result);
}


inline v4
ToV4(v3 XYZ, r32 W)
{
    v4 Result;
    
    Result.xyz = XYZ;
    Result.w = W;
    
    return(Result);
}

//
// NOTE(casey): scalar operations
//

inline r32
Square(r32 A)
{
    r32 Result = A*A;
    
    return(Result);
}

inline r32
Lerp(r32 A, r32 t, r32 B)
{
    r32 Result = (1.0f - t)*A + t*B;
    
    return(Result);
}

inline u32
Clamp(u32 Min, u32 Value, u32 Max)
{
	u32 Result = Value;
	
	if(Result < Min)
	{
		Result = Min;
	}
	else if(Result > Max)
	{
		Result = Max;
	}
	
	return(Result);
}

inline r32
Clamp(r32 Min, r32 Value, r32 Max)
{
    r32 Result = Value;
    
    if(Result < Min)
    {
        Result = Min;
    }
    else if(Result > Max)
    {
        Result = Max;
    }
    
    return(Result);
}

inline r32
Clamp01(r32 Value)
{
    r32 Result = Clamp(0.0f, Value, 1.0f);
    
    return(Result);
}


inline r32
Clamp01MapToRange(r32 Min, r32 t, r32 Max)
{
	r32 Result = 0;
	
	r32 Range = Max - Min;
	if(Range != 0.0f)
	{
		Result = Clamp01((t - Min)/Range);
	}
	
	return(Result);
}


inline r32
SafeRatioN(r32 Numerator, r32 Divisor, r32 N)
{
	r32 Result = N;
	
	if(Divisor != 0.0f)
	{
		Result = Numerator / Divisor;
	}
	
	return(Result);
}

inline r32
SafeRatio0(r32 Numerator, r32 Divisor)
{
	r32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
	
	return(Result);
}

inline r32
SafeRatio1(r32 Numerator, r32 Divisor)
{
	r32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
	
	return(Result);
}

//
// NOTE(casey): v2 operations
//

inline v2 
Perp(v2 A)
{
    v2 Result = {-A.y, A.x};
    
    return(Result);
}

inline v2
PerpMeditate(v2 A)
{
	v2 Result = Perp(A);
	
	return(Result);
}

inline v2
operator*(real32 A, v2 B)
{
    v2 Result;
    
    Result.x = A*B.x;
    Result.y = A*B.y;
    
    return(Result);
}

inline v2
operator*(v2 A, real32 B)
{
    v2 Result;
    
    Result.x = A.x*B;
    Result.y = A.y*B;
    
    return(Result);
}

inline v2 &
operator*=(v2 &B, real32 A)
{
    B = A * B;
    return(B);
}

inline v2 
operator/(v2 A, r32 B)
{
    v2 Result;
    
    Result.x = A.x/B;
    Result.y = A.y/B;
    
    return(Result);
}

inline v2 
operator-(v2 A)
{
    v2 Result;
    
    Result.x = -A.x;
    Result.y = -A.y;
    
    return(Result);
}

inline v2
PerpObserve(v2 A)
{
	v2 Result = -Perp(A);
	
	return(Result);
}

inline v2 
operator-(v2 A, v2 B)
{
    v2 Result;
    
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    
    return(Result);
}

inline v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return(A);
}

inline v2 
operator+(v2 A, v2 B)
{
    v2 Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    
    return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.x*B.x + A.y*B.y;
    
    return(Result);
}

inline r32
LengthSq(v2 A)
{
    r32 Result = Inner(A, A);
    
    return(Result);
}

inline r32
Length(v2 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    
    return(Result);
}

inline v2
Normalise(v2 A)
{
    v2 Result = A / Length(A);
    
    return(Result);
}

inline v2
NormaliseInvSq(v2 A)
{
	v2 Result = A / LengthSq(A);
	
	return(Result);
}

inline b32
operator==(v2 A, v2 B)
{
    b32 Result;
    
    Result = ((A.x == B.x) && (A.y == B.y));
    
    return(Result);
}

inline v2
Clamp01(v2 A)
{
    v2 Result;
    
    Result.x = Clamp01(A.x);
    Result.y = Clamp01(A.y);
    
    return(Result);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result;
    
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    
    return(Result);
}

inline v2
Lerp(v2 A, r32 t, v2 B)
{
	v2 Result = (1.0f - t)*A + t*B;
    
    return(Result);
}


//
// NOTE(casey): v2i operations
//

inline v2i 
Perp(v2i A)
{
    v2i Result = {-A.y, A.x};
    
    return(Result);
}

inline v2i
PerpMeditate(v2i A)
{
	v2i Result = Perp(A);
	
	return(Result);
}

inline v2i
operator*(i32 A, v2i B)
{
    v2i Result;
    
    Result.x = A*B.x;
    Result.y = A*B.y;
    
    return(Result);
}

inline v2i
operator*(v2i A, i32 B)
{
    v2i Result;
    
    Result.x = A.x*B;
    Result.y = A.y*B;
    
    return(Result);
}

inline v2i &
operator*=(v2i &B, i32 A)
{
    B = A * B;
    return(B);
}

inline v2i
operator/(v2i A, i32 B)
{
    v2i Result;
    
    Result.x = A.x/B;
    Result.y = A.y/B;
    
    return(Result);
}

inline v2i 
operator-(v2i A)
{
    v2i Result;
    
    Result.x = -A.x;
    Result.y = -A.y;
    
    return(Result);
}

inline v2i
PerpObserve(v2i A)
{
	v2i Result = -Perp(A);
	
	return(Result);
}

inline v2i 
operator-(v2i A, v2i B)
{
    v2i Result;
    
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    
    return(Result);
}

inline v2i &
operator-=(v2i &A, v2i B)
{
    A = A - B;
    return(A);
}

inline v2i 
operator+(v2i A, v2i B)
{
    v2i Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    
    return(Result);
}

inline v2i &
operator+=(v2i &A, v2i B)
{
    A = A + B;
    return(A);
}

inline i32
Inner(v2i A, v2i B)
{
    i32 Result = A.x*B.x + A.y*B.y;
    
    return(Result);
}

inline i32
LengthSq(v2i A)
{
    i32 Result = Inner(A, A);
    
    return(Result);
}

//TODO(moritz): Maybe these should be bool?
inline b32
operator==(v2i A, v2i B)
{
    b32 Result;
    
    Result = ((A.x == B.x) && (A.y == B.y));
    
    return(Result);
}

inline b32
operator!=(v2i A, v2i B)
{
	b32 Result;
	
	Result = !(A == B);
	
	return(Result);
}

inline v2i
Hadamard(v2i A, v2i B)
{
    v2i Result;
    
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    
    return(Result);
}


//
// NOTE(casey): v3 operations
//

inline v3 
operator+(v3 A, v3 B)
{
    v3 Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    
    return(Result);
}

inline v3 &
operator+=(v3 &B, v3 A)
{
	B = A + B;
	return(B);
}

inline v3
operator*(v3 A, real32 B)
{
    v3 Result;
    
    Result.x = A.x*B;
    Result.y = A.y*B;
    Result.z = A.z*B;
    
    return(Result);
}

inline v3
operator*(real32 A, v3 B)
{
    v3 Result;
    
    Result.x = A*B.x;
    Result.y = A*B.y;
    Result.z = A*B.z;
    
    return(Result);
}

inline v3 &
operator*=(v3 &B, real32 A)
{
    B = A * B;
    return(B);
}

inline v3
operator/(v3 A, r32 B)
{
    v3 Result;
    
    Result.x = A.x/B;
    Result.y = A.y/B;
    Result.z = A.z/B;
    
    return(Result);
}

inline v3
operator-(v3 A)
{
    v3 Result;
    
    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    
    return(Result);
}

inline v3
operator-(v3 A, v3 B)
{
	v3 Result;
	
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	
	return(Result);
}


inline v3 &
operator-=(v3 &B, v3 A)
{
	B = B - A;
	return(B);
}

inline b32
operator==(v3 A, v3 B)
{
    b32 Result;
    
    Result = ((A.x == B.x) && (A.y == B.y) && (A.z == B.z));
    
    return(Result);
}

inline b32
operator!=(v3 A, v3 B)
{
	b32 Result;
	Result = !(A == B);
	
	return(Result);
}

inline r32
Inner(v3 A, v3 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    
    return(Result);
}

inline r32
LengthSq(v3 A)
{
    r32 Result = Inner(A, A);
    
    return(Result);
}

inline r32
Length(v3 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    
    return(Result);
}

inline v3
Normalise(v3 A)
{
    v3 Result = A / Length(A);
    
    return(Result);
}

inline v3
Lerp(v3 A, r32 t, v3 B)
{
    v3 Result = (1.0f - t)*A + t*B;
    
    return(Result);
}


inline v3
Clamp01(v3 A)
{
    v3 Result;
    
    Result.x = Clamp01(A.x);
    Result.y = Clamp01(A.y);
    Result.z = Clamp01(A.z);
    
    return(Result);
}


inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result;
    
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    
    return(Result);
}

inline v3
Cross(v3 A, v3 B)
{
	v3 Result;
	
	//cx = aybz − azby
	//cy = azbx − axbz
	//cz = axby − aybx
	
	Result.x = A.y*B.z - A.z*B.y;
	Result.y = A.z*B.x - A.x*B.z;
	Result.z = A.x*B.y - A.y*B.x;
	
	return(Result);
}

inline v3
Inverse(v3 A)
{
	v3 Result = V3(1.0f/A.x,
				   1.0f/A.y,
				   1.0f/A.z);
	return(Result);
}

inline v3
V3SignOf(v3 A)
{
	v3 Result = V3(FloatSignOf(A.x),
				   FloatSignOf(A.y),
				   FloatSignOf(A.z));
	
	return(Result);
}

inline v3
Step(v3 Edge, v3 A)
{
	v3 Diff = A - Edge;
	v3 One = V3(1, 1, 1);
	v3 Result = 0.5f*(V3SignOf(Diff) + One);
	
	return(Result);
}


//
// NOTE(moritz): v3i operations
//

inline b32
operator==(v3i A, v3i B)
{
	b32 Result = ((A.x == B.x) &&
				  (A.y == B.y) &&
				  (A.z == B.z));
	
	return(Result);
}

//
// NOTE(casey): v4 operations
//

inline v4 
operator*(v4 A, r32 B)
{
    v4 Result;
    
    Result.x = A.x*B;
    Result.y = A.y*B;
    Result.z = A.z*B;
    Result.w = A.w*B;
    
    return(Result);
}

inline v4 
operator*(r32 A, v4 B)
{
    v4 Result;
    
    Result.x = A*B.x;
    Result.y = A*B.y;
    Result.z = A*B.z;
    Result.w = A*B.w;
    
    return(Result);
}

inline v4
operator+(v4 A, v4 B)
{
    v4 Result;
    
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;
    
    return(Result);
}


inline v4 &
operator*=(v4 &B, real32 A)
{
    B = A * B;
    return(B);
}

inline v4
operator/(v4 A, r32 B)
{
    v4 Result;
    
    Result.x = A.x/B;
    Result.y = A.y/B;
    Result.z = A.z/B;
    Result.w = A.w/B;
    
    return(Result);
}

inline v4 &
operator+=(v4 &B, v4 A)
{
	B = A + B;
	return(B);
}

inline r32
Inner(v4 A, v4 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
    
    return(Result);
}

inline r32
LengthSq(v4 A)
{
    r32 Result = Inner(A, A);
    
    return(Result);
}

inline r32
Length(v4 A)
{
    r32 Result = SquareRoot(LengthSq(A));
    
    return(Result);
}

inline v4
Normalise(v4 A)
{
    v4 Result = A / Length(A);
    
    return(Result);
}

inline v4
Lerp(v4 A, r32 t, v4 B)
{
    v4 Result = (1.0f - t)*A + t*B;
    
    return(Result);
}

inline v4
Hadamard(v4 A, v4 B)
{
    v4 Result;
    
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    Result.w = A.w * B.w;
    
    return(Result);
}

inline v4
Clamp01(v4 A)
{
    v4 Result;
    
    Result.x = Clamp01(A.x);
    Result.y = Clamp01(A.y);
    Result.z = Clamp01(A.z);
	Result.w = Clamp01(A.w);
    
    return(Result);
}

//
//NOTE(casey): Rectangle2
//

struct rectangle2
{  
    v2 Min;
    v2 Max;
};


inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result;
    
    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;
    
    return(Result);
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;
    
    Result.Min = Min;
    Result.Max = Min + Dim;
    
    return(Result);
}

inline v2
GetDim(rectangle2 Rect)
{
	v2 Result = Rect.Max - Rect.Min;
	return(Result);
}

inline b32
IsInsideRectangle(rectangle2 Rect, v2 TestPoint)
{
    b32 Result = false;
    
    //TODO(moritz): What about edge cases (literally)?
    if((TestPoint.x > Rect.Min.x)&&
       (TestPoint.x < Rect.Max.x)&&
       (TestPoint.y > Rect.Min.y)&&
       (TestPoint.y < Rect.Max.y))
    {
        Result = true;
    }
    return(Result);
}

//
//
//

struct rectangle2i
{
	v2i Min;
	v2i Max;
};

inline b32
IsPartOfRectangle2i(rectangle2i Rect, v2i Test)
{
	b32 Result = false;
    
    if((Test.x >= Rect.Min.x)&&
       (Test.x <= Rect.Max.x)&&
       (Test.y >= Rect.Min.y)&&
       (Test.y <= Rect.Max.y))
    {
        Result = true;
    }
    return(Result);
}

inline rectangle2i
Union(rectangle2i A, rectangle2i B)
{
	rectangle2i Result;
	
	Result.Min.x = (A.Min.x < B.Min.x) ? A.Min.x : B.Min.x;
	Result.Min.y = (A.Min.y < B.Min.y) ? A.Min.y : B.Min.y;
	Result.Max.x = (A.Max.x > B.Max.x) ? A.Max.x : B.Max.x;
	Result.Max.y = (A.Max.y > B.Max.y) ? A.Max.y : B.Max.y;
	return(Result);
}

inline rectangle2i
Intersect(rectangle2i A, rectangle2i B)
{
	rectangle2i Result;
	
	Result.Min.x = (A.Min.x < B.Min.x) ? B.Min.x : A.Min.x;
	Result.Min.y = (A.Min.y < B.Min.y) ? B.Min.y : A.Min.y;
	Result.Max.x = (A.Max.x > B.Max.x) ? B.Max.x : A.Max.x;
	Result.Max.y = (A.Max.y > B.Max.y) ? B.Max.y : A.Max.y;
	
	return(Result);
}

inline i32
GetClampedRectArea(rectangle2i A)
{
	i32 Result = 0;
	
	i32 Width  = (A.Max.x - A.Min.x);
	i32 Height = (A.Max.y - A.Min.y);
	
	if((Width > 0) && (Height > 0))
	{
		Result = Width*Height;
	}
	
	return(Result);
}

inline rectangle2i
InvertedInfinityRectangle()
{
	rectangle2i Result;
	Result.Min.x = Result.Min.y = INT32_MAX;
	Result.Max.x = Result.Max.y = -INT32_MAX;
	return(Result);
}

inline b32
HasArea(rectangle2i A)
{
	b32 Result = ((A.Max.x > A.Min.x) && (A.Max.y > A.Min.y));
	
	return(Result);
}

//
//NOTE(moritz): AABB
//

struct aabb
{
	v3 Min;
	v3 Max;
};

inline aabb
InvertedInfinityAABB()
{
	aabb Result;
	Result.Min.x = Result.Min.y = Result.Min.z = F32Max;
	Result.Max.x = Result.Max.y = Result.Max.z = -F32Max;
	return(Result);
}


//NOTE(moritz): Matrix stuff

struct m4x4
{
	r32 E[4][4];
};

inline v4
MatrixMulLeftV4(m4x4 Matrix, v4 A)
{
	v4 Result;
	
	v4 Column0 = V4(Matrix.E[0][0], 
					Matrix.E[1][0],
					Matrix.E[2][0],
					Matrix.E[3][0]);
	
	v4 Column1 = V4(Matrix.E[0][1], 
					Matrix.E[1][1],
					Matrix.E[2][1],
					Matrix.E[3][1]);
	
	v4 Column2 = V4(Matrix.E[0][2], 
					Matrix.E[1][2],
					Matrix.E[2][2],
					Matrix.E[3][2]);
	
	v4 Column3 = V4(Matrix.E[0][3], 
					Matrix.E[1][3],
					Matrix.E[2][3],
					Matrix.E[3][3]);
	
	Result = A.x*Column0 + A.y*Column1 + A.z*Column2 + A.w*Column3;
	
	return(Result);
}

//B gets multiplied to the right of A
inline m4x4
MatrixMul(m4x4 A, m4x4 B)
{
	m4x4 Result;
	
	v4 RowA0 = V4(A.E[0][0], A.E[0][1], A.E[0][2], A.E[0][3]);
	v4 RowA1 = V4(A.E[1][0], A.E[1][1], A.E[1][2], A.E[1][3]);
	v4 RowA2 = V4(A.E[2][0], A.E[2][1], A.E[2][2], A.E[2][3]);
	v4 RowA3 = V4(A.E[3][0], A.E[3][1], A.E[3][2], A.E[3][3]);
	
	v4 ColumnB0 = V4(B.E[0][0], 
					 B.E[1][0],
					 B.E[2][0],
					 B.E[3][0]);
	
	v4 ColumnB1 = V4(B.E[0][1], 
					 B.E[1][1],
					 B.E[2][1],
					 B.E[3][1]);
	
	v4 ColumnB2 = V4(B.E[0][2], 
					 B.E[1][2],
					 B.E[2][2],
					 B.E[3][2]);
	
	v4 ColumnB3 = V4(B.E[0][3], 
					 B.E[1][3],
					 B.E[2][3],
					 B.E[3][3]);
	
	Result = 
	{
		{
			{Inner(RowA0, ColumnB0), Inner(RowA0, ColumnB1), Inner(RowA0, ColumnB2), Inner(RowA0, ColumnB3)},
			{Inner(RowA1, ColumnB0), Inner(RowA1, ColumnB1), Inner(RowA1, ColumnB2), Inner(RowA1, ColumnB3)},
			{Inner(RowA2, ColumnB0), Inner(RowA2, ColumnB1), Inner(RowA2, ColumnB2), Inner(RowA2, ColumnB3)},
			{Inner(RowA3, ColumnB0), Inner(RowA3, ColumnB1), Inner(RowA3, ColumnB2), Inner(RowA3, ColumnB3)},
		}
	};
	
	return(Result);
}

inline m4x4
MatrixIdentity4x4()
{
	m4x4 Result = 
	{
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		}
	};
	
	return(Result);
}

struct m3x3
{
	r32 E[3][3];
};

inline v3
MatrixMulLeftV3(m3x3 Matrix, v3 A)
{
	v3 Result;
	
	v3 Column0 = V3(Matrix.E[0][0], 
					Matrix.E[1][0],
					Matrix.E[2][0]);
	
	v3 Column1 = V3(Matrix.E[0][1], 
					Matrix.E[1][1],
					Matrix.E[2][1]);
	
	v3 Column2 = V3(Matrix.E[0][2], 
					Matrix.E[1][2],
					Matrix.E[2][2]);
	
	Result = A.x*Column0 + A.y*Column1 + A.z*Column2;
	
	return(Result);
}

//B gets multiplied to the right of A
inline m3x3
MatrixMul(m3x3 A, m3x3 B)
{
	m3x3 Result;
	
	v3 RowA0 = V3(A.E[0][0], A.E[0][1], A.E[0][2]);
	v3 RowA1 = V3(A.E[1][0], A.E[1][1], A.E[1][2]);
	v3 RowA2 = V3(A.E[2][0], A.E[2][1], A.E[2][2]);
	
	v3 ColumnB0 = V3(B.E[0][0], 
					 B.E[1][0],
					 B.E[2][0]);
	
	v3 ColumnB1 = V3(B.E[0][1], 
					 B.E[1][1],
					 B.E[2][1]);
	
	v3 ColumnB2 = V3(B.E[0][2], 
					 B.E[1][2],
					 B.E[2][2]);
	
	Result = 
	{
		{
			{Inner(RowA0, ColumnB0), Inner(RowA0, ColumnB1), Inner(RowA0, ColumnB2)},
			{Inner(RowA1, ColumnB0), Inner(RowA1, ColumnB1), Inner(RowA1, ColumnB2)},
			{Inner(RowA2, ColumnB0), Inner(RowA2, ColumnB1), Inner(RowA2, ColumnB2)}
		}
	};
	
	return(Result);
}

inline m3x3
MatrixIdentity3x3()
{
	m3x3 Result = 
	{
		{
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1}
		}
	};
	
	return(Result);
}

inline m3x3
MatrixXRotation(math_constants *MathConstants, f32 Angle)
{
	f32 s, c;
	SinCos(MathConstants, Angle, &s, &c);
	m3x3 XRotation =
	{
		{
			1, 0,  0,
			0, c, -s,
			0, s,  c,
		}
	};
	
	m3x3 Result = XRotation;
	return(Result);
}

inline m3x3 
MatrixYRotation(math_constants *MathConstants, f32 Angle)
{
	f32 s, c;
	SinCos(MathConstants, Angle, &s, &c);
	m3x3 YRotation =
	{
		{
			c, 0, -s,
			0, 1,  0,
			s, 0,  c,
		}
	};
	
	m3x3 Result = YRotation;
	return(Result);
}

inline m3x3
MatrixZRotation(math_constants *MathConstants, f32 Angle)
{
	f32 s, c;
	SinCos(MathConstants, Angle, &s, &c);
	m3x3 ZRotation =
	{
		{
			c, -s, 0,
			s,  c, 0,
			0, 0,  1,
		}
	};
	
	m3x3 Result = ZRotation;
	return(Result);
}

#define HANDMADE_MATH_H

#endif //HANDMADE_MATH_H
