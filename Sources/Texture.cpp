#include "Texture.h"

#include <algorithm>

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Texture::Texture(const std::string &path, uint32_t desiredChannelCount)
:	m_Path(path)
{
	m_Pixels = LoadImageTextureWithSTB(m_Path, &m_Width, &m_Height, &m_Channel, desiredChannelCount);

	m_MipmapLevels = floor(log2(std::max(m_Width, m_Height))) + 1;
}

//----------------------------------------------------------------

void	Texture::FreeTexture()
{
	m_Pixels.clear();
}

//----------------------------------------------------------------

LIGHTLYY_END
