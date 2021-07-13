#include "Tools/LoaderFbx.h"

#include <assimp/scene.h>

//----------------------------------------------------------------

namespace LoaderFbx
{

//----------------------------------------------------------------

extern bool	Load(const std::string& path, const int flags, std::vector<glm::vec3>& allPosition, std::vector<glm::vec2>& allUV, std::vector<glm::vec3>& allNormal, std::vector<glm::vec4>& allDiffuseColor, std::vector<uint16_t>& allIndices, std::vector<std::string>& allDiffuseTextures, std::vector<std::string>& allNormalTextures)
{
	Assimp::Importer	importer;

	const aiScene	*scene = importer.ReadFile(path.c_str(), flags);

	if (scene)
	{
		unsigned int numMeshes = scene->mNumMeshes;

		for (uint32_t idxMesh = 0; idxMesh < numMeshes; idxMesh++)
		{
			const aiMaterial* mtl = nullptr;

			unsigned int idxMtl = scene->mMeshes[idxMesh]->mMaterialIndex;
			if (idxMtl >= 0)
			{
				mtl = scene->mMaterials[idxMtl];
			}

			// Vertex
			unsigned int numVertex = scene->mMeshes[idxMesh]->mNumVertices;
			for (uint32_t idxVert = 0; idxVert < numVertex; ++idxVert)
			{
				//Vertices
				glm::vec3 pos;
				pos.x = scene->mMeshes[idxMesh]->mVertices[idxVert].x;
				pos.y = scene->mMeshes[idxMesh]->mVertices[idxVert].y;
				pos.z = scene->mMeshes[idxMesh]->mVertices[idxVert].z;
				allPosition.push_back(pos);

				// Normals
				glm::vec3 normal;
				if (scene->mMeshes[idxMesh]->HasNormals())
				{
					normal.x = scene->mMeshes[idxMesh]->mNormals[idxVert].x;
					normal.y = scene->mMeshes[idxMesh]->mNormals[idxVert].y;
					normal.z = scene->mMeshes[idxMesh]->mNormals[idxVert].z;
				}
				else
				{
					normal.x = 0.0f;
					normal.y = 0.0f;
					normal.z = 0.0f;
				}
				allNormal.push_back(normal);

				// Texture Coords
				glm::vec2 texCoord;
				if (scene->mMeshes[idxMesh]->HasTextureCoords(0))
				{
					texCoord.x = scene->mMeshes[idxMesh]->mTextureCoords[0][idxVert].x;
					texCoord.y = scene->mMeshes[idxMesh]->mTextureCoords[0][idxVert].y;
				}
				else
				{
					texCoord.x = 0.0f;
					texCoord.y = 0.0f;
				}
				allUV.push_back(texCoord);

				// VertexColors
				glm::vec4 color;
				if (scene->mMeshes[idxMesh]->HasVertexColors(0))
				{
					color.x = scene->mMeshes[idxMesh]->mColors[0][idxVert].r;
					color.y = scene->mMeshes[idxMesh]->mColors[0][idxVert].g;
					color.z = scene->mMeshes[idxMesh]->mColors[0][idxVert].b;
					color.w = scene->mMeshes[idxMesh]->mColors[0][idxVert].a;
				}
				else if (mtl)
				{
					aiColor4D diffuse;
					if (aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS)
					{
						color.x = diffuse.r;
						color.y = diffuse.g;
						color.z = diffuse.b;
						color.w = diffuse.a;
					}
				}
				else
				{
					color.x = 1.0f;
					color.y = 1.0f;
					color.z = 1.0f;
					color.w = 1.0f;
				}
				allDiffuseColor.push_back(color);
			}

			//Faces
			unsigned int numFaces = scene->mMeshes[idxMesh]->mNumFaces;
			for (uint32_t idxFace = 0; idxFace < numFaces; ++idxFace)
			{
				unsigned int numIndices = scene->mMeshes[idxMesh]->mFaces->mNumIndices;
				if (numIndices == 3)
				{
					allIndices.push_back(scene->mMeshes[idxMesh]->mFaces[idxFace].mIndices[0]);
					allIndices.push_back(scene->mMeshes[idxMesh]->mFaces[idxFace].mIndices[1]);
					allIndices.push_back(scene->mMeshes[idxMesh]->mFaces[idxFace].mIndices[2]);
				}
			}

			if (mtl)
			{
				aiString diffuse_path;
				if (aiGetMaterialTexture(mtl, aiTextureType::aiTextureType_DIFFUSE, 0, &diffuse_path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
				{
					size_t pos = path.find_last_of("/");
					std::string folderPath = "";
					if (pos != std::string::npos)
						folderPath = path.substr(0, pos);

					if (folderPath.size() > 0)
					{
						char lastChar = folderPath[folderPath.size() - 1];
						if (lastChar != '/')
							folderPath.push_back('/');
					}

					std::string	diffusePath = diffuse_path.data;
					pos = diffusePath.find_last_of("/");
					if (pos == std::string::npos)
						pos = diffusePath.find_last_of("\\");

					if (pos != std::string::npos)
						diffusePath = diffusePath.substr(pos + 1, diffusePath.size() - pos);

					allDiffuseTextures.push_back(folderPath + diffusePath);
				}

				aiString normal_path;
				if (aiGetMaterialTexture(mtl, aiTextureType::aiTextureType_NORMALS, 0, &normal_path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
				{
					size_t pos = path.find_last_of("/");
					std::string folderPath = "";
					if (pos != std::string::npos)
						folderPath = path.substr(0, pos);

					if (folderPath.size() > 0)
					{
						char lastChar = folderPath[folderPath.size() - 1];
						if (lastChar != '/')
							folderPath.push_back('/');
					}

					allNormalTextures.push_back(folderPath + normal_path.data);
				}
			}
		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------

} // namespace LoaderFbx

//----------------------------------------------------------------