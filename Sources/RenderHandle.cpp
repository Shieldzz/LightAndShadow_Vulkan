#include "RenderHandle.h"

#include "Device.h"
#include "Mesh.h"
#include "Skybox.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

RenderHandle::RenderHandle()
:	m_Surface(nullptr),
	m_Swapchain(nullptr),
	m_SwapchainImages(std::vector<VkImage>()),
	m_SwapchainImageViews(std::vector<VkImageView>()),
	m_Fences(std::vector<VkFence>()),
	m_AcquireSemaphores(std::vector<VkSemaphore>()),
	m_PresentSemaphores(std::vector<VkSemaphore>()),
	m_MaxQueueFamilyCount(0),
	m_QueuesFamilyProperties(std::vector<VkQueueFamilyProperties>()),
	m_PresentQueue(Queue()),
	m_ClearColor({ 0.55f, 0.55f, 0.55f, 1.f }),
	m_CurrentFrame(0)
{
	m_SurfaceFormat.format = VK_FORMAT_UNDEFINED;
	
	m_ClearValues.resize(3);

	m_ClearValues[0].color = m_ClearColor;
	m_ClearValues[1].depthStencil = { 1.f, 0 };
	m_ClearValues[2].color = m_ClearColor;
}

//----------------------------------------------------------------

RenderHandle::~RenderHandle()
{
}

//----------------------------------------------------------------

bool	RenderHandle::Setup(const VkInstance instance,
							const Window *window,
							const VkPhysicalDevice physicalDevice,
							const std::vector<COMMAND_TYPE> &renderCommandsType,
							uint32_t pendingFrames)
{
	// create surface
#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR	surfaceInfo = Initializers::Surface::CreateInfo(GetModuleHandle(NULL), window->GetWindowHandle());

	CHECK_API_SUCCESS(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &m_Surface));
#endif

	RetrieveQueueFamilyProperties(physicalDevice);

	// setup all render commands
	const uint32_t				renderCommandsCount = static_cast<uint32_t>(renderCommandsType.size());

	m_RenderCommands.resize(renderCommandsCount);

	for (uint32_t renderCommandIndex = 0; renderCommandIndex < renderCommandsCount; ++renderCommandIndex)
	{
		m_RenderCommands[renderCommandIndex] = new RenderCommand(renderCommandsType[renderCommandIndex]);
		if (!m_RenderCommands[renderCommandIndex]->Setup(m_QueuesFamilyProperties))
			return false;
	}

	m_SingleCommand = new RenderCommand(COMMAND_TYPE::GraphicsOp);
	if (!m_SingleCommand->Setup(m_QueuesFamilyProperties))
		return false;

	m_PendingFrames = pendingFrames;

	m_Fences.resize(pendingFrames);
	m_AcquireSemaphores.resize(pendingFrames);
	m_PresentSemaphores.resize(pendingFrames);

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::Prepare(const VkPhysicalDevice physicalDevice, const VkDevice logicalDevice)
{
	// create queues, command pool and command buffer for each render command 
	const uint32_t	renderCommandsCount = static_cast<uint32_t>(m_RenderCommands.size());
	for (uint32_t renderCommandIndex = 0; renderCommandIndex < renderCommandsCount; ++renderCommandIndex)
	{
		RenderCommand	*renderCommand = m_RenderCommands[renderCommandIndex];
		if (!renderCommand->Prepare(logicalDevice))
			return false;

		if (m_PresentQueue.m_ApiQueue == nullptr &&
			renderCommand->CanPresentSurface(physicalDevice, m_Surface))
			m_PresentQueue = renderCommand->GetQueue();
	}

	m_SingleCommand->Prepare(logicalDevice);

	// create fences and semaphores
	VkFenceCreateInfo		fenceCreateInfo = Initializers::Fence::CreateInfo();
	VkSemaphoreCreateInfo	semaphoreCreateInfo = Initializers::Semaphore::CreateInfo();

	for (uint32_t frameIndex = 0; frameIndex < m_PendingFrames; ++frameIndex)
	{
		CHECK_API_SUCCESS(vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &m_Fences[frameIndex])); // TODO: add error management

		CHECK_API_SUCCESS(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &m_AcquireSemaphores[frameIndex])); // TODO: add error management
		CHECK_API_SUCCESS(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &m_PresentSemaphores[frameIndex]));
	}

	return true;
}

