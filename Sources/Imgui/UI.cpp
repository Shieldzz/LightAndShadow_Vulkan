#include "imgui/UI.h"
#include "Scene.h"

#include "Sphere.h"
#include "Light.h"
#include "Mesh.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

UI::UI()
:	m_CurrentObject(nullptr),
	m_HierarchyVisible(true),
	m_ObjectPanelVisible(false),
	m_ObjectSelectedIndex(-1)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
}

//----------------------------------------------------------------

UI::~UI()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

//----------------------------------------------------------------

bool	UI::Setup(const Window *window, const UIInitInfo &initInfo)
{
	if (!ImGui_ImplGlfw_InitForVulkan(const_cast<GLFWwindow*>(window->GetApiWindow()), true))
		return false;

	m_LogicalDevice = initInfo.m_LogicalDevice;

	ImGui_ImplVulkan_InitInfo	info = { };
	{
		info.Instance = initInfo.m_Instance;
		info.PhysicalDevice = initInfo.m_PhysicalDevice;
		info.Device = initInfo.m_LogicalDevice;
		info.Queue = initInfo.m_GraphicsQueue;
		info.QueueFamily = initInfo.m_GraphicsQueueIndex;
		info.DescriptorPool = initInfo.m_DescPool;
		info.MinImageCount = (initInfo.m_DoubleBuffering) ? 2 : 1;
		info.ImageCount = initInfo.m_PendingFrames;
		info.MSAASamples = initInfo.m_AALevel;
	}

	if (!ImGui_ImplVulkan_Init(&info, initInfo.m_RenderPass))
		return false;

	m_Meshes = {
		new Mesh(initInfo.m_LogicalDevice, ENGINE_DATA_PATH"Models/ironman/ironman.fbx", "IronMan"),
		new Mesh(initInfo.m_LogicalDevice, ENGINE_DATA_PATH"Models/Teapot/teapot.obj", "Teapot")
	};

	m_Meshes[1]->SetScale({ 0.1f, 0.1f, 0.1f });
	m_Meshes[1]->SetMaterial(Material(ENGINE_DATA_PATH"Textures/Basic.jpg"));

	return UploadFont(initInfo);
}

//----------------------------------------------------------------

void	UI::Render(VkCommandBuffer commandBuffer)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	RenderHierarchyPanel();

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

//----------------------------------------------------------------

bool	UI::UploadFont(const UIInitInfo &initInfo)
{
	VkCommandBufferBeginInfo	beginInfo = { };
	{
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	}

	CHECK_API_SUCCESS(vkBeginCommandBuffer(initInfo.m_CommandBuffer, &beginInfo));

	if (!ImGui_ImplVulkan_CreateFontsTexture(initInfo.m_CommandBuffer))
		return false;

	VkSubmitInfo				endInfo = { };
	{
		endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		endInfo.commandBufferCount = 1;
		endInfo.pCommandBuffers = &initInfo.m_CommandBuffer;
	}

	CHECK_API_SUCCESS(vkEndCommandBuffer(initInfo.m_CommandBuffer));

	CHECK_API_SUCCESS(vkQueueSubmit(initInfo.m_GraphicsQueue, 1, &endInfo, VK_NULL_HANDLE));

	CHECK_API_SUCCESS(vkDeviceWaitIdle(initInfo.m_LogicalDevice));

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	return true;
}

//----------------------------------------------------------------

