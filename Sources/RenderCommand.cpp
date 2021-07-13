#include "RenderCommand.h"

#include "Texture.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

RenderCommand::RenderCommand(COMMAND_TYPE type)
:	m_CommandType(type),
	m_CommandPool(nullptr),
	m_CommandBuffer(nullptr),
	m_Queue(Queue())
{

}

//----------------------------------------------------------------

RenderCommand::~RenderCommand()
{
	
}

//----------------------------------------------------------------

bool	RenderCommand::Setup(const std::vector<VkQueueFamilyProperties> &queuesFamilyProperties)
{
	if (AllocateQueue(m_CommandType, queuesFamilyProperties))
	{
		m_IsSetup = true;
		return true;
	}

	return false;
}

//----------------------------------------------------------------

bool	RenderCommand::Prepare(const VkDevice logicalDevice)
{
	if (m_IsSetup)
		return CreateQueue(logicalDevice) && CreateCommandPool(logicalDevice) && CreateCommandBuffer(logicalDevice);

	return false; // TODO: add error render command not setup
}

//----------------------------------------------------------------

void	RenderCommand::Shutdown(const VkDevice logicalDevice)
{
	if (m_CommandBuffer != nullptr)
		vkFreeCommandBuffers(logicalDevice, m_CommandPool, 1, &m_CommandBuffer);

	m_CommandBuffer = nullptr;

	if (m_CommandPool != nullptr)
		vkDestroyCommandPool(logicalDevice, m_CommandPool, nullptr);

	m_CommandPool = nullptr;
}

//----------------------------------------------------------------

