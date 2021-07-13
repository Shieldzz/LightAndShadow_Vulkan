 #pragma once

#include <memory>
#include <chrono>

#include "Initializers.h"
#include "Window.h"
#include "RenderHandle.h"
#include "imgui/UI.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

using chrono_time = std::chrono::high_resolution_clock::time_point;

class Scene;

//----------------------------------------------------------------

class Device
{
public:
	Device() = delete;
	Device(const char *applicationName, const char *engineName);
	~Device();

	bool					Setup();
	void					Update();

	bool					AllocateMemory(VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags memoryType, VkDeviceMemory &outMemory);
	uint32_t				FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	// getters
	float					GetDeltaTime() const { return m_DeltaTime; }
	uint64_t				GetUBOMinAlignment() const { return m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment; }
	VkSampleCountFlagBits	GetMaxAALevel() const { return m_MaxAALevel; }
	const VkDescriptorPool	GetDescriptorPool() const { return m_DescriptorPool; }

	static std::unique_ptr<Device>			m_Device;

private:
	bool		CreateInstance();
	bool		CreatePhysicalDevice();
	bool		CreateLogicalDevice();

	bool		CreateDescriptorPool();

	bool		HasRequiredFeatures();

	void		LoadMemoryProperties();
	void		RetrieveMaxAntialiasingLevel();

	const char								*m_ApplicationName;
	const char								*m_EngineName;

	Window									*m_Window;
	RenderHandle							*m_RenderHandle;
	UI										*m_UI;
	Scene									*m_Scene;

	uint32_t								m_MaxPhysicalDevicesCount;

	chrono_time								m_StartTime;
	chrono_time								m_PreviousTime;
	float									m_DeltaTime;

	uint64_t								m_Frame;

	// ==== Vulkan Data ====
	VkInstance								m_Instance;

	VkPhysicalDevice						m_PhysicalDevice;
	VkPhysicalDeviceProperties				m_PhysicalDeviceProperties;

	VkDevice								m_LogicalDevice;

	VkPhysicalDeviceMemoryProperties		m_MemoryProperties;
	std::vector<VkMemoryPropertyFlags>		m_MemoryPropertiesFlags;

	VkSampleCountFlagBits					m_MaxAALevel;

	std::vector<const char*>				m_ExtensionNames;
	std::vector<const char*>				m_LayerNames;

	VkDescriptorPool						m_DescriptorPool;

#if defined(VULKAN_ENABLE_VALIDATION_LAYER)
	VkDebugReportCallbackEXT				m_DebugCallback;
#endif
}; // class Device

//----------------------------------------------------------------

LIGHTLYY_END