//----------------------------------------------------------------

void	RenderHandle::Shutdown(const VkInstance instance, const VkDevice logicalDevice)
{
	if (m_Surface != nullptr && m_Swapchain != nullptr)
	{
		// destroy swapchain image views
		const uint32_t	imageViewsCount = static_cast<uint32_t>(m_SwapchainImageViews.size());
		if (imageViewsCount > 0)
		{
			for (uint32_t imageViewIndex = 0; imageViewIndex < imageViewsCount; ++imageViewIndex)
				vkDestroyImageView(logicalDevice, m_SwapchainImageViews[imageViewIndex], nullptr);
		}

		// destroy swapchain
		vkDestroySwapchainKHR(logicalDevice, m_Swapchain, nullptr);

		// destroy surface
		vkDestroySurfaceKHR(instance, m_Surface, nullptr);
	}

	// destroy command pool and buffers
	const uint32_t	renderCommandsCount = static_cast<uint32_t>(m_RenderCommands.size());
	for (uint32_t renderCommandIndex = 0; renderCommandIndex < renderCommandsCount; ++renderCommandIndex)
		m_RenderCommands[renderCommandIndex]->Shutdown(logicalDevice);

	m_SingleCommand->Shutdown(logicalDevice);

	// destroy fences
	for (uint32_t frameIndex = 0; frameIndex < m_PendingFrames; ++frameIndex)
	{
		vkDestroyFence(logicalDevice, m_Fences[frameIndex], nullptr);

		vkDestroySemaphore(logicalDevice, m_AcquireSemaphores[frameIndex], nullptr);
		vkDestroySemaphore(logicalDevice, m_PresentSemaphores[frameIndex], nullptr);
	}
}

//----------------------------------------------------------------

void	RenderHandle::BeginRender(const VkDevice logicalDevice)
{
	uint64_t		timeout = UINT64_MAX;

	CHECK_API_SUCCESS(vkWaitForFences(logicalDevice, 1, &m_Fences[m_CurrentFrame], false, timeout));
	CHECK_API_SUCCESS(vkResetFences(logicalDevice, 1, &m_Fences[m_CurrentFrame]));

	RenderCommand	*command = m_RenderCommands[m_CurrentFrame];
	command->Reset();

	VkSemaphore		*acquireSem = &m_AcquireSemaphores[m_CurrentFrame];
	VkSemaphore		*presentSem = &m_PresentSemaphores[m_CurrentFrame];

	CHECK_API_SUCCESS(vkAcquireNextImageKHR(logicalDevice, m_Swapchain, timeout, *acquireSem, VK_NULL_HANDLE, &m_CurrentSwapchainImage));
	command->Begin();
}

//----------------------------------------------------------------

void	RenderHandle::EndRender()
{
	RenderCommand		*command = m_RenderCommands[m_CurrentFrame];
	command->End();

	VkCommandBuffer		commandBuffer = command->GetBuffer();

	VkSemaphore			*acquireSem = &m_AcquireSemaphores[m_CurrentFrame];
	VkSemaphore			*presentSem = &m_PresentSemaphores[m_CurrentFrame];

	VkSubmitInfo		submitInfo = { };
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &stageMask;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = acquireSem;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = presentSem;
	}

	CHECK_API_SUCCESS(vkQueueSubmit(command->GetQueue().m_ApiQueue, 1, &submitInfo, m_Fences[m_CurrentFrame]));
	
//	vkQueueWaitIdle(command->GetQueue().m_ApiQueue);
	
	VkPresentInfoKHR	presentInfo = { };
	{
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = presentSem;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_CurrentSwapchainImage;
	}

	CHECK_API_SUCCESS(vkQueuePresentKHR(m_PresentQueue.m_ApiQueue, &presentInfo));

