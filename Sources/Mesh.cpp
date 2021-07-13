#include "Mesh.h"

#include "Tools/LoaderFbx.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Mesh::Mesh(const VkDevice logicalDevice, const std::string &path, const std::string &name)
:	Object(name),
	m_Path(path)
{
	m_IsOpaque = true;

	if (m_Path != "")
	{
		std::vector<glm::vec3>		allPosition; 
		std::vector<glm::vec2>		allUV;
		std::vector<glm::vec3>		allNormal;
		std::vector<glm::vec4>		allDiffuseColor;
		std::vector<std::string>	allDiffuseTextures;
		std::vector<std::string>	allNormalTextures;

		if (LoaderFbx::Load(m_Path,
							LoaderFbx::FLIP_UV | LoaderFbx::TRIANGULATE | LoaderFbx::PRE_TRANSFORM_VERTICES,
							allPosition,
							allUV,
							allNormal,
							allDiffuseColor,
							m_Indices,
							allDiffuseTextures,
							allNormalTextures))
		{
			std::vector<VertexData>	vertices;
			for (int idx = 0; idx < allPosition.size(); ++idx)
			{
				VertexData	data;
				data.m_Pos = allPosition[idx];
				data.m_TexCoord = allUV[idx];
				data.m_Normal = allNormal[idx];
				data.m_Color = allDiffuseColor[idx];

				vertices.push_back(data);
			}

			m_VertexBuffer = Buffer<VertexData>(logicalDevice, BUFFER_TYPE::Vertex, vertices.size());
			m_IndexBuffer = Buffer<uint16_t>(logicalDevice, BUFFER_TYPE::Index, m_Indices.size());
			UpdateData(logicalDevice, vertices, m_Indices);

			if (!allDiffuseTextures.empty() && !allNormalTextures.empty())
				m_Material = Material(allDiffuseTextures[0], allNormalTextures[0]);
			else if (!allDiffuseTextures.empty())
				m_Material = Material(allDiffuseTextures[0]);
			else
				m_Material = Material();
		}
		else
			m_Material = Material(m_Path);
	}	
}

//----------------------------------------------------------------

Mesh::~Mesh()
{
	m_Indices.clear();

	// Note: need to add desctruction of Buffer -> delete here? manager with refs on created buffer?
}

//----------------------------------------------------------------

void	Mesh::Render(const VkCommandBuffer commandBuffer, uint32_t isOpaque)
{
	if (static_cast<uint32_t>(m_Material.GetAlbedo().a) == isOpaque)
	{
		VkDeviceSize	offsets = 0;

		const VkBuffer vertexBuffer = m_VertexBuffer.GetApiBuffer();

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetApiBuffer(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
	}
}

//----------------------------------------------------------------

void	Mesh::UpdateData(const VkDevice logicalDevice, const std::vector<VertexData> &vertices, const std::vector<uint16_t> &indices)
{
	m_VertexBuffer.UpdateData(logicalDevice, vertices);
	m_IndexBuffer.UpdateData(logicalDevice, indices);
}

//----------------------------------------------------------------

LIGHTLYY_END
