
/*
NOTE(moritz): 

This bvh code is loosely based on the corresponding chaper in "Physically Based Rendering: From Theory To Implementation" - 3rd edition

If I were to do this again, I would probably construct the bvh in local space of the mesh 
and do it offline (instead of once at startup everytime) and save that to a custom mesh file format.
*/

inline b32
IsRightChild(s32 NodeIndex)
{
	b32 Result = ((NodeIndex % 2) == 1);
	return(Result);
}

inline s32
Partition(u32 *UnsortedTriangleOffsets, bounding_box *Triangles, s32 Start, s32 End,
		  s32 PivotIndex, u32 Axis)
{
	f32 PivotValue = Triangles[UnsortedTriangleOffsets[PivotIndex]].Center.E[Axis];
	
	//Swap pivot to the end
	u32 Swap = UnsortedTriangleOffsets[End];
	UnsortedTriangleOffsets[End] = UnsortedTriangleOffsets[PivotIndex];
	UnsortedTriangleOffsets[PivotIndex] = Swap;
	
	s32 FinalPivotIndex = Start;
	for(s32 ElementIndex = Start;
		ElementIndex < (End - 1);
		++ElementIndex)
	{
		if(Triangles[UnsortedTriangleOffsets[ElementIndex]].Center.E[Axis] >= PivotValue) continue;
		
		u32 Swap = UnsortedTriangleOffsets[FinalPivotIndex];
		UnsortedTriangleOffsets[FinalPivotIndex] = UnsortedTriangleOffsets[ElementIndex];
		UnsortedTriangleOffsets[ElementIndex] = Swap;
		
		++FinalPivotIndex;
	}
	
	Swap = UnsortedTriangleOffsets[FinalPivotIndex];
	UnsortedTriangleOffsets[FinalPivotIndex] = UnsortedTriangleOffsets[End];
	UnsortedTriangleOffsets[End] = Swap;
	
	return(FinalPivotIndex);
}

//NOTE(moritz): Using an iterative form of the quickselect algorithm to find the median
//and partition triangles based on that. 
inline s32
SelectMedianAndPartition(u32 *UnsortedTriangleOffsets, bounding_box *Triangles, random_series *Series, s32 Start, s32 End,
						 s32 Mid, u32 Axis)
{
	s32 IterationStart  = Start;
	s32 IterationMid    = Mid;
	s32 IterationEnd    = End;
	
	u32 IterationLength = (u32)(End - Start) + 1;
	s32 IterationPivotIndex = Start + (s32)(XORShift32(Series) % IterationLength);
	
	IterationPivotIndex = Partition(UnsortedTriangleOffsets, Triangles, IterationStart, IterationEnd, IterationPivotIndex, Axis);
	
	while(IterationMid != IterationPivotIndex)
	{
		if(IterationMid < IterationPivotIndex)
		{
			IterationEnd   = IterationPivotIndex - 1;
		}
		else
		{
			IterationStart = IterationPivotIndex + 1;
		}
		
		IterationLength = (u32)(IterationEnd - IterationStart) + 1;
		IterationPivotIndex = IterationStart + (s32)(XORShift32(Series) % IterationLength);
		
		IterationPivotIndex = Partition(UnsortedTriangleOffsets, Triangles, IterationStart, IterationEnd, IterationPivotIndex, Axis);
	}
	
	return(IterationMid);
}

inline void 
InitiBVHLeaf(bvh_node *NewNode, u32 *UnsortedTriangleOffsets, 
			 bounding_box *Triangles,
			 s32 FirstPrimitive, s32 OnePastLastPrimitive,
			 v3 BoundsCentroidMin, v3 BoundsCentroidMax,
			 u32 *SortedTriangleOffsets,
			 s32 *SortedTriangleCount)
{
	s32 FirstPrimitiveOffset = *SortedTriangleCount;
	for(s32 UnsortedTriangleIndex = FirstPrimitive;
		UnsortedTriangleIndex < OnePastLastPrimitive;
		++UnsortedTriangleIndex)
	{
		SortedTriangleOffsets[(*SortedTriangleCount)++] = Triangles[UnsortedTriangleOffsets[UnsortedTriangleIndex]].TriangleOffset;
	}
	
	NewNode->FirstPrimitive = FirstPrimitiveOffset;
	NewNode->PrimitiveCount = OnePastLastPrimitive - FirstPrimitive;
	NewNode->AABBMin = BoundsCentroidMin;
	NewNode->AABBMax = BoundsCentroidMax;
	NewNode->Children[0] = NewNode->Children[1] = 0;
}