//	vkQueueWaitIdle(command->GetQueue().m_ApiQueue);
	
	m_CurrentFrame = (m_CurrentFrame + 1) % m_PendingFrames;
}

//----------------------------------------------------------------

bool	RenderHandle::CreateSwapchain(	const VkPhysicalDevice physicalDevice,
										const VkDevice logicalDevice,
										const SWAPCHAIN_USAGE swapchainUsage,
										bool useSRGB,
										bool doubleBuffering)
{
	// retrieve surface formats count supported by surface
	uint32_t						surfaceFormatsCount = 0;
	CHECK_API_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatsCount, nullptr));

	if (surfaceFormatsCount == 0)
		return false;

	std::vector<VkSurfaceFormatKHR>	surfaceFormats = std::vector<VkSurfaceFormatKHR>();
	surfaceFormats.resize(surfaceFormatsCount);

	// retrieve supported surface formats
	CHECK_API_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &surfaceFormatsCount, surfaceFormats.data()));

	const VkFormat	preferredFormat = (useSRGB) ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;

	// choose surface format
	for (uint32_t surfaceFormatIndex = 0; surfaceFormatIndex < surfaceFormatsCount; ++surfaceFormatIndex)
	{
		const VkSurfaceFormatKHR	&currSurfaceFormat = surfaceFormats[surfaceFormatIndex];

		if (currSurfaceFormat.format == preferredFormat &&
			currSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_SurfaceFormat = currSurfaceFormat;
			break;
		}
	}

	// fallback on first one by default
	if (m_SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		m_SurfaceFormat = surfaceFormats[0];

	VkSurfaceCapabilitiesKHR	surfaceCapabilities;
	CHECK_API_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCapabilities));

	m_SwapchainExtent = surfaceCapabilities.currentExtent;

	VkImageUsageFlags			swapchainImageUsage = swapchainUsage & surfaceCapabilities.supportedUsageFlags;

	if (swapchainImageUsage != swapchainUsage) // it means that one bit as been removed (ie. one or more usage flags isn't supported)
	{
		// TODO: add log error
		// Note: loop to find specific usage flag that cause this error?
		return false;
	}

	m_DoubleBuffering = doubleBuffering;

	VkSwapchainCreateInfoKHR	 swapchainCreateInfo = Initializers::Swapchain::CreateInfo(	m_Surface,
																							m_SurfaceFormat,
																							m_SwapchainExtent,
																							swapchainImageUsage,
																							surfaceCapabilities.currentTransform,
																							doubleBuffering);

	CHECK_API_SUCCESS(vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain));

	// retrieve swapchain images count
	uint32_t						imagesCount = 0;
	CHECK_API_SUCCESS(vkGetSwapchainImagesKHR(logicalDevice, m_Swapchain, &imagesCount, nullptr));

	// retrieve actual images
	m_SwapchainImages.resize(imagesCount);
	CHECK_API_SUCCESS(vkGetSwapchainImagesKHR(logicalDevice, m_Swapchain, &imagesCount, m_SwapchainImages.data()));

	const VkImageSubresourceRange	subRange = Initializers::Image::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1, 0);

	// create image views
	CreateImageViews(logicalDevice, m_SwapchainImages, VK_IMAGE_VIEW_TYPE_2D, m_SurfaceFormat.format, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, m_SwapchainImageViews);

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::CreateImages(	const VkDevice logicalDevice,
									const VkImageCreateInfo &imageCreateInfo,
									VkMemoryPropertyFlags memoryType,
									uint32_t imagesCount,
									std::vector<VkImage> &outImages,
									std::vector<VkDeviceMemory> &outMemories) const
{
	outImages.resize(imagesCount);
	outMemories.resize(imagesCount);

	for (uint32_t imageIndex = 0; imageIndex < imagesCount; ++imageIndex)
	{
		CHECK_API_SUCCESS(vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &outImages[imageIndex])); // TODO: add error management

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, outImages[imageIndex], &memRequirements);

		if (!Device::m_Device->AllocateMemory(memRequirements, memoryType, outMemories[imageIndex]))
			return false;

		CHECK_API_SUCCESS(vkBindImageMemory(logicalDevice, outImages[imageIndex], outMemories[imageIndex], 0));
	}

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::CreateImageViews(	const VkDevice logicalDevice,
										const std::vector<VkImage> &images,
										VkImageViewType imageViewType,
										VkFormat format,
										const VkComponentMapping &swizzleComponents,
										const VkImageSubresourceRange &subRange,
										std::vector<VkImageView> &outImageViews) const
{
	const uint32_t			imagesCount = static_cast<uint32_t>(images.size());
	if (imagesCount == 0) // TODO: Add error attempting to create image views without images
		return false;

	outImageViews.resize(imagesCount);

	VkImageViewCreateInfo	imageViewCreateInfo = Initializers::Image::ViewCreateInfo(imageViewType, format, swizzleComponents, subRange);

	for (uint32_t imageIndex = 0; imageIndex < imagesCount; ++imageIndex)
	{
		imageViewCreateInfo.image = images[imageIndex];
		CHECK_API_SUCCESS(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &outImageViews[imageIndex])); // TODO: return false if creation fails
	}

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::CreateSampler(	const VkDevice logicalDevice,
										const VkSamplerCreateInfo &samplerCreateInfo,
										uint32_t imagesCount,
										std::vector<VkSampler> &outSamplers) const
{
	outSamplers.resize(imagesCount);

	for (uint32_t imageIndex = 0; imageIndex < imagesCount; ++imageIndex)
	{
		CHECK_API_SUCCESS(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &outSamplers[imageIndex])); // TODO: return false if creation fails
	}

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::PrepareTexture(const VkDevice logicalDevice, Texture &texture) const
{
	const std::vector<stbi_uc>		&pixels = texture.GetPixels();

	if (pixels.size() == 0)
		return false;

	VkExtent2D			extent2D = {};
	{
		extent2D.height = texture.GetHeight();
		extent2D.width = texture.GetWidth();
	}

	// create texture image
	VkImageCreateInfo	imageCreateInfo = Initializers::Image::CreateInfo(	VK_IMAGE_TYPE_2D, 
																			extent2D,
																			texture.GetMipmapLevels(),
																			1,
																			VK_FORMAT_R8G8B8A8_UNORM,
																			VK_IMAGE_TILING_OPTIMAL,
																			VK_IMAGE_LAYOUT_UNDEFINED,
																			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
																			VK_SAMPLE_COUNT_1_BIT);

	std::vector<VkImage>			outImages;
	std::vector<VkDeviceMemory>		outMemories;
	CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, outImages, outMemories);

	//create texture image view
	VkImageSubresourceRange			subRange = Initializers::Image::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, texture.GetMipmapLevels(), 0, 1, 0);
	std::vector<VkImageView>		outImageViews;
	CreateImageViews(logicalDevice, outImages, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, outImageViews);

	// create sampler
	VkSamplerCreateInfo				samplerCreateInfo = {};
	{
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext = nullptr;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.maxAnisotropy = 1.f;
		samplerCreateInfo.maxLod = static_cast<float>(texture.GetMipmapLevels());
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	}

	std::vector<VkSampler>			outSamplers;
	CreateSampler(logicalDevice, samplerCreateInfo, 1, outSamplers);

	texture.m_Image = outImages[0];
	texture.m_ImageView = outImageViews[0];
	texture.m_Memory = outMemories[0];
	texture.m_Sampler = outSamplers[0];

	// submit texture
	const uint32_t	bufferSize = texture.GetWidth() * texture.GetHeight() * 4;
	Buffer<stbi_uc>	stagingBuffer = Buffer<stbi_uc>(logicalDevice, BUFFER_TYPE::TransferSource, bufferSize);
	stagingBuffer.UpdateData(logicalDevice, pixels);

	m_SingleCommand->SingleSubmit(texture, stagingBuffer.GetApiBuffer(), subRange, true); // FIXME: Retrieve correct graphic command

	texture.FreeTexture();

	return true;
}

