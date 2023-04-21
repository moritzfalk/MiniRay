#include "miniray.h"

#include "bvh.cpp"

enum UserAction 
{
	UserAction_MoveForward,
	UserAction_MoveLeft,
	UserAction_MoveRight,
	UserAction_MoveBack,
	UserAction_MoveUp,
	UserAction_MoveDown,
	
	UserAction_TurnLeft,
	UserAction_TurnRight,
	UserAction_TurnUp,
	UserAction_TurnDown,
	
	UserAction_Count
};

global bool GlobalKeyIsDown[UserAction_Count] = {};

internal void
AddSphere(world *World, v3 Position, f32 Radius, material_type Material)
{
	if((World->SphereCount + 1) >= MAX_SPHERE_COUNT) return;
	
	World->Spheres[World->SphereCount++] = {Position, Square(Radius), Radius, Material};
}

internal void
AddPlane(world *World, f32 Offset, v3 Normal, material_type Material)
{
	if((World->PlaneCount + 1) >= MAX_PLANE_COUNT) return;
	
	World->Planes[World->PlaneCount++] = {Normal, Offset, Material};
}

internal void
SetAreaLight(world *World, f32 Offset, v3 Normal, s32 Steps, f32 Intensity)
{
	area_light *AreaLight = &World->AreaLight;
	
	AreaLight->Normal = Normal;
	AreaLight->Offset = Offset;
	
	AreaLight->UHalfBound = 1.0f;
	AreaLight->VHalfBound = 0.5f;
	
	AreaLight->Steps     = Steps;
	AreaLight->StepSizeU = (2.0f*AreaLight->UHalfBound)/(f32)Steps;
	AreaLight->StepSizeV = (2.0f*AreaLight->VHalfBound)/(f32)Steps;
	AreaLight->Intensity = Intensity;
	
	//NOTE(moritz): Analogous to SetCameraTransform
	AreaLight->UBase = Normalise(Cross(V3(0, 1, 0), AreaLight->Normal));
	AreaLight->VBase = Normalise(Cross(AreaLight->Normal, AreaLight->UBase));
}

internal void
SetCameraTransform(world *World, v3 CameraWorldPos, v3 CameraTarget)
{
	World->CameraPos = CameraWorldPos;
	World->CameraZBasis = Normalise(CameraWorldPos - CameraTarget);
	World->CameraXBasis = Normalise(Cross(V3(0, 1, 0), World->CameraZBasis));
	World->CameraYBasis = Normalise(Cross(World->CameraZBasis, World->CameraXBasis));
	
	v3 X = World->CameraXBasis;
	v3 Y = World->CameraYBasis;
	v3 Z = World->CameraZBasis;
	
	v3 P = World->CameraPos;
}

inline v3 *
GetPixelPointer(frame_buffer FrameBuffer, u32 X, u32 Y)
{
	v3 *Result = ((v3 *)FrameBuffer.Memory) + Y*FrameBuffer.Width + X;
	return(Result);
}

//NOTE(moritz): Credit to Tavian Barnes: https://tavianator.com/2011/ray_box.html
inline b32
IntersectAABB(v3 AABBMin, v3 AABBMax, v3 RayOrigin, v3 RayDirInv)
{
	f32 tMin = 0.0f;
	f32 tMax = F32Max;
	
	for (s32 i = 0; 
		 i < 3; 
		 ++i) 
	{
		f32 t1 = (AABBMin.E[i] - RayOrigin.E[i]) * RayDirInv.E[i];
		f32 t2 = (AABBMax.E[i] - RayOrigin.E[i]) * RayDirInv.E[i];
		
		tMin = Min(Max(t1, tMin), Max(t2, tMin));
		tMax = Max(Min(t1, tMax), Min(t2, tMax));
	}
	
	return(tMin <= tMax);
}

