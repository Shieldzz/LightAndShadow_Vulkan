#pragma once

#include <vector>
#include <fstream>
#include <string>

#include <stb_image.h>

//----------------------------------------------------------------

#define LIGHTLYY_BEGIN namespace Lightlyy {
#define LIGHTLYY_END }
#define ENGINE_DATA_PATH "../../Data/"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

static std::vector<char>	LoadFile(const std::string &path, uint32_t openMode)
{
	std::ifstream file(path, openMode);

	if (file.is_open())
	{
		size_t fileSize = file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	return std::vector<char>();
}

//----------------------------------------------------------------

static std::vector<stbi_uc> LoadImageTextureWithSTB(const std::string &path, uint32_t *width, uint32_t *height, uint32_t *channel, uint32_t	desiredChannelCount)
{
	stbi_uc*				pixels = stbi_load(path.c_str(), reinterpret_cast<int*>(width), reinterpret_cast<int*>(height), reinterpret_cast<int*>(channel), desiredChannelCount);

	std::vector<stbi_uc>	p = std::vector<stbi_uc>(pixels, pixels + ((*width) * (*height) * desiredChannelCount));

	stbi_image_free(pixels);

	return p;
}

//----------------------------------------------------------------

LIGHTLYY_END
