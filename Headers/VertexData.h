#pragma once

#include "glm/glm.hpp"

#include "Initializers.h"
#include "Utility.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct VertexData
{
	glm::vec3	m_Pos;
	glm::vec2	m_TexCoord;
	glm::vec3	m_Normal;
	glm::vec4	m_Color;
}; // struct VertexData

//----------------------------------------------------------------

struct VertexDescription // must mirror VertexData
{
	VertexDescription()
	{
		m_BindingDesc = { };
		{
			m_BindingDesc.binding = 0;
			m_BindingDesc.stride = sizeof(VertexData);
			m_BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		m_AttribsDesc.resize(4);

		m_AttribsDesc[0].binding = 0;
		m_AttribsDesc[0].location = 0;
		m_AttribsDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_AttribsDesc[0].offset = offsetof(VertexData, VertexData::m_Pos);

		m_AttribsDesc[1].binding = 0;
		m_AttribsDesc[1].location = 1;
		m_AttribsDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
		m_AttribsDesc[1].offset = offsetof(VertexData, VertexData::m_TexCoord);
		
		m_AttribsDesc[2].binding = 0;
		m_AttribsDesc[2].location = 2;
		m_AttribsDesc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_AttribsDesc[2].offset = offsetof(VertexData, VertexData::m_Normal);

		m_AttribsDesc[3].binding = 0;
		m_AttribsDesc[3].location = 3;
		m_AttribsDesc[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		m_AttribsDesc[3].offset = offsetof(VertexData, VertexData::m_Color);
	}

	const VkVertexInputBindingDescription&					GetBindingDesc() const { return m_BindingDesc; }
	const std::vector<VkVertexInputAttributeDescription>&	GetAttributesDesc() const { return m_AttribsDesc; }

private:
	VkVertexInputBindingDescription					m_BindingDesc;
	std::vector<VkVertexInputAttributeDescription>	m_AttribsDesc;
}; // struct VertexDescription

//----------------------------------------------------------------

LIGHTLYY_END