internal b32
RenderTile(work_queue *Queue)
{
	u64 CurrentWorkOrderIndex;
	u64 OriginalNextOrderToRead = Queue->NextOrderToRead;
	u64 NewNextOrderToRead = ((OriginalNextOrderToRead + 1) % Queue->MaxWorkOrderCount);
	if(OriginalNextOrderToRead == Queue->NextOrderToWrite) return(false);
	CurrentWorkOrderIndex = LockedCompareExchangeAndReturnPreviousValue(&Queue->NextOrderToRead, NewNextOrderToRead, OriginalNextOrderToRead);
	if(CurrentWorkOrderIndex != OriginalNextOrderToRead) return(false);
	
	work_order *Order = Queue->WorkOrders + CurrentWorkOrderIndex;
	
	world *World = Order->World;
	frame_buffer FrameBuffer = Order->FrameBuffer;
	u32 MinX = Order->MinX; 
	u32 MinY = Order->MinY; 
	u32 OnePastMaxX = Order->OnePastMaxX; 
	u32 OnePastMaxY = Order->OnePastMaxY;
	
	random_series Series = Order->Series;
	
	f32 FOVConstant = 1.0f;
	f32 Widthf  = (f32)FrameBuffer.Width;
	f32 Heightf = (f32)FrameBuffer.Height;
	
	u64 RayCount = 0;
	
	f32 Tolerance = 0.0001f;
	
	//TODO(moritz): Antialiasing is currently busted
#if (SSAA_4x == 1)
	u32 PixelSampleCount = 4;
	f32 OneOverPixelSampleCountf = 0.25f;
#else
	u32 PixelSampleCount = 1;
	f32 OneOverPixelSampleCountf = 1.0f;
#endif
	
	v3 X = World->CameraXBasis;
	v3 Y = World->CameraYBasis;
	v3 Z = World->CameraZBasis;
	
	m3x3 CameraForwardRotation = 
	{
		{
			X.x, Y.x, Z.x,
			X.y, Y.y, Z.y,
			X.z, Y.z, Z.z,
		}
	};
	
	v2 PixelSampleOffsets[4] = {{0.25f, 0.25f}, {0.25f, 0.75f}, {0.75f, 0.75f}, {0.75f, 0.25f}};
	if(PixelSampleCount == 1)
		PixelSampleOffsets[0] = {{0.5f, 0.5f}};
	for(u32 PixelSampleIndex = 0;
		PixelSampleIndex < PixelSampleCount;
		++PixelSampleIndex)
	{
		for(u32 Y = MinY;
			Y < OnePastMaxY;
			++Y)
		{
			v3 *Out = GetPixelPointer(FrameBuffer, MinX, Y);
			for(u32 X = MinX;
				X < OnePastMaxX;
				++X)
			{
				f32 x = (((2.0f*((f32)X + PixelSampleOffsets[PixelSampleIndex].x))/Widthf)  - 1.0f)*World->FOVConstant*(Widthf/Heightf);
				f32 y = (((2.0f*((f32)Y + PixelSampleOffsets[PixelSampleIndex].y))/Heightf) - 1.0f)*World->FOVConstant;
				v3 RayDir = Normalise(V3(x, y, -1));
				v3 RayOrigin = V3(0, 0, 0);
				
				//NOTE(moritz): Transform Ray into world space
				RayOrigin = World->CameraPos;
				RayDir = Normalise(MatrixMulLeftV3(CameraForwardRotation, RayDir));
				
				v3 FinalColor = V3(0, 0, 0);
				
				//TODO(moritz): Tune this...
				u32 BounceCount    = 1;
				u32 MaxBounceCount = 2;
				f32 PreviousReflectionAlbedo = 1.0f;
				
				v3 BackgroundColor = {};
				v3 ContribColor = BackgroundColor; 
				
				v3 OneOverRayDir = V3(1.0f/RayDir.x, 1.0f/RayDir.y, 1.0f/RayDir.z);
				
				u32 DirIsNegative[3] = {(RayDir.x < 0.0f), (RayDir.y < 0.0f), (RayDir.z < 0.0f)};
				
				for(u32 BounceIndex = 0;
					BounceIndex < BounceCount;
					++BounceIndex)
				{
					///
					//Stats
					++RayCount;
					///
					
					intersection_result Intersection = {};
					
					//RaySceneIntersect
					{
						Intersection.MinT = F32Max;
						
						u32 CurrentNodeIndex = 0;
						u32 ToVisitCount = 0;
						u32 NodesToVisit[64] = {};
						
						for(;;)
						{
							bvh_node_compact *CurrentNode = World->CompactNodes + CurrentNodeIndex;
							f32 MinTriT = F32Max;
							if(IntersectAABB(CurrentNode->AABBMin, CurrentNode->AABBMax, RayOrigin, OneOverRayDir))
							{
								if(CurrentNode->PrimitiveCount)
								{
									//Node is leaf, therefore do intersection tests on all primitives in this node
									u32 *FirstTriangle = World->SortedTriangleOffsets + CurrentNode->TrianglesOffset;
									for(u32 TriangleIndex = 0;
										TriangleIndex < CurrentNode->PrimitiveCount;
										++TriangleIndex)
									{
										u32 IndexBufferOffset = FirstTriangle[TriangleIndex];
										
										Assert(IndexBufferOffset < World->IndexCount);
										
										v3i Indices0 = World->TransformedIndices[IndexBufferOffset + 0];
										v3i Indices1 = World->TransformedIndices[IndexBufferOffset + 1];
										v3i Indices2 = World->TransformedIndices[IndexBufferOffset + 2];
										
										v3 Vertex0 = World->TransformedVertices[Indices0.E[0]];
										v3 Vertex1 = World->TransformedVertices[Indices1.E[0]];
										v3 Vertex2 = World->TransformedVertices[Indices2.E[0]];
										
										v3 Normal0 = World->TransformedNormals[Indices0.E[2]];
										v3 Normal1 = World->TransformedNormals[Indices1.E[2]];
										v3 Normal2 = World->TransformedNormals[Indices2.E[2]];
										
										///NOTE(moritz): Intersection code by Inigo Quilez
										{
											v3 V1V0 = Vertex1 - Vertex0;
											v3 V2V0 = Vertex2 - Vertex0;
											v3 RayOriginV0 = RayOrigin - Vertex0;
											v3 N = Cross(V1V0, V2V0);
											v3 Q = Cross(RayOriginV0, RayDir);
											
											f32 D = 1.0f/Inner(RayDir, N);
											f32 U = D*Inner(-Q, V2V0);
											f32 V = D*Inner( Q, V1V0);
											f32 t = D*Inner(-N, RayOriginV0);
											if( (U<0.0f) || (V<0.0f) || ((U+V)>1.0f) ){t = -1.0f;}
											
											if((t > 0.0f) && (t < Intersection.MinT))
											{
												f32 W = 1 - U - V;
												v3 ShadingN = (V*Normal2 + U*Normal1 + W*Normal0);
												
												Intersection.MinT = t;
												Intersection.Material = Material_WhiteRubber;
												Intersection.N = ShadingN;
											}
										}
										///
									}
									
									if(!ToVisitCount) break;
									CurrentNodeIndex = NodesToVisit[--ToVisitCount];
								}
								else
								{
									//Move to near node, put far node on stack...
									if(DirIsNegative[CurrentNode->Axis])
									{
										NodesToVisit[ToVisitCount++] = CurrentNodeIndex + 1;
										CurrentNodeIndex = CurrentNode->SecondChildOffset;
									}
									else
									{
										NodesToVisit[ToVisitCount++] = CurrentNode->SecondChildOffset;
										CurrentNodeIndex++;
									}
								}
							}
							else
							{
								if(!ToVisitCount) break;
								CurrentNodeIndex = NodesToVisit[--ToVisitCount];
							}
						}
						
						
						//NOTE(moritz): Credit to Inigo Quilez:
						//https://iquilezles.org/articles/intersectors/
						//Ray GroundPlane (only in y) intersection
						//f32 MinPlaneT = F32Max;
						for(u32 PlaneIndex = 0;
							PlaneIndex < World->PlaneCount;
							++PlaneIndex)
						{
							plane *CurrentPlane = World->Planes + PlaneIndex;
							
							f32 Side = CurrentPlane->Offset - Inner(RayOrigin, CurrentPlane->Normal);
							
							f32 MinPlaneT = SafeRatio0(Side, Inner(RayDir, CurrentPlane->Normal));
							
							if(MinPlaneT > 0.0f)
							{
								if(MinPlaneT < Intersection.MinT)
								{
									Intersection.MinT = MinPlaneT;
									Intersection.Material = CurrentPlane->MaterialType;
									Intersection.N = CurrentPlane->Normal;
								}
							}
						}
						
						//Ray Sphere intersection
						f32 MinSphereT = F32Max;
						for(u32 SphereIndex = 0;
							SphereIndex < World->SphereCount;
							++SphereIndex)
						{
							sphere *CurrentSphere = World->Spheres + SphereIndex;
							v3 PosDiff = CurrentSphere->Position - RayOrigin;
							f32 Projection = Inner(PosDiff, RayDir);
							
							if(Projection > 0.0f)
							{
								//Distance of ray to sphere center
								v3 ProjectionDiff = CurrentSphere->Position - (RayOrigin + RayDir*Projection);
								
								f32 ProjectionDiffSquare = LengthSq(ProjectionDiff);
								
								if(ProjectionDiffSquare <= CurrentSphere->RadiusSquare)
								{
									//Find intersection point of ray on sphere hull (and then adjust projection)
									f32 ProjectionDeltaSquare = CurrentSphere->RadiusSquare - ProjectionDiffSquare;
									f32 ProjectionDelta = SquareRoot(ProjectionDeltaSquare);
									
									MinSphereT = Projection - ProjectionDelta;
									
									if(MinSphereT < Intersection.MinT)
									{
										Intersection.MinT = MinSphereT;
										Intersection.Material = CurrentSphere->MaterialType;
										Intersection.N = Normalise((RayOrigin + RayDir*MinSphereT) - CurrentSphere->Position);
									}
								}
							}
						}
						
						{
							area_light AreaLight = World->AreaLight;
							f32 Side = AreaLight.Offset - Inner(RayOrigin, AreaLight.Normal);
							
							f32 MinAreaLightT = SafeRatio0(Side, Inner(RayDir, AreaLight.Normal));
							
							if(MinAreaLightT > 0.0f)
							{
								v3 HitPoint = RayOrigin + MinAreaLightT*RayDir;
								
								//Check wether we are inside UBounds and VBounds
								//Project onto Bases and test length... Or length squared I guess
								v3 HitDiff = HitPoint - AreaLight.Normal*AreaLight.Offset;
								
								f32 HitU = Inner(AreaLight.UBase, HitDiff);
								f32 HitV = Inner(AreaLight.VBase, HitDiff);
								
								if((HitU >= -AreaLight.UHalfBound) &&
								   (HitU <=  AreaLight.UHalfBound) &&
								   (HitV >= -AreaLight.VHalfBound) &&
								   (HitV <=  AreaLight.VHalfBound))
								{
									if(MinAreaLightT < Intersection.MinT)
									{
										Intersection.MinT = MinAreaLightT;
										Intersection.Material = Material_Light;
										Intersection.N = AreaLight.Normal;
									}
								}
							}
						}
						
						if(Intersection.Material != Material_None)
						{
							v3 HitPoint = RayOrigin + RayDir*Intersection.MinT;
							v3 HitN = Intersection.N;
							material CurrentMaterial = World->Materials[Intersection.Material];
							
							f32 DiffuseLightIntensity = 0.0f;
							f32 SpecularLightIntensity = 0.0f;
							
							v3 ReflectionOrigin = RayOrigin;
							v3 ReflectionDir = RayDir;
							
							//Reflection pass
							if((CurrentMaterial.IsReflective) && (BounceCount < MaxBounceCount))
							{
								ReflectionOrigin = (Inner(RayDir, HitN) < 0) ? (HitPoint + HitN*0.001f) : (HitPoint - HitN*0.001f);
								ReflectionDir = RayDir - 2.0f*HitN*Inner(RayDir, HitN);
								++BounceCount;
								PreviousReflectionAlbedo *= CurrentMaterial.Albedo.E[2];
							}
							
							if(Intersection.Material == Material_Light)
							{
								//NOTE(moritz): This means HitP is on the Lamp itself...
								//No need to do shadow rays....
								ContribColor = V3(1.f, 1.f, 1.f);
								continue;
							}
							
							//NOTE(moritz): Shadow ray for area/point light
							{
								b32 ShadowHit = false;
								area_light AreaLight = World->AreaLight;
								
								++RayCount;
								
								//NOTE(moritz): Single sample version
								f32 JitterU = 0;
								f32 JitterV = 0;
								if(!World->RenderingShadows)
								{
									JitterU = 0.5f;
									JitterV = 0.5f;
									
									World->GlobalLightUf = 0;
									World->GlobalLightVf = 0;
								}
								else
								{
									JitterU = 0.5f + 0.5f*RandomBilateral(&Series);
									JitterV = 0.5f + 0.5f*RandomBilateral(&Series);
								}
								
								v3 AreaLightCenter = AreaLight.Normal*AreaLight.Offset;
								
								//TODO(moritz): Make this more readable by putting the
								//terms into separate variables
								v3 LightPosition =
									AreaLightCenter +
									AreaLight.UBase*(-AreaLight.UHalfBound +
													 AreaLight.StepSizeU*
													 (World->GlobalLightUf + JitterU)) +
									AreaLight.VBase*(-AreaLight.VHalfBound +
													 AreaLight.StepSizeV*
													 (World->GlobalLightVf + JitterV));
								///
								
								v3 LightPointDiff = HitPoint - LightPosition;
								v3 LightDir = -Normalise(LightPointDiff);
								
								//Shadow pass
								v3 ShadowOrigin = HitPoint + HitN*0.01f;
								f32 ShadowMaxT = Length(LightPointDiff);
								
								//NOTE(moritz): This effectively "backface culls" pixels
								//that are behind the area light.
								//NOT THOROUGHLY TESTED.
								f32 HitPointVisibility = -Inner(LightDir, AreaLight.Normal);
								
								//Shadow ray intersection test
								if(HitPointVisibility > 0.0f)
								{
									
									OneOverRayDir = Inverse(LightDir);
									DirIsNegative[0] = (LightDir.x < 0.0f);
									DirIsNegative[1] = (LightDir.y < 0.0f);
									DirIsNegative[2] = (LightDir.z < 0.0f);
									
									f32 ShadowMinT = ShadowMaxT;
									
									u32 CurrentNodeIndex = 0;
									u32 ToVisitCount = 0;
									u32 NodesToVisit[64] = {};
									
									for(;;)
									{
										bvh_node_compact *CurrentNode = World->CompactNodes + CurrentNodeIndex;
										
										f32 MinTriT = F32Max;
										u32 AABBIntersectMask = 0;
										b32 AnyIntersect = false;
										
										if(IntersectAABB(CurrentNode->AABBMin, CurrentNode->AABBMax,
														 ShadowOrigin,
														 OneOverRayDir))
										{
											if(CurrentNode->PrimitiveCount)
											{
												//Node is leaf, therefore do intersection tests on all primitives in this node
												//NOTE(moritz): PrimitiveCount should always be 1 I think...
												u32 *FirstTriangle = World->SortedTriangleOffsets + CurrentNode->TrianglesOffset;
												for(u32 TriangleIndex = 0;
													TriangleIndex < CurrentNode->PrimitiveCount;
													++TriangleIndex)
												{
													u32 IndexBufferOffset = FirstTriangle[TriangleIndex];
													
													v3i Indices0 = World->TransformedIndices[IndexBufferOffset + 0];
													v3i Indices1 = World->TransformedIndices[IndexBufferOffset + 1];
													v3i Indices2 = World->TransformedIndices[IndexBufferOffset + 2];
													
													v3 Vertex0 = World->TransformedVertices[Indices0.E[0]];
													v3 Vertex1 = World->TransformedVertices[Indices1.E[0]];
													v3 Vertex2 = World->TransformedVertices[Indices2.E[0]];
													
													{
														v3 V1V0 = Vertex1 - Vertex0;
														v3 V2V0 = Vertex2 - Vertex0;
														v3 ShadowOriginV0 = ShadowOrigin - Vertex0;
														v3 CalcN = Cross(V1V0, V2V0);
														v3 Q = Cross(ShadowOriginV0, LightDir);
														
														f32 D = 1.0f/Inner(LightDir, CalcN);
														f32 U = D*Inner(-Q, V2V0);
														f32 V = D*Inner( Q, V1V0);
														f32 t = D*Inner(-CalcN, ShadowOriginV0);
														if( (U<0.0f) || (V<0.0f) || ((U+V)>1.0f) ){t = -1.0f;}
														
														
														if((t > 0.0f) && (t < ShadowMinT))
														{
															ShadowMinT = t;
															ShadowHit = true;
														}
													}
													///
												}
												
												if(!ToVisitCount) break;
												CurrentNodeIndex = NodesToVisit[--ToVisitCount];
											}
											else
											{
												//Move to near node, put far node on stack...
												if(DirIsNegative[CurrentNode->Axis])
												{
													NodesToVisit[ToVisitCount++] = CurrentNodeIndex + 1;
													CurrentNodeIndex = CurrentNode->SecondChildOffset;
												}
												else
												{
													NodesToVisit[ToVisitCount++] = CurrentNode->SecondChildOffset;
													CurrentNodeIndex++;
												}
											}
										}
										else
										{
											if(!ToVisitCount) break;
											CurrentNodeIndex = NodesToVisit[--ToVisitCount];
										}
									}
									
									//NOTE(moritz): Source https://iquilezles.org/articles/intersectors/
									f32 MinPlaneT = F32Max;
									for(u32 PlaneIndex = 0;
										PlaneIndex < World->PlaneCount;
										++PlaneIndex)
									{
										plane *CurrentPlane = World->Planes + PlaneIndex;
										
										v3 PlaneNormal = CurrentPlane->Normal;
										
										f32 Side = CurrentPlane->Offset - Inner(ShadowOrigin, PlaneNormal);
										
										f32 Denom = Inner(LightDir, PlaneNormal);
										
										if((Denom < -Tolerance) || (Denom > Tolerance))
										{
											MinPlaneT = Side / Denom;
											if((MinPlaneT > 0.0f) &&
											   (MinPlaneT < ShadowMinT))
											{
												ShadowMinT = MinPlaneT;
												ShadowHit = true;
											}
										}
									}
									
									//Ray Sphere intersection
									f32 MinSphereT = F32Max;
									for(u32 SphereIndex = 0;
										SphereIndex < World->SphereCount;
										++SphereIndex)
									{
										sphere *CurrentSphere = World->Spheres + SphereIndex;
										
										v3 PosDiff = CurrentSphere->Position - ShadowOrigin;
										f32 Projection = Inner(PosDiff, LightDir);
										
										if(Projection > 0.0f)
										{
											//Distance of ray to sphere center
											v3 ProjectionDiff = CurrentSphere->Position - (ShadowOrigin + LightDir*Projection);
											
											f32 ProjectionDiffSquare = LengthSq(ProjectionDiff);
											
											if(ProjectionDiffSquare <= CurrentSphere->RadiusSquare)
											{
												//Find intersection point of ray on sphere hull (and then adjust projection)
												f32 ProjectionDeltaSquare = CurrentSphere->RadiusSquare - ProjectionDiffSquare;
												f32 ProjectionDelta = SquareRoot(ProjectionDeltaSquare);
												
												MinSphereT = Projection - ProjectionDelta;
												
												if(MinSphereT < ShadowMinT)
												{
													ShadowMinT = MinSphereT;
													ShadowHit = true;
												}
											}
										}
									}
									
								}
								else
								{
									//NOTE(moritz): HitPoint is not visible for the light
									//Simply due to strictly treating the light as an area
									//So it is effectively in shadow...
									ShadowHit = true;
								}
								
								f32 LightAlign = Inner(HitN, LightDir);
								
								if(!ShadowHit)
								{
									f32 LightContribution = AreaLight.Intensity; 
									
									//NOTE(moritz): The light contribution does not depend
									//anymore on the final amount of 
									//samples we are going to take,
									//but on the current number of samples 
									//that have been taken.
									//This allows the brightness of the image to stay constant
									//and not accumulate from black to final image over time...
									//But for each iteration we have to take the color that 
									//we have already written and adjust it since a new
									//contribution gets added in but the the total light
									//contribution has to stay normalised to the intensity
									//of the light source at all times...
									if(!World->CameraMoved)
										//LightContribution = AreaLight.ContributionPerSample;
										LightContribution = AreaLight.Intensity/World->GlobalLightSampleCountf;
									
									
									LightContribution *= HitPointVisibility;
									
									DiffuseLightIntensity += LightContribution*Max(0.0f, LightAlign);
									v3 SpecularReflection = LightDir - 2.0f*HitN*Inner(LightDir, HitN);
									SpecularLightIntensity += PowN(Max(0.0f, Inner(SpecularReflection, RayDir)), CurrentMaterial.SpecularExponent)*LightContribution;
								}
							}
							
							if(BounceIndex)
							{
								ContribColor += PreviousReflectionAlbedo*(CurrentMaterial.DiffuseColor*DiffuseLightIntensity*CurrentMaterial.Albedo.E[0]
																		  + V3(1, 1, 1)*SpecularLightIntensity*CurrentMaterial.Albedo.E[1]);
							}
							else
							{
								ContribColor = 
									CurrentMaterial.DiffuseColor*DiffuseLightIntensity*CurrentMaterial.Albedo.E[0]
									+ V3(1, 1, 1)*SpecularLightIntensity*CurrentMaterial.Albedo.E[1];
							}
							
							RayOrigin = ReflectionOrigin;
							RayDir = ReflectionDir;
							
							OneOverRayDir = V3(1.0f/RayDir.x, 1.0f/RayDir.y, 1.0f/RayDir.z);
							DirIsNegative[0] = (RayDir.x < 0.0f);
							DirIsNegative[1] = (RayDir.y < 0.0f);
							DirIsNegative[2] = (RayDir.z < 0.0f);
						}
						else
						{
							if(BounceIndex)
							{
								ContribColor += PreviousReflectionAlbedo*BackgroundColor;
							}
						}
					}
				}
				FinalColor = ContribColor;
				
				//TODO(moritz): Differentiate when to add and when to replace...
				//TODO(moritz): Sky background color is broken...
				
				if(!World->CameraMoved)
				{
					f32 N = World->GlobalLightSampleCountf;
					v3 OldColor = *Out;
					v3 NewColor = (OldColor*((N - 1.0f)/N) + FinalColor*OneOverPixelSampleCountf);
					//v3 NewColor = (OldColor*((N - 1.0f)/N) + FinalColor);
					
					//*Out++ += FinalColor*OneOverPixelSampleCountf;
					*Out++ = NewColor;
				}
				else
					*Out++ = FinalColor*OneOverPixelSampleCountf;
				
			}
		}
		
	}
	
	LockedAddAndReturnPreviousValue(&Queue->RayCount, RayCount);
	LockedAddAndReturnPreviousValue(&Queue->TileRetiredCount, 1);
	
	LockedAddAndReturnPreviousValue(&Queue->CompletionCount, 1);
	
	return(true);
}

