#include "Scene.h"

#include <algorithm>

#include "Utility.h"
#include "Sphere.h"
#include "ShadowData.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Scene::Scene(const RenderHandle *renderHandle)
:	m_RenderHandle(renderHandle),
	m_RenderPassObjects(nullptr),
	m_FrameBuffersObjects(std::vector<VkFramebuffer>()),
	m_ImagesObjects(std::vector<VkImage>()),
	m_ImageViewsObjects(std::vector<VkImageView>()),
	m_ObjectsMemories(std::vector<VkDeviceMemory>())
{
	m_Meshes = std::vector<Mesh*>();

	m_Camera = new Camera(45.f, glm::vec2(1280, 720), 0.1f, 1000.f, glm::vec3(0.f, 1.f, 0.f));
	m_Camera->SetPosition(glm::vec3(0.f, 0.f, 5.f));

	m_Shadow = new Shadow(0.1f, 1000.0f);
}

//----------------------------------------------------------------

Scene::~Scene()
{
	m_RenderHandle = nullptr;

	for (int idx = 0; idx < m_Meshes.size(); ++idx)
	{
		delete m_Meshes[idx];
		m_Meshes[idx] = nullptr;
	}
	m_Meshes.clear();
	
	if (m_Camera != nullptr)
		delete m_Camera;

	m_Camera = nullptr;
}

//----------------------------------------------------------------

bool	Scene::Setup(const VkDevice logicalDevice)
{
	if (!CreateRenderPasses(logicalDevice))
		return false;

	if (!CreateDepthBuffer(logicalDevice))
		return false;

	if (!CreateImagesForObjects(logicalDevice))
		return false;

	if (!CreateFrameBuffers(logicalDevice))
		return false;

	if (!CreateRenderPassesShadow(logicalDevice))
		return false;

	if (!CreateFrameBuffersShadowSpotLight(logicalDevice))
		return false;

	if (!CreateFrameBuffersShadowCascade(logicalDevice))
		return false;

	Mesh	*sphere = new Sphere(logicalDevice, 64, ENGINE_DATA_PATH"Textures/Basic.jpg", "Sphere");
	AddObject(sphere);
	m_Meshes.push_back(sphere);
	m_RenderHandle->PrepareTexture(logicalDevice, sphere->GetMaterial().GetTexture());
	m_Meshes[0]->SetPosition({ -4.f, 0.f, 0.f });

	Mesh	*teapot = new Mesh(logicalDevice, ENGINE_DATA_PATH"Models/Teapot/teapot.obj", "Teapot");
	teapot->SetMaterial(Material(ENGINE_DATA_PATH"Textures/Basic.jpg"));
	AddObject(teapot);
	m_Meshes.push_back(teapot);
	m_RenderHandle->PrepareTexture(logicalDevice, teapot->GetMaterial().GetTexture());
	m_Meshes[1]->SetPosition({ 1.f, 0.f, 0.f });
	m_Meshes[1]->SetScale({ 0.1f, 0.1f, 0.1f });

	Mesh	*ironman = new Mesh(logicalDevice, ENGINE_DATA_PATH"Models/ironman/ironman.fbx", "IronMan");
	AddObject(ironman);
	m_Meshes.push_back(ironman);
	m_RenderHandle->PrepareTexture(logicalDevice, ironman->GetMaterial().GetTexture());
	m_Meshes[2]->SetPosition({ 0.f, 2.f, 0.f });

	LightData lightData;
	{
		lightData.m_Color = glm::vec4(1, 1, 1, 1);
		lightData.m_Position = glm::vec4(0, 0, 5, 1);
		lightData.m_Direction = glm::vec4(0, 1, 1, 1);
	
		lightData.m_Radius = 200.0f;
		lightData.m_Angle = 1.f;
		lightData.m_Intensity = 1.f;
		lightData.m_Attenuation = 0.f;
		lightData.m_Type = (uint32_t)OBJECT_TYPE::DirectionalLight;
	}
	AddLight(new Light(0, "Light", lightData));
	m_Lights[0]->SetEulerAngles({ -45.f, 0.f, 0.f });

	m_Skybox = Skybox(logicalDevice, ENGINE_DATA_PATH"Skyboxes/Teide/", ".jpg");
	m_RenderHandle->PrepareSkybox(logicalDevice, m_Skybox);

	// load shaders
	VkShaderModule	meshVertexShaderModule = nullptr;
	VkShaderModule	meshFragShaderModule = nullptr;

	VkShaderModule	offscreenVertexShaderModule = nullptr;
	VkShaderModule	offscreenFragShaderModule = nullptr;

	VkShaderModule	skyboxVertexShaderModule = nullptr;
	VkShaderModule	skyboxFragShaderModule = nullptr;

	VkShaderModule	shadowCascadeVertexShaderModule = nullptr;
	VkShaderModule	shadowSpotLightVertexShaderModule = nullptr;

	const uint32_t	shadersOpenMode = std::ios::binary | std::ios::ate;

	if (!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Mesh.vert.spv", shadersOpenMode), meshVertexShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Mesh.frag.spv", shadersOpenMode), meshFragShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Offscreen.vert.spv", shadersOpenMode), offscreenVertexShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Offscreen.frag.spv", shadersOpenMode), offscreenFragShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Skybox.vert.spv", shadersOpenMode), skyboxVertexShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/Skybox.frag.spv", shadersOpenMode), skyboxFragShaderModule) ||
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/ShadowCascade.vert.spv", shadersOpenMode), shadowCascadeVertexShaderModule) || // Only shadow Directionnal with cascade shadow Map
		!CreateShaderModule(logicalDevice, LoadFile(ENGINE_DATA_PATH"Shaders/ShadowSpotLight.vert.spv", shadersOpenMode), shadowSpotLightVertexShaderModule)) // Only for SpotLight shadowMap
		return false;

	if (!CreateGraphicsPipelines(logicalDevice,
								{ meshVertexShaderModule, meshFragShaderModule },
								{ offscreenVertexShaderModule, offscreenFragShaderModule },
								{ skyboxVertexShaderModule, skyboxFragShaderModule }, 
								{ shadowCascadeVertexShaderModule },
								{ shadowSpotLightVertexShaderModule }))
		return false;

	vkDestroyShaderModule(logicalDevice, meshVertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, meshFragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, offscreenVertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, offscreenFragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, skyboxVertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, skyboxFragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, shadowCascadeVertexShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, shadowSpotLightVertexShaderModule, nullptr);

	return true;
}

//----------------------------------------------------------------

void	Scene::Prepare(const VkDevice logicalDevice)
{
	const uint8_t	currentFrame = m_RenderHandle->GetCurrentFrame();

	VP				vp = { };
	{
		vp.m_View = m_Camera->GetView();
		vp.m_Proj = m_Camera->GetProjection();
	}

	const std::vector<VP>	vpData = { vp };
	m_VPBuffers[currentFrame].UpdateData(logicalDevice, vpData);

	if (m_Lights.size() > 0)
	{
		m_Shadow->UpdateCascadeShadow(m_Lights[0]->GetPosition(), vp.m_Proj, vp.m_View);
	}

	const std::vector<ShadowInfoCascade> shadowInfoCascadeData = { m_Shadow->GetShadowInfoCascade() };
	m_ShadowCascadeBuffers[currentFrame].UpdateData(logicalDevice, shadowInfoCascadeData);

	const uint32_t			meshesCount = static_cast<uint32_t>(m_Meshes.size());

	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOWMAP_CASCADE_COUNT; ++cascadeIndex)
		{
			MeshData	*meshData = reinterpret_cast<MeshData*>((reinterpret_cast<uint64_t>(m_MeshesData) + ((meshIndex * SHADOWMAP_CASCADE_COUNT + cascadeIndex) * m_PerMeshBufferAlignment)));
			Mesh		*mesh = m_Meshes[meshIndex];

			(*meshData).m_Model = mesh->GetModel();

			Material	&material = mesh->GetMaterial();
			(*meshData).m_Material.m_Albedo = material.GetAlbedo();
			(*meshData).m_Material.m_Roughness = material.GetRoughness();
			(*meshData).m_Material.m_Metallic = material.GetMetallic();
			(*meshData).m_Material.m_Reflectance = material.GetReflectance();

			(*meshData).m_CascadeShadowIndex = cascadeIndex;

			(*meshData).m_LodBias = mesh->GetLodBias();
		}
	}

	m_PerMeshBuffers[currentFrame].UpdateData(logicalDevice, m_MeshesData);

	UBOLights	lights = {};
	lights.m_count = m_Lights.size();

	for (uint32_t lightIndex = 0; lightIndex < m_Lights.size(); ++lightIndex)
	{
		lights.m_lights[lightIndex] = m_Lights[lightIndex]->GetData();
		lights.m_lights[lightIndex].m_Position = lights.m_lights[lightIndex].m_Position * m_Camera->GetInvView();
		glm::vec3	lightDir = glm::vec3(lights.m_lights[lightIndex].m_Direction) * glm::mat3(m_Camera->GetInvView());
		lights.m_lights[lightIndex].m_Direction = glm::vec4(lightDir, 1.0);
	}

	const std::vector<UBOLights> lightData = { lights };
	m_LightBuffers[currentFrame].UpdateData(logicalDevice, lightData);

	// WARNING only 4 max spotLight 
	for (uint32_t lightIndex = 0; lightIndex < m_Lights.size(); ++lightIndex)
	{
		if (m_Lights[lightIndex]->GetType() == (uint32_t)OBJECT_TYPE::SpotLight)
		{
			m_Shadow->UpdateSpotLightShadow(m_Lights[lightIndex], vp.m_Proj, vp.m_View);
			const std::vector<ShadowInfoSpotLight> shadowSpotLight = { m_Shadow->GetShadowInfoSpotLight() };
			m_ShadowSpotLightBuffers[currentFrame].UpdateData(logicalDevice, shadowSpotLight);
		}
	}
}

