/* date = October 16th 2022 8:47 pm */

#ifndef MEMORY_H

struct memory_arena
{
	u8 *Base;
	umm Size;
	umm Used;
	
	s32 TempMemCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	umm Used;
};

inline umm
GetAlignmentOffset(memory_arena *Arena, umm Alignment)
{
	umm WritePointer = (umm)Arena->Base + Arena->Used;
	
	umm AlignmentMask = Alignment - 1;
	umm AlignmentOffset = Alignment - (WritePointer & AlignmentMask);
	
	return(AlignmentOffset);
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, Count*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)
internal void *
PushSize_(memory_arena *Arena, umm Size, umm Alignment = 4)
{
	umm ResultWritePointer = (umm)Arena->Base + Arena->Used;
	umm AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
	Size += AlignmentOffset;
	
	Assert((Arena->Used + Size) < Arena->Size);
	
	void *Result = (void *)(ResultWritePointer + AlignmentOffset);
	Arena->Used += Size;
	
	return(Result);
}

internal temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
	temporary_memory Result = {};
	
	Result.Used = Arena->Used;
	Result.Arena = Arena;
	
	++Arena->TempMemCount;
	
	return(Result);
}

internal void
EndTemporaryMemory(temporary_memory TempMem)
{
	memory_arena *Arena = TempMem.Arena;
	Assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	Assert(Arena->TempMemCount > 0);
	--Arena->TempMemCount;
}

internal void
CheckArena(memory_arena *Arena)
{
	Assert((Arena->TempMemCount == 0));
}

internal void
InitialiseArena(memory_arena *Arena, umm Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (u8 *)Base;
	Arena->Used = 0;
	Arena->TempMemCount = 0;
}

internal memory_arena *
SubArena(memory_arena *Arena, umm Size)
{
	memory_arena *SubArena = PushStruct(Arena, memory_arena);
	
	SubArena->Size = Size - sizeof(memory_arena);
	SubArena->Base = (u8 *)PushSize_(Arena, SubArena->Size);
	SubArena->Used = 0;
	SubArena->TempMemCount = 0;
	
	return(SubArena);
}

inline void
Copy(void *SourceInit, void *DestInit, umm Size)
{
	u8 *Source = (u8 *)SourceInit;
	u8 *Dest   = (u8 *)DestInit;
	while(Size--)
	{
		*Dest++ = *Source++;
	}
}

inline void
ZeroSize(void *DestInit, umm Size)
{
	u8 *Dest = (u8 *)DestInit;
	while(Size--)
	{
		*Dest++ = 0;
	}
}

internal memory_arena *
InitialiseArena(umm Size, void *Memory)
{
	memory_arena *Result = (memory_arena *)Memory;
	
	Result->Size = Size - sizeof(memory_arena);
	Result->Base = (u8 *)Memory + sizeof(memory_arena);
	Result->Used = 0;
	Result->TempMemCount = 0;
	
	return(Result);
}

#define MEMORY_H

#endif //MEMORY_H