//----------------------------------------------------------------

bool	RenderHandle::PrepareSkybox(const VkDevice logicalDevice, Skybox &skybox) const
{
	Texture						&texture = skybox.GetTexturesUnsafe()[0];

	VkExtent2D					extent2D = {};
	{
		extent2D.height = texture.GetHeight();
		extent2D.width = texture.GetWidth();
	}

	// create skybox image
	VkImageCreateInfo			imageCreateInfo = Initializers::Image::CreateInfo(	VK_IMAGE_TYPE_2D,
																					extent2D,
																					1, 6,
																					VK_FORMAT_R8G8B8A8_UNORM,
																					VK_IMAGE_TILING_OPTIMAL,
																					VK_IMAGE_LAYOUT_UNDEFINED,
																					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
																					VK_SAMPLE_COUNT_1_BIT,
																					VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

	std::vector<VkImage>		outImages;
	std::vector<VkDeviceMemory>	outMemories;
	CreateImages(logicalDevice, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, outImages, outMemories);

	//create skybox image view
	VkImageSubresourceRange		subRange = Initializers::Image::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 6, 0);
	std::vector<VkImageView>	outImageViews;
	CreateImageViews(logicalDevice, outImages, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM, { VK_COMPONENT_SWIZZLE_IDENTITY }, subRange, outImageViews);

	// create sampler
	VkSamplerCreateInfo			samplerCreateInfo = {};
	{
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.maxAnisotropy = 1;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	}

	std::vector<VkSampler>		outSamplers;
	CreateSampler(logicalDevice, samplerCreateInfo, 1, outSamplers);

	texture.m_Image = outImages[0];
	texture.m_ImageView = outImageViews[0];
	texture.m_Memory = outMemories[0];
	texture.m_Sampler = outSamplers[0];

	// submit skybox
	Buffer<stbi_uc>					stagingBuffer = Buffer<stbi_uc>(logicalDevice, BUFFER_TYPE::TransferSource, skybox.GetSize());
	const std::vector<Texture>		&textures = skybox.GetTextures();
	const std::vector<std::vector<stbi_uc>>	&pixels =
	{
		textures[0].GetPixels(),
		textures[1].GetPixels(),
		textures[2].GetPixels(),
		textures[3].GetPixels(),
		textures[4].GetPixels(),
		textures[5].GetPixels()
	};

	stagingBuffer.UpdateData(logicalDevice, pixels);

	m_SingleCommand->SingleSubmit(texture, stagingBuffer.GetApiBuffer(), subRange, false); // FIXME: Retrieve correct graphic command

	// free skybox pixels (no longer useful)
	skybox.FreeTextures();

	return true;
}