//----------------------------------------------------------------

void	Scene::Render(const UI *ui)
{
	const VkCommandBuffer			commandBuffer = m_RenderHandle->GetCurrentCommandBuffer();

	const uint32_t					imageIndex = m_RenderHandle->GetCurrentFrame();// SwapchainImage();
	const std::vector<VkClearValue>	&clearValues = m_RenderHandle->GetClearValues();
	const uint8_t					currentFrame = m_RenderHandle->GetCurrentFrame();
	const uint32_t					meshesCount = static_cast<uint32_t>(m_Meshes.size());

	// RenderPass Shadow
	VkRenderPassBeginInfo			shadowRenderPassBeginInfo = { };
	{
		shadowRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		shadowRenderPassBeginInfo.renderPass = m_RenderPassShadow;
		shadowRenderPassBeginInfo.renderArea.extent = m_Shadow->GetExtent2D();
		shadowRenderPassBeginInfo.clearValueCount = 1;
		shadowRenderPassBeginInfo.pClearValues = &clearValues[1];
	}
	// TODO -> RenderPass SpotLight -> sans cascade 
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[5]);

	// render meshes
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		for (uint32_t spotLightIndex = 0; spotLightIndex < SHADOWMAP_SPOTLIGHT_COUNT; ++spotLightIndex)
		{
			shadowRenderPassBeginInfo.framebuffer = m_FrameBuffersShadowSpotLight[spotLightIndex]; // new for spotlight
			vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

			uint32_t	perMeshBufferOffset = meshIndex * m_PerMeshBufferAlignment * SHADOWMAP_CASCADE_COUNT;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PiplelineLayoutShadowCascade, 0, 1, &m_UniformDescriptions[1].GetDescriptors()[imageIndex], 1, &perMeshBufferOffset);

			m_Meshes[meshIndex]->Render(commandBuffer, 0);
			m_Meshes[meshIndex]->Render(commandBuffer, 1);
			
			vkCmdEndRenderPass(commandBuffer);
		}
	}


	// RenderPass Directionnal Light Shadow Cascade

	// bind pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[4]);

	for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOWMAP_CASCADE_COUNT; ++cascadeIndex)
	{
		// Update framebuffer for cascade framebuffer
		shadowRenderPassBeginInfo.framebuffer = m_FrameBuffersShadowCascade[cascadeIndex];
		vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// render meshes
		for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
		{
			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

			uint32_t	perMeshBufferOffset = ((meshIndex * SHADOWMAP_CASCADE_COUNT + cascadeIndex) * m_PerMeshBufferAlignment);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PiplelineLayoutShadowCascade, 0, 1, &m_UniformDescriptions[1].GetDescriptors()[imageIndex], 1, &perMeshBufferOffset);

			m_Meshes[meshIndex]->Render(commandBuffer, 0);
			m_Meshes[meshIndex]->Render(commandBuffer, 1);
		}

		// end render
		vkCmdEndRenderPass(commandBuffer);
	}


	// bind render pass
	VkRenderPassBeginInfo			renderPassBeginInfo = { };
	{
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_RenderPassObjects;
		renderPassBeginInfo.framebuffer = m_FrameBuffersObjects[imageIndex];
		renderPassBeginInfo.renderArea.extent = m_RenderHandle->GetSwapchainExtent();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();
	}

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// bind pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[0]);
	
	// subpass 0
	// render opaque meshes
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		uint32_t	perMeshBufferOffset = meshIndex * m_PerMeshBufferAlignment * SHADOWMAP_CASCADE_COUNT;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayoutObjects, 0, 1, &m_UniformDescriptions[meshIndex + 2].GetDescriptors()[imageIndex], 1, &perMeshBufferOffset);
	
		m_Meshes[meshIndex]->Render(commandBuffer, 1);
	}
	
	// render skybox
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[1]);
	uint32_t	offset = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Skybox.m_PipelineLayout, 0, 1, &m_UniformDescriptions[2].GetDescriptors()[imageIndex], 1, &offset);
	m_Skybox.Render(commandBuffer);
	
	// render transparent meshes
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[2]);
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		uint32_t	perMeshBufferOffset = meshIndex * m_PerMeshBufferAlignment * SHADOWMAP_CASCADE_COUNT;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayoutObjects, 0, 1, &m_UniformDescriptions[meshIndex + 2].GetDescriptors()[imageIndex], 1, &perMeshBufferOffset);

		m_Meshes[meshIndex]->Render(commandBuffer, 0);
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[3]);
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		uint32_t	perMeshBufferOffset = meshIndex * m_PerMeshBufferAlignment * SHADOWMAP_CASCADE_COUNT;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayoutObjects, 0, 1, &m_UniformDescriptions[meshIndex + 2].GetDescriptors()[imageIndex], 1, &perMeshBufferOffset);

		m_Meshes[meshIndex]->Render(commandBuffer, 0);
	}

	// render UI
	const_cast<UI*>(ui)->Render(commandBuffer);

	// subpass 1
	/*vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelinesObjects[1]);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PiplelineLayoutOffscreen, 0, 1, &m_UniformDescriptions[0].GetDescriptors()[imageIndex], 0, nullptr);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);*/

	// end render
	vkCmdEndRenderPass(commandBuffer);
}

//----------------------------------------------------------------

void	Scene::Shutdown(const VkDevice logicalDevice)
{
	// destroy meshes
	const uint32_t	meshesCount = static_cast<uint32_t>(m_Meshes.size());
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
		delete m_Meshes[meshIndex];

	m_Meshes.clear();

	const uint32_t	vpBuffersCount = static_cast<uint32_t>(m_VPBuffers.size());
	for (uint32_t bufferIndex = 0; bufferIndex < vpBuffersCount; ++bufferIndex)
		m_VPBuffers[bufferIndex].Destroy(logicalDevice);

	m_VPBuffers.clear();

	const uint32_t	perMeshBuffersCount = static_cast<uint32_t>(m_PerMeshBuffers.size());
	for (uint32_t bufferIndex = 0; bufferIndex < perMeshBuffersCount; ++bufferIndex)
		m_PerMeshBuffers[bufferIndex].Destroy(logicalDevice);

	m_PerMeshBuffers.clear();

	// destroy frame buffers and pipelines
	const uint32_t	frameBuffersCount = static_cast<uint32_t>(m_FrameBuffersObjects.size());
	for (uint32_t frameIndex = 0; frameIndex < frameBuffersCount; ++frameIndex)
	{
		vkDestroyImage(logicalDevice, m_ImagesObjects[frameIndex], nullptr);
		vkDestroyImageView(logicalDevice, m_ImageViewsObjects[frameIndex], nullptr);
		vkFreeMemory(logicalDevice, m_ObjectsMemories[frameIndex], nullptr);

		vkDestroyFramebuffer(logicalDevice, m_FrameBuffersObjects[frameIndex], nullptr);

		vkDestroyPipeline(logicalDevice, m_GraphicsPipelinesObjects[frameIndex], nullptr);
	}

	vkDestroyPipelineLayout(logicalDevice, m_PipelineLayoutObjects, nullptr);
	vkDestroyPipelineLayout(logicalDevice, m_PiplelineLayoutOffscreen, nullptr);

	m_ImagesObjects.clear();
	m_ImageViewsObjects.clear();
	m_ObjectsMemories.clear();
	m_FrameBuffersObjects.clear();

	vkDestroyImage(logicalDevice, m_DepthImage, nullptr);
	vkDestroyImageView(logicalDevice, m_DepthImageView, nullptr);
	vkFreeMemory(logicalDevice, m_DepthMemory, nullptr);

	if (m_RenderPassObjects != nullptr)
		vkDestroyRenderPass(logicalDevice, m_RenderPassObjects, nullptr);

	m_RenderPassObjects = nullptr;
	m_RenderHandle = nullptr;
}

//----------------------------------------------------------------

void	Scene::AddObject(Object *object)
{
	object->SetName(GetDefinitiveObjectName(object->GetName()));

	m_Objects.push_back(object);
}

//----------------------------------------------------------------

void	Scene::RemoveObject(Object *object)
{
	std::vector<Object*>::iterator	objectFound = std::find(m_Objects.begin(), m_Objects.end(), object);
	if (objectFound != m_Objects.end())
	{
		uint32_t	index = objectFound - m_Objects.begin();
		m_Objects.erase(objectFound);
		m_ObjectsNames.erase(m_ObjectsNames.begin() + index);
	}
}

//----------------------------------------------------------------

void	Scene::AddMesh(const VkDevice logicalDevice, Mesh *mesh)
{
	// TODO: check if all resources used by mesh are correctly initialized and ready for rendering

	AddObject(mesh);

	m_Meshes.push_back(mesh);

	m_RenderHandle->PrepareTexture(logicalDevice, mesh->GetMaterial().GetTexture());
	// TODO: finish that 
	//m_RenderHandle->PrepareTexture(logicalDevice, mesh->GetMaterial().GetNormalTexture());

	UpdateMeshBuffer(logicalDevice, static_cast<uint32_t>(m_Meshes.size() - 1));
}

//----------------------------------------------------------------

void	Scene::DeleteMesh(Mesh *mesh)
{
	std::vector<Mesh*>::iterator	meshFound = std::find(m_Meshes.begin(), m_Meshes.end(), mesh);
	if (meshFound != m_Meshes.end())
	{
		RemoveObject(mesh);

		uint32_t	index = meshFound - m_Meshes.begin();
		m_Meshes.erase(meshFound);
		m_UniformDescriptions.erase(m_UniformDescriptions.begin() + (index + 2));
	}
}

//----------------------------------------------------------------

void	Scene::DeleteLight(Light *light)
{
	std::vector<Light*>::iterator	lightFound = std::find(m_Lights.begin(), m_Lights.end(), light);
	if (lightFound != m_Lights.end())
	{
		RemoveObject(light);

		uint32_t	index = lightFound - m_Lights.begin();
		m_Lights.erase(lightFound);
	}
}