//TODO(moritz): Do iterative version...
internal bvh_node *
BuildBVH(memory_arena *Arena, random_series *Series, bounding_box *Triangles, u32 *UnsortedTriangleOffsets, 
		 s32 FirstPrimitive, s32 OnePastLastPrimitive,
		 u32 *NodeCount, u32 *SortedTriangleOffsets,
		 s32 *SortedTriangleCount)
{
	bvh_node *NewNode = PushStruct(Arena, bvh_node);
	(*NodeCount)++;
	
	v3 BoundsCentroidMin = {F32Max, F32Max, F32Max};
	v3 BoundsCentroidMax = {-F32Max, -F32Max, -F32Max};
	for(s32 UnsortedTriangleIndex = FirstPrimitive;
		UnsortedTriangleIndex < OnePastLastPrimitive;
		++UnsortedTriangleIndex)
	{
		s32 TriangleIndex = UnsortedTriangleOffsets[UnsortedTriangleIndex];
		bounding_box *CurrentBB = Triangles + TriangleIndex;
		
		BoundsCentroidMax.x = Max(BoundsCentroidMax.x, CurrentBB->Max.x);
		BoundsCentroidMin.x = Min(BoundsCentroidMin.x, CurrentBB->Min.x);
		
		BoundsCentroidMax.y = Max(BoundsCentroidMax.y, CurrentBB->Max.y);
		BoundsCentroidMin.y = Min(BoundsCentroidMin.y, CurrentBB->Min.y);
		
		BoundsCentroidMax.z = Max(BoundsCentroidMax.z, CurrentBB->Max.z);
		BoundsCentroidMin.z = Min(BoundsCentroidMin.z, CurrentBB->Min.z);
	}
	
	s32 PrimitiveCount = OnePastLastPrimitive - FirstPrimitive;
	
	if(PrimitiveCount == 1)
	{
		InitiBVHLeaf(NewNode, UnsortedTriangleOffsets,
					 Triangles, FirstPrimitive, OnePastLastPrimitive,
					 BoundsCentroidMin, BoundsCentroidMax, SortedTriangleOffsets,
					 SortedTriangleCount);
		return(NewNode);
	}
	else
	{
		v3 CentroidBounds = BoundsCentroidMax - BoundsCentroidMin;
		
		s32 SplitAxis = 0; //Assume X
		if(CentroidBounds.y > CentroidBounds.x)
		{
			SplitAxis = 1; //Y
		}
		
		if(CentroidBounds.z > CentroidBounds.E[SplitAxis])
		{
			SplitAxis = 2; //Z
		}
		
		int Mid = (FirstPrimitive + OnePastLastPrimitive) / 2;
		
		if(BoundsCentroidMax.E[SplitAxis] == BoundsCentroidMin.E[SplitAxis])
		{
			//NOTE(moritz): Make leaf
			InitiBVHLeaf(NewNode, UnsortedTriangleOffsets,
						 Triangles, FirstPrimitive, OnePastLastPrimitive,
						 BoundsCentroidMin, BoundsCentroidMax, SortedTriangleOffsets,
						 SortedTriangleCount);
			return(NewNode);
		}
		else
		{
			Mid = SelectMedianAndPartition(UnsortedTriangleOffsets, Triangles, Series, FirstPrimitive, OnePastLastPrimitive - 1, Mid, SplitAxis);
			
			NewNode->Children[0] = BuildBVH(Arena, Series, Triangles, UnsortedTriangleOffsets, FirstPrimitive,
											Mid, NodeCount, SortedTriangleOffsets,
											SortedTriangleCount);
			NewNode->Children[1] = BuildBVH(Arena, Series, Triangles, UnsortedTriangleOffsets, Mid, OnePastLastPrimitive,
											NodeCount, SortedTriangleOffsets,
											SortedTriangleCount);
			
			v3 MinChildBounds = V3(-F32Max, -F32Max, -F32Max);
			v3 MaxChildBounds = V3(F32Max, F32Max, F32Max);
			
			NewNode->AABBMin.x = Min(NewNode->Children[0]->AABBMin.x, NewNode->Children[1]->AABBMin.x);
			NewNode->AABBMin.y = Min(NewNode->Children[0]->AABBMin.y, NewNode->Children[1]->AABBMin.y);
			NewNode->AABBMin.z = Min(NewNode->Children[0]->AABBMin.z, NewNode->Children[1]->AABBMin.z);
			
			NewNode->AABBMax.x = Max(NewNode->Children[0]->AABBMax.x, NewNode->Children[1]->AABBMax.x);
			NewNode->AABBMax.y = Max(NewNode->Children[0]->AABBMax.y, NewNode->Children[1]->AABBMax.y);
			NewNode->AABBMax.z = Max(NewNode->Children[0]->AABBMax.z, NewNode->Children[1]->AABBMax.z);
			
			NewNode->Axis = SplitAxis;
			NewNode->PrimitiveCount = 0;
			
			return(NewNode);
		}
	}
}

inline s32
FlattenBVHTree(bvh_node *Node, 
			   bvh_node_compact *CompactNodes, s32 *Offset)
{
	bvh_node_compact *CompactNode = CompactNodes + *Offset;
	CompactNode->AABBMin = Node->AABBMin;
	CompactNode->AABBMax = Node->AABBMax;
	
	s32 MyOffset = (*Offset)++;
	if(Node->PrimitiveCount)
	{
		CompactNode->TrianglesOffset = Node->FirstPrimitive;
		CompactNode->PrimitiveCount  = (u16)Node->PrimitiveCount;
	}
	else
	{
		CompactNode->Axis = (u8)Node->Axis;
		CompactNode->PrimitiveCount = 0;
		FlattenBVHTree(Node->Children[0], CompactNodes, Offset);
		CompactNode->SecondChildOffset = FlattenBVHTree(Node->Children[1], CompactNodes, Offset);
	}
	
	return(MyOffset);
}

internal void
PushTriangleBB(bounding_box_manager *BBManager, v3 Min, v3 Max, v3 Center, u32 TriangleOffset)
{
	if(BBManager->TriangleCount < BBManager->MaxTriangleCount)
	{
		bounding_box *BB = BBManager->Triangles + BBManager->TriangleCount++;
		BB->Min = Min;
		BB->Max = Max;
		BB->Center = Center;
		BB->TriangleOffset = TriangleOffset;
	}
}

internal bounding_box_manager *
InitialiseBBManager(memory_arena *Arena, u32 MaxSphereCount, u32 MaxTriangleCount)
{
	bounding_box_manager *BBManager = PushStruct(Arena, bounding_box_manager);
	BBManager->Triangles = PushArray(Arena, MaxTriangleCount, bounding_box);
	BBManager->MaxTriangleCount = MaxTriangleCount;
	BBManager->TriangleCount = 0;
	
	return(BBManager);
}
