/* date = April 21st 2023 1:50 pm */

#ifndef MINIRAY_H

#if (MINIRAY_WIN32 == 1)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <windows.h>
#else
#error "You need to specify a platform!"
#endif

#include "types.h"

#include "miniray_intrinsics.h"
#include "handmade_math.h"

#include "memory.h"
#include "fobj_file_format.h"

#include "random.h"

#include "bvh.h"


struct read_file_result
{
	u32 ContentsSize;
	void *Contents;
};

struct frame_buffer
{
	u32 Width;
	u32 Height;
	
	void *Memory;
};

enum material_type : u32
{
	Material_None,
	
	Material_RedRubber,
	Material_MattFloor,
	Material_BlueMattWall,
	Material_RedMattWall,
	Material_Metal,
	Material_Ivory,
	Material_WhiteRubber,
	Material_Mirror,
	
	Material_Light,
	
	Material_TriDEBUG,
	
	Material_Count,
};

struct material
{
	v3 DiffuseColor;
	v3 Albedo; //(Diffuse, Specular, Reflection)
	s32 SpecularExponent;
	b32 IsReflective;
};

struct intersection_result
{
	f32 MinT;  
	u32 Material; 
	v3 N; 
};


struct sphere
{
	v3 Position;
	f32 RadiusSquare;
	f32 Radius;
	material_type MaterialType;
};

struct plane
{
	v3  Normal;
	f32 Offset;
	
	material_type MaterialType;
};

struct area_light
{
	v3 Normal;
	v3 UBase;
	v3 VBase;
	f32 Offset;
	f32 UHalfBound;
	f32 VHalfBound;
	s32 Steps;
	f32 StepSizeU;
	f32 StepSizeV;
	f32 Intensity;
	
	v3 Color;
};

#define MAX_SPHERE_COUNT 32
#define MAX_PLANE_COUNT 4
struct world
{
	u32 VertexCount;
	v3 *Vertices;
	
	u32 NormalCount;
	v3 *Normals;
	
	u32 IndexCount;
	v3i *Indices;
	
	v3  *TransformedVertices;
	v3  *TransformedNormals;
	v3i *TransformedIndices;
	
	f32 ModelScale;
	v3 ModelOffset;
	
	u32 SphereCount;
	sphere Spheres[MAX_SPHERE_COUNT];
	
	area_light AreaLight;
	
	material Materials[Material_Count];
	
	u32 PlaneCount;
	plane Planes[MAX_PLANE_COUNT]; 
	
	///
	v3 CameraTarget;
	v3 CameraPos;
	v3 CameraXBasis;
	v3 CameraYBasis;
	v3 CameraZBasis;
	
	v3 CameraForwardInit;
	v3 CameraLeftInit;
	f32 CameraYaw;
	f32 CameraPitch;
	
	///
	
	//NOTE(moritz): This is the flattened BVH tree
	u32 NodeCount;
	bvh_node_compact *CompactNodes;
	u32 *SortedTriangleOffsets;
	
	b32 CameraMoved;
	b32 RenderingShadows;
	b32 RenderingShadowsFinished;
	b32 FrameBufferClearedForTransition;
	f32 FOVConstant;
	
	f32 GlobalLightUf;
	f32 GlobalLightVf;
	s32 GlobalLightUs;
	s32 GlobalLightVs;
	f32 GlobalLightSampleCountf;
};



//NOTE(moritz): Platform specific things
internal u64 LockedAddAndReturnPreviousValue(u64 volatile *Value, u64 Addend);
internal u64 LockedCompareExchangeAndReturnPreviousValue(u64 volatile *Value, u64 New, u64 Expected);

struct work_order
{
	world *World;
	frame_buffer FrameBuffer;
	u32 MinX;
	u32 MinY;
	u32 OnePastMaxX;
	u32 OnePastMaxY;
	
	random_series Series;
};

struct work_queue
{
	u64 MaxWorkOrderCount;
	work_order *WorkOrders;
	
	volatile u64 CompletionGoal;
	volatile u64 CompletionCount;
	
	volatile u64 NextOrderToRead;
	volatile u64 NextOrderToWrite;
	
	//NOTE(moritz): Some stats
	volatile u64 RayCount;
	volatile u64 TileRetiredCount;
	
	HANDLE SemaphoreHandle;
};


#define MINIRAY_H

#endif //MINIRAY_H
