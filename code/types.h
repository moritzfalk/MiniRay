/* date = October 16th 2022 8:48 pm */

#ifndef TYPES_H

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32   bool32;

typedef size_t memory_index;
typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8   i8;
typedef int16  i16;
typedef int32  i32;
typedef int64  i64;

typedef int8   s8;
typedef int16  s16;
typedef int32  s32;
typedef int64  s64;

typedef bool32 b32;

typedef uint8  u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float  real32;
typedef double real64;

typedef real32 r32;
typedef real32 f32;

typedef real64 r64;
typedef real64 f64;

typedef u64 umm;

#define internal static
#define local_persist static
#define global static

#define Pi32 3.1415926535

#ifdef MINIRAY_DEBUG
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression) 
#endif

#define InvalidCodePath Assert(!"InvalidCodePath");

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define U64Max ULONG_MAX
#define F32Max FLT_MAX
#define U32Max ((u32) - 1)

#define TYPES_H

#endif //TYPES_H
