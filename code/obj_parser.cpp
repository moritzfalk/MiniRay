#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define OUTPUT_SOA false

//TODO(moritz): Remove this!
//NOTE(moritz): Only used for snprintf atm
#include <stdio.h>

//TODO(moritz): Maybe add in parsing for e notation (like: "2.0e-02" and stuff like that)?

#include "types.h"

#include "miniray_intrinsics.h"
#include "handmade_math.h"

#include "memory.h"
#include "fobj_file_format.h"

internal void *
Win32AllocateMemory(size_t Size)
{
	void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return(Result);
}

internal void
Win32DeallocateMemory(void *Memory)
{
	VirtualFree(Memory, 0, MEM_RELEASE);
}

internal u32
SafeTruncateToUInt32(u64 Value)
{
	Assert(Value <= U64Max);
	u32 Result = (u32)Value;
	return Result;
}

struct read_file_result
{
	u32 ContentsSize;
	void *Contents;
};

internal read_file_result
Win32ReadEntireFile(char *FileName)
{
	read_file_result Result = {};
	
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{ 
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			u32 FileSize32 = SafeTruncateToUInt32(FileSize.QuadPart);
			Result.Contents = Win32AllocateMemory(FileSize32);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
				   (FileSize32 == BytesRead))
				{
					//NOTE(moritz): File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					Win32DeallocateMemory(Result.Contents);
					Result = {};
				}
			}
		}
	}
	
	return(Result);
}

internal b32
Win32WriteEntireFile(char *FileName, u32 MemorySize, void *Memory)
{
	b32 Result = false;
	
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			//NOTE(moritz): File written successfully
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			//TODO(moritz): Error!
		}
		
		CloseHandle(FileHandle);
	}
	else
	{
		//TODO(moritz): Error!
	}
	
	return(Result);
}

struct string
{
	char *Data;
	size_t Count;
};

internal string
String(char *CString)
{
	string Result = {};
	
	Result.Data = CString;
	while(*CString++)
	{
		Result.Count++;
	}
	
	return(Result);
}

internal f32
SloppySteaksStringToFloat(string FloatString)
{
	f32 Result = F32Max;
	
	//Count digits before dot and after...
	u32 PreDotDigitCount  = 0;
	u32 PostDotDigitCount = 0;
	b32 SawDot = false;
	b32 IsNegative = (FloatString.Data[0] == '-');
	for(u32 CharIndex = 0;
		CharIndex < FloatString.Count;
		++CharIndex)
	{
		if(FloatString.Data[CharIndex] == '.')
		{
			Assert(!SawDot);
			SawDot = true;
		}
		else
		{
			if(!SawDot)
			{
				++PreDotDigitCount;
			}
			else
			{
				++PostDotDigitCount;
			}
		}
	}
	
	//Split into predot digits string and postdot digits string
	string PreDotDigitsString = FloatString;
	PreDotDigitsString.Count = PreDotDigitCount;
	if(IsNegative)
	{
		--PreDotDigitsString.Count;
		PreDotDigitsString.Data++;
	}
	
	string PostDotDigitsString = FloatString;
	PostDotDigitsString.Count = PostDotDigitCount;
	PostDotDigitsString.Data = FloatString.Data + FloatString.Count - PostDotDigitCount;
	
	//Read predot digits string right to left and multiply by 10^0, 10^1 ....
	char *DigitAt = PreDotDigitsString.Data + (PreDotDigitsString.Count - 1);
	f32 Base = 1;
	f32 PreDotResult = 0.0f;
	for(u32 PreDotDigitIndex = 0;
		PreDotDigitIndex < PreDotDigitsString.Count;
		++PreDotDigitIndex)
	{
		u8 Value = *DigitAt-- - '0';
		PreDotResult += Base*(f32)Value;
		Base *= 10.0f;
	}
	
	//Read postdot digits string left to right and multiply by 10^-1, 10^-2 ...
	DigitAt = PostDotDigitsString.Data;
	Base = 1.0f/10.0f;
	f32 PostDotResult = 0.0f;
	for(u32 PostDotDigitIndex = 0;
		PostDotDigitIndex < PostDotDigitsString.Count;
		++PostDotDigitIndex)
	{
		u8 Value = *DigitAt++ - '0';
		PostDotResult += Base*(f32)Value;
		Base *= (1.0f/10.0f);
	}
	
	Result = PreDotResult + PostDotResult;
	if(IsNegative)
	{
		Result *= -1.0f;
	}
	
	return(Result);
}