internal void
InitialiseWorld(memory_arena *Arena, umm ScratchBufferSize, read_file_result MeshFile, u32 TriangleCount,
				umm TransformedVerticesSize, umm TransformedNormalsSize, umm TransformedIndicesSize,
				math_constants *MathConstants, world **World_)
{
	fobj_header *Header = (fobj_header *)MeshFile.Contents;
	
	*World_ = PushStruct(Arena, world);
	
	world *World = *World_;
	
	World->VertexCount = Header->VertexCount;
	World->Vertices = (v3 *)((u8 *)MeshFile.Contents + Header->Vertices);
	World->NormalCount = Header->NormalCount;
	World->Normals = (v3 *)((u8 *)MeshFile.Contents + Header->Normals);
	World->IndexCount = Header->IndexCount;
	World->Indices = (v3i *)((u8 *)MeshFile.Contents + Header->Indices);
	
	World->TransformedVertices = (v3 *)((u8 *)Arena + ScratchBufferSize);
	World->TransformedNormals  = World->TransformedVertices + World->VertexCount;
	World->TransformedIndices  = (v3i *)(World->TransformedNormals  + World->NormalCount);
	
	//Build scene
	World->GlobalLightSampleCountf = 1;
	
	//Set camera
	World->CameraPos     = V3(0, 0, 7.0f);
	World->CameraTarget  = V3(0, 0, 0);
	World->CameraForwardInit = Normalise(World->CameraTarget - World->CameraPos);
	World->CameraLeftInit    = Normalise(Cross(V3(0, 1, 0), World->CameraForwardInit));
	World->CameraYaw   = 0.0f;
	World->CameraPitch = 0.0f;
	
	//Init Groundplane
	AddPlane(World, -2.0f, V3(0, 1, 0), Material_MattFloor); 
	//Back plane
	AddPlane(World, -5.0f, V3(0, 0, 1), Material_BlueMattWall);
	//Right plane
	AddPlane(World, -20.0f, V3(-1, 0, 0), Material_RedMattWall);
	
	///Init materials
	World->Materials[Material_RedRubber]    = {{0.7f, 0.2f,  0.1f},  {0.9f, 0.1f, 0.0f},    10, false};
	World->Materials[Material_MattFloor]    = {{0.1f, 0.1f,  0.1f},  {0.9f, 0.0f, 0.0f},     0, false};
	World->Materials[Material_BlueMattWall] = {{0.1f, 0.1f,  0.3f},  {0.9f, 0.0f, 0.0f},     0, false};
	World->Materials[Material_RedMattWall]  = {{0.3f, 0.1f,  0.1f},  {0.9f, 0.0f, 0.0f},     0, false};
	World->Materials[Material_Metal]        = {{0.2f, 0.4f,  0.55f}, {0.3f, 0.6f, 0.4f},    50, true};
	World->Materials[Material_Ivory]        = {{0.7f, 0.65f, 0.6f},  {0.6f, 0.3f, 0.1f},    50, true};
	World->Materials[Material_WhiteRubber]  = {{0.7f, 0.65f,  0.6f},  {0.9f, 0.1f, 0.0f},    10, false};
	World->Materials[Material_Mirror]       = {{1.0f, 1.0f,  1.0f},  {0.0f, 10.0f, 0.8f}, 1425, true};
	World->Materials[Material_Light]        = {};
	
	World->Materials[Material_TriDEBUG]     = {};
	
	AddSphere(World, V3(4.0f, 2.5f, 2.2f), 0.25f, Material_Mirror);
	SetAreaLight(World, -10.0f, Normalise(V3(-0.25f, -0.25f, -0.25f)), 8, 0.75f);
	
	World->ModelScale = .5f; //Lady scale
	World->ModelOffset = V3(3.5f, -2.0f, 1.5f);
	
	f32 Angle = 0;
	
	m3x3 WorldRotation = MatrixYRotation(MathConstants, Angle);
	
	World->CameraMoved = true;
	
	bounding_box_manager *BBManager = InitialiseBBManager(Arena, World->SphereCount, TriangleCount);
	
	u32 *UnsortedTriangleOffsets = PushArray(Arena, TriangleCount, u32);
	
	for(u32 TriangleBBIndex = 0;
		TriangleBBIndex < TriangleCount;
		++TriangleBBIndex)
	{
		UnsortedTriangleOffsets[TriangleBBIndex] = TriangleBBIndex;
	}
	
	World->SortedTriangleOffsets = PushArray(Arena, TriangleCount*2, u32);
	
	Copy(World->Indices, World->TransformedIndices, TransformedIndicesSize);
	
	Copy(World->Vertices, World->TransformedVertices, TransformedVerticesSize);
	for(u32 VertexIndex = 1;
		VertexIndex < World->VertexCount;
		++VertexIndex)
	{
		//NOTE(moritz): Uniform scaling
		v3 Scaled = (World->TransformedVertices[VertexIndex]*World->ModelScale);
		
		v3 Rotated = MatrixMulLeftV3(WorldRotation, Scaled);
		
		v3 WorldPosition = Rotated + World->ModelOffset;
		
		World->TransformedVertices[VertexIndex] = WorldPosition;
	}
	
	Copy(World->Normals, World->TransformedNormals, TransformedNormalsSize);
	for(u32 NormalIndex = 1;
		NormalIndex < World->NormalCount;
		++NormalIndex)
	{
		v3 N = World->TransformedNormals[NormalIndex];
		v3 RotatedN = MatrixMulLeftV3(WorldRotation, N);
		
		World->TransformedNormals[NormalIndex] = RotatedN;
	}
	
	//Build Bounding Box list
	BBManager->TriangleCount = 0;
	for(u32 FaceIndex = 0;
		FaceIndex < World->IndexCount;
		FaceIndex += 3)
	{
		v3i Indices0 = World->TransformedIndices[FaceIndex + 0];
		v3i Indices1 = World->TransformedIndices[FaceIndex + 1];
		v3i Indices2 = World->TransformedIndices[FaceIndex + 2];
		
		v3 Vertex0 = World->TransformedVertices[Indices0.E[0]];
		v3 Vertex1 = World->TransformedVertices[Indices1.E[0]];
		v3 Vertex2 = World->TransformedVertices[Indices2.E[0]];
		
		f32 MinTriangleX = Min(Vertex0.x, Min(Vertex1.x, Vertex2.x));
		f32 MaxTriangleX = Max(Vertex0.x, Max(Vertex1.x, Vertex2.x));
		
		f32 MinTriangleY = Min(Vertex0.y, Min(Vertex1.y, Vertex2.y));
		f32 MaxTriangleY = Max(Vertex0.y, Max(Vertex1.y, Vertex2.y));
		
		f32 MinTriangleZ = Min(Vertex0.z, Min(Vertex1.z, Vertex2.z));
		f32 MaxTriangleZ = Max(Vertex0.z, Max(Vertex1.z, Vertex2.z));
		
		v3 BBMin = V3(MinTriangleX, MinTriangleY, MinTriangleZ);
		v3 BBMax = V3(MaxTriangleX, MaxTriangleY, MaxTriangleZ);
		v3 BBCenter = 0.5f*(BBMin + BBMax);
		
		PushTriangleBB(BBManager, BBMin, BBMax, BBCenter, FaceIndex);
	}
	
	//NOTE(moritz): Build acceleration structure:
	u32 NodeCount = 0;
	s32 SortedTriangleCount = 0;
	random_series Series = {420};
	
	bvh_node *RootNode = BuildBVH(Arena, &Series, BBManager->Triangles, UnsortedTriangleOffsets, 0, 
								  BBManager->TriangleCount, &NodeCount,
								  World->SortedTriangleOffsets, &SortedTriangleCount);
	
	//NOTE(moritz): Recursive flattened tree construction
	bvh_node_compact *CompactRoot = PushArray(Arena, NodeCount, bvh_node_compact);
	s32 Offset = 0;
	FlattenBVHTree(RootNode, CompactRoot, &Offset);
	
	World->CompactNodes = CompactRoot;
	World->NodeCount = NodeCount;
}

