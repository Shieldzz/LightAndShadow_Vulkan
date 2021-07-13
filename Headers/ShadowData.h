#pragma once

#include "glm/glm.hpp"

#include "Initializers.h"
#include "Utility.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct ShadowData
{
	glm::vec3	m_Pos;
}; // struct ShadowData

//----------------------------------------------------------------

struct ShadowDescription // must mirror VertexData
{
	ShadowDescription()
	{
		m_BindingDesc = { };
		{
			m_BindingDesc.binding = 0;
			m_BindingDesc.stride = sizeof(ShadowData);
			m_BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		m_AttribsDesc.resize(1);

		m_AttribsDesc[0].binding = 0;
		m_AttribsDesc[0].location = 0;
		m_AttribsDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_AttribsDesc[0].offset = offsetof(ShadowData, ShadowData::m_Pos);
	}

	const VkVertexInputBindingDescription&					GetBindingDesc() const { return m_BindingDesc; }
	const std::vector<VkVertexInputAttributeDescription>&	GetAttributesDesc() const { return m_AttribsDesc; }

private:
	VkVertexInputBindingDescription					m_BindingDesc;
	std::vector<VkVertexInputAttributeDescription>	m_AttribsDesc;
}; // struct VertexDescription

//----------------------------------------------------------------

LIGHTLYY_END