internal b32
IsWhiteSpace(char C)
{
	b32 Result = ((C == ' ') ||
				  (C == '\t')||
				  (C == '\n')||
				  (C == '\r'));
	return(Result);
}

internal string
GetNextFloatStringFromLine(string *Line)
{
	string Result = {};
	
	Result.Data = Line->Data;
	
	while(Line->Data[0] && 
		  (!IsWhiteSpace(Line->Data[0])))
	{
		Result.Count++;
		++Line->Data;
	}
	++Line->Data;
	
	return(Result);
}

internal string
GetNextIndexStringFromLine(string *Line)
{
	string Result = {};
	
	Result.Data = Line->Data;
	
	while(Line->Data[0] &&
		  (!IsWhiteSpace(Line->Data[0])) &&
		  (Line->Data[0] != '/'))
	{
		Result.Count++;
		++Line->Data;
	}
	++Line->Data;
	
	return(Result);
}

inline void
AdvanceLineToFirstDigit(string *Line)
{
	while(((Line->Data[0] < '0') ||
		   (Line->Data[0] > '9')) &&
		  (Line->Data[0] != '-'))
	{
		Line->Data++;
		Line->Count--;
	}
}

//NOTE(moritz): negative indices are not possible btw (they are, but I am not going to bother)
internal i32
StringToInt(string IntString)
{
	//0 means none
	i32 Result = 0;
	
	i32 Base = 1;
	for(u32 DigitIndex = 0;
		DigitIndex < IntString.Count;
		++DigitIndex)
	{
		char Digit = IntString.Data[IntString.Count - 1 - DigitIndex];
		i32  DigitValue = (i32)(Digit - '0');
		
		i32 Value = DigitValue*Base;
		Result += Value;
		
		Base *= 10;
	}
	
	return(Result);
}

struct link
{
	link *Next;
	string Line;
};

struct list
{
	link *First;
	link *Last;
};

struct link_pool
{
	link *FirstFreeLink;
	link_pool *Next;
};

inline string
BeginTemporaryLine(link *Link)
{
	string Result = {};
	
	Result.Data  = Link->Line.Data;
	Result.Count = Link->Line.Count;
	
	return(Result);
}

internal link *
AllocateLink(link *FirstFreeLink)
{
	if(!FirstFreeLink)
	{
		u32 LinksPerPool = 4096;
		link *NewLinks = (link *)Win32AllocateMemory(LinksPerPool*sizeof(link));
		if(NewLinks)
		{
			for(u32 LinkIndex = 0;
				LinkIndex < (LinksPerPool - 1);
				++LinkIndex)
			{
				NewLinks[LinkIndex].Next = &NewLinks[LinkIndex + 1];
			}
			
			NewLinks[LinksPerPool - 1].Next = 0;
		}
		else
		{
			//fprintf(stderr, "ERROR: Cannot allocate memory.\n");
			//exit(-1);
			//TODO(moritz): Error!
		}
		
		FirstFreeLink = NewLinks;
	}
	
	link *Result = FirstFreeLink;
	FirstFreeLink = Result->Next;
	
	Result->Next = 0;
	Result->Line.Count = 0;
	return(Result);
}

internal void
Append(list *List, link *Link)
{
	List->Last = (List->Last ? List->Last->Next : List->First) = Link;
}

inline b32
IsVertexLine(string Line)
{
	b32 Result = ((Line.Data[0] == 'v') &&
				  (Line.Data[1] == ' '));
	return(Result);
}

inline b32
IsNormalLine(string Line)
{
	b32 Result = ((Line.Data[0] == 'v') &&
				  (Line.Data[1] == 'n'));
	return(Result);
}