internal void
UpdateAndRender(world *World, math_constants *MathConstants, 
				frame_buffer FrameBuffer, umm FrameBufferSize, 
				work_queue *Queue, 
				u32 TileCountX, u32 TileCountY, 
				u32 TileWidth, u32 TileHeight,
				f32 dt)
{
	//Update camera
	{
		f32 CameraSpeed = 2.0f;
		f32 CameraMoveAmount = CameraSpeed*dt;
		
		f32 CameraTurnSpeed = 0.5f*Pi32;
		f32 CameraTurnAmount = CameraTurnSpeed*dt;
		
		if(GlobalKeyIsDown[UserAction_TurnLeft])
		{
			World->CameraYaw -= CameraTurnAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_TurnRight])
		{
			World->CameraYaw += CameraTurnAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_TurnUp])
		{
			World->CameraPitch += CameraTurnAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_TurnDown])
		{
			World->CameraPitch -= CameraTurnAmount;
			World->CameraMoved = true;
		}
		
		//Wrap Yaw to avoid floating point precision errors
		while(World->CameraYaw >= 2.0f*Pi32)
			World->CameraYaw -= 2.0f*Pi32;
		while(World->CameraYaw <= -2.0f*Pi32)
			World->CameraYaw += 2.0f*Pi32;
		
		//Clamp yaw to avoid Cross with world-up to be zero....
		f32 PitchLimit = Pi32*0.4f;
		if(World->CameraPitch >= PitchLimit)
			World->CameraPitch = PitchLimit;
		if(World->CameraPitch <= -PitchLimit)
			World->CameraPitch = -PitchLimit;
		
		//NOTE(moritz): Update camera transform
		m3x3 CameraForwardRotation = MatrixMul(MatrixYRotation(MathConstants, World->CameraYaw),
											   MatrixXRotation(MathConstants, World->CameraPitch));
		
		v3 CameraForward = Normalise(MatrixMulLeftV3(CameraForwardRotation, World->CameraForwardInit));
		v3 CameraLeft    = Normalise(MatrixMulLeftV3(CameraForwardRotation, World->CameraLeftInit));
		
		if(GlobalKeyIsDown[UserAction_MoveForward])
		{
			World->CameraPos += CameraForward*CameraMoveAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_MoveLeft])
		{
			World->CameraPos += CameraLeft*CameraMoveAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_MoveRight])
		{
			World->CameraPos -= CameraLeft*CameraMoveAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_MoveBack])
		{
			World->CameraPos -= CameraForward*CameraMoveAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_MoveUp])
		{
			World->CameraPos += V3(0, 1, 0)*CameraMoveAmount;
			World->CameraMoved = true;
		}
		if(GlobalKeyIsDown[UserAction_MoveDown])
		{
			World->CameraPos -= V3(0, 1, 0)*CameraMoveAmount;
			World->CameraMoved = true;
		}
		
		World->CameraTarget = World->CameraPos + CameraForward;
		SetCameraTransform(World, World->CameraPos, World->CameraTarget);
	}
	
	if(!World->CameraMoved && !World->FrameBufferClearedForTransition)
	{
		//NOTE(moritz):Clear FrameBuffer when we switch from simple point lighting to soft shadow
		//calculations, since lighting contribution needs to get added up sampling light source.
		//Hast to start from zero...
		ZeroSize(FrameBuffer.Memory, FrameBufferSize);
		World->FrameBufferClearedForTransition = true;
	}
	
	if(!World->CameraMoved && !World->RenderingShadowsFinished)
	{
		World->RenderingShadows = true;
	}
	else
	{
		World->RenderingShadows = false;
	}
	
	b32 RenderWorkNeedsToHappen = (World->CameraMoved || World->RenderingShadows);
	
	if(RenderWorkNeedsToHappen)
	{
		if(World->CameraMoved)
		{
			World->FrameBufferClearedForTransition = false;
			
			World->GlobalLightUf = 0.5f;
			World->GlobalLightUf = 0.5f;
			World->RenderingShadowsFinished = false;
		}
		
		v3 *FrameBufferOut = (v3 *)FrameBuffer.Memory;
		
		f32 FOVAngle = (1.0f/6.0f)*Pi32;
		f32 FOVSin, FOVCos;
		SinCos(MathConstants, FOVAngle, &FOVSin, &FOVCos);
		World->FOVConstant = FOVSin/FOVCos;
		f32 Widthf  = (f32)FrameBuffer.Width;
		f32 Heightf = (f32)FrameBuffer.Height;
		
		//Initialise Work Queue for current frame
		{
			Queue->CompletionGoal = 0;
			Queue->CompletionCount = 0;
			
			Queue->RayCount = 0;
			Queue->TileRetiredCount = 0;
		}
		
		for(u32 TileY = 0;
			TileY < TileCountY;
			++TileY)
		{
			u32 MinY = TileY*TileHeight;
			u32 OnePastMaxY = MinY + TileHeight;
			if(OnePastMaxY > FrameBuffer.Height)
			{
				OnePastMaxY = FrameBuffer.Height;
			}
			for(u32 TileX = 0;
				TileX < TileCountX;
				++TileX)
			{
				u32 MinX = TileX*TileWidth;
				u32 OnePastMaxX = MinX + TileWidth;
				if(OnePastMaxX > FrameBuffer.Width)
				{
					OnePastMaxX = FrameBuffer.Width;
				}
				
				u64 NewNextOrderToWrite = ((Queue->NextOrderToWrite + 1) % Queue->MaxWorkOrderCount);
				Assert(NewNextOrderToWrite != Queue->NextOrderToRead);
				
				work_order *Order = Queue->WorkOrders + Queue->NextOrderToWrite;
				
				Order->World = World;
				Order->FrameBuffer = FrameBuffer;
				Order->MinX = MinX;
				Order->MinY = MinY;
				Order->OnePastMaxX = OnePastMaxX;
				Order->OnePastMaxY = OnePastMaxY;
				
				//NOTE(moritz): Constant term needs to be there for Tile (0, 0) to get properly seeded...
				//XORShift does not function properly when seeded with 0
				Order->Series = {TileX*1569 + TileY*1963 + 7567};
				
				//NOTE(moritz): This LockedAdd is for fencing... Just to be safe
				LockedAddAndReturnPreviousValue(&Queue->NextOrderToWrite, 0);
				
				//NOTE(moritz): Signaling to threads to go pick up work
				//while the main thread is still filling out work orders is a lot
				//faster :)
				Queue->NextOrderToWrite = NewNextOrderToWrite;
				++Queue->CompletionGoal;
				ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
			}
		}
		
		//NOTE(moritz): This LockedAdd is for fencing... Just to be safe
		LockedAddAndReturnPreviousValue(&Queue->NextOrderToWrite, 0);
		
		while(Queue->CompletionGoal != Queue->CompletionCount)
		{
			RenderTile(Queue);
		}
		
		//NOTE(moritz): Manage soft shadow sampling coordinates
		if(World->RenderingShadows)
		{
			World->GlobalLightUf += 1.0f;
			++World->GlobalLightUs;
			
			World->GlobalLightSampleCountf += 1.0f;
			
			if(World->GlobalLightUs >= World->AreaLight.Steps)
			{
				World->GlobalLightUf = 0.0f;
				World->GlobalLightUs = 0;
				
				World->GlobalLightVf += 1.0f;
				++World->GlobalLightVs;
				if(World->GlobalLightVs >= World->AreaLight.Steps)
				{
					World->GlobalLightVf = 0.0f;
					World->GlobalLightVs = 0;
					World->RenderingShadowsFinished = true;
					
					World->GlobalLightSampleCountf = 1.0f;
				}
			}
		}
		else
		{
			World->GlobalLightUf = 0.0f;
			World->GlobalLightUs = 0;
			World->GlobalLightVf = 0.0f;
			World->GlobalLightVs = 0;
			World->GlobalLightSampleCountf = 1.0f;
		}
		
		if(World->CameraMoved)
		{
			World->CameraMoved = false;
		}
		
	}
	
}

#if (MINIRAY_WIN32 == 1)
#include "win32_miniray.cpp"
#endif