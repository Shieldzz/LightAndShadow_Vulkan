#pragma once

#include <string>

#include "Utility.h"
#include "Initializers.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct Texture
{
	// Constructor
	Texture() {};
	Texture(const std::string &path, uint32_t desiredChannelCount);

	void						FreeTexture();

	// Getter
	const std::string&			GetPath() const { return m_Path; };
	uint32_t					GetWidth() const { return m_Width; };
	uint32_t					GetHeight() const { return m_Height; };
	uint32_t					GetChannel() const { return m_Channel; };
	uint32_t					GetMipmapLevels() const { return m_MipmapLevels; }

	const std::vector<stbi_uc>&	GetPixels() const { return m_Pixels; }

	VkImage					m_Image;
	VkImageView				m_ImageView;
	VkDeviceMemory			m_Memory;
	VkSampler				m_Sampler;

private:
	std::string				m_Path;

	uint32_t				m_Width;
	uint32_t				m_Height;
	uint32_t				m_Channel;
	uint32_t				m_MipmapLevels;

	std::vector<stbi_uc>	m_Pixels;
}; // struct Texture

//----------------------------------------------------------------

LIGHTLYY_END