void	UI::RenderHierarchyPanel()
{
	ImGuiIO	&io = ImGui::GetIO();
	if (io.KeysDown[GLFW_KEY_DELETE] /*|| io.KeysDown[GLFW_KEY_BACKSPACE]*/)
	{
		if (m_CurrentObject != nullptr)
		{
			if (m_CurrentObject->GetType() == OBJECT_TYPE::Mesh)
			{
				m_CurrentScene->DeleteMesh(reinterpret_cast<Mesh*>(m_CurrentObject));
				m_CurrentObject = nullptr;
				m_ObjectSelectedIndex = -1;
				m_ObjectPanelVisible = false;
			}
			else if (m_CurrentObject->GetType() == OBJECT_TYPE::DirectionalLight || m_CurrentObject->GetType() == OBJECT_TYPE::PointLight || m_CurrentObject->GetType() == OBJECT_TYPE::SpotLight)
			{
				m_CurrentScene->DeleteLight(reinterpret_cast<Light*>(m_CurrentObject));
				m_CurrentObject = nullptr;
				m_ObjectPanelVisible = false;
				m_ObjectSelectedIndex = -1;
			}
		}
	}

	ImGui::SetNextWindowSize({ 150.f, 600.f });
	ImGui::SetNextWindowPos({ 0.f, 0.f });
	ImGui::Begin("Hierarchy", &m_HierarchyVisible, ImGuiWindowFlags_MenuBar);

	// menu bar
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Add"))
		{
			if (ImGui::MenuItem("Sphere"))
			{
				m_CurrentScene->AddMesh(m_LogicalDevice, new Sphere(m_LogicalDevice, 64, ENGINE_DATA_PATH"Textures/Basic.jpg", "Sphere"));
			}

			if (ImGui::BeginMenu("Mesh"))
			{
				const uint32_t	meshesCount = static_cast<uint32_t>(m_Meshes.size());
				for (uint32_t meshIndex = 0; meshIndex < meshesCount; ++meshIndex)
				{
					Mesh	*mesh = m_Meshes[meshIndex];
					if (ImGui::MenuItem(mesh->GetName().c_str()))
						m_CurrentScene->AddMesh(m_LogicalDevice, mesh);
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Directional Light"))
			{
				if (m_CurrentScene->GetLightObjects().size() < 4)
				{
					LightData data;
					{
						data.m_Color = glm::vec4(1, 1, 1, 1);
						data.m_Position = glm::vec4(0, 0, 0, 1);
						data.m_Direction = glm::vec4(0, 0, 1, 1);

						data.m_Radius = 200.0f;
						data.m_Angle = 0.7f;
						data.m_Intensity = 1.f;
						data.m_Attenuation = 0.f;
						data.m_Type = (uint32_t)OBJECT_TYPE::DirectionalLight;
					}
					m_CurrentScene->AddLight(new Light(0, "Light", data));
				}
			}

			if (ImGui::MenuItem("Spot Light"))
			{
				if (m_CurrentScene->GetLightObjects().size() < 4)
				{
					LightData data;
					{
						data.m_Color = glm::vec4(1, 1, 1, 1);
						data.m_Position = glm::vec4(0, 0, 0, 1);
						data.m_Direction = glm::vec4(0, 0, 1, 1);

						data.m_Radius = 200.0f;
						data.m_Angle = 0.7f;
						data.m_Intensity = 1.f;
						data.m_Attenuation = 0.5f;
						data.m_Type = (uint32_t)OBJECT_TYPE::SpotLight;
					}
					m_CurrentScene->AddLight(new Light(0, "Light", data));
				}
			}

			if (ImGui::MenuItem("Point Light"))
			{
				if (m_CurrentScene->GetLightObjects().size() < 4)
				{
					LightData data;
					{
						data.m_Color = glm::vec4(1, 1, 1, 1);
						data.m_Position = glm::vec4(0, 0, 0, 1);
						data.m_Direction = glm::vec4(0, 0, 1, 1);

						data.m_Radius = 200.0f;
						data.m_Angle = 0.7f;
						data.m_Intensity = 1.f;
						data.m_Attenuation = 0.5f;
						data.m_Type = (uint32_t)OBJECT_TYPE::PointLight;
					}
					m_CurrentScene->AddLight(new Light(0, "Light", data));
				}
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// objects
	ImGui::BeginChild("Scroll bar");

	const std::vector<Object*>	sceneObjects = m_CurrentScene->GetSceneObjects();
	const uint32_t				objectsCount = static_cast<uint32_t>(sceneObjects.size());
	for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
	{
		Object	*currentObject = sceneObjects[objectIndex];
		if (ImGui::Selectable(currentObject->GetName().c_str(), (m_ObjectSelectedIndex == objectIndex)))
		{
			m_ObjectSelectedIndex = objectIndex;
			m_CurrentObject = currentObject;

			m_ObjectPanelVisible = true;
		}
	}
	ImGui::EndChild();

	ImGui::End();

	if (m_ObjectPanelVisible)
		RenderObjectPanel(m_CurrentObject);

	//RenderCascadeShadowPanel();

}

//----------------------------------------------------------------

void	UI::RenderObjectPanel(Object *object)
{
	ImGui::SetNextWindowSize({ 250.f, 500.f });
	ImGui::SetNextWindowPos({ 1280.f, 0.f }, ImGuiCond_Always, { 1.f, 0.f });
	ImGui::Begin(object->GetName().c_str(), &m_ObjectPanelVisible);

	ImGui::Text("Object");

	glm::vec3	pos = object->GetPosition();
	ImGui::InputFloat3("Position", glm::value_ptr(pos), "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
	object->SetPosition(pos);

	glm::vec3	rot = object->GetEulerAngles();
	ImGui::InputFloat3("Rotation", glm::value_ptr(rot), "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
	glm::quat	rotX = glm::angleAxis(rot.x * (3.14159265359f / 180.f), glm::vec3(1.f, 0.f, 0.f));
	glm::quat	rotY = glm::angleAxis(rot.y * (3.14159265359f / 180.f), glm::vec3(0.f, 1.f, 0.f));
	glm::quat	rotZ = glm::angleAxis(rot.z * (3.14159265359f / 180.f), glm::vec3(0.f, 0.f, 1.f));
	object->SetRotation(rotX * rotY * rotZ);
	object->SetEulerAngles(rot);

	glm::vec3	scale = object->GetScale();
	ImGui::InputFloat3("Scale", glm::value_ptr(scale), "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
	object->SetScale(scale);

	if (object->GetType() == OBJECT_TYPE::Mesh)
	{
		MeshObjectPanel(object);
	}
	else if (object->GetType() == OBJECT_TYPE::DirectionalLight || object->GetType() == OBJECT_TYPE::SpotLight || object->GetType() == OBJECT_TYPE::PointLight)
	{
		LightObjectPanel(object);
	}

	ImGui::End();
}

//----------------------------------------------------------------

void	UI::RenderCascadeShadowPanel()
{
	ImGui::SetNextWindowSize({ 250.f, 500.f });
	ImGui::SetNextWindowPos({ 1280.f, 500.f }, ImGuiCond_Always, { 1.f, 0.f });
	std::string name = "Casacade Shadow";
	bool isVisible = true;
	ImGui::Begin(name.c_str(), &isVisible);
	
	ImGui::Text("Shadow");

	float cascadeSplitCoeff = m_CurrentScene->GetShadow()->GetCascadeSplitCoeff();
	ImGui::InputFloat("Cascade Split Coeff", &cascadeSplitCoeff, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
	m_CurrentScene->GetShadow()->SetCascadeSplitCoeff(cascadeSplitCoeff);

	bool showCascade = m_CurrentScene->GetShadow()->IsShowCascade();
	ImGui::Checkbox("Show Cascade", &showCascade);
	m_CurrentScene->GetShadow()->ShowCascadeShadow(showCascade);

	bool showPCFFilter = m_CurrentScene->GetShadow()->IsShowPCFFilter();
	ImGui::Checkbox("Show PCF Filter", &showPCFFilter);
	m_CurrentScene->GetShadow()->ShowPCFFilter(showPCFFilter);

	ImGui::End();
}

//----------------------------------------------------------------

void	UI::MeshObjectPanel(Object *object)
{
	ImGui::Text("Material");

	Mesh *mesh = reinterpret_cast<Mesh*>(object);

	if (mesh != nullptr)
	{
		Material	&material = mesh->GetMaterial();

		glm::vec4	albedo = material.GetAlbedo();
		ImGui::ColorEdit4("Color", glm::value_ptr(albedo), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB);
		material.SetAlbedo(albedo);

		float		roughness = material.GetRoughness();
		ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f, "%.4f");
		material.SetRoughness(roughness);

		float		metallic = material.GetMetallic();
		ImGui::SliderFloat("Metallic", &metallic, 0.f, 1.f, "%.4f");
		material.SetMetallic(metallic);

		float		reflectance = material.GetReflectance();
		ImGui::SliderFloat("Reflectance", &reflectance, 0.f, 1.f, "%.4f");
		material.SetReflectance(reflectance);

		float		lodBias = mesh->GetLodBias();
		ImGui::SliderFloat("Lod Bias", &lodBias, 0.f, 5.f, "%.3f");
		mesh->SetLodBias(lodBias);
	}
}
//----------------------------------------------------------------

void	UI::LightObjectPanel(Object *object)
{
	ImGui::Text("Light");

	Light	*light = reinterpret_cast<Light*>(object);

	if (light != nullptr)
	{
		glm::vec4	color = light->GetColor();
		ImGui::ColorEdit4("Color", glm::value_ptr(color), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB);
		light->SetColor(color);

		float	intensity = light->GetIntensity();
		ImGui::InputFloat("Intensity", &intensity, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
		light->SetIntensity(intensity);

		switch (object->GetType())
		{
		case OBJECT_TYPE::PointLight:
		{
			float	radius = light->GetRadius();
			ImGui::InputFloat("Radius", &radius, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
			light->SetRadius(radius);

			float	attenuation = light->GetAttenuation();
			ImGui::SliderFloat("Attenuation", &attenuation, 0.f, 1.f, "%.4f");
			light->SetAttenuation(attenuation);
			break;
		}
		case OBJECT_TYPE::SpotLight:
		{
			float	radius = light->GetRadius();
			ImGui::InputFloat("Radius", &radius, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
			light->SetRadius(radius);

			float	attenuation = light->GetAttenuation();
			ImGui::SliderFloat("Attenuation", &attenuation, 0.f, 1.f, "%.4f");
			light->SetAttenuation(attenuation);

			float	angle = light->GetAngle();
			ImGui::InputFloat("Angle", &angle, 0.f, 0.f, "%.4f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
			light->SetAngle(angle);
			break;
		}
		}
	}
}

//----------------------------------------------------------------

LIGHTLYY_END
