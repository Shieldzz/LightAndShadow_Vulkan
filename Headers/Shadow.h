#pragma once

#include "Initializers.h"
#include "Light.h"

#include "glm/glm.hpp"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

#define SHADOWMAP_SPOTLIGHT_COUNT 1 // same to cascade -> TODO :  

//----------------------------------------------------------------

#define SHADOWMAP_CASCADE_COUNT 4
#define SHADOWMAP_DIM 2048

//----------------------------------------------------------------

struct ShadowInfoSpotLight
{
	glm::mat4	m_LightSpace;
};

struct ShadowInfoCascade
{
	glm::mat4	m_LightSpace[SHADOWMAP_CASCADE_COUNT];
	float		m_CascadeSplits[SHADOWMAP_CASCADE_COUNT];
	int			m_ShowCascade;
	int			m_ShowPCFFilter;
};

//----------------------------------------------------------------

// Calculate split depths base on view camera frustum
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
// https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp

class Shadow 
{
public:
	// Contructor
	Shadow(float nearClip = 0.1f, float farClip = 100.0f);

	// Spot Light -> doesn't use cascade Shadow
	void				UpdateSpotLightShadow(const Light* spotLight, const glm::mat4 &cameraProj, const glm::mat4 &cameraView);


	// CascadeShadow -> Directionnal Light
	void				UpdateCascadeShadow(const glm::vec3& directionnalLightPos, const glm::mat4 &cameraProj, const glm::mat4 &cameraView);
	void				CalculateSplits();
	void				CalculateOrthoFrustum(const glm::vec3& directionnalLightPos, const glm::mat4 &cameraProj, const glm::mat4 &cameraView);

	// Getters
	ShadowInfoSpotLight	GetShadowInfoSpotLight() const	{ return m_ShadowInfoSpotLight; }
	ShadowInfoCascade	GetShadowInfoCascade() const	{ return m_ShadowInfoCascade; }
	VkExtent2D			GetExtent2D() const				{ return m_Extent; }
	float				GetCascadeSplitCoeff() const	{ return m_CascadeSplitLambda; }
	bool				IsShowCascade() const			{ return m_ShadowInfoCascade.m_ShowCascade; }
	bool				IsShowPCFFilter() const			{ return m_ShadowInfoCascade.m_ShowPCFFilter; }


	void				SetCascadeSplitCoeff(float cascadeSplitCoeff) { m_CascadeSplitLambda = cascadeSplitCoeff; }
	
	
	void				ShowCascadeShadow(bool showCascade) { m_ShadowInfoCascade.m_ShowCascade = showCascade; }
	void				ShowPCFFilter(bool showPCFFilter)		{ m_ShadowInfoCascade.m_ShowPCFFilter = showPCFFilter; }

private:
	// Spot Light Shadow
	ShadowInfoSpotLight			m_ShadowInfoSpotLight;

	// Cascade Shadow
	float						m_CascadeSplits[SHADOWMAP_CASCADE_COUNT];
	float						m_CascadeSplitLambda = 0.95f;

	glm::vec4					m_FrustumCascadeSplits;
	glm::mat4					m_LightSpace[SHADOWMAP_CASCADE_COUNT];

	float						m_NearClip;
	float						m_FarClip;

	float						m_ClipRange;
	float						m_MinZ;
	float						m_MaxZ;
	float						m_Range;
	float						m_Ratio;

	VkExtent2D					m_Extent;

	ShadowInfoCascade			m_ShadowInfoCascade;
}; // class Shadow

//----------------------------------------------------------------

LIGHTLYY_END
