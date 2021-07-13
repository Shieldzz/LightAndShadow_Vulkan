#pragma once

#include "Initializers.h"
#include "RenderHandle.h"
#include "Camera.h"
#include "Mesh.h"
#include "Light.h"
#include "Shadow.h"
#include "Skybox.h"
#include "UniformDescription.h"
#include "imgui/UI.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct VP
{
	glm::mat4	m_View;
	glm::mat4	m_Proj;
};

//----------------------------------------------------------------

class Scene
{
public:
	Scene() = delete;
	Scene(const RenderHandle *renderHandle);
	~Scene();

	bool						Setup(const VkDevice logicalDevice);
	void						Prepare(const VkDevice logicalDevice);
	void						Render(const UI *ui);
	void						Shutdown(const VkDevice logicalDevice);

	void						AddMesh(const VkDevice logicalDevice, Mesh *mesh);
	void						DeleteMesh(Mesh *mesh);
	void						DeleteLight(Light *light);
	void						AddLight(Light *light);

	// getters
	const VkRenderPass			GetRenderPassObjects() const { return m_RenderPassObjects; }
	const std::vector<Object*>&	GetSceneObjects() const { return m_Objects; }
	Camera*						GetCameraUnsafe() const { return m_Camera; }
	const Camera*				GetCamera() const { return m_Camera; }
	const std::vector<Light*>&	GetLightObjects() const { return m_Lights; }
	Shadow*						GetShadow() const { return m_Shadow; }

private:
	bool						CreateRenderPasses(const VkDevice logicalDevice);
	bool						CreateFrameBuffers(const VkDevice logicalDevice);

	bool						CreateRenderPassesShadow(const VkDevice logicalDevice);
	bool						CreateFrameBuffersShadowCascade(const VkDevice logicalDevice);
	bool						CreateFrameBuffersShadowSpotLight(const VkDevice logicalDevice);

	bool						CreateGraphicsPipelines(const VkDevice logicalDevice,
														const std::vector<VkShaderModule> &meshModules,
														const std::vector<VkShaderModule> &offscreenModules,
														const std::vector<VkShaderModule> &skyboxModules,
														const std::vector<VkShaderModule> &shadowModules,
														const std::vector<VkShaderModule> &shadowSpotLightModule);
	bool						CreatePipelineLayoutObjects(const VkDevice logicalDevice);
	bool						CreatePipelineLayoutOffscreen(const VkDevice logicalDevice);
	bool						CreatePipelineLayoutSkybox(const VkDevice logicalDevice);
	bool						CreatePipelineLayoutShadowCascade(const VkDevice logicalDevice); // ShadowMap -> Only Directionnal 
	bool						CreatePipelineLayoutShadowSpotLight(const VkDevice logicalDevice); // ShadowMap -> Only SpotLight

	bool						CreateDepthBuffer(const VkDevice logicalDevice);
	bool						CreateImagesForObjects(const VkDevice logicalDevice);

	bool						CreateShaderModule(const VkDevice logicalDevice, const std::vector<char> &shaderByteCode, VkShaderModule &shaderModule);

	void						AddObject(Object *object);
	void						RemoveObject(Object *object);

	bool						UpdateMeshBuffer(const VkDevice logicalDevice, uint32_t newMeshIndex);

	std::string					GetDefinitiveObjectName(const std::string &name);

	const RenderHandle				*m_RenderHandle;

	VkRenderPass					m_RenderPassObjects;
	std::vector<VkFramebuffer>		m_FrameBuffersObjects;

	VkRenderPass					m_RenderPassShadow;
	std::vector<VkFramebuffer>		m_FrameBuffersShadowCascade;
	std::vector<VkFramebuffer>		m_FrameBuffersShadowSpotLight;

	std::vector<VkPipeline>			m_GraphicsPipelinesObjects;
	VkPipelineLayout				m_PipelineLayoutObjects;
	VkPipelineLayout				m_PiplelineLayoutOffscreen;
	VkPipelineLayout				m_PiplelineLayoutShadowCascade;
	VkPipelineLayout				m_PiplelineLayoutShadowSpotLight;
	std::vector<UniformDescription>	m_UniformDescriptions;

	std::vector<Buffer<VP>>			m_VPBuffers;
	std::vector<Buffer<MeshData>>	m_PerMeshBuffers;
	std::vector<Buffer<UBOLights>>	m_LightBuffers;
	std::vector<Buffer<ShadowInfoCascade>>	m_ShadowCascadeBuffers;
	std::vector<Buffer<ShadowInfoSpotLight>>m_ShadowSpotLightBuffers;

	uint32_t						m_PerMeshBufferAlignment;
	uint32_t						m_PerSpotShadowBufferAlignment;

	VkImage							m_DepthImage;
	VkImageView						m_DepthImageView;
	VkDeviceMemory					m_DepthMemory;

	std::vector<VkImage>			m_ImagesObjects;
	std::vector<VkImageView>		m_ImageViewsObjects;
	std::vector<VkDeviceMemory>		m_ObjectsMemories;

	// ShadowCascade
	std::vector<VkImage>			m_ShadowCascadeImages;
	std::vector<VkImageView>		m_ShadowCascadeImageViews;
	std::vector<VkDeviceMemory>		m_ShadowCascadeMemories;
	VkSampler						m_ShadowCascadeSampler;

	// Shadow SpotLight
	std::vector<VkImage>			m_ShadowSpotLightImages;
	std::vector<VkImageView>		m_ShadowSpotLightImageViews;
	std::vector<VkDeviceMemory>		m_ShadowSpotLightMemories;
	VkSampler						m_ShadowSpotLightSampler;

	std::vector<Mesh*>				m_Meshes;
	std::vector<Light*>				m_Lights;
	int								m_SpotLightCount;

	MeshData*						m_MeshesData;

	std::vector<Object*>			m_Objects; // contains meshes and lights
	std::vector<std::string>		m_ObjectsNames;

	Camera							*m_Camera;

	Skybox							m_Skybox;

	Shadow							*m_Shadow;
}; // class Scene

//----------------------------------------------------------------

LIGHTLYY_END