bool RenderHandle::PrepareShadow(const VkDevice logicalDevice, VkSampler &sampler) const
{
	VkSamplerCreateInfo shadowSampler = {};
	{
		shadowSampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		shadowSampler.magFilter = VK_FILTER_LINEAR;
		shadowSampler.minFilter = VK_FILTER_LINEAR;
		shadowSampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		shadowSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadowSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadowSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadowSampler.compareEnable = false;
		shadowSampler.compareOp = VK_COMPARE_OP_ALWAYS;
		shadowSampler.mipLodBias = 0.0f;
		shadowSampler.maxAnisotropy = 1.0f;
		shadowSampler.minLod = 0.0f;
		shadowSampler.maxLod = 0.0f;
		shadowSampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}

	CHECK_API_SUCCESS(vkCreateSampler(logicalDevice, &shadowSampler, nullptr, &sampler));

	return true;
}

//----------------------------------------------------------------

void	RenderHandle::RetrieveQueueFamilyProperties(const VkPhysicalDevice physicalDevice)
{
	// retrieve queue family count
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &m_MaxQueueFamilyCount, nullptr);

	if (m_MaxQueueFamilyCount == 0)
		return;

	m_QueuesFamilyProperties.resize(m_MaxQueueFamilyCount);

	// retrieve queue family properties
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &m_MaxQueueFamilyCount, m_QueuesFamilyProperties.data());
}

//----------------------------------------------------------------

LIGHTLYY_END