inline b32
IsFaceLine(string Line)
{
	b32 Result = ((Line.Data[0] == 'f') &&
				  (Line.Data[1] == ' '));
	return(Result);
}

int 
main(int argc, char **argv)
{
	//TODO(moritz): Do proper error checking and reporting...
	Assert(argc == 2);
	char *InputFileName = argv[1];
	char *OutputFileEnding = ".fobj";
	
	char OutputFileName[512] = {};
	char *InputFileNameAt  = InputFileName;
	char *OutputFileNameAt = OutputFileName;
	char *OutputFileEndingAt = OutputFileEnding;
	
	while(*InputFileNameAt != '.')
	{
		*OutputFileNameAt++ = *InputFileNameAt++;
	}
	
	while(*OutputFileEndingAt)
	{
		*OutputFileNameAt++ = *OutputFileEndingAt++;
	}
	
	read_file_result MeowFile = Win32ReadEntireFile(InputFileName);
	link *FirstFreeLink = 0;
	list List = {};
	
	char *Buffer = (char *)MeowFile.Contents;
	Buffer[MeowFile.ContentsSize] = 0;
	size_t BOL = 0;
	
	///NOTE(moritz): Store each line of obj file into a linked list, then iterate over said list...
	//This allows for more sane allocation for mesh data...
	for(size_t At = 0;
		At < MeowFile.ContentsSize;
		)
	{
		int C0 = Buffer[At];
		int C1 = Buffer[At + 1];
		int HitEOL = 0;
		size_t EOL = At;
		
		if(((C0 == '\r') && (C1 == '\n')) ||
		   ((C0 == '\n') && (C1 == '\r')))
		{
			At += 2;
			HitEOL = 1;
		}
		else if(C0 == '\n')
		{
			At += 1;
			HitEOL = 1;
		}
		else if(C0 == '\r')
		{
			At += 1;
			HitEOL = 1;
		}
		else if(C1 == 0) 
		{
			At += 1;
			HitEOL = 1;
		}
		else
		{
			++At;
		}
		
		if(HitEOL)
		{
			if(BOL < EOL)
			{
				link *Link = AllocateLink(FirstFreeLink);
				
				Link->Line.Count = EOL - BOL;
				Link->Line.Data  = Buffer + BOL;
				
				Append(&List, Link);
			}
			
			BOL = At;
		}
	}
	////
	
	//NOTE(moritz): Iterate over List and count verts, normals etc... for allocation purposes
	
	//File representation
	umm FileVertCount   = 0;
	umm FileFaceCount   = 0;
	umm FileIndexCount  = 0;
	umm FileNormalCount = 0;
	
	//Runtime representation
	umm ModelVertCount   = 0;
	umm ModelIndexCount  = 0;
	umm ModelFaceCount   = 0;
	umm ModelNormalCount = 0;
	
	for(link *Link = List.First;
		Link;
		Link = Link->Next)
	{
		if(IsVertexLine(Link->Line))
		{
			++FileVertCount;
			++ModelVertCount;
		}
		else if(IsNormalLine(Link->Line))
		{
			++FileNormalCount;
			++ModelNormalCount;
		}
		else if(IsFaceLine(Link->Line))
		{
			++FileFaceCount;
			
			Assert(!IsWhiteSpace(Link->Line.Data[2]));
			u32 FaceStride = 0;
			b32 WasWhiteSpace = false;
			for(u32 CharIndex = 0;
				CharIndex < Link->Line.Count;
				++CharIndex)
			{
				char C  = Link->Line.Data[CharIndex];
				char C1 = Link->Line.Data[CharIndex + 1];
				
				if((IsWhiteSpace(C) && !WasWhiteSpace && !IsWhiteSpace(C1)) ||
				   (C1 == '\0'))
				{
					WasWhiteSpace = true;
					FaceStride++;
				}
				else
				{
					WasWhiteSpace = false;
				}
			}
			
			FileIndexCount += FaceStride;
			
			if(FaceStride == 3)
			{
				//Face is a triangle
				ModelIndexCount += FaceStride;
				++ModelFaceCount;
			}
			else if(FaceStride == 4)
			{
				//Face is a quad
				ModelIndexCount += 6;
				ModelFaceCount += 2;
			}
			else
			{
				//Face is neither, can not process!
				//TODO(moritz): ERROR!
				InvalidCodePath;
			}
		}
	}
	
	Assert((ModelFaceCount*3) == ModelIndexCount);
	
	//Reserve one extra slot for the 0 vert and 0 normal
	umm VertexMemorySize = sizeof(v3)*(ModelVertCount + 1);
	umm NormalMemorySize = sizeof(v3)*(ModelNormalCount + 1);
	umm IndexMemorySize  = sizeof(v3i)*ModelIndexCount;
	umm ModelDataSize = VertexMemorySize + NormalMemorySize + IndexMemorySize;
	
	void *ModelMemory = Win32AllocateMemory(ModelDataSize);
	Assert(ModelMemory);
	
	//NOTE(moritz): We start VertCount at 1, so that we have a 0 vert. OBJ format starts indexing verts for faces at 1 etc...
	u32 VertexCount = 1;
	v3 *ModelVertices = (v3 *)ModelMemory;
	u32 NormalCount = 1;
	v3 *ModelNormals = (v3 *)((u8 *)ModelVertices + VertexMemorySize);
	
	//0 is the null index, and means _no vert_ used etc...
	u32 IndexCount = 0;
	v3i *ModelIndices = (v3i *)((u8 *)ModelNormals + NormalMemorySize);
	
	//NOTE(moritz): Now actually process the data 
	for(link *Link = List.First;
		Link;
		Link = Link->Next)
	{
		if(IsVertexLine(Link->Line) || IsNormalLine(Link->Line))
		{
			string TempLine = BeginTemporaryLine(Link);
			AdvanceLineToFirstDigit(&TempLine);
			string XFloat = GetNextFloatStringFromLine(&TempLine);
			string YFloat = GetNextFloatStringFromLine(&TempLine);
			string ZFloat = GetNextFloatStringFromLine(&TempLine);
			
			v3 LoadedVec = V3(SloppySteaksStringToFloat(XFloat),
							  SloppySteaksStringToFloat(YFloat),
							  SloppySteaksStringToFloat(ZFloat));
			
			if(IsVertexLine(Link->Line))
			{
				ModelVertices[VertexCount++] = LoadedVec;
			}
			else
			{
				ModelNormals[NormalCount++] = LoadedVec;
			}
		}
		else if(IsFaceLine(Link->Line))
		{
			u32 FaceStride = 0;
			b32 WasWhiteSpace = false;
			for(u32 CharIndex = 0;
				CharIndex < Link->Line.Count;
				++CharIndex)
			{
				char C  = Link->Line.Data[CharIndex];
				char C1 = Link->Line.Data[CharIndex + 1];
				
				if((IsWhiteSpace(C) && !WasWhiteSpace && !IsWhiteSpace(C1)) ||
				   (C1 == '\0'))
				{
					WasWhiteSpace = true;
					FaceStride++;
				}
				else
				{
					WasWhiteSpace = false;
				}
			}
			
			if(FaceStride == 3)
			{
				string TempLine = BeginTemporaryLine(Link);
				AdvanceLineToFirstDigit(&TempLine);
				//NOTE(moritz): Processing triangle
				for(u32 FaceStrideIndex = 0;
					FaceStrideIndex < FaceStride;
					++FaceStrideIndex)
				{
					string VertIndex   = GetNextIndexStringFromLine(&TempLine);
					string UVIndex     = GetNextIndexStringFromLine(&TempLine);
					string NormalIndex = GetNextIndexStringFromLine(&TempLine);
					
					v3i LoadedVec = V3i(StringToInt(VertIndex),
										StringToInt(UVIndex),
										StringToInt(NormalIndex));
					
					Assert((LoadedVec.E[0] >= 0) &&
						   (LoadedVec.E[1] >= 0) &&
						   (LoadedVec.E[2] >= 0))
						
						ModelIndices[IndexCount++] = LoadedVec;
				}
			}
			else if(FaceStride == 4)
			{
				//NOTE(moritz): Processing Quad...
				//Split into two triangles _assuming_ ccw ordering
				//(0, 1, 2, 3) -> (0, 1, 2) + (0, 2, 3)
				
				string TempLine = BeginTemporaryLine(Link);
				AdvanceLineToFirstDigit(&TempLine);
				
				v3i TempIndexStore[4];
				for(u32 FaceStrideIndex = 0;
					FaceStrideIndex < FaceStride;
					++FaceStrideIndex)
				{
					string VertIndex   = GetNextIndexStringFromLine(&TempLine);
					string UVIndex     = GetNextIndexStringFromLine(&TempLine);
					string NormalIndex = GetNextIndexStringFromLine(&TempLine);
					
					v3i LoadedVec = V3i(StringToInt(VertIndex),
										StringToInt(UVIndex),
										StringToInt(NormalIndex));
					
					TempIndexStore[FaceStrideIndex] = LoadedVec;
				}
				
				ModelIndices[IndexCount++] = TempIndexStore[0];
				ModelIndices[IndexCount++] = TempIndexStore[1];
				ModelIndices[IndexCount++] = TempIndexStore[2];
				
				ModelIndices[IndexCount++] = TempIndexStore[0];
				ModelIndices[IndexCount++] = TempIndexStore[2];
				ModelIndices[IndexCount++] = TempIndexStore[3];
			}
		}
	}
	
	//Write fobj file...
	Assert(ModelDataSize < U32Max);
	Assert((ModelDataSize + sizeof(fobj_header)) < U32Max);
	u32 FileSize32 = (u32)ModelDataSize + sizeof(fobj_header);
	void *FileBuffer = Win32AllocateMemory(FileSize32);
	fobj_header *Header = (fobj_header *)FileBuffer;
	Header->MagicValue  = FOBJ_MAGIC_VALUE;
	Header->Version     = FOBJ_VERSION;
	Header->VertexCount = VertexCount;
	Header->NormalCount = NormalCount;
	Header->IndexCount  = IndexCount;
	
	if(OUTPUT_SOA)
	{
		//NOTE(moritz): Process into SOA
		// xyz xyz -> xx yy zz
		f32 *FloatFileBufferAt = (f32 *)((u8 *)FileBuffer + sizeof(fobj_header));
		
		//Vertex SOA
		for(s32 Dim = 0;
			Dim < 3;
			++Dim)
		{
			for(u32 VertexIndex = 0;
				VertexIndex < VertexCount;
				++VertexIndex)
			{
				*FloatFileBufferAt++ = ModelVertices[VertexIndex].E[Dim];
			}
		}
		
		//Normal SOA
		for(s32 Dim = 0;
			Dim < 3;
			++Dim)
		{
			for(u32 NormalIndex = 0;
				NormalIndex < NormalCount;
				++NormalIndex)
			{
				*FloatFileBufferAt++ = ModelNormals[NormalIndex].E[Dim];
			}
		}
		
		//Index SOA
		//vI,tI,nI vI,tI,nI -> vIvI tItI nInI
		s32 *IntFileBufferAt = (s32 *)FloatFileBufferAt; 
		for(s32 Dim = 0;
			Dim < 3;
			++Dim)
		{
			for(u32 IndexIndex = 0;
				IndexIndex < IndexCount;
				++IndexIndex)
			{
				*IntFileBufferAt++ = ModelIndices[IndexIndex].E[Dim]; //HA HA
			}
		}
	}
	else
	{
		//NOTE(moritz): "Regular" AOS output
		Copy(ModelMemory, (u8 *)FileBuffer + sizeof(fobj_header), ModelDataSize);
	}
	
	Header->Vertices    = (u64)(sizeof(fobj_header));
	Header->Normals     = Header->Vertices + VertexMemorySize;
	Header->Indices     = Header->Normals + NormalMemorySize;
	
	Win32WriteEntireFile(OutputFileName, FileSize32, FileBuffer);
	
	return(0);
}