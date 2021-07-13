#pragma once

#include "Initializers.h"
#include "Window.h"
#include "RenderCommand.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

class	Mesh;
struct	Skybox;

//----------------------------------------------------------------

enum SWAPCHAIN_USAGE
{
	TransferRead = 1,
	TransferWrite = 2,
	ImageSampler = 4,
	ImageView = 8,
	ColorAttachment = 16,
	DepthStencilAttachment = 32,
	InputAttachment = 128,
	AnyAttachment = 64,
	ImageShadingRate = 256,
	DensityMapAttachment = 512,

	Count = 10
};

//----------------------------------------------------------------

enum class SHADER_STAGE
{
	Vertex = 1,
	Fragment = 16,
	Compute = 32
};

//----------------------------------------------------------------

class RenderHandle
{
public:
	RenderHandle();
	~RenderHandle();

	bool								Setup(	const VkInstance instance,
												const Window *window,
												const VkPhysicalDevice physicalDevice,
												const std::vector<COMMAND_TYPE> &renderCommandsType,
												uint32_t pendingFrames);
	bool								Prepare(const VkPhysicalDevice physicalDevice, const VkDevice logicalDevice);
	void								Shutdown(const VkInstance instance, const VkDevice logicalDevice);

	void								BeginRender(const VkDevice logicalDevice);
	void								EndRender();

	bool								CreateSwapchain(const VkPhysicalDevice physicalDevice,
														const VkDevice logicalDevice,
														const SWAPCHAIN_USAGE swapchainUsage,
														bool useSRGB,
														bool doubleBuffering = true);

	bool								CreateImages(const VkDevice logicalDevice,
													const VkImageCreateInfo &imageCreateInfo,
													VkMemoryPropertyFlags memoryType,
													uint32_t imagesCount,
													std::vector<VkImage> &outImages,
													std::vector<VkDeviceMemory> &outMemories) const;

	bool								CreateImageViews(	const VkDevice logicalDevice,
															const std::vector<VkImage> &images,
															VkImageViewType imageViewType,
															VkFormat format,
															const VkComponentMapping &swizzleComponents,
															const VkImageSubresourceRange &subRange,
															std::vector<VkImageView> &outImageViews) const;

	bool								CreateSampler(	const VkDevice logicalDevice,
														const VkSamplerCreateInfo &samplerCreateInfo,
														uint32_t imagesCount,
														std::vector<VkSampler> &outSamplers) const;

	bool								PrepareTexture(const VkDevice logicalDevice, Texture &texture) const;
	bool								PrepareSkybox(const VkDevice logicalDevice, Skybox &skybox) const;
	bool								PrepareShadow(const VkDevice logicalDevice, VkSampler &sampler) const;

	// getters
	const VkSurfaceKHR							GetSurface() const { return m_Surface; }
	const VkSurfaceFormatKHR&					GetSurfaceFormat() const { return m_SurfaceFormat; }
	const std::vector<VkImageView>&				GetSwapchainImageViews() const { return m_SwapchainImageViews; }
	const VkExtent2D&							GetSwapchainExtent() const { return m_SwapchainExtent; }
	const std::vector<RenderCommand*>&			GetRenderCommands() const { return m_RenderCommands; }
	const VkCommandBuffer						GetCurrentCommandBuffer() const { return m_RenderCommands[m_CurrentFrame]->GetBuffer(); }
	const std::vector<VkClearValue>&			GetClearValues() const { return m_ClearValues; }
	bool										GetDoubleBuffering() const { return m_DoubleBuffering; }
	uint32_t									GetPendingFramesCount() const { return m_PendingFrames; }
	uint8_t										GetCurrentFrame() const { return m_CurrentFrame; }
	uint32_t									GetCurrentSwapchainImage() const { return m_CurrentSwapchainImage; }
	RenderCommand							*m_SingleCommand;

private:
	void								RetrieveQueueFamilyProperties(const VkPhysicalDevice physicalDevice);

	VkSurfaceKHR							m_Surface;
	VkSurfaceFormatKHR						m_SurfaceFormat;

	VkSwapchainKHR							m_Swapchain;
	std::vector<VkImage>					m_SwapchainImages;
	std::vector<VkImageView>				m_SwapchainImageViews;
	VkExtent2D								m_SwapchainExtent;

	VkExtent2D								m_ShadowExtent;

	std::vector<VkFence>					m_Fences;

	std::vector<VkSemaphore>				m_AcquireSemaphores;
	std::vector<VkSemaphore>				m_PresentSemaphores;

	uint32_t								m_MaxQueueFamilyCount;
	std::vector<VkQueueFamilyProperties>	m_QueuesFamilyProperties;

	std::vector<RenderCommand*>				m_RenderCommands;

	Queue									m_PresentQueue;

	VkClearColorValue						m_ClearColor;
	std::vector<VkClearValue>				m_ClearValues;

	bool									m_DoubleBuffering;
	uint32_t								m_PendingFrames;

	uint8_t									m_CurrentFrame;
	uint32_t								m_CurrentSwapchainImage;
}; // class RenderHandle

//----------------------------------------------------------------

LIGHTLYY_END
