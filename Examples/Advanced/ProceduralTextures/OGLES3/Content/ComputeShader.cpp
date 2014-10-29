// This file was created by Filewrap 1.2
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: ComputeShader.csh ********

// File data
static const char _ComputeShader_csh[] = 
	"#version 310 es\r\n"
	"\r\n"
	"#define NUM_BOXES         12\r\n"
	"\r\n"
	"/****************************************************************************\r\n"
	"Expected define inputs\r\n"
	"NUM_SAMPLEPOINTS\r\n"
	"WORKGROUPSIZE_X\r\n"
	"WORKGROUPSIZE_Y\r\n"
	"****************************************************************************/\r\n"
	"\r\n"
	"\r\n"
	"/* \r\n"
	" *  Sample distribution index that defines the amount of sample points \r\n"
	" *  that reside within a bounding box.\r\n"
	" */\r\n"
	"const int SAMPLE_DISTRIBUTION[64]= int[](\r\n"
	"\r\n"
	"\t2, 3, 2, 2, 2, 1, 2, 1, 3, 0, 2, 3, 0, 0, 2, 2,\r\n"
	"\t0, 3, 1, 1, 1, 2, 0, 2, 2, 2, 1, 1, 0, 2, 1, 2,\t\t\r\n"
	"\t1, 3, 3, 1, 2, 1, 3, 3, 2, 0, 2, 1, 3, 2, 2, 2,\r\n"
	"\t2, 0, 0, 3, 2, 2, 2, 3, 0, 2, 2, 0, 2, 2, 1, 1\r\n"
	");\r\n"
	"\r\n"
	"const vec2 consts=vec2(0.,1.);\r\n"
	"\r\n"
	"shared vec2 randomPoints[NUM_SAMPLEPOINTS * NUM_BOXES];\r\n"
	"\r\n"
	"uniform layout(rgba8, binding=0) writeonly highp image2D dstImage;\r\n"
	"uniform float uniform_input_scale;\r\n"
	"\r\n"
	"/* \r\n"
	" *  Generates a set of random points based on the bounding box indices.\r\n"
	" */\r\n"
	"void generatePseudoRandomSamplePoints(const ivec2 gid, int offset)\r\n"
	"{\t\r\n"
	"\t// Calculate initial seed value based on work group indices\r\n"
	"\tuint seed = uint(gid.x * 702395077 + gid.y * 915488749);\t\r\n"
	"\t\r\n"
	"\t// Determine number of points the reside within the current bounding box.\r\n"
	"\tint numpoints = SAMPLE_DISTRIBUTION[seed >> 26];\r\n"
	"\t// Pre-calculate common values\r\n"
	"\tvec2 gidf = vec2(gid.x, gid.y);\r\n"
	"\tvec2 rangeXforms = vec2(WORKGROUPSIZE_X, WORKGROUPSIZE_Y);\r\n"
	"\r\n"
	"\t// Generate random points based on the seed\r\n"
	"\tfor (int i=0; i < NUM_SAMPLEPOINTS; ++i)\r\n"
	"\t{\r\n"
	"\t\t\r\n"
	"\t\trandomPoints[i+offset] = vec2(-9999999.9f, -9999999.9f); \r\n"
	"\t\tif (i<numpoints)\r\n"
	"\t\t{\r\n"
	"\t\t\tvec2 randvals;\r\n"
	"\t\t\tseed = 1402024253u * seed + 586950981u;\r\n"
	"\t\t\trandvals.x = float(seed) * 0.0000000002328306436538696f;\r\n"
	"\t\t\tseed = 1402024253u * seed + 586950981u;\r\n"
	"\t\t\trandvals.y = float(seed) * 0.0000000002328306436538696f;\r\n"
	"\t\t\r\n"
	"\t\t\trandomPoints[i+offset] = (gidf + randvals) * rangeXforms;\r\n"
	"\t\t}\r\n"
	"\t}\r\n"
	"}\r\n"
	"\r\n"
	"\r\n"
	"float distance_metric(vec2 a1, vec2 a2)\r\n"
	"{\r\n"
	"#if (defined Euclid)\r\n"
	"\treturn distance(a1, a2);\r\n"
	"#elif (defined Manhattan)\r\n"
	"\treturn abs(a1.x - a2.x) + abs(a1.y - a2.y);\r\n"
	"\t//return dot(abs(a1 - a2), consts.yy));\r\n"
	"#elif (defined Chessboard)\r\n"
	"\treturn max(abs(a1.x - a2.x), abs(a1.y - a2.y));\r\n"
	"\t//max(dot(abs(a1 - a2), consts.yy));\r\n"
	"#endif\r\n"
	"}\r\n"
	"\r\n"
	"#if (defined Euclid)\r\n"
	"const vec4 scale = vec4(0.1677, 0.3354, 0.6708, 0.523);\r\n"
	"#elif (defined Manhattan)\r\n"
	"const vec4 scale = vec4(0.75f, 0.65f, 0.55f, 0.45f);\r\n"
	"#elif (defined Chessboard)\r\n"
	"const vec4 scale = vec4(0.4141f, 0.8282f, 0.9191f, 0.9999f);\r\n"
	"#endif\t\r\n"
	"\r\n"
	"\r\n"
	"layout (local_size_x = WORKGROUPSIZE_X, local_size_y = WORKGROUPSIZE_Y) in;\r\n"
	"void main()\r\n"
	"{\r\n"
	"\r\n"
	"\tivec2 gid = ivec2(gl_WorkGroupID.xy);\r\n"
	"\tivec2 lid = ivec2(gl_LocalInvocationID.xy);\r\n"
	"\tvec2 curr_point_coords = vec2(gl_GlobalInvocationID.xy);\r\n"
	"\r\n"
	"\tif (lid.x < 3)\r\n"
	"\t{\r\n"
	"\t\tint offset = NUM_SAMPLEPOINTS * 4 * lid.x + NUM_SAMPLEPOINTS * lid.y;\r\n"
	"\t\tgeneratePseudoRandomSamplePoints(gid + ivec2(lid.y - 1, lid.x - 1), offset);\r\n"
	"\t}\r\n"
	"\tbarrier();\r\n"
	"\r\n"
	"\t\r\n"
	"\t///* Calculate n nearest neighbours */\r\n"
	"\tfloat dist = distance_metric(randomPoints[0], curr_point_coords);\r\n"
	"\tvec4 min_distances = vec4(dist, dist, dist, dist);\r\n"
	"\t\t\r\n"
	"\tfor (int i=1; i < NUM_BOXES * NUM_SAMPLEPOINTS; ++i)\r\n"
	"\t{\r\n"
	"\t\tdist = distance_metric(randomPoints[i], curr_point_coords);\r\n"
	"\r\n"
	"\t\tif (dist < min_distances.x)\r\n"
	"\t\t{\r\n"
	"\t\t\tmin_distances.yzw = min_distances.xyz;\r\n"
	"\t\t\tmin_distances.x = dist;\r\n"
	"\t\t}\r\n"
	"\t\telse if (dist < min_distances.y)\r\n"
	"\t\t{\r\n"
	"\t\t\tmin_distances.zw = min_distances.yz;\r\n"
	"\t\t\tmin_distances.y = dist;\r\n"
	"\t\t}\r\n"
	"\t\telse if (dist < min_distances.z)\r\n"
	"\t\t{\r\n"
	"\t\t\tmin_distances.w = min_distances.z;\r\n"
	"\t\t\tmin_distances.z = dist;\r\n"
	"\t\t}\r\n"
	"\t\telse if (dist < min_distances.w)\r\n"
	"\t\t{\r\n"
	"\t\t\tmin_distances.w = dist;\r\n"
	"\t\t}\t\t\r\n"
	"\t}\r\n"
	"\t\t\t\r\n"
	"\t/*\r\n"
	"\t * Normalise and write back result\r\n"
	"\t */\t\r\n"
	"\r\n"
	"\tmin_distances = min_distances * scale * uniform_input_scale;\r\n"
	"\r\n"
	"\timageStore(dstImage, ivec2(gl_GlobalInvocationID.xy), min_distances);\r\n"
	"}\r\n";

// Register ComputeShader.csh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_ComputeShader_csh("ComputeShader.csh", _ComputeShader_csh, 3839);

// ******** End: ComputeShader.csh ********
