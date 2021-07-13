#pragma once

#include "Utility.h"
#include "Mesh.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

class Sphere : public Mesh
{
public:
	Sphere(const VkDevice logicalDevice, uint32_t precision, const std::string &path, const std::string &name, const bool bTriangleStrip = false);
	virtual ~Sphere();
}; // class Sphere

//----------------------------------------------------------------

LIGHTLYY_END
