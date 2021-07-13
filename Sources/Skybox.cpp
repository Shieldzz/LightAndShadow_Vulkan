#include "Skybox.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Skybox::Skybox(const VkDevice logicalDevice, const std::string &folderPath, const std::string &extension)
{
	m_Size = 0;

	const char	*skyboxFaces[6] =
	{
		"right",
		"left",
		"up",
		"down",
		"back",
		"front"
	};

	m_Textures.resize(6);
	for (uint32_t textureIndex = 0; textureIndex < 6; ++textureIndex)
	{
		Texture	&texture = m_Textures[textureIndex];
		texture = Texture(folderPath + skyboxFaces[textureIndex] + extension, 4);

		m_Size += texture.GetWidth() * texture.GetHeight() * 4;
	}

	m_VertexBuffer = Buffer<glm::vec3>(logicalDevice, BUFFER_TYPE::Vertex, 8); // 8 = cube points
	m_VertexBuffer.UpdateData(logicalDevice, m_Vertices);

	m_IndexBuffer = Buffer<uint16_t>(logicalDevice, BUFFER_TYPE::Index, 36); // 36 = triangles points
	m_IndexBuffer.UpdateData(logicalDevice, m_Indices);

	m_BindingDesc = { };
	{
		m_BindingDesc.binding = 0;
		m_BindingDesc.stride = sizeof(glm::vec3);
		m_BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	m_AttribsDesc.binding = 0;
	m_AttribsDesc.location = 0;
	m_AttribsDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
	m_AttribsDesc.offset = 0;
}

//----------------------------------------------------------------

void	Skybox::Render(const VkCommandBuffer commandBuffer)
{
	VkDeviceSize	offsets = 0;

	const VkBuffer	vertexBuffer = m_VertexBuffer.GetApiBuffer();

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetApiBuffer(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
}

//----------------------------------------------------------------

void	Skybox::FreeTextures()
{
	for (uint32_t textureIndex = 0; textureIndex < 6; ++textureIndex)
	{
		m_Textures[textureIndex].FreeTexture();
	}
}

//----------------------------------------------------------------

LIGHTLYY_END
