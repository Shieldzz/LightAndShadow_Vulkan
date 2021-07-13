#pragma once

//----------------------------------------------------------------

#if defined(_WIN32)

#if !defined(VK_USE_PLATFORM_WIN32_KHR)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#elif defined(_GNUC_)
#endif

//----------------------------------------------------------------

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

//----------------------------------------------------------------

#include <SDKDDKVer.h>

#include "volk/volk.h"

#include <iostream>
#include <vector>

#include "Utility.h"

//----------------------------------------------------------------

#if defined(_DEBUG)
#define VULKAN_ENABLE_VALIDATION_LAYER
#endif

//----------------------------------------------------------------

#if defined(_DEBUG)
#define CHECK_API_SUCCESS(func) \
			if (VK_SUCCESS != func) \
			{ \
				std::cout << "\033[1;31m[FAIL]\033[0m\t\t\t" << (#func) << std::endl; /*__debugbreak();*/ \
			} \
			else \
			{ \
				std::cout << "\033[1;32m[SUCCESS]\033[0m\t\t" << (#func) << std::endl; \
			}
#else
#define CHECK_API_SUCCESS(func) func
#endif

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

// ===== TODO =====
//
//	Add description for each parameters of each functions
//
// ================

namespace Initializers
{
	namespace Application
	{
		inline VkApplicationInfo	Info(const char *appName, const char *engineName)
		{
			VkApplicationInfo	appInfo = { };
			{
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = appName;
				appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.pEngineName = engineName;
				appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.apiVersion = VK_API_VERSION_1_0;
			}

			return appInfo;
		}

	} // namespace Application

	namespace Device
	{
		inline VkDeviceCreateInfo		CreateInfo(	const std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos,
													const std::vector<const char*> &deviceExtensions)
		{
			VkDeviceCreateInfo deviceCreateInfo = { };
			{
				deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
				deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
				deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			}

			return deviceCreateInfo;
		}

		inline VkDeviceQueueCreateInfo	QueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount, float *queuePriorities)
		{
			VkDeviceQueueCreateInfo	queueCreateInfo = { };
			{
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
				queueCreateInfo.queueCount = queueCount;
				queueCreateInfo.pQueuePriorities = queuePriorities;
			}

			return queueCreateInfo;
		}

	} // namespace Device

	namespace Instance
	{
		inline VkInstanceCreateInfo	CreateInfo(VkApplicationInfo &appInfo, const std::vector<const char*> &extensionNames, std::vector<const char*> &layerNames)
		{
			VkInstanceCreateInfo	instanceInfo = { };
			{
				instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				instanceInfo.pApplicationInfo = &appInfo;
				instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
				instanceInfo.ppEnabledExtensionNames = extensionNames.data();
#if  defined(VULKAN_ENABLE_VALIDATION_LAYER)
				layerNames.push_back("VK_LAYER_LUNARG_standard_validation");
				instanceInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
				instanceInfo.ppEnabledLayerNames = layerNames.data();
#else
				instanceInfo.enabledExtensionCount--;
#endif
			}

			return instanceInfo;
		}

	} // namespace Instance

	namespace Surface
	{
		inline VkWin32SurfaceCreateInfoKHR	CreateInfo(HINSTANCE moduleInstance, HWND windowHandle)
		{
			VkWin32SurfaceCreateInfoKHR	surfaceInfo = { };
			{
				surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				surfaceInfo.hinstance = moduleInstance;
				surfaceInfo.hwnd = windowHandle;
			}

			return surfaceInfo;
		}

	} // namespace Surface

	namespace Swapchain
	{
		inline VkSwapchainCreateInfoKHR	CreateInfo(	const VkSurfaceKHR surface,
													const VkSurfaceFormatKHR &surfaceFormat,
													const VkExtent2D &swapchainExtent,
													const VkImageUsageFlags swapchainUsage,
													const VkSurfaceTransformFlagBitsKHR swapchainTransform,
													bool doubleBuffering = true,
													bool singleSharing = true)
		{
			VkSwapchainCreateInfoKHR	swapchainInfo = { };
			{
				swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				swapchainInfo.surface = surface;
				swapchainInfo.minImageCount = (doubleBuffering) ? 2 : 1;
				swapchainInfo.imageFormat = surfaceFormat.format;
				swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
				swapchainInfo.imageExtent = swapchainExtent;
				swapchainInfo.imageArrayLayers = 1; // 2 for stereo
				swapchainInfo.imageUsage = swapchainUsage;
				swapchainInfo.imageSharingMode = (singleSharing) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
				swapchainInfo.preTransform = swapchainTransform;
				swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				swapchainInfo.presentMode = (doubleBuffering) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
				swapchainInfo.clipped = VK_TRUE;
			}

			return swapchainInfo;
		}

	} // namespace Swapchain

	namespace RenderPass
	{
		inline VkRenderPassCreateInfo	CreateInfo(	const std::vector<VkAttachmentDescription> &attachmentsDescription,
													const std::vector<VkSubpassDescription> &subpassesDesription,
													const std::vector<VkSubpassDependency> &subpassesDependency)
		{
			const uint32_t			attachmentsCount = static_cast<uint32_t>(attachmentsDescription.size());
			const uint32_t			subpassesCount = static_cast<uint32_t>(subpassesDesription.size());
			const uint32_t			dependencyCount = static_cast<uint32_t>(subpassesDependency.size());

			VkRenderPassCreateInfo	renderPassInfo = { };
			{
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.attachmentCount = attachmentsCount;
				renderPassInfo.pAttachments = attachmentsDescription.data();
				renderPassInfo.subpassCount = subpassesCount;
				renderPassInfo.pSubpasses = subpassesDesription.data();
				renderPassInfo.dependencyCount = dependencyCount;
				renderPassInfo.pDependencies = subpassesDependency.data();
			}

			return renderPassInfo;
		}

	} // namespace RenderPass

	namespace Buffer
	{
		inline VkBufferCreateInfo		CreateInfo(uint32_t bufferType, uint64_t sizeInBytes)
		{
			VkBufferCreateInfo bufferInfo = { };
			{
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.usage = bufferType;
				bufferInfo.size = sizeInBytes;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}

			return bufferInfo;
		}

		inline VkFramebufferCreateInfo	FrameCreateInfo(const VkRenderPass renderPass, const std::vector<VkImageView> &imageViews, const VkExtent2D &extent, uint32_t layersCount)
		{
			const uint32_t			imageViewsCount = static_cast<uint32_t>(imageViews.size());

			VkFramebufferCreateInfo	frameBufferInfo = { };
			{
				frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				frameBufferInfo.renderPass = renderPass;
				frameBufferInfo.attachmentCount = imageViewsCount;
				frameBufferInfo.pAttachments = imageViews.data();
				frameBufferInfo.width = extent.width;
				frameBufferInfo.height = extent.height;
				frameBufferInfo.layers = layersCount;
			}

			return frameBufferInfo;
		}

	} // namespace Buffer

	namespace Image
	{
		inline VkImageCreateInfo		CreateInfo(	VkImageType type,
													const VkExtent2D &extent,
													uint32_t mipLevels,
													uint32_t arrayLayers,
													VkFormat format,
													VkImageTiling tiling,
													VkImageLayout layout,
													VkImageUsageFlags usage,
													VkSampleCountFlagBits aaLevel,
													VkImageCreateFlagBits flags = (VkImageCreateFlagBits)0)
		{
			VkImageCreateInfo imageInfo = { };
			{
				imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageInfo.imageType = type;
				imageInfo.extent.width = extent.width;
				imageInfo.extent.height = extent.height;
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = mipLevels;
				imageInfo.arrayLayers = arrayLayers;
				imageInfo.format = format;
				imageInfo.tiling = tiling;
				imageInfo.initialLayout = layout;
				imageInfo.usage = usage;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				imageInfo.samples = aaLevel;
				imageInfo.flags = flags;
			}

			return imageInfo;
		}

		inline VkImageViewCreateInfo	ViewCreateInfo(	VkImageViewType imageViewType,
														VkFormat format,
														VkComponentMapping swizzleComponents,
														VkImageSubresourceRange subRange)
		{
			VkImageViewCreateInfo	imageViewInfo = { };
			{
				imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewInfo.viewType = imageViewType;
				imageViewInfo.format = format;
				imageViewInfo.subresourceRange = subRange;
				imageViewInfo.components = swizzleComponents;
			}

			return imageViewInfo;
		}

		inline VkImageSubresourceRange	SubresourceRange(VkImageAspectFlags aspect, uint32_t levelCount, uint32_t firstMipmapLevel, uint32_t layerCount, uint32_t baseArrayLayer)
		{
			VkImageSubresourceRange	subRange = { };
			{
				subRange.aspectMask = aspect;
				subRange.levelCount = levelCount;
				subRange.baseMipLevel = firstMipmapLevel;
				subRange.layerCount = layerCount;
				subRange.baseArrayLayer = baseArrayLayer;
			}

			return subRange;
		}

	} // namespace Image
	
	namespace Pool
	{
		inline VkCommandPoolCreateInfo				CommandCreateInfo(VkCommandPoolCreateFlags createFlags, uint32_t queueFamilyIndex)
		{
			VkCommandPoolCreateInfo	commandPoolInfo = { };
			{
				commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
			}

			return commandPoolInfo;
		}

		inline VkDescriptorPoolCreateInfo			DescriptorCreateInfo(const std::vector<VkDescriptorPoolSize> &descPoolSizes)
		{
			uint32_t					maxDescriptorsCount = 0;

			const uint32_t				descPoolSizesCount = static_cast<uint32_t>(descPoolSizes.size());
			for (uint32_t descIndex = 0; descIndex < descPoolSizesCount; ++descIndex)
				maxDescriptorsCount += descPoolSizes[descIndex].descriptorCount;

			VkDescriptorPoolCreateInfo	descPoolInfo = { };
			{
				descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descPoolInfo.maxSets = maxDescriptorsCount;
				descPoolInfo.poolSizeCount = descPoolSizesCount;
				descPoolInfo.pPoolSizes = descPoolSizes.data();
			}

			return descPoolInfo;
		}

		inline std::vector<VkDescriptorPoolSize>	DescriptorSizes(uint32_t samplerCount,
																	uint32_t combinedImageSamplerCount,
																	uint32_t sampledImageCount,
																	uint32_t storageImageCount,
																	uint32_t uniformBufferCount,
																	uint32_t uniformBufferDynamicCount,
																	uint32_t uniformTexelBufferCount,
																	uint32_t storageBufferCount,
																	uint32_t storageBufferDynamicCount,
																	uint32_t storageTexelByfferCount,
																	uint32_t inputAttachmentCount)
		{
			const uint32_t						descCount = (samplerCount > 0) +
															(combinedImageSamplerCount > 0) +
															(sampledImageCount > 0) +
															(storageImageCount > 0) +
															(uniformBufferCount > 0) +
															(uniformBufferDynamicCount > 0) +
															(uniformTexelBufferCount > 0) +
															(storageBufferCount > 0) +
															(storageBufferDynamicCount > 0) +
															(storageTexelByfferCount > 0) +
															(inputAttachmentCount > 0);

			std::vector<VkDescriptorPoolSize>	descPoolSizes;
			descPoolSizes.reserve(descCount);

			if (samplerCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, samplerCount });
			if (combinedImageSamplerCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, combinedImageSamplerCount });
			if (sampledImageCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledImageCount });
			if (storageImageCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageCount });
			if (uniformBufferCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount });
			if (uniformBufferDynamicCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, uniformBufferDynamicCount });
			if (uniformTexelBufferCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, uniformTexelBufferCount });
			if (storageBufferCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount });
			if (storageBufferDynamicCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, storageBufferDynamicCount });
			if (storageTexelByfferCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, storageTexelByfferCount });
			if (inputAttachmentCount > 0)
				descPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, inputAttachmentCount });

			return descPoolSizes;
		}

	} // namespace Pool

	namespace Command
	{
		// @Param1 The command pool used to allocate buffers
		// @Param2 The level of the buffer (0) Primary, (1) Secondary
		// @Param3 Number of buffers to allocate
		inline VkCommandBufferAllocateInfo	BufferAllocateInfo(const VkCommandPool commandPool, uint32_t level, uint32_t count)
		{
			if (level > 1)
				level = 0;

			VkCommandBufferAllocateInfo allocInfo = { };
			{
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = count;
				allocInfo.level = static_cast<VkCommandBufferLevel>(level);
			}

			return allocInfo;
		}

	} // namespace Command

	namespace Fence
	{
		inline VkFenceCreateInfo	CreateInfo()
		{
			VkFenceCreateInfo fenceInfo = { };
			{
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			}

			return fenceInfo;
		}
	} // namespace Fence

	namespace Semaphore
	{
		inline VkSemaphoreCreateInfo	CreateInfo()
		{
			VkSemaphoreCreateInfo semInfo = { };
			{
				semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			}

			return semInfo;
		}

	} // namespace Semaphore

	namespace Debug
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL	VulkanReportFunc(	VkDebugReportFlagsEXT flags,
																	VkDebugReportObjectTypeEXT objType,
																	uint64_t obj,
																	size_t location,
																	int32_t code,
																	const char* layerPrefix,
																	const char* msg,
																	void* userData)
		{
			std::cout << "\033[1;34m[VULKAN VALIDATION]\033[0m\t" << msg << std::endl;

			return VK_FALSE;
		}

		inline void		SetupDebugCallback(const VkInstance instance, VkDebugReportCallbackEXT *debugCallback)
		{
#if defined(VULKAN_ENABLE_VALIDATION_LAYER)
			VkDebugReportCallbackCreateInfoEXT	debugCallbackInfo = { };
			{
				debugCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
				debugCallbackInfo.flags =	VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
											VK_DEBUG_REPORT_WARNING_BIT_EXT |
											VK_DEBUG_REPORT_ERROR_BIT_EXT |
											VK_DEBUG_REPORT_DEBUG_BIT_EXT |
											VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
				debugCallbackInfo.pfnCallback = VulkanReportFunc;

			}

			CHECK_API_SUCCESS(vkCreateDebugReportCallbackEXT(instance, &debugCallbackInfo, nullptr, debugCallback));

			if (debugCallback == nullptr)
				printf("test");
#endif
		}
	} // namespace Debug

} // namespace Initializers

//----------------------------------------------------------------

LIGHTLYY_END
