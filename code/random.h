/* date = April 20th 2023 8:31 pm */

#ifndef RANDOM_H

struct random_series
{
	u32 State;
};

//NOTE(moritz): Yoinked from wikipedia
inline u32
XORShift32(random_series *Series)
{
	u32 X = Series->State;
	X ^= X << 13;
	X ^= X >> 17;
	X ^= X <<  5;
	
	Series->State = X;
	
	return(X);
}

internal f32
RandomUnilateral(random_series *Series)
{
	f32 Result = ((f32)XORShift32(Series)/(f32)U32Max);
	return(Result);
}

internal f32
RandomBilateral(random_series *Series)
{
	f32 Result = -1.0f + 2.0f*RandomUnilateral(Series);
	return(Result);
}

internal f32
RandomRange(random_series *Series, f32 Min, f32 Max)
{
	f32 Result = Min + (Max - Min)*RandomUnilateral(Series);
	return(Result);
}


#define RANDOM_H

#endif //RANDOM_H
