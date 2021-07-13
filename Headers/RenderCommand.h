#pragma once

#include "Initializers.h"

#include "glm/glm.hpp"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct Texture;

//----------------------------------------------------------------

enum COMMAND_TYPE
{
	GraphicsOp = 1,
	ComputeOp = 2,
	TransferOp = 4,
	SparseMemoryOp = 8,
	ProtectedMemory = 16
};

//----------------------------------------------------------------

struct Queue
{
	Queue() : m_FamilyIndex(0), m_ApiQueue(nullptr) { }
	Queue(uint32_t familyIndex, COMMAND_TYPE type) : m_FamilyIndex(familyIndex), m_ApiQueue(nullptr), m_Type(type) { }

	uint32_t		m_FamilyIndex;
	VkQueue			m_ApiQueue;
	COMMAND_TYPE	m_Type;
}; // struct Queue

//----------------------------------------------------------------

class RenderCommand
{
public:
	RenderCommand() = delete;
	RenderCommand(COMMAND_TYPE type);
	~RenderCommand();

	bool					Setup(const std::vector<VkQueueFamilyProperties> &queuesFamilyProperties);
	bool					Prepare(const VkDevice logicalDevice);
	void					Shutdown(const VkDevice logicalDevice);

	void					Begin();
	void					SingleSubmit(const Texture &texture, VkBuffer buffer, VkImageSubresourceRange subRange, bool generateMipmaps);
	void					End();
	void					Reset();

	bool					CanPresentSurface(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface);

	// getters
	const VkCommandBuffer	GetBuffer() const { return m_CommandBuffer; }
	const Queue&			GetQueue() const { return m_Queue; }

private:
	bool			CreateCommandPool(const VkDevice logicalDevice);
	bool			CreateCommandBuffer(const VkDevice logicalDevice);
	bool			CreateQueue(const VkDevice logicalDevice);

	bool			AllocateQueue(COMMAND_TYPE queue, const std::vector<VkQueueFamilyProperties> &queuesFamilyProperties);
	
	COMMAND_TYPE	m_CommandType;

	VkCommandPool	m_CommandPool;
	VkCommandBuffer	m_CommandBuffer;

	Queue			m_Queue;

	bool			m_IsSetup;
}; // class RenderCommand

//----------------------------------------------------------------

LIGHTLYY_END
