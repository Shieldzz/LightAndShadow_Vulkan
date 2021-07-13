#pragma once

#include <string>
#include <vector>

#include "Object.h"
#include "Material.h"
#include "Buffer.h"
#include "VertexData.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct MeshData
{
	glm::mat4				m_Model;

	MaterialData			m_Material;

	uint32_t	m_CascadeShadowIndex;

	float	m_LodBias;
};

//----------------------------------------------------------------

class Mesh : public Object
{
public:
	// Constructor
	Mesh(const VkDevice logicalDevice, const std::string &path, const std::string &name);

	// Destructor
	virtual ~Mesh();

	void			Render(const VkCommandBuffer commandBuffer, uint32_t isOpaque);

	void			UpdateData(const VkDevice logicalDevice, const std::vector<VertexData> &vertices, const std::vector<uint16_t> &indices);

	// Getter
	std::string		GetPath() const { return m_Path; };
	bool			IsOpaque() const { return m_IsOpaque; }
	Material&		GetMaterial() { return m_Material; };
	float			GetLodBias() const { return m_LodBias; }

	// Setter
	virtual void	SetPosition(const glm::vec3 &position) override { Object::SetPosition(position); };
	virtual void	SetRotation(const glm::quat &rotation) override { Object::SetRotation(rotation); };
	virtual void	SetScale(const glm::vec3 &scale) override { Object::SetScale(scale); };

	void			SetPath(const std::string &path) { m_Path = path; };
	void			SetOpaque(const bool &opaque) { m_IsOpaque = opaque; };
	void			SetMaterial(const Material &material) { m_Material = material; };
	void			SetLodBias(float lodBias) { m_LodBias = lodBias; };

protected:
	std::string				m_Path;
	bool					m_IsOpaque;
	Material				m_Material;
	std::vector<uint16_t>	m_Indices;

	float					m_LodBias;

	Buffer<VertexData>		m_VertexBuffer;
	Buffer<uint16_t>		m_IndexBuffer;
}; // class Mesh

//----------------------------------------------------------------

LIGHTLYY_END
