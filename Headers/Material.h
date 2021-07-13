#pragma once

#include "glm/glm.hpp"

#include "Texture.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct MaterialData
{
	MaterialData()
	:	m_Albedo({ 1.f, 1.f, 1.f, 1.f }),
		m_Roughness(0.5f),
		m_Metallic(0.f),
		m_Reflectance(0.5f)
	{
		
	}

	glm::vec4	m_Albedo;
	float		m_Roughness;
	float		m_Metallic;
	float		m_Reflectance;
}; // struct MaterialData

//----------------------------------------------------------------

class Material
{
public:
	// Constructor
	Material();
	Material(const std::string &texturePath);
	Material(const std::string &texturePath, const std::string &normalPath);

	// Getter
	float		GetRoughness() const { return m_Data.m_Roughness; };
	float		GetMetallic() const { return m_Data.m_Metallic; };
	float		GetReflectance() const { return m_Data.m_Reflectance; };
	glm::vec4	GetAlbedo() const { return m_Data.m_Albedo; };
	Texture&	GetTexture() { return m_Texture; };
	Texture&	GetNormalTexture() { return m_NormalTexture; };

	// Setter
	void		SetRoughness(const float roughness) { m_Data.m_Roughness = roughness; };
	void		SetMetallic(const float metallic) { m_Data.m_Metallic = metallic; };
	void		SetReflectance(const float reflectance) { m_Data.m_Reflectance = reflectance; };
	void		SetAlbedo(const glm::vec4 &albedo) { m_Data.m_Albedo = albedo; };
	void		SetTexture(const Texture &texture) { m_Texture = texture; };

private:
	MaterialData	m_Data;

	Texture			m_Texture;
	Texture			m_NormalTexture;
}; // class Material

//----------------------------------------------------------------

LIGHTLYY_END
