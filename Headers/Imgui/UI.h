#pragma once

#include "Utility.h"
#include "Initializers.h"

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "glm/gtc/type_ptr.hpp"

#include "Window.h"
#include "Object.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

class Scene;
class Mesh;

//----------------------------------------------------------------

struct UIInitInfo
{
	VkInstance				m_Instance;
	VkPhysicalDevice		m_PhysicalDevice;
	VkDevice				m_LogicalDevice;
	VkRenderPass			m_RenderPass;
	VkCommandBuffer			m_CommandBuffer;

	VkQueue					m_GraphicsQueue;
	uint32_t				m_GraphicsQueueIndex;

	VkDescriptorPool		m_DescPool;

	bool					m_DoubleBuffering;
	uint32_t				m_PendingFrames;
	VkSampleCountFlagBits	m_AALevel;
};

//----------------------------------------------------------------

class UI
{
public:
	UI();
	~UI();

	bool	Setup(const Window *window, const UIInitInfo &initInfo);
	void	Render(VkCommandBuffer commandBuffer);

	void	SetCurrentScene(Scene *scene) { m_CurrentScene = scene; }

private:
	bool	UploadFont(const UIInitInfo &initInfo);

	void	RenderHierarchyPanel();
	void	RenderObjectPanel(Object *object);
	void	RenderCascadeShadowPanel();
	void	MeshObjectPanel(Object *object);
	void	LightObjectPanel(Object *object);

	Scene				*m_CurrentScene;
	Object				*m_CurrentObject;

	VkDevice			m_LogicalDevice;

	bool				m_HierarchyVisible;
	bool				m_ObjectPanelVisible;

	int32_t				m_ObjectSelectedIndex;

	std::vector<Mesh*>	m_Meshes;
}; // class UI

//----------------------------------------------------------------

LIGHTLYY_END
