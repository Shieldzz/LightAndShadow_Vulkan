#pragma once

#include "Utility.h"
#include "Texture.h"
#include "Buffer.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct Skybox
{
	Skybox() {};
	Skybox(const VkDevice logicalDevice, const std::string &folderPath, const std::string &extension);

	void										Render(const VkCommandBuffer commandBuffer);

	void										FreeTextures();

	// getters
	std::vector<Texture>&						GetTexturesUnsafe() { return m_Textures; }
	const std::vector<Texture>&					GetTextures() const { return m_Textures; }
	uint32_t									GetSize() const { return m_Size; }
	const VkVertexInputBindingDescription&		GetBindingDesc() const { return m_BindingDesc; }
	const VkVertexInputAttributeDescription&	GetAttributesDesc() const { return m_AttribsDesc; }

	VkPipelineLayout					m_PipelineLayout;

private:
	std::vector<glm::vec3>				m_Vertices =
	{
		{-1.f, -1.f, -1.f},
		{-1.f,  1.f, -1.f},
		{ 1.f,  1.f, -1.f},
		{ 1.f, -1.f, -1.f},
		{-1.f, -1.f,  1.f},
		{-1.f,  1.f,  1.f},
		{ 1.f,  1.f,  1.f},
		{ 1.f, -1.f,  1.f}
	};

	std::vector<uint16_t>				m_Indices =
	{
		// front
		0, 1, 2, 2, 3, 0,

		// back
		7, 6, 5, 5, 4, 7,

		// right
		3, 2, 6, 6, 7, 3,

		// left
		4, 5, 1, 1, 0, 4,

		// up
		5, 6, 2, 2, 1, 5,

		// down
		0, 3, 7, 7, 4, 0
	};

	std::vector<Texture>				m_Textures;

	uint32_t							m_Size;

	Buffer<glm::vec3>					m_VertexBuffer;
	Buffer<uint16_t>					m_IndexBuffer;

	VkVertexInputBindingDescription		m_BindingDesc;
	VkVertexInputAttributeDescription	m_AttribsDesc;
};

//----------------------------------------------------------------

LIGHTLYY_END
