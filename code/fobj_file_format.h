/* date = October 16th 2022 10:13 pm */

#ifndef FOBJ_FILE_FORMAT_H

#pragma pack(push, 1)

#define FOBJ_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24)) 
struct fobj_header
{
#define FOBJ_MAGIC_VALUE FOBJ_CODE('f', 'o', 'b', 'j')
	u32 MagicValue;
	
#define FOBJ_VERSION 0
	
	u32 Version;
	
	u32 VertexCount;
	u32 NormalCount;
	u32 IndexCount;
	
	u64 Vertices;
	u64 Normals;
	u64 Indices;
};

#pragma pack(pop)

#define FOBJ_FILE_FORMAT_H

#endif //FOBJ_FILE_FORMAT_H
