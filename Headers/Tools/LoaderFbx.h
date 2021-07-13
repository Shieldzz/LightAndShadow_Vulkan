#pragma once

#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "glm/glm.hpp"

//----------------------------------------------------------------

namespace LoaderFbx
{

//----------------------------------------------------------------

enum IMPORT_FLAG
	{
		// No values
		NONE						= 0,

		// Calculates the tangents and bitangents for the imported meshes.
		CALC_TANGENT_SPACE			= 0x1,

		// Identifies and joins identical vertex data sets within all imported meshes.
		JOINT_IDENTICAL_VERTICES	= 0x2,

		// Converts all the imported data to a left-handed coordinate space.
		LEFT_HANDED					= 0x4,

		// Triangulates all faces of all meshes.
		TRIANGULATE					= 0x8,

		// Removes some parts of the data structure (animations, materials, light sources, cameras, textures, vertex components).
		REMOVE_COMPONENT			= 0x10,

		// Generates normals for all faces of all meshes.
		GEN_NORMALS					= 0x20,

		// Generates smooth normals for all vertices in the mesh.
		GEN_SMOOTH_NORMALS			= 0x40,

		// Splits large meshes into smaller sub-meshes.
		SPLIT_LARGE_MESHES			= 0x80,

		// Removes the node graph and pre-transforms all vertices with the local transformation matrices of their nodes.
		PRE_TRANSFORM_VERTICES		= 0x100,

		// Limits the number of bones simultaneously affecting a single vertex to a maximum value.
		LIMIT_BONE_WEIGHT			= 0x200,

		// Validates the imported scene data structure. This makes sure that all indices are valid, all animations and bones are linked correctly, all material references are correct .. etc.
		VALIDATE_DATA_STRUCTURE		= 0x400,

		// Reorders triangles for better vertex cache locality.
		IMPROVE_CACHE_LOCALITY		= 0x800,

		// Searches for redundant/unreferenced materials and removes them.
		REMOVE_REDUNDANT_MATERIALS	= 0x1000,

		// This step tries to determine which meshes have normal vectors that are facing inwards and inverts them.
		FIX_INFACING_NORMALS		= 0x2000,

		// This step splits meshes with more than one primitive type in homogeneous sub - meshes.
		SORT_BY_P_TYPE				= 0x8000,

		// This step searches all meshes for degenerate primitives and converts them to proper lines or points.
		FIND_DEGENERATES			= 0x10000,

		// This step searches all meshes for invalid data, such as zeroed normal vectors or invalid UV coords and removes / fixes them.This is intended to get rid of some common exporter errors.
		FIND_INVALID_DATA			= 0x20000,

		// This step converts non - UV mappings(such as spherical or cylindrical mapping) to proper texture coordinate channels.
		GEN_UV_COORDS				= 0x40000,

		// This step applies per - texture UV transformations and bakes them into stand - alone vtexture coordinate channels.
		TRANSFORM_UV_COORDS			= 0x80000,

		// This step searches for duplicate meshes and replaces them with references to the first mesh.
		FIND_INSTANCES				= 0x100000,

		// A postprocessing step to reduce the number of meshes.
		OPTIMIZE_MESHES				= 0x200000,

		// A postprocessing step to optimize the scene hierarchy.
		OPTIMIZE_GRAPH				= 0x400000,

		// This step flips all UV coordinates along the y-axis and adjusts material settings and bitangents accordingly.
		FLIP_UV						= 0x800000,

		/*
			By default, the face winding order is counter clockwise.
			This step adjusts the output face winding order to be clockwise.
		*/
		CLOCKWISE					= 0x1000000,

		// This step splits meshes with many bones into sub-meshes so that each su-bmesh has fewer or as many bones as a given limit.
		SPLIT_BY_BONE_COUNT			= 0x2000000,

		// This step removes bones losslessly or according to some threshold.
		DEBONE						= 0x4000000
	};

//----------------------------------------------------------------

extern bool	Load(const std::string& path, const int flags, std::vector<glm::vec3>& allPosition, std::vector<glm::vec2>& allUV, std::vector<glm::vec3>& allNormal, std::vector<glm::vec4>& allDiffuseColor, std::vector<uint16_t>& allIndices, std::vector<std::string>& allDiffuseTextures, std::vector<std::string>& allNormalTextures);

//----------------------------------------------------------------

} // namespace LoaderFbx

//----------------------------------------------------------------