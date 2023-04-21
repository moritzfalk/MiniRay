/* date = April 21st 2023 1:57 pm */

#ifndef BVH_H

struct bounding_box
{
	v3 Min;
	v3 Max;
	v3 Center;
	
	u32 TriangleOffset;
};

struct bounding_box_manager
{
	u32 MaxTriangleCount;
	u32 TriangleCount;
	bounding_box *Triangles;
};


//NOTE(moritz): BVH implementation based on 
//"Physically Based Rendering: From Theory to Implementation"
//3rd edition
struct bvh_node
{
	v3 AABBMin;
	v3 AABBMax;
	
	s32 PrimitiveCount;
	s32 FirstPrimitive;
	
	//0 -> x, 1 -> y, 2 -> z
	u32 Axis;
	bvh_node *Children[2];
};

struct bvh_node_compact
{
	v3 AABBMin;
	v3 AABBMax;
	
	union 
	{
		s32 TrianglesOffset;
		s32 SecondChildOffset;
	};
	
	u16 PrimitiveCount; //0 -> inner node
	
	//0 -> x, 1 -> y, 2 -> z
	u8 Axis;
	
	u8 Padding;
};


#define BVH_H

#endif //BVH_H
