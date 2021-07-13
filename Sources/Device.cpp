#include "Device.h"

#include <algorithm>

#include "Scene.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

std::unique_ptr<Device> Device::m_Device = nullptr;

//----------------------------------------------------------------

Device::Device(const char *applicationName, const char* engineName)
:	m_ApplicationName(applicationName),
	m_EngineName(engineName),
	m_MaxPhysicalDevicesCount(9), // one for intergrated GPU and 8 for others GPU
	m_Instance(nullptr),
	m_PhysicalDevice(nullptr),
	m_LogicalDevice(nullptr),
	m_DescriptorPool(nullptr)
{
	m_Window = new Window();
	m_RenderHandle = new RenderHandle();
	m_UI = new UI();
	m_Scene = new Scene(m_RenderHandle);

	if (m_Device == nullptr)
		m_Device = std::unique_ptr<Device>(this);
}

//----------------------------------------------------------------

Device::~Device()
{
	if (m_Instance == VK_NULL_HANDLE)
		return;

	if (m_LogicalDevice == VK_NULL_HANDLE)
		return;

	// destroy UI
	if (m_UI != nullptr)
		delete m_UI;

	m_UI = nullptr;

	// destroy window
	if (m_Window != nullptr)
		delete m_Window;

	m_Window = nullptr;

	// destroy scene
	if (m_Scene != nullptr)
	{
		m_Scene->Shutdown(m_LogicalDevice);

		delete m_Scene;
	}

	m_Scene = nullptr;

	// destroy render handle
	if (m_RenderHandle != nullptr)
	{
		m_RenderHandle->Shutdown(m_Instance, m_LogicalDevice);

		delete m_RenderHandle;
	}

	m_RenderHandle = nullptr;

	if (m_DescriptorPool != nullptr)
		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);

	m_DescriptorPool = nullptr;

#if defined(VULKAN_ENABLE_VALIDATION_LAYER)
	if (m_DebugCallback != nullptr)
		vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugCallback, nullptr);
#endif

	// destroy device
	vkDestroyDevice(m_LogicalDevice, nullptr);

	// destroy instance
	vkDestroyInstance(m_Instance, nullptr);
}

//----------------------------------------------------------------

bool	Device::Setup() // TODO: Add runtime errors on fail
{
	CHECK_API_SUCCESS(volkInitialize());

	if (!CreateInstance())
		return false;

#if defined(VULKAN_ENABLE_VALIDATION_LAYER)
	Initializers::Debug::SetupDebugCallback(m_Instance, &m_DebugCallback);
#endif

	// create window
	if (!m_Window->Setup(m_ApplicationName))
		return false;

	// choose physical device
	if (!CreatePhysicalDevice())
		return false;

	// check required features
	if (!HasRequiredFeatures())
		return false;

	// retrieve memory flags
	LoadMemoryProperties();

	RetrieveMaxAntialiasingLevel();

	// create render handle
	std::vector<COMMAND_TYPE>	renderCommandsType = { COMMAND_TYPE::GraphicsOp, COMMAND_TYPE::GraphicsOp }; // TODO: manage multiple pending frames

	if (!m_RenderHandle->Setup(m_Instance, m_Window, m_PhysicalDevice, renderCommandsType, 2))
		return false;
	
	if (!CreateLogicalDevice())
		return false;

	if (!m_RenderHandle->Prepare(m_PhysicalDevice, m_LogicalDevice))
		return false;

	// create swapchain
	const SWAPCHAIN_USAGE	swapchainUsage = static_cast<SWAPCHAIN_USAGE>(SWAPCHAIN_USAGE::ColorAttachment | SWAPCHAIN_USAGE::InputAttachment | SWAPCHAIN_USAGE::TransferWrite);
	if (!m_RenderHandle->CreateSwapchain(m_PhysicalDevice, m_LogicalDevice, swapchainUsage, true))
		return false;

	if (!CreateDescriptorPool())
		return false;

	if (!m_Scene->Setup(m_LogicalDevice))
		return false;

	// create UI
	const RenderCommand		*command = m_RenderHandle->GetRenderCommands()[0];

	UIInitInfo				uiInitInfo;
	{
		uiInitInfo.m_Instance = m_Instance;
		uiInitInfo.m_PhysicalDevice = m_PhysicalDevice;
		uiInitInfo.m_LogicalDevice = m_LogicalDevice;
		uiInitInfo.m_RenderPass = m_Scene->GetRenderPassObjects();
		uiInitInfo.m_CommandBuffer = command->GetBuffer();
		uiInitInfo.m_GraphicsQueue = command->GetQueue().m_ApiQueue;
		uiInitInfo.m_GraphicsQueueIndex = command->GetQueue().m_FamilyIndex;
		uiInitInfo.m_DescPool = m_DescriptorPool;
		uiInitInfo.m_DoubleBuffering = m_RenderHandle->GetDoubleBuffering();
		uiInitInfo.m_PendingFrames = m_RenderHandle->GetPendingFramesCount();
		uiInitInfo.m_AALevel = m_MaxAALevel;
	}

	if (!m_UI->Setup(m_Window, uiInitInfo))
		return false;

	m_UI->SetCurrentScene(m_Scene);

	m_Window->m_Camera = m_Scene->GetCameraUnsafe();

	m_PreviousTime = std::chrono::high_resolution_clock::now();
	m_StartTime = m_PreviousTime;

	m_Frame = 0;
	Update(); // Note: move this call outside device? in main?

	return true;
}

//----------------------------------------------------------------

