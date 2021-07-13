#include "Shadow.h"
#include "glm/gtx/quaternion.hpp"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Shadow::Shadow(float nearClip, float farClip)
	: m_NearClip(nearClip), m_FarClip(farClip)
{
	m_ClipRange = m_FarClip - m_NearClip;
	m_MinZ		= m_NearClip;
	m_MaxZ		= m_NearClip + m_ClipRange;
	m_Range		= m_MaxZ - m_MinZ;
	m_Ratio		= m_MaxZ / m_MinZ;

	m_ShadowInfoCascade.m_ShowCascade = 0;
	m_ShadowInfoCascade.m_ShowPCFFilter = 1;

	m_Extent = { SHADOWMAP_DIM, SHADOWMAP_DIM };
}

//----------------------------------------------------------------

void Shadow::UpdateSpotLightShadow(const Light* spotLight, const glm::mat4 & cameraProj, const glm::mat4 & cameraView)
{
	glm::mat4	depthProjectionMatrix = glm::perspective(spotLight->GetAngle(), 1280.f / 720.0f, 1.0f, spotLight->GetRadius());
	depthProjectionMatrix[1][1] *= -1.f;
	glm::vec3	lightDir = glm::normalize(glm::vec3(spotLight->GetDirection()));
	lightDir *= -1;
	glm::mat4	depthViewMatrix = glm::lookAt(spotLight->GetPosition(), lightDir - spotLight->GetPosition(), glm::vec3(0.0f, 1.0f, 0.0f)); // value to the light -> TODO

	m_ShadowInfoSpotLight.m_LightSpace = depthProjectionMatrix * depthViewMatrix;
}

//----------------------------------------------------------------

void		Shadow::UpdateCascadeShadow(const glm::vec3& directionnalLightPos, const glm::mat4 &cameraProj, const glm::mat4 &cameraView)
{
	CalculateSplits();
	CalculateOrthoFrustum(directionnalLightPos, cameraProj, cameraView);
}

//----------------------------------------------------------------

void		Shadow::CalculateSplits()
{
	float size = static_cast<float>(SHADOWMAP_CASCADE_COUNT);
	for (uint32_t idxSplit = 0; idxSplit < SHADOWMAP_CASCADE_COUNT; idxSplit++)
	{
		float p = (idxSplit + 1) / size;
		float log = m_MinZ * std::pow(m_Ratio, p);
		float uniform = m_MinZ + m_Range * p;
		float d = m_CascadeSplitLambda * (log - uniform) + uniform;
		m_CascadeSplits[idxSplit] = (d - m_NearClip) / m_ClipRange;
	}
}

//----------------------------------------------------------------

void		Shadow::CalculateOrthoFrustum(const glm::vec3& directionnalLightPos, const glm::mat4 &cameraProj, const glm::mat4 &cameraView)
{
	float lastSplitDist = 0.0f;
	for (uint32_t idxSplitDist = 0; idxSplitDist < SHADOWMAP_CASCADE_COUNT; idxSplitDist++)
	{
		float splitDist = m_CascadeSplits[idxSplitDist];

		glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into worldSpace
		glm::mat4 invCam = glm::inverse(cameraProj * cameraView);

		uint32_t frustumSize = 8;
		for (uint32_t idxFrustum = 0; idxFrustum < frustumSize; idxFrustum++)
		{
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[idxFrustum], 1.0f);
			frustumCorners[idxFrustum] = invCorner / invCorner.w;
		}

		uint32_t halfFrustumSize = frustumSize / 2;
		for (uint32_t idxFrustum = 0; idxFrustum < halfFrustumSize; idxFrustum++)
		{
			glm::vec3 dist = frustumCorners[halfFrustumSize + idxFrustum] - frustumCorners[idxFrustum];
			frustumCorners[halfFrustumSize + idxFrustum] = frustumCorners[idxFrustum] + (dist * splitDist);
			frustumCorners[idxFrustum] = frustumCorners[idxFrustum] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t idxCenter = 0; idxCenter < frustumSize; idxCenter++)
			frustumCenter += frustumCorners[idxCenter];

		frustumCenter /= frustumSize;

		float radius = 0.0f;
		for (uint32_t idxRadius = 0; idxRadius < frustumSize; idxRadius++)
		{
			float distance = glm::length(frustumCorners[idxRadius] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f; // ??? 2x size frustum???

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		// ONLY FOR DIRECTION LIGHT

		glm::vec3 light = glm::normalize(-directionnalLightPos);

		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
		lightOrthoMatrix[1][1] *= -1.f;
		glm::mat4 lightViewMatrix = glm::lookAt(
			frustumCenter - light * -minExtents.z,
			frustumCenter,
			glm::vec3(0.0f, 1.0f, 0.0f));

		m_ShadowInfoCascade.m_CascadeSplits[idxSplitDist] = (m_NearClip + splitDist * m_ClipRange) * -1.0f;
		m_ShadowInfoCascade.m_LightSpace[idxSplitDist] = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = m_CascadeSplits[idxSplitDist];
	}
}

//----------------------------------------------------------------

LIGHTLYY_END

