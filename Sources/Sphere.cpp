#include "Sphere.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

#define M_PI 3.14159265359f

//----------------------------------------------------------------

Sphere::Sphere(const VkDevice logicalDevice, uint32_t precision, const std::string &path, const std::string &name, const bool bTriangleStrip)
:	Mesh(logicalDevice, path, name)
{
	std::vector<VertexData>	vertices;

	if (bTriangleStrip)
	{
		vertices.reserve((precision + 1) * (precision + 1));
		m_Indices.reserve((precision + 1) * (precision + 1));
		
		for (unsigned int y = 0; y <= precision; ++y)
		{
			for (unsigned int x = 0; x <= precision; ++x)
			{
				float		xSegment = (float)x / (float)precision;
				float		ySegment = (float)y / (float)precision;
		
				float		theta = ySegment * M_PI;
				float		phi = xSegment * 2.f * M_PI;
		
				float		xPos = std::cos(phi) * std::sin(theta);
				float		yPos = std::cos(theta);
				float		zPos = std::sin(phi) * std::sin(theta);
		
				VertexData	vtx
				{
					glm::vec3(xPos, yPos, zPos),
					glm::vec2(xSegment, ySegment),
					glm::vec3(xPos, yPos, zPos),
					glm::vec4(0.f, 0.f, 0.f, 0.f)
				};
		
				vertices.push_back(vtx);
			}
		}
		
		bool	oddRow = false;
		for (uint32_t y = 0; y < precision; ++y)
		{
			if (!oddRow)
			{
				for (uint32_t x = 0; x <= precision; ++x)
				{
					m_Indices.push_back((y + 1)	* (precision + 1) + x);
					m_Indices.push_back(y * (precision + 1) + x);
				}
			}
			else
			{
				for (int32_t x = precision; x >= 0; --x)
				{
					m_Indices.push_back(y			* (precision + 1) + x);
					m_Indices.push_back((y + 1)	* (precision + 1) + x);
				}
			}
		
			oddRow = !oddRow;
		}
	}
	else
	{
		float sectorStep = 2 * M_PI / precision;
		float stackStep = M_PI / precision;
		float sectorAngle, stackAngle;

		vertices.reserve((precision + 1) * (precision + 1));

		for (int i = 0; i <= precision; ++i)
		{
			float stackAngle = M_PI / 2 - i * stackStep;
			float xy = cosf(stackAngle);
			float y = sinf(stackAngle);

			for (int j = 0; j <= precision; ++j)
			{
				sectorAngle = j * sectorStep;

				float x = xy * cosf(sectorAngle);
				float z = -xy * sinf(sectorAngle);

				float s = (float)j / precision;
				float t = (float)i / precision;

				VertexData	vtx
				{
					glm::vec3(x, y, z),
					glm::vec2(s, t),
					glm::vec3(x, y, z),
					glm::vec4(0.f, 0.f, 0.f, 0.f)
				};
				vertices.push_back(vtx);
			}
		}

		m_Indices.reserve(precision * precision * 3);
		for (int i = 0; i < precision; ++i)
		{
			int k1 = i * (precision + 1);
			int k2 = k1 + precision + 1;

			for (int j = 0; j < precision; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					m_Indices.push_back(k1);
					m_Indices.push_back(k2);
					m_Indices.push_back(k1 + 1);
				}

				if (i != (precision - 1))
				{
					m_Indices.push_back(k1 + 1);
					m_Indices.push_back(k2);
					m_Indices.push_back(k2 + 1);
				}
			}
		}
	}

	m_VertexBuffer = Buffer<VertexData>(logicalDevice, BUFFER_TYPE::Vertex, static_cast<uint32_t>(vertices.size()));
	m_IndexBuffer = Buffer<uint16_t>(logicalDevice, BUFFER_TYPE::Index, static_cast<uint32_t>(m_Indices.size()));
	UpdateData(logicalDevice, vertices, m_Indices);
}

//----------------------------------------------------------------

Sphere::~Sphere()
{

}

//----------------------------------------------------------------

LIGHTLYY_END