void		Device::Update()
{
	while (!m_Window->IsClosed())
	{
		chrono_time	currentTime = std::chrono::high_resolution_clock::now();
		m_DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_PreviousTime).count();
		m_PreviousTime = currentTime;

		m_RenderHandle->BeginRender(m_LogicalDevice);

		if (m_Frame != 0)
		{
			m_Scene->Prepare(m_LogicalDevice);

			m_Scene->Render(m_UI);
		}

		m_RenderHandle->EndRender();

		m_Window->RetrieveInputs();

		++m_Frame;
	}
}

//----------------------------------------------------------------

// TODO: store found memory type to speed up things
bool		Device::AllocateMemory(VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlags memoryType, VkDeviceMemory &outMemory)
{
	VkMemoryAllocateInfo memAllocInfo = { };
	{
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memoryRequirements.size;
		memAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memoryType);
	}

	CHECK_API_SUCCESS(vkAllocateMemory(m_LogicalDevice, &memAllocInfo, nullptr, &outMemory)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

uint32_t	Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	int i = 0;
	for (auto propertyFlags : m_MemoryPropertiesFlags)
	{
		if ((typeFilter & (1 << i)) && (propertyFlags & properties) == properties)
			return i;

		i++;
	}

	return 0;
}

//----------------------------------------------------------------

bool	Device::CreateInstance()
{
	m_ExtensionNames = { VK_KHR_SURFACE_EXTENSION_NAME
#if defined(_WIN32)
		, VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
		, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

	m_LayerNames = std::vector<const char*>();

	// create instance
	VkApplicationInfo		applicationInfo = Initializers::Application::Info(m_ApplicationName, m_EngineName);

	VkInstanceCreateInfo	instanceCreateInfo = Initializers::Instance::CreateInfo(applicationInfo, m_ExtensionNames, m_LayerNames);

	CHECK_API_SUCCESS(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance));

	volkLoadInstance(m_Instance);

	return true;
}

//----------------------------------------------------------------

bool	Device::CreatePhysicalDevice()
{
	std::vector<VkPhysicalDevice>	physicalDevices(m_MaxPhysicalDevicesCount);
	// physicalDevices array will be resized to m_MaxPhysicalDevicesCount even if there's not enough device
	CHECK_API_SUCCESS(vkEnumeratePhysicalDevices(m_Instance, &m_MaxPhysicalDevicesCount, physicalDevices.data()));

	for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < m_MaxPhysicalDevicesCount; ++physicalDeviceIndex)
	{
		const VkPhysicalDevice	currPhysicalDevice = physicalDevices[physicalDeviceIndex];

		if (currPhysicalDevice != nullptr) // is physical device valid
		{
			VkPhysicalDeviceProperties	currPhysicalDeviceProperties;

			vkGetPhysicalDeviceProperties(currPhysicalDevice, &currPhysicalDeviceProperties);

			if (currPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) // choose separate GPU by default (higher performance in most cases)
			{
				m_PhysicalDevice = currPhysicalDevice;
				m_PhysicalDeviceProperties = currPhysicalDeviceProperties;
				break;
			}
		}
	}

	if (m_PhysicalDevice == nullptr) // no separate GPU
	{
		if (physicalDevices[0] != nullptr) // fallback on first one
			m_PhysicalDevice = physicalDevices[0];
		else // no physical device, abort
			return false;
	}

	return true;
}

//----------------------------------------------------------------

bool	Device::CreateLogicalDevice()
{
	float									queuePriorities[] = { 1.f };
	// TODO: Manage multiple queues
	VkDeviceQueueCreateInfo					deviceQueueInfo = Initializers::Device::QueueCreateInfo(m_RenderHandle->GetRenderCommands()[0]->GetQueue().m_FamilyIndex, 1, queuePriorities); 

	std::vector<VkDeviceQueueCreateInfo>	queueInfos = { deviceQueueInfo };
	std::vector<const char*>				deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo						deviceInfo = Initializers::Device::CreateInfo(queueInfos, deviceExtensions);

	CHECK_API_SUCCESS(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_LogicalDevice));

	volkLoadDevice(m_LogicalDevice);

	return true;
}

//----------------------------------------------------------------

bool	Device::CreateDescriptorPool()
{
	const std::vector<VkDescriptorPoolSize>	descPoolSizes = Initializers::Pool::DescriptorSizes(100, 100, 100, 100, 100, 100, 0, 100, 100, 0, 200);

	VkDescriptorPoolCreateInfo				descPoolCreateInfo = Initializers::Pool::DescriptorCreateInfo(descPoolSizes);

	CHECK_API_SUCCESS(vkCreateDescriptorPool(m_LogicalDevice, &descPoolCreateInfo, nullptr, &m_DescriptorPool)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	Device::HasRequiredFeatures()
{
	VkFormatProperties	formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);

	// TODO: add error management
	return (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
}

//----------------------------------------------------------------

void	Device::LoadMemoryProperties()
{
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

	m_MemoryPropertiesFlags.resize(m_MemoryProperties.memoryTypeCount);

	for (uint32_t propertyIndex = 0; propertyIndex < m_MemoryProperties.memoryTypeCount; propertyIndex++)
		m_MemoryPropertiesFlags[propertyIndex] = m_MemoryProperties.memoryTypes[propertyIndex].propertyFlags;
}

//----------------------------------------------------------------

void	Device::RetrieveMaxAntialiasingLevel()
{
	VkSampleCountFlags	counts = std::min(m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts, m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts);

	if (counts & VK_SAMPLE_COUNT_64_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_64_BIT;
		return;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_32_BIT;
		return;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_16_BIT;
		return;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_8_BIT;
		return;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_4_BIT;
		return;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT)
	{
		m_MaxAALevel = VK_SAMPLE_COUNT_2_BIT;
		return;
	}

	m_MaxAALevel = VK_SAMPLE_COUNT_1_BIT;
}

//----------------------------------------------------------------

LIGHTLYY_END