void	RenderCommand::Begin()
{
	VkCommandBufferBeginInfo	beginInfo = { };
	{
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	}

	CHECK_API_SUCCESS(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
}

//----------------------------------------------------------------

void	RenderCommand::SingleSubmit(const Texture &texture, VkBuffer buffer, VkImageSubresourceRange subRange, bool generateMipmaps)
{
	// TODO: create function to encapsulate layout transition (barrier)
	VkCommandBufferBeginInfo	beginInfo = { };
	{
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	}

	CHECK_API_SUCCESS(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

	VkImageMemoryBarrier		acquireBarrier = { };
	{
		acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		acquireBarrier.image = texture.m_Image;
		acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		acquireBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		acquireBarrier.srcAccessMask = 0;
		acquireBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		acquireBarrier.subresourceRange = subRange;
	}

	vkCmdPipelineBarrier(	m_CommandBuffer,
							VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
							VK_PIPELINE_STAGE_TRANSFER_BIT,
							0,
							0, nullptr,
							0, nullptr,
							1, &acquireBarrier);
	
	const uint32_t				textureWidth = static_cast<uint32_t>(texture.GetWidth());
	const uint32_t				textureHeight = static_cast<uint32_t>(texture.GetHeight());

	VkBufferImageCopy			bufferImageCopy = { };
	{
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = subRange.layerCount;
		bufferImageCopy.imageOffset = { 0, 0, 0 };
		bufferImageCopy.imageExtent = { textureWidth, textureHeight, 1 };
	}

	vkCmdCopyBufferToImage(	m_CommandBuffer,
							buffer,
							texture.m_Image,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1,
							&bufferImageCopy);

	if (!generateMipmaps)
	{
		{
			acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			acquireBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		vkCmdPipelineBarrier(	m_CommandBuffer,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								0,
								0, nullptr,
								0, nullptr,
								1, &acquireBarrier);
	}

	if (generateMipmaps)
	{
		for (uint32_t mipmapLevel = 1; mipmapLevel < texture.GetMipmapLevels(); ++mipmapLevel)
		{
			VkImageBlit				blitImage = { };

			// source image
			blitImage.srcOffsets[0] = { 0, 0, 0 };
			blitImage.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitImage.srcSubresource.baseArrayLayer = 0;
			blitImage.srcSubresource.layerCount = 1;
			// -1 to get previous image
			blitImage.srcSubresource.mipLevel = mipmapLevel - 1;
			blitImage.srcOffsets[1] = { static_cast<int32_t>(textureWidth >> (mipmapLevel - 1)), static_cast<int32_t>(textureHeight >> (mipmapLevel - 1)), 1 };

			// destination image
			blitImage.dstOffsets[0] = { 0, 0, 0 };
			blitImage.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitImage.dstSubresource.baseArrayLayer = 0;
			blitImage.dstSubresource.layerCount = 1;
			blitImage.dstSubresource.mipLevel = mipmapLevel;
			blitImage.dstOffsets[1] = { static_cast<int32_t>(textureWidth >> (mipmapLevel)), static_cast<int32_t>(textureHeight >> (mipmapLevel)), 1 };

			subRange.baseMipLevel = mipmapLevel - 1;
			subRange.levelCount = 1;

			VkImageMemoryBarrier	mipmapBarrier = { };
			{
				mipmapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				mipmapBarrier.image = texture.m_Image;
				mipmapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				mipmapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				mipmapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				mipmapBarrier.subresourceRange = subRange;
			}

			vkCmdPipelineBarrier(	m_CommandBuffer,
									VK_PIPELINE_STAGE_TRANSFER_BIT,
									VK_PIPELINE_STAGE_TRANSFER_BIT,
									0,
									0, nullptr,
									0, nullptr,
									1, &mipmapBarrier);

			vkCmdBlitImage(	m_CommandBuffer,
							texture.m_Image,
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							texture.m_Image,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1, &blitImage,
							VK_FILTER_LINEAR);

			{
				mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				mipmapBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}

			vkCmdPipelineBarrier(m_CommandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &mipmapBarrier);
		}

		subRange.baseMipLevel = texture.GetMipmapLevels() - 1;
		subRange.levelCount = 1;

		{
			acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			acquireBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			acquireBarrier.subresourceRange = subRange;
		}

		vkCmdPipelineBarrier(	m_CommandBuffer,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								0,
								0, nullptr,
								0, nullptr,
								1, &acquireBarrier);
	}

	CHECK_API_SUCCESS(vkEndCommandBuffer(m_CommandBuffer));

	VkSubmitInfo				submitInfo = { };
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;
	}

	CHECK_API_SUCCESS(vkQueueSubmit(m_Queue.m_ApiQueue, 1, &submitInfo, VK_NULL_HANDLE));
	CHECK_API_SUCCESS(vkQueueWaitIdle(m_Queue.m_ApiQueue));

	Reset();
}

//----------------------------------------------------------------

void	RenderCommand::End()
{
	CHECK_API_SUCCESS(vkEndCommandBuffer(m_CommandBuffer));
}

//----------------------------------------------------------------

void	RenderCommand::Reset()
{
	CHECK_API_SUCCESS(vkResetCommandBuffer(m_CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

//----------------------------------------------------------------

bool	RenderCommand::CanPresentSurface(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
{
	// find whether or not this queue family can present
	VkBool32	canPresentSurface;
	CHECK_API_SUCCESS(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, m_Queue.m_FamilyIndex, surface, &canPresentSurface)); // TODO: add error management

	return canPresentSurface;
}

//----------------------------------------------------------------

bool	RenderCommand::CreateCommandPool(const VkDevice logicalDevice)
{
	VkCommandPoolCreateInfo	commandPoolCreateInfo = Initializers::Pool::CommandCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_Queue.m_FamilyIndex);

	CHECK_API_SUCCESS(vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &m_CommandPool)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	RenderCommand::CreateCommandBuffer(const VkDevice logicalDevice)
{
	VkCommandBufferAllocateInfo	commandBufferAllocateInfo = Initializers::Command::BufferAllocateInfo(m_CommandPool, 0, 1);

	CHECK_API_SUCCESS(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &m_CommandBuffer)); // TODO: add error management

	return true;
}

//----------------------------------------------------------------

bool	RenderCommand::CreateQueue(const VkDevice logicalDevice)
{
	// queue creation
	vkGetDeviceQueue(logicalDevice, m_Queue.m_FamilyIndex, 0, &m_Queue.m_ApiQueue); // FIXME: third parameters (queueIndex) is not correct in every situations

	return true;
}

//----------------------------------------------------------------

bool	RenderCommand::AllocateQueue(COMMAND_TYPE queue, const std::vector<VkQueueFamilyProperties> &queuesFamilyProperties)
{
	bool			queueRetrieved = false;

	// loop on family properties to find valid queue
	const uint32_t	maxQueueFamilyCount = static_cast<uint32_t>(queuesFamilyProperties.size());
	for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < maxQueueFamilyCount; ++queueFamilyIndex)
	{
		const VkQueueFamilyProperties	&currQueueFamilyProperty = queuesFamilyProperties[queueFamilyIndex];
		if (currQueueFamilyProperty.queueCount > 0 &&
			currQueueFamilyProperty.queueFlags & queue)
		{
			m_Queue = Queue(queueFamilyIndex, queue);
			queueRetrieved = true;
			break;
		}
	}

	if (!queueRetrieved) // no valid queue family
		return false;

	return true;
}

//----------------------------------------------------------------

LIGHTLYY_END