//----------------------------------------------------------------

void	Scene::AddLight(Light *light)
{
	AddObject(light);

	m_Lights.push_back(light);
	
	if (light->GetType() == (uint32_t)OBJECT_TYPE::SpotLight)
	{
		//CreateFrameBuffersShadowSpotLight();


		m_SpotLightCount++;
	}

}

//----------------------------------------------------------------

bool	Scene::CreateRenderPasses(const VkDevice logicalDevice) // TODO: better management for this function
{
	const VkFormat							surfaceFormat = m_RenderHandle->GetSurfaceFormat().format;
	const VkSampleCountFlagBits				antiAliasingLevel = Device::m_Device->GetMaxAALevel();

	std::vector<VkAttachmentDescription>	attachmentsDescription;
	attachmentsDescription.resize(3);
	{
		attachmentsDescription[0] = { };
		attachmentsDescription[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDescription[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDescription[0].format = surfaceFormat;
		attachmentsDescription[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDescription[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentsDescription[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[0].samples = antiAliasingLevel;

		attachmentsDescription[1] = { };
		attachmentsDescription[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDescription[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[1].format = VK_FORMAT_D32_SFLOAT;
		attachmentsDescription[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDescription[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachmentsDescription[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[1].samples = antiAliasingLevel;

		/*attachmentsDescription[2] = { };
		attachmentsDescription[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDescription[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[2].format = surfaceFormat;
		attachmentsDescription[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDescription[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentsDescription[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[2].samples = VK_SAMPLE_COUNT_1_BIT;*/

		attachmentsDescription[2] = { };
		attachmentsDescription[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDescription[2].format = surfaceFormat;
		attachmentsDescription[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDescription[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentsDescription[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription[2].samples = VK_SAMPLE_COUNT_1_BIT;
	}

	VkAttachmentReference	objectsColorRef = { };
	{
		objectsColorRef.attachment = 0;
		objectsColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference	offscreenColorRef = { };
	{
		offscreenColorRef.attachment = 0;
		offscreenColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference	offscreenInputRef = { };
	{
		offscreenInputRef.attachment = 2;
		offscreenInputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAttachmentReference	depthRef = { };
	{
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference	colorResolve = { };
	{
		colorResolve.attachment = 2;
		colorResolve.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	std::vector<VkSubpassDescription>		subpassesDescription;
	subpassesDescription.resize(1);
	{
		// first subpass for objects
		subpassesDescription[0] = { };
		subpassesDescription[0].colorAttachmentCount = 1;
		subpassesDescription[0].pColorAttachments = &objectsColorRef;
		subpassesDescription[0].pDepthStencilAttachment = &depthRef;
		subpassesDescription[0].pResolveAttachments = &colorResolve;
		subpassesDescription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		// second subpass for offscreen render
		/*subpassesDescription[1] = { };
		subpassesDescription[1].colorAttachmentCount = 1;
		subpassesDescription[1].pColorAttachments = &offscreenColorRef;
		subpassesDescription[1].inputAttachmentCount = 1;
		subpassesDescription[1].pInputAttachments = &offscreenInputRef;
		subpassesDescription[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;*/
	}

	std::vector<VkSubpassDependency>		subpassesDependency;
	subpassesDependency.resize(2);
	{
		subpassesDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassesDependency[0].dstSubpass = 0;
		subpassesDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassesDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassesDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassesDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassesDependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		/*subpassesDependency[1].srcSubpass = 0;
		subpassesDependency[1].dstSubpass = 1;
		subpassesDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassesDependency[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassesDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassesDependency[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassesDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;*/

		subpassesDependency[1].srcSubpass = 0;
		subpassesDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassesDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassesDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassesDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassesDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassesDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}

	// create render pass for objects (opaque, transparent, skybox and lights)
	const VkRenderPassCreateInfo	renderPassCreateInfo = Initializers::RenderPass::CreateInfo(attachmentsDescription, subpassesDescription, subpassesDependency);

	CHECK_API_SUCCESS(vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPassObjects)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreateFrameBuffers(const VkDevice logicalDevice)
{
	VkFramebufferCreateInfo			frameBufferCreateInfo = Initializers::Buffer::FrameCreateInfo(	m_RenderPassObjects,
																									std::vector<VkImageView>(),
																									m_RenderHandle->GetSwapchainExtent(), 1);

	const uint32_t					framesCount = m_RenderHandle->GetPendingFramesCount();
	m_FrameBuffersObjects.resize(framesCount);

	const std::vector<VkImageView>	&swapchainImageViews = m_RenderHandle->GetSwapchainImageViews();

	for (uint32_t frameIndex = 0; frameIndex < framesCount; ++frameIndex)
	{
		const VkImageView	attachments[3] = { m_ImageViewsObjects[frameIndex], m_DepthImageView, swapchainImageViews[frameIndex] };
		frameBufferCreateInfo.attachmentCount = 3;
		frameBufferCreateInfo.pAttachments = attachments;

		CHECK_API_SUCCESS(vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo, nullptr, &m_FrameBuffersObjects[frameIndex])); // TODO: add error management
	}
	
	return true;
}

//----------------------------------------------------------------

bool Scene::CreateRenderPassesShadow(const VkDevice logicalDevice)
{
	// SHADOW RENDERPASS
	VkAttachmentDescription attachmentsDescription = {};
	{
		attachmentsDescription.format = VK_FORMAT_D32_SFLOAT;
		attachmentsDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentsDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAttachmentReference shadowRef = {};
	{
		shadowRef.attachment = 0;
		shadowRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription shadowSubpass = {};
	{
		shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		shadowSubpass.flags = 0;
		shadowSubpass.pDepthStencilAttachment = &shadowRef;
	}

	VkRenderPassCreateInfo shadowRenderPassInfo = {};
	{
		shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		shadowRenderPassInfo.attachmentCount = 1;
		shadowRenderPassInfo.pAttachments = &attachmentsDescription;
		shadowRenderPassInfo.subpassCount = 1;
		shadowRenderPassInfo.pSubpasses = &shadowSubpass;
	}

	CHECK_API_SUCCESS(vkCreateRenderPass(logicalDevice, &shadowRenderPassInfo, nullptr, &m_RenderPassShadow));

	return true;
}

//----------------------------------------------------------------

bool Scene::CreateFrameBuffersShadowCascade(const VkDevice logicalDevice)
{
	VkExtent2D extent = { SHADOWMAP_DIM, SHADOWMAP_DIM };

	VkFramebufferCreateInfo			frameBufferCreateInfo = Initializers::Buffer::FrameCreateInfo(m_RenderPassShadow,
		std::vector<VkImageView>(),
		extent,
		1);

	VkImageCreateInfo		imageCreateInfo = Initializers::Image::CreateInfo(VK_IMAGE_TYPE_2D,
		extent,
		1, SHADOWMAP_CASCADE_COUNT,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SAMPLE_COUNT_1_BIT);

	m_RenderHandle->CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, m_ShadowCascadeImages, m_ShadowCascadeMemories);

	VkImageSubresourceRange subRange = { };
	{
		subRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		subRange.baseMipLevel = 0;
		subRange.levelCount = 1;
		subRange.baseArrayLayer = 0;
		subRange.layerCount = SHADOWMAP_CASCADE_COUNT;
	}

	m_RenderHandle->CreateImageViews(logicalDevice, m_ShadowCascadeImages, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_D32_SFLOAT, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, m_ShadowCascadeImageViews);

	m_FrameBuffersShadowCascade.resize(SHADOWMAP_CASCADE_COUNT);


	// One image and frambuffer per cascade
	for (uint32_t cascadeIndex = 0; cascadeIndex < SHADOWMAP_CASCADE_COUNT; ++cascadeIndex)
	{
		subRange.baseArrayLayer = cascadeIndex;
		subRange.layerCount = 1;

		std::vector<VkImageView>	cascadeImageViews;
		m_RenderHandle->CreateImageViews(logicalDevice, m_ShadowCascadeImages, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_D32_SFLOAT, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, cascadeImageViews);

		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments = &cascadeImageViews[0];

		CHECK_API_SUCCESS(vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo, nullptr, &m_FrameBuffersShadowCascade[cascadeIndex]));
	}

	return true;
}

//----------------------------------------------------------------

bool Scene::CreateFrameBuffersShadowSpotLight(const VkDevice logicalDevice)
{
	VkExtent2D extent = { SHADOWMAP_DIM, SHADOWMAP_DIM };

	VkFramebufferCreateInfo			frameBufferCreateInfo = Initializers::Buffer::FrameCreateInfo(m_RenderPassShadow,
		std::vector<VkImageView>(),
		extent,
		1);

	VkImageCreateInfo		imageCreateInfo = Initializers::Image::CreateInfo(VK_IMAGE_TYPE_2D,
		extent,
		1, SHADOWMAP_SPOTLIGHT_COUNT,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SAMPLE_COUNT_1_BIT);

	m_RenderHandle->CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, m_ShadowSpotLightImages, m_ShadowSpotLightMemories);

	VkImageSubresourceRange subRange = { };
	{
		subRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		subRange.baseMipLevel = 0;
		subRange.levelCount = 1;
		subRange.baseArrayLayer = 0;
		subRange.layerCount = SHADOWMAP_SPOTLIGHT_COUNT;
	}

	m_RenderHandle->CreateImageViews(logicalDevice, m_ShadowSpotLightImages, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_D32_SFLOAT, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, m_ShadowSpotLightImageViews);

	m_FrameBuffersShadowSpotLight.resize(SHADOWMAP_SPOTLIGHT_COUNT);

	// One image and frambuffer per light
	for (uint32_t spotLightIndex = 0; spotLightIndex < SHADOWMAP_SPOTLIGHT_COUNT; ++spotLightIndex)
	{
		subRange.baseArrayLayer = spotLightIndex;
		subRange.layerCount = 1;

		std::vector<VkImageView>	spotLightImageViews;
		m_RenderHandle->CreateImageViews(logicalDevice, m_ShadowSpotLightImages, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_D32_SFLOAT, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, spotLightImageViews);

		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments = &spotLightImageViews[0];

		CHECK_API_SUCCESS(vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo, nullptr, &m_FrameBuffersShadowSpotLight[spotLightIndex]));
	}

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreateGraphicsPipelines(	const VkDevice logicalDevice,
										const std::vector<VkShaderModule> &meshModules,
										const std::vector<VkShaderModule> &offscreenModules,
										const std::vector<VkShaderModule> &skyboxModules,
										const std::vector<VkShaderModule> &shadowCascadeModule,
										const std::vector<VkShaderModule> &shadowSpotLightModule)
{
	// TODO : Encapsulate
	m_ShadowCascadeBuffers = { Buffer<ShadowInfoCascade>(logicalDevice, BUFFER_TYPE::Uniform, 1), Buffer<ShadowInfoCascade>(logicalDevice, BUFFER_TYPE::Uniform, 1) };
	m_ShadowSpotLightBuffers = { Buffer<ShadowInfoSpotLight>(logicalDevice, BUFFER_TYPE::Uniform, 1), Buffer<ShadowInfoSpotLight>(logicalDevice, BUFFER_TYPE::Uniform, 1) };

	uint64_t						uboMinAlignment = Device::m_Device->GetUBOMinAlignment();
	m_PerMeshBufferAlignment = static_cast<uint32_t>((sizeof(MeshData) + uboMinAlignment - 1) & ~(uboMinAlignment - 1));

	const uint32_t					meshesCount = static_cast<uint32_t>(m_Meshes.size());

	m_VPBuffers = { Buffer<VP>(logicalDevice, BUFFER_TYPE::Uniform, 1), Buffer<VP>(logicalDevice, BUFFER_TYPE::Uniform, 1) };
	// TODO: pre allocate buffer, if it exceeds, create new buffer chunk
	m_PerMeshBuffers =
	{
		Buffer<MeshData>(logicalDevice, static_cast<BUFFER_TYPE>(BUFFER_TYPE::Uniform), 100, m_PerMeshBufferAlignment),
		Buffer<MeshData>(logicalDevice, static_cast<BUFFER_TYPE>(BUFFER_TYPE::Uniform), 100, m_PerMeshBufferAlignment)
	};

	m_MeshesData = static_cast<MeshData*>(_aligned_malloc(static_cast<size_t>(meshesCount * SHADOWMAP_CASCADE_COUNT), static_cast<size_t>(m_PerMeshBufferAlignment)));

	// TODO : Multi light
	m_LightBuffers =
	{
		Buffer<UBOLights>(logicalDevice, BUFFER_TYPE::Uniform, 1),
		Buffer<UBOLights>(logicalDevice, BUFFER_TYPE::Uniform, 1)
	};

	m_PerSpotShadowBufferAlignment = static_cast<uint32_t>((sizeof(ShadowInfoSpotLight) + uboMinAlignment - 1) & ~(uboMinAlignment - 1));

	if (!CreatePipelineLayoutOffscreen(logicalDevice))
		return false;

	if (!CreatePipelineLayoutShadowCascade(logicalDevice))
		return false;

	/*if (!CreatePipelineLayoutShadowSpotLight(logicalDevice))
		return false;*/

	if (!CreatePipelineLayoutObjects(logicalDevice))
		return false;

	if (!CreatePipelineLayoutSkybox(logicalDevice))
		return false;

	VkPipelineShaderStageCreateInfo			meshVertexStage = { };
	{
		meshVertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		meshVertexStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Vertex);
		meshVertexStage.module = meshModules[0];
		meshVertexStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			meshFragStage = { };
	{
		meshFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		meshFragStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Fragment);
		meshFragStage.module = meshModules[1];
		meshFragStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			meshShaderStages[] = { meshVertexStage, meshFragStage };

	VkPipelineShaderStageCreateInfo			offscreenVertexStage = { };
	{
		offscreenVertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		offscreenVertexStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Vertex);
		offscreenVertexStage.module = offscreenModules[0];
		offscreenVertexStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			offscreenFragStage = { };
	{
		offscreenFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		offscreenFragStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Fragment);
		offscreenFragStage.module = offscreenModules[1];
		offscreenFragStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			offscreenShaderStages[] = { offscreenVertexStage, offscreenFragStage };

	VkPipelineShaderStageCreateInfo			skyboxVertexStage = { };
	{
		skyboxVertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		skyboxVertexStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Vertex);
		skyboxVertexStage.module = skyboxModules[0];
		skyboxVertexStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			skyboxFragStage = { };
	{
		skyboxFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		skyboxFragStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Fragment);
		skyboxFragStage.module = skyboxModules[1];
		skyboxFragStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			skyboxShaderStages[] = { skyboxVertexStage, skyboxFragStage };

	// Shadow Pipeline Shader
	// Directionnal Light => cascade shadow
	VkPipelineShaderStageCreateInfo			shadowCascadeVertexStage = { };
	{
		shadowCascadeVertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shadowCascadeVertexStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Vertex);
		shadowCascadeVertexStage.module = shadowCascadeModule[0];
		shadowCascadeVertexStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			shadowCascadeShaderStages[] = { shadowCascadeVertexStage };

	// SpotLight
	VkPipelineShaderStageCreateInfo			shadowSpotLightVertexStage = { };
	{
		shadowSpotLightVertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shadowSpotLightVertexStage.stage = static_cast<VkShaderStageFlagBits>(SHADER_STAGE::Vertex);
		shadowSpotLightVertexStage.module = shadowSpotLightModule[0];
		shadowSpotLightVertexStage.pName = "main";
	}

	VkPipelineShaderStageCreateInfo			shadowSpotLightShaderStages[] = { shadowSpotLightVertexStage };

	VertexDescription						meshVertexDesc = VertexDescription();
	const VkVertexInputBindingDescription	&meshVertexBindingDesc = meshVertexDesc.GetBindingDesc();
	const std::vector<VkVertexInputAttributeDescription>	&meshAttribsDesc = meshVertexDesc.GetAttributesDesc();

	VkPipelineVertexInputStateCreateInfo	meshVertexStateCreateInfo = { };
	{
		meshVertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		meshVertexStateCreateInfo.vertexBindingDescriptionCount = 1;
		meshVertexStateCreateInfo.pVertexBindingDescriptions = &meshVertexBindingDesc;
		meshVertexStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(meshAttribsDesc.size());
		meshVertexStateCreateInfo.pVertexAttributeDescriptions = meshAttribsDesc.data();
	}

	VkPipelineVertexInputStateCreateInfo	offscreenVertexStateCreateInfo = { };
	{
		offscreenVertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		offscreenVertexStateCreateInfo.vertexBindingDescriptionCount = 0;
		offscreenVertexStateCreateInfo.vertexAttributeDescriptionCount = 0;
	}

	VkPipelineVertexInputStateCreateInfo	skyboxVertexStateCreateInfo = { };
	{
		skyboxVertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		skyboxVertexStateCreateInfo.vertexBindingDescriptionCount = 1;
		skyboxVertexStateCreateInfo.pVertexBindingDescriptions = &m_Skybox.GetBindingDesc();
		skyboxVertexStateCreateInfo.vertexAttributeDescriptionCount = 1;
		skyboxVertexStateCreateInfo.pVertexAttributeDescriptions = &m_Skybox.GetAttributesDesc();
	}

	/*ShadowDescription						shadowVertexDesc = ShadowDescription();
	const VkVertexInputBindingDescription	&shadowVertexBindingDesc = shadowVertexDesc.GetBindingDesc();
	const std::vector<VkVertexInputAttributeDescription>	&shadowAttribsDesc = shadowVertexDesc.GetAttributesDesc();

	VkPipelineVertexInputStateCreateInfo	shadowVertexStateCreateInfo = { };
	{
		shadowVertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		shadowVertexStateCreateInfo.vertexBindingDescriptionCount = 1;
		shadowVertexStateCreateInfo.pVertexBindingDescriptions = &shadowVertexBindingDesc;
		shadowVertexStateCreateInfo.vertexAttributeDescriptionCount = 1;
		shadowVertexStateCreateInfo.pVertexAttributeDescriptions = shadowAttribsDesc.data();
	}*/

	VkPipelineInputAssemblyStateCreateInfo	pipelineInputAssembly = { };
	{
		pipelineInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineInputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	VkPipelineInputAssemblyStateCreateInfo	skyboxPipelineInputAssembly = { };
	{
		skyboxPipelineInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		skyboxPipelineInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		skyboxPipelineInputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	VkPipelineRasterizationStateCreateInfo	rasterizerStateCreateInfo = { };
	{
		rasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerStateCreateInfo.lineWidth = 1.f;
		rasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerStateCreateInfo.depthBiasEnable = VK_FALSE;
	}

	VkPipelineRasterizationStateCreateInfo	transparentFrontRasterizerState = rasterizerStateCreateInfo;
	transparentFrontRasterizerState.cullMode = VK_CULL_MODE_FRONT_BIT;

	VkPipelineRasterizationStateCreateInfo	shadowRasterizerStateCreateInfo = rasterizerStateCreateInfo;
	shadowRasterizerStateCreateInfo.depthBiasEnable = VK_TRUE;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_DEPTH_BIAS };

	VkPipelineDynamicStateCreateInfo shadowDynamicStateCreateInfo = { };
	{
		shadowDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		shadowDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();
		shadowDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	}

	VkPipelineRasterizationStateCreateInfo	skyboxRasterizerStateCreateInfo = rasterizerStateCreateInfo;
	skyboxRasterizerStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

	VkPipelineMultisampleStateCreateInfo	multisampling = { };
	{
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = Device::m_Device->GetMaxAALevel();
	}

	VkPipelineMultisampleStateCreateInfo	shadowMultisampling = multisampling;
	shadowMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState		colorBlendAttachment = { };
	{
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;//SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}

	VkPipelineColorBlendStateCreateInfo		colorBlendingOpaque = { };
	{
		colorBlendingOpaque.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingOpaque.logicOpEnable = VK_FALSE;
		colorBlendingOpaque.attachmentCount = 1;
		colorBlendingOpaque.pAttachments = &colorBlendAttachment;
	}

	colorBlendAttachment.blendEnable = VK_TRUE;

	VkPipelineColorBlendStateCreateInfo		colorBlendingTransparent = { };
	{
		colorBlendingTransparent.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingTransparent.logicOpEnable = VK_FALSE;
		colorBlendingTransparent.attachmentCount = 1;
		colorBlendingTransparent.pAttachments = &colorBlendAttachment;
	}

	VkPipelineDepthStencilStateCreateInfo	opaqueDepthState = { };
	{
		opaqueDepthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		opaqueDepthState.depthTestEnable = VK_TRUE;
		opaqueDepthState.depthWriteEnable = VK_TRUE;
		opaqueDepthState.depthCompareOp = VK_COMPARE_OP_LESS;
		opaqueDepthState.depthBoundsTestEnable = VK_FALSE;
	}

	VkPipelineDepthStencilStateCreateInfo	transparentDepthState = { };
	{
		transparentDepthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		transparentDepthState.depthTestEnable = VK_TRUE;
		transparentDepthState.depthWriteEnable = VK_FALSE;
		transparentDepthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		transparentDepthState.depthBoundsTestEnable = VK_FALSE;
	}

	VkPipelineDepthStencilStateCreateInfo	shadowStencilCreateInfo = opaqueDepthState;
	shadowStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineDepthStencilStateCreateInfo	 skyboxDepthState = { };
	{
		skyboxDepthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		skyboxDepthState.depthTestEnable = VK_TRUE;
		skyboxDepthState.depthWriteEnable = VK_FALSE;
		skyboxDepthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		skyboxDepthState.depthBoundsTestEnable = VK_FALSE;
	}

	const VkExtent2D						swapchainExtent = m_RenderHandle->GetSwapchainExtent();

	VkViewport								viewport = { };
	{
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}

	VkRect2D								scissor = { };
	{
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;
	}

	VkPipelineViewportStateCreateInfo		viewportState = { };
	{
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
	}

	// Shadow Viewport
	VkViewport								shadowViewport = { };
	{
		shadowViewport.x = 0.0f;
		shadowViewport.y = 0.0f;
		shadowViewport.width = SHADOWMAP_DIM;
		shadowViewport.height = SHADOWMAP_DIM;
		shadowViewport.minDepth = 0.0f;
		shadowViewport.maxDepth = 1.0f;
	}

	VkRect2D								shadowScissor = { };
	{
		shadowScissor.offset = { 0, 0 };
		shadowScissor.extent = { SHADOWMAP_DIM , SHADOWMAP_DIM };
	}

	VkPipelineViewportStateCreateInfo		shadowViewportState = { };
	{
		shadowViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		shadowViewportState.pNext = nullptr;
		shadowViewportState.viewportCount = 1;
		shadowViewportState.pViewports = &shadowViewport;
		shadowViewportState.scissorCount = 1;
		shadowViewportState.pScissors = &shadowScissor;
	}

	VkGraphicsPipelineCreateInfo			pipelinesInfoObjects[6];
	{
		// opaque objects
		pipelinesInfoObjects[0] = { };
		pipelinesInfoObjects[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[0].stageCount = 2;
		pipelinesInfoObjects[0].pStages = meshShaderStages;
		pipelinesInfoObjects[0].pVertexInputState = &meshVertexStateCreateInfo;
		pipelinesInfoObjects[0].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[0].pViewportState = &viewportState;
		pipelinesInfoObjects[0].pRasterizationState = &rasterizerStateCreateInfo;
		pipelinesInfoObjects[0].pMultisampleState = &multisampling;
		pipelinesInfoObjects[0].pColorBlendState = &colorBlendingOpaque;
		pipelinesInfoObjects[0].layout = m_PipelineLayoutObjects;
		pipelinesInfoObjects[0].renderPass = m_RenderPassObjects;
		pipelinesInfoObjects[0].subpass = 0;
		pipelinesInfoObjects[0].pDepthStencilState = &opaqueDepthState;

		// offscreen
		/*pipelinesInfoObjects[1] = { };
		pipelinesInfoObjects[1].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[1].stageCount = 2;
		pipelinesInfoObjects[1].pStages = offscreenShaderStages;
		pipelinesInfoObjects[1].pVertexInputState = &offscreenVertexStateCreateInfo;
		pipelinesInfoObjects[1].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[1].pViewportState = &viewportState;
		pipelinesInfoObjects[1].pRasterizationState = &rasterizerStateCreateInfo;
		pipelinesInfoObjects[1].pMultisampleState = &multisampling;
		pipelinesInfoObjects[1].pColorBlendState = &colorBlendingOpaque;
		pipelinesInfoObjects[1].layout = m_PiplelineLayoutOffscreen;
		pipelinesInfoObjects[1].renderPass = m_RenderPassObjects;
		pipelinesInfoObjects[1].subpass = 1;*/

		// skybox
		pipelinesInfoObjects[1] = { };
		pipelinesInfoObjects[1].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[1].stageCount = 2;
		pipelinesInfoObjects[1].pStages = skyboxShaderStages;
		pipelinesInfoObjects[1].pVertexInputState = &skyboxVertexStateCreateInfo;
		pipelinesInfoObjects[1].pInputAssemblyState = &skyboxPipelineInputAssembly;
		pipelinesInfoObjects[1].pViewportState = &viewportState;
		pipelinesInfoObjects[1].pRasterizationState = &skyboxRasterizerStateCreateInfo;
		pipelinesInfoObjects[1].pMultisampleState = &multisampling;
		pipelinesInfoObjects[1].pColorBlendState = &colorBlendingOpaque;
		pipelinesInfoObjects[1].layout = m_Skybox.m_PipelineLayout;
		pipelinesInfoObjects[1].renderPass = m_RenderPassObjects;
		pipelinesInfoObjects[1].subpass = 0;
		pipelinesInfoObjects[1].pDepthStencilState = &skyboxDepthState;

		// transparents objects front
		pipelinesInfoObjects[2] = { };
		pipelinesInfoObjects[2].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[2].stageCount = 2;
		pipelinesInfoObjects[2].pStages = meshShaderStages;
		pipelinesInfoObjects[2].pVertexInputState = &meshVertexStateCreateInfo;
		pipelinesInfoObjects[2].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[2].pViewportState = &viewportState;
		pipelinesInfoObjects[2].pRasterizationState = &transparentFrontRasterizerState;
		pipelinesInfoObjects[2].pMultisampleState = &multisampling;
		pipelinesInfoObjects[2].pColorBlendState = &colorBlendingTransparent;
		pipelinesInfoObjects[2].layout = m_PipelineLayoutObjects;
		pipelinesInfoObjects[2].renderPass = m_RenderPassObjects;
		pipelinesInfoObjects[2].subpass = 0;
		pipelinesInfoObjects[2].pDepthStencilState = &transparentDepthState;

		// transparent objects back
		pipelinesInfoObjects[3] = { };
		pipelinesInfoObjects[3].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[3].stageCount = 2;
		pipelinesInfoObjects[3].pStages = meshShaderStages;
		pipelinesInfoObjects[3].pVertexInputState = &meshVertexStateCreateInfo;
		pipelinesInfoObjects[3].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[3].pViewportState = &viewportState;
		pipelinesInfoObjects[3].pRasterizationState = &rasterizerStateCreateInfo;
		pipelinesInfoObjects[3].pMultisampleState = &multisampling;
		pipelinesInfoObjects[3].pColorBlendState = &colorBlendingTransparent;
		pipelinesInfoObjects[3].layout = m_PipelineLayoutObjects;
		pipelinesInfoObjects[3].renderPass = m_RenderPassObjects;
		pipelinesInfoObjects[3].subpass = 0;
		pipelinesInfoObjects[3].pDepthStencilState = &transparentDepthState;

		// shadow Cascade
		pipelinesInfoObjects[4] = { };
		pipelinesInfoObjects[4].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[4].stageCount = 1;
		pipelinesInfoObjects[4].pStages = shadowCascadeShaderStages;
		pipelinesInfoObjects[4].pVertexInputState = &meshVertexStateCreateInfo;
		pipelinesInfoObjects[4].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[4].pViewportState = &shadowViewportState;
		pipelinesInfoObjects[4].pRasterizationState = &shadowRasterizerStateCreateInfo;
		pipelinesInfoObjects[4].pMultisampleState = &shadowMultisampling;
		pipelinesInfoObjects[4].layout = m_PiplelineLayoutShadowCascade;
		pipelinesInfoObjects[4].renderPass = m_RenderPassShadow;
		pipelinesInfoObjects[4].subpass = 0;
		pipelinesInfoObjects[4].pDepthStencilState = &shadowStencilCreateInfo;
		pipelinesInfoObjects[4].pDynamicState = &shadowDynamicStateCreateInfo;

		// shadow SpotLight
		pipelinesInfoObjects[5] = { };
		pipelinesInfoObjects[5].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelinesInfoObjects[5].stageCount = 1;
		pipelinesInfoObjects[5].pStages = shadowSpotLightShaderStages;
		pipelinesInfoObjects[5].pVertexInputState = &meshVertexStateCreateInfo;
		pipelinesInfoObjects[5].pInputAssemblyState = &pipelineInputAssembly;
		pipelinesInfoObjects[5].pViewportState = &shadowViewportState;
		pipelinesInfoObjects[5].pRasterizationState = &shadowRasterizerStateCreateInfo;
		pipelinesInfoObjects[5].pMultisampleState = &shadowMultisampling;
		pipelinesInfoObjects[5].layout = m_PiplelineLayoutShadowCascade;
		pipelinesInfoObjects[5].renderPass = m_RenderPassShadow;
		pipelinesInfoObjects[5].subpass = 0;
		pipelinesInfoObjects[5].pDepthStencilState = &shadowStencilCreateInfo;
		pipelinesInfoObjects[5].pDynamicState = &shadowDynamicStateCreateInfo;
	}

	m_GraphicsPipelinesObjects.resize(6);
	// TODO: add pipeline cache?
	CHECK_API_SUCCESS(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 6, pipelinesInfoObjects, nullptr, m_GraphicsPipelinesObjects.data()));

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreatePipelineLayoutObjects(const VkDevice logicalDevice)
{
	const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>>	descriptions =
	{
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // matrices view and proj
		std::make_tuple(DESCRIPTION_TYPE::UniformBufferDynamic, static_cast<SHADER_STAGE>((int)SHADER_STAGE::Vertex | (int)SHADER_STAGE::Fragment)), // dynamic per mesh
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // mesh texture
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // skybox
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Fragment), // light
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // shadowMapCascadeSampler
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, static_cast<SHADER_STAGE>((int)SHADER_STAGE::Vertex | (int)SHADER_STAGE::Fragment)), // shadowBufferCasacade
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // shadowMapSpotLight
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // shadowBufferSpotLight
	};

	std::vector<DescriptionInfo>	infos = std::vector<DescriptionInfo>(descriptions.size() * m_RenderHandle->GetPendingFramesCount(), DescriptionInfo());

	// matrices view and proj buffer
	VkDescriptorBufferInfo			bufferInfoFirstFrame = { };
	{
		bufferInfoFirstFrame.buffer = m_VPBuffers[0].GetApiBuffer();
		bufferInfoFirstFrame.offset = 0;
		bufferInfoFirstFrame.range = sizeof(VP);
	}

	VkDescriptorBufferInfo			bufferInfoSecondFrame = { };
	{
		bufferInfoSecondFrame.buffer = m_VPBuffers[1].GetApiBuffer();
		bufferInfoSecondFrame.offset = 0;
		bufferInfoSecondFrame.range = sizeof(VP);
	}

	// dynamic per mesh buffer
	VkDescriptorBufferInfo			bufferInfoDynamicFirstFrame = { };
	{
		bufferInfoDynamicFirstFrame.buffer = m_PerMeshBuffers[0].GetApiBuffer();
		bufferInfoDynamicFirstFrame.offset = 0;
		bufferInfoDynamicFirstFrame.range = VK_WHOLE_SIZE;
	}

	VkDescriptorBufferInfo			bufferInfoDynamicSecondFrame = { };
	{
		bufferInfoDynamicSecondFrame.buffer = m_PerMeshBuffers[1].GetApiBuffer();
		bufferInfoDynamicSecondFrame.offset = 0;
		bufferInfoDynamicSecondFrame.range = VK_WHOLE_SIZE;
	}

	// light buffer
	VkDescriptorBufferInfo			lightBufferFirstFrame = { };
	{
		lightBufferFirstFrame.buffer = m_LightBuffers[0].GetApiBuffer();
		lightBufferFirstFrame.offset = 0;
		lightBufferFirstFrame.range = sizeof(UBOLights);
	}

	VkDescriptorBufferInfo			lightBufferSecondFrame = { };
	{
		lightBufferSecondFrame.buffer = m_LightBuffers[1].GetApiBuffer();
		lightBufferSecondFrame.offset = 0;
		lightBufferSecondFrame.range = sizeof(UBOLights);
	}

	// cascade shadow buffer -> Directionnal
	VkDescriptorBufferInfo			shadowCascadeBufferInfoFirstFrame = { };
	{
		shadowCascadeBufferInfoFirstFrame.buffer = m_ShadowCascadeBuffers[0].GetApiBuffer();
		shadowCascadeBufferInfoFirstFrame.offset = 0;
		shadowCascadeBufferInfoFirstFrame.range = sizeof(ShadowInfoCascade);
	}

	VkDescriptorBufferInfo			shadowCascadeBufferInfoSecondFrame = { };
	{
		shadowCascadeBufferInfoSecondFrame.buffer = m_ShadowCascadeBuffers[1].GetApiBuffer();
		shadowCascadeBufferInfoSecondFrame.offset = 0;
		shadowCascadeBufferInfoSecondFrame.range = sizeof(ShadowInfoCascade);
	}

	// spotLight shadow buffer 
	VkDescriptorBufferInfo			shadowSpotLightBufferInfoFirstFrame = { };
	{
		shadowSpotLightBufferInfoFirstFrame.buffer = m_ShadowSpotLightBuffers[0].GetApiBuffer();
		shadowSpotLightBufferInfoFirstFrame.offset = 0;
		shadowSpotLightBufferInfoFirstFrame.range = sizeof(ShadowInfoSpotLight);
	}

	VkDescriptorBufferInfo			shadowSpotLightBufferInfoSecondFrame = { };
	{
		shadowSpotLightBufferInfoSecondFrame.buffer = m_ShadowSpotLightBuffers[1].GetApiBuffer();
		shadowSpotLightBufferInfoSecondFrame.offset = 0;
		shadowSpotLightBufferInfoSecondFrame.range = sizeof(ShadowInfoSpotLight);
	}

	// skybox image
	const Texture					&skyboxTexture = m_Skybox.GetTextures()[0];
	VkDescriptorImageInfo			imageInfoSkybox = { };
	{
		imageInfoSkybox.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoSkybox.imageView = skyboxTexture.m_ImageView;
		imageInfoSkybox.sampler = skyboxTexture.m_Sampler;
	}

	m_RenderHandle->PrepareShadow(logicalDevice, m_ShadowCascadeSampler);

	// Shadow image
	VkDescriptorImageInfo			imageInfoShadowMapCascade = { };
	{
		imageInfoShadowMapCascade.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoShadowMapCascade.imageView = m_ShadowCascadeImageViews[0];
		imageInfoShadowMapCascade.sampler = m_ShadowCascadeSampler;
	}

	m_RenderHandle->PrepareShadow(logicalDevice, m_ShadowSpotLightSampler);

	VkDescriptorImageInfo			imageInfoShadowMapSpotLight = { };
	{
		imageInfoShadowMapSpotLight.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoShadowMapSpotLight.imageView = m_ShadowSpotLightImageViews[0];
		imageInfoShadowMapSpotLight.sampler = m_ShadowSpotLightSampler;
	}


	const uint32_t					meshesCount = static_cast<uint32_t>(m_Meshes.size());
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
	{
		const Texture			&texture = m_Meshes[meshIndex]->GetMaterial().GetTexture();

		// mesh image
		VkDescriptorImageInfo	imageInfo = { };
		{
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texture.m_ImageView;
			imageInfo.sampler = texture.m_Sampler;
		}

		// FIXME: dirty 
		// First Frame
		infos[0].m_DescBufferInfo = &bufferInfoFirstFrame;
		infos[1].m_DescBufferInfo = &bufferInfoDynamicFirstFrame;
		infos[2].m_DescImageInfo = &imageInfo;
		infos[3].m_DescImageInfo = &imageInfoSkybox;
		infos[4].m_DescBufferInfo = &lightBufferFirstFrame;
		infos[5].m_DescImageInfo = &imageInfoShadowMapCascade;	
		infos[6].m_DescBufferInfo = &shadowCascadeBufferInfoFirstFrame;
		infos[7].m_DescImageInfo = &imageInfoShadowMapSpotLight;
		infos[8].m_DescBufferInfo = &shadowSpotLightBufferInfoFirstFrame;

		// 2E Frame
		infos[9].m_DescBufferInfo = &bufferInfoSecondFrame;
		infos[10].m_DescBufferInfo = &bufferInfoDynamicSecondFrame;
		infos[11].m_DescImageInfo = &imageInfo;
		infos[12].m_DescImageInfo = &imageInfoSkybox;
		infos[13].m_DescBufferInfo = &lightBufferSecondFrame;
		infos[14].m_DescImageInfo = &imageInfoShadowMapCascade;
		infos[15].m_DescBufferInfo = &shadowCascadeBufferInfoSecondFrame;
		infos[16].m_DescImageInfo = &imageInfoShadowMapSpotLight;
		infos[17].m_DescBufferInfo = &shadowSpotLightBufferInfoSecondFrame;

		m_UniformDescriptions.push_back(UniformDescription(logicalDevice, descriptions, infos, m_RenderHandle->GetPendingFramesCount(), true));
	}

	VkPipelineLayoutCreateInfo	pipelineLayoutObjectsInfo = { };
	{
		pipelineLayoutObjectsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutObjectsInfo.setLayoutCount = 2;
		pipelineLayoutObjectsInfo.pSetLayouts = m_UniformDescriptions[2].GetDescriptorLayouts().data();
	}

	CHECK_API_SUCCESS(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutObjectsInfo, nullptr, &m_PipelineLayoutObjects)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreatePipelineLayoutOffscreen(const VkDevice logicalDevice)
{
	const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>>	descriptions =
	{
		std::make_tuple(DESCRIPTION_TYPE::InputAttachment, SHADER_STAGE::Fragment)
	};

	std::vector<DescriptionInfo>	infos = std::vector<DescriptionInfo>(2, DescriptionInfo());

	VkDescriptorImageInfo			imageInfoFirstFrame = { };
	{
		imageInfoFirstFrame.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoFirstFrame.imageView = m_RenderHandle->GetSwapchainImageViews()[0];
		imageInfoFirstFrame.sampler = VK_NULL_HANDLE;
	}

	VkDescriptorImageInfo			imageInfoSecondFrame = { };
	{
		imageInfoSecondFrame.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoSecondFrame.imageView = m_RenderHandle->GetSwapchainImageViews()[1];
		imageInfoSecondFrame.sampler = VK_NULL_HANDLE;
	}

	infos[0].m_DescImageInfo = &imageInfoFirstFrame;
	infos[1].m_DescImageInfo = &imageInfoSecondFrame;

	m_UniformDescriptions.push_back(UniformDescription(logicalDevice, descriptions, infos, 2, true));

	VkPipelineLayoutCreateInfo	pipelineLayoutOffscreenInfo = { };
	{
		pipelineLayoutOffscreenInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutOffscreenInfo.setLayoutCount = 2;
		pipelineLayoutOffscreenInfo.pSetLayouts = m_UniformDescriptions[0].GetDescriptorLayouts().data();
	}

	CHECK_API_SUCCESS(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutOffscreenInfo, nullptr, &m_PiplelineLayoutOffscreen)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreatePipelineLayoutSkybox(const VkDevice logicalDevice)
{
	VkPipelineLayoutCreateInfo	skyboxPipelineLayoutCreateInfo = { };
	{
		skyboxPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		skyboxPipelineLayoutCreateInfo.setLayoutCount = 2;
		skyboxPipelineLayoutCreateInfo.pSetLayouts = m_UniformDescriptions[3].GetDescriptorLayouts().data();
	}

	CHECK_API_SUCCESS(vkCreatePipelineLayout(logicalDevice, &skyboxPipelineLayoutCreateInfo, nullptr, &m_Skybox.m_PipelineLayout));

	return true;
}

//----------------------------------------------------------------

bool Scene::CreatePipelineLayoutShadowCascade(const VkDevice logicalDevice)
{
	const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>>	descriptions =
	{
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // shadowBuffer
		std::make_tuple(DESCRIPTION_TYPE::UniformBufferDynamic, SHADER_STAGE::Vertex), // dynamic per mesh
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // shadowBuffer
	};

	std::vector<DescriptionInfo>	infos = std::vector<DescriptionInfo>(descriptions.size() * m_RenderHandle->GetPendingFramesCount(), DescriptionInfo());

	VkDescriptorBufferInfo			bufferInfoFirstFrame = { };
	{
		bufferInfoFirstFrame.buffer = m_ShadowCascadeBuffers[0].GetApiBuffer();
		bufferInfoFirstFrame.offset = 0;
		bufferInfoFirstFrame.range = sizeof(ShadowInfoCascade);
	}

	VkDescriptorBufferInfo			bufferInfoSecondFrame = { };
	{
		bufferInfoSecondFrame.buffer = m_ShadowCascadeBuffers[1].GetApiBuffer();
		bufferInfoSecondFrame.offset = 0;
		bufferInfoSecondFrame.range = sizeof(ShadowInfoCascade);
	}

	// dynamic per mesh buffer
	VkDescriptorBufferInfo			bufferInfoDynamicFirstFrame = { };
	{
		bufferInfoDynamicFirstFrame.buffer = m_PerMeshBuffers[0].GetApiBuffer();
		bufferInfoDynamicFirstFrame.offset = 0;
		bufferInfoDynamicFirstFrame.range = VK_WHOLE_SIZE;
	}

	VkDescriptorBufferInfo			bufferInfoDynamicSecondFrame = { };
	{
		bufferInfoDynamicSecondFrame.buffer = m_PerMeshBuffers[1].GetApiBuffer();
		bufferInfoDynamicSecondFrame.offset = 0;
		bufferInfoDynamicSecondFrame.range = VK_WHOLE_SIZE;
	}

	VkDescriptorBufferInfo			bufferInfoSpotLightFirstFrame = { };
	{
		bufferInfoSpotLightFirstFrame.buffer = m_ShadowSpotLightBuffers[0].GetApiBuffer();
		bufferInfoSpotLightFirstFrame.offset = 0;
		bufferInfoSpotLightFirstFrame.range = sizeof(ShadowInfoSpotLight);
	}

	VkDescriptorBufferInfo			bufferInfoSpotLightSecondFrame = { };
	{
		bufferInfoSpotLightSecondFrame.buffer = m_ShadowSpotLightBuffers[1].GetApiBuffer();
		bufferInfoSpotLightSecondFrame.offset = 0;
		bufferInfoSpotLightSecondFrame.range = sizeof(ShadowInfoSpotLight);
	}

	// FIXME: dirty
	infos[0].m_DescBufferInfo = &bufferInfoFirstFrame;
	infos[1].m_DescBufferInfo = &bufferInfoDynamicFirstFrame;
	infos[2].m_DescBufferInfo = &bufferInfoSpotLightFirstFrame;
	infos[3].m_DescBufferInfo = &bufferInfoSecondFrame;
	infos[4].m_DescBufferInfo = &bufferInfoDynamicSecondFrame;
	infos[5].m_DescBufferInfo = &bufferInfoSpotLightSecondFrame;

	m_UniformDescriptions.push_back(UniformDescription(logicalDevice, descriptions, infos, m_RenderHandle->GetPendingFramesCount(), true));

	VkPipelineLayoutCreateInfo	shadowPipelineLayoutCreateInfo = { };
	{
		shadowPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		shadowPipelineLayoutCreateInfo.setLayoutCount = 2;
		shadowPipelineLayoutCreateInfo.pSetLayouts = m_UniformDescriptions[1].GetDescriptorLayouts().data();
	}

	CHECK_API_SUCCESS(vkCreatePipelineLayout(logicalDevice, &shadowPipelineLayoutCreateInfo, nullptr, &m_PiplelineLayoutShadowCascade));

	return true;
}

//----------------------------------------------------------------

bool Scene::CreatePipelineLayoutShadowSpotLight(const VkDevice logicalDevice)
{
	const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>>	descriptions =
	{
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // shadowBuffer
		std::make_tuple(DESCRIPTION_TYPE::UniformBufferDynamic, SHADER_STAGE::Vertex), // dynamic per mesh
	};

	std::vector<DescriptionInfo>	infos = std::vector<DescriptionInfo>(descriptions.size() * m_RenderHandle->GetPendingFramesCount(), DescriptionInfo());

	VkDescriptorBufferInfo			bufferInfoFirstFrame = { };
	{
		bufferInfoFirstFrame.buffer = m_ShadowSpotLightBuffers[0].GetApiBuffer();
		bufferInfoFirstFrame.offset = 0;
		bufferInfoFirstFrame.range = sizeof(ShadowInfoSpotLight);
	}

	VkDescriptorBufferInfo			bufferInfoSecondFrame = { };
	{
		bufferInfoSecondFrame.buffer = m_ShadowSpotLightBuffers[1].GetApiBuffer();
		bufferInfoSecondFrame.offset = 0;
		bufferInfoSecondFrame.range = sizeof(ShadowInfoSpotLight);
	}

	// dynamic per mesh buffer
	VkDescriptorBufferInfo			bufferInfoDynamicFirstFrame = { };
	{
		bufferInfoDynamicFirstFrame.buffer = m_PerMeshBuffers[0].GetApiBuffer();
		bufferInfoDynamicFirstFrame.offset = 0;
		bufferInfoDynamicFirstFrame.range = VK_WHOLE_SIZE;
	}

	VkDescriptorBufferInfo			bufferInfoDynamicSecondFrame = { };
	{
		bufferInfoDynamicSecondFrame.buffer = m_PerMeshBuffers[1].GetApiBuffer();
		bufferInfoDynamicSecondFrame.offset = 0;
		bufferInfoDynamicSecondFrame.range = VK_WHOLE_SIZE;
	}

	// FIXME: dirty
	infos[0].m_DescBufferInfo = &bufferInfoFirstFrame;
	infos[1].m_DescBufferInfo = &bufferInfoDynamicFirstFrame;
	infos[2].m_DescBufferInfo = &bufferInfoSecondFrame;
	infos[3].m_DescBufferInfo = &bufferInfoDynamicSecondFrame;

	m_UniformDescriptions.push_back(UniformDescription(logicalDevice, descriptions, infos, m_RenderHandle->GetPendingFramesCount(), true));

	VkPipelineLayoutCreateInfo	shadowPipelineLayoutCreateInfo = { };
	{
		shadowPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		shadowPipelineLayoutCreateInfo.setLayoutCount = 2;
		shadowPipelineLayoutCreateInfo.pSetLayouts = m_UniformDescriptions[2].GetDescriptorLayouts().data();
	}

	CHECK_API_SUCCESS(vkCreatePipelineLayout(logicalDevice, &shadowPipelineLayoutCreateInfo, nullptr, &m_PiplelineLayoutShadowSpotLight));

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreateDepthBuffer(const VkDevice logicalDevice)
{
	VkImageCreateInfo			imageCreateInfo = Initializers::Image::CreateInfo(	VK_IMAGE_TYPE_2D,
																					m_RenderHandle->GetSwapchainExtent(),
																					1, 1,
																					VK_FORMAT_D32_SFLOAT,
																					VK_IMAGE_TILING_OPTIMAL,
																					VK_IMAGE_LAYOUT_UNDEFINED,
																					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
																					Device::m_Device->GetMaxAALevel());

	std::vector<VkImage>		outImages;
	std::vector<VkDeviceMemory>	outMemories;
	if (!m_RenderHandle->CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, outImages, outMemories))
	{
		return false; // TODO: add log error
	}

	m_DepthImage = outImages[0];
	m_DepthMemory = outMemories[0];

	VkImageSubresourceRange		subRange = Initializers::Image::SubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT, 1, 0, 1, 0);

	std::vector<VkImageView>	outImageViews;

	if (!m_RenderHandle->CreateImageViews(	logicalDevice,
											outImages,
											VK_IMAGE_VIEW_TYPE_2D,
											VK_FORMAT_D32_SFLOAT,
											{ VK_COMPONENT_SWIZZLE_IDENTITY },
											subRange,
											outImageViews))
	{
		return false; // TODO: add log error
	}

	m_DepthImageView = outImageViews[0];

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreateImagesForObjects(const VkDevice logicalDevice)
{
	VkImageCreateInfo		imageCreateInfo = Initializers::Image::CreateInfo(	VK_IMAGE_TYPE_2D,
																				m_RenderHandle->GetSwapchainExtent(),
																				1, 1,
																				m_RenderHandle->GetSurfaceFormat().format,
																				VK_IMAGE_TILING_OPTIMAL,
																				VK_IMAGE_LAYOUT_UNDEFINED,
																				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
																				Device::m_Device->GetMaxAALevel());

	if (!m_RenderHandle->CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_RenderHandle->GetPendingFramesCount(), m_ImagesObjects, m_ObjectsMemories))
	{
		return false; // TODO: add log error
	}

	VkImageSubresourceRange	subRange = Initializers::Image::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1, 0);

	if (!m_RenderHandle->CreateImageViews(	logicalDevice,
											m_ImagesObjects,
											VK_IMAGE_VIEW_TYPE_2D,
											m_RenderHandle->GetSurfaceFormat().format,
											{ VK_COMPONENT_SWIZZLE_IDENTITY },
											subRange,
											m_ImageViewsObjects))
	{
		return false; // TODO: add log error
	}

	return true;
}

//----------------------------------------------------------------

bool	Scene::CreateShaderModule(const VkDevice logicalDevice, const std::vector<char> &shaderByteCode, VkShaderModule &shaderModule)
{
	VkShaderModuleCreateInfo	shaderModuleCreateInfo = { };
	{
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = shaderByteCode.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const unsigned int*>(shaderByteCode.data());
	}

	CHECK_API_SUCCESS(vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	Scene::UpdateMeshBuffer(const VkDevice logicalDevice, uint32_t newMeshIndex)
{
	// add uniform description
	const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>>	descriptions =
	{
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // matrices view and proj
		std::make_tuple(DESCRIPTION_TYPE::UniformBufferDynamic, static_cast<SHADER_STAGE>((int)SHADER_STAGE::Vertex | (int)SHADER_STAGE::Fragment)), // dynamic per mesh
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // mesh texture
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // skybox
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Fragment), // light
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // shadowMapCascadeSampler
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, static_cast<SHADER_STAGE>((int)SHADER_STAGE::Vertex | (int)SHADER_STAGE::Fragment)), // shadowBufferCasacade
		std::make_tuple(DESCRIPTION_TYPE::CombinedImageSampler, SHADER_STAGE::Fragment), // shadowMapSpotLight
		std::make_tuple(DESCRIPTION_TYPE::UniformBuffer, SHADER_STAGE::Vertex), // shadowBufferSpotLight
	};

	std::vector<DescriptionInfo>	infos = std::vector<DescriptionInfo>(descriptions.size() * m_RenderHandle->GetPendingFramesCount(), DescriptionInfo());

	// matrices view and proj buffer
	VkDescriptorBufferInfo			bufferInfoFirstFrame = { };
	{
		bufferInfoFirstFrame.buffer = m_VPBuffers[0].GetApiBuffer();
		bufferInfoFirstFrame.offset = 0;
		bufferInfoFirstFrame.range = sizeof(VP);
	}

	VkDescriptorBufferInfo			bufferInfoSecondFrame = { };
	{
		bufferInfoSecondFrame.buffer = m_VPBuffers[1].GetApiBuffer();
		bufferInfoSecondFrame.offset = 0;
		bufferInfoSecondFrame.range = sizeof(VP);
	}

	// dynamic per mesh buffer
	VkDescriptorBufferInfo			bufferInfoDynamicFirstFrame = { };
	{
		bufferInfoDynamicFirstFrame.buffer = m_PerMeshBuffers[0].GetApiBuffer();
		bufferInfoDynamicFirstFrame.offset = 0;
		bufferInfoDynamicFirstFrame.range = VK_WHOLE_SIZE;
	}

	VkDescriptorBufferInfo			bufferInfoDynamicSecondFrame = { };
	{
		bufferInfoDynamicSecondFrame.buffer = m_PerMeshBuffers[1].GetApiBuffer();
		bufferInfoDynamicSecondFrame.offset = 0;
		bufferInfoDynamicSecondFrame.range = VK_WHOLE_SIZE;
	}

	// light buffer
	VkDescriptorBufferInfo			lightBufferFirstFrame = { };
	{
		lightBufferFirstFrame.buffer = m_LightBuffers[0].GetApiBuffer();
		lightBufferFirstFrame.offset = 0;
		lightBufferFirstFrame.range = sizeof(UBOLights);
	}

	VkDescriptorBufferInfo			lightBufferSecondFrame = { };
	{
		lightBufferSecondFrame.buffer = m_LightBuffers[1].GetApiBuffer();
		lightBufferSecondFrame.offset = 0;
		lightBufferSecondFrame.range = sizeof(UBOLights);
	}

	// cascade shadow buffer -> Directionnal
	VkDescriptorBufferInfo			shadowCascadeBufferInfoFirstFrame = { };
	{
		shadowCascadeBufferInfoFirstFrame.buffer = m_ShadowCascadeBuffers[0].GetApiBuffer();
		shadowCascadeBufferInfoFirstFrame.offset = 0;
		shadowCascadeBufferInfoFirstFrame.range = sizeof(ShadowInfoCascade);
	}

	VkDescriptorBufferInfo			shadowCascadeBufferInfoSecondFrame = { };
	{
		shadowCascadeBufferInfoSecondFrame.buffer = m_ShadowCascadeBuffers[1].GetApiBuffer();
		shadowCascadeBufferInfoSecondFrame.offset = 0;
		shadowCascadeBufferInfoSecondFrame.range = sizeof(ShadowInfoCascade);
	}

	// spotLight shadow buffer 
	VkDescriptorBufferInfo			shadowSpotLightBufferInfoFirstFrame = { };
	{
		shadowSpotLightBufferInfoFirstFrame.buffer = m_ShadowSpotLightBuffers[0].GetApiBuffer();
		shadowSpotLightBufferInfoFirstFrame.offset = 0;
		shadowSpotLightBufferInfoFirstFrame.range = sizeof(ShadowInfoSpotLight);
	}

	VkDescriptorBufferInfo			shadowSpotLightBufferInfoSecondFrame = { };
	{
		shadowSpotLightBufferInfoSecondFrame.buffer = m_ShadowSpotLightBuffers[1].GetApiBuffer();
		shadowSpotLightBufferInfoSecondFrame.offset = 0;
		shadowSpotLightBufferInfoSecondFrame.range = sizeof(ShadowInfoSpotLight);
	}

	// skybox image
	const Texture					&skyboxTexture = m_Skybox.GetTextures()[0];
	VkDescriptorImageInfo			imageInfoSkybox = { };
	{
		imageInfoSkybox.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoSkybox.imageView = skyboxTexture.m_ImageView;
		imageInfoSkybox.sampler = skyboxTexture.m_Sampler;
	}

	// Shadow image
	VkDescriptorImageInfo			imageInfoShadowMapCascade = { };
	{
		imageInfoShadowMapCascade.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoShadowMapCascade.imageView = m_ShadowCascadeImageViews[0];
		imageInfoShadowMapCascade.sampler = m_ShadowCascadeSampler;
	}

	m_RenderHandle->PrepareShadow(logicalDevice, m_ShadowSpotLightSampler);

	VkDescriptorImageInfo			imageInfoShadowMapSpotLight = { };
	{
		imageInfoShadowMapSpotLight.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoShadowMapSpotLight.imageView = m_ShadowSpotLightImageViews[0];
		imageInfoShadowMapSpotLight.sampler = m_ShadowSpotLightSampler;
	}

	const Texture					&texture = m_Meshes[newMeshIndex]->GetMaterial().GetTexture();

	// mesh image
	VkDescriptorImageInfo			imageInfo = { };
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.m_ImageView;
		imageInfo.sampler = texture.m_Sampler;
	}

	// FIXME: dirty 
	// First Frame
	infos[0].m_DescBufferInfo = &bufferInfoFirstFrame;
	infos[1].m_DescBufferInfo = &bufferInfoDynamicFirstFrame;
	infos[2].m_DescImageInfo = &imageInfo;
	infos[3].m_DescImageInfo = &imageInfoSkybox;
	infos[4].m_DescBufferInfo = &lightBufferFirstFrame;
	infos[5].m_DescImageInfo = &imageInfoShadowMapCascade;
	infos[6].m_DescBufferInfo = &shadowCascadeBufferInfoFirstFrame;
	infos[7].m_DescImageInfo = &imageInfoShadowMapSpotLight;
	infos[8].m_DescBufferInfo = &shadowSpotLightBufferInfoFirstFrame;

	// 2E Frame
	infos[9].m_DescBufferInfo = &bufferInfoSecondFrame;
	infos[10].m_DescBufferInfo = &bufferInfoDynamicSecondFrame;
	infos[11].m_DescImageInfo = &imageInfo;
	infos[12].m_DescImageInfo = &imageInfoSkybox;
	infos[13].m_DescBufferInfo = &lightBufferSecondFrame;
	infos[14].m_DescImageInfo = &imageInfoShadowMapCascade;
	infos[15].m_DescBufferInfo = &shadowCascadeBufferInfoSecondFrame;
	infos[16].m_DescImageInfo = &imageInfoShadowMapSpotLight;
	infos[17].m_DescBufferInfo = &shadowSpotLightBufferInfoSecondFrame;

	m_UniformDescriptions.push_back(UniformDescription(logicalDevice, descriptions, infos, m_RenderHandle->GetPendingFramesCount(), true));
	return true;
}

//----------------------------------------------------------------

std::string	Scene::GetDefinitiveObjectName(const std::string &name)
{
	std::string	defName = name;
	uint32_t	nameIndex = 0;

	while (std::find(m_ObjectsNames.begin(), m_ObjectsNames.end(), defName) != m_ObjectsNames.end())
	{
		++nameIndex;

		defName = name + std::to_string(nameIndex);
	}

	m_ObjectsNames.push_back(defName);
	return defName;
}

//----------------------------------------------------------------

LIGHTLYY_END
