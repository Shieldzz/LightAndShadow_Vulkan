#include "Material.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Material::Material()
{
	m_Data = MaterialData();
}

//----------------------------------------------------------------

Material::Material(const std::string &texturePath)
{
	m_Data = MaterialData();

	m_Texture = Texture(texturePath, 4);
}

//----------------------------------------------------------------

Material::Material(const std::string &texturePath, const std::string &normalPath)
{
	m_Data = MaterialData();

	m_Texture = Texture(texturePath, 4);
	m_NormalTexture = Texture(normalPath, 4);
}

//----------------------------------------------------------------

LIGHTLYY_END
