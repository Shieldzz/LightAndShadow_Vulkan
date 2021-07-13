#pragma once

#include "Initializers.h"
#include "Device.h"
#include "Utility.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

enum BUFFER_TYPE
{
	TransferSource = 1,
	TransferDest = 2,
	UniformTexel = 4,
	StorageTexel = 8,
	Uniform = 16,
	Storage = 32,
	Index = 64,
	Vertex = 128,
	Indirect = 256,
};

//----------------------------------------------------------------

template<typename T>
struct Buffer
{
	Buffer() {};

	// static buffer
	Buffer(const VkDevice logicalDevice, BUFFER_TYPE type, uint32_t dataCount)
	{
		m_Size = sizeof(T) * dataCount;
		VkBufferCreateInfo		bufferCreateInfo = Initializers::Buffer::CreateInfo(type, m_Size);

		CHECK_API_SUCCESS(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &m_Buffer));

		VkMemoryRequirements	memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, m_Buffer, &memRequirements);

		if (!Device::m_Device->AllocateMemory(memRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Memory))
			std::cout << "Can't allocate memory for buffer" << std::endl; // TODO: change this for real logger

		CHECK_API_SUCCESS(vkBindBufferMemory(logicalDevice, m_Buffer, m_Memory, 0));
	}

	// dynamic buffer
	Buffer(const VkDevice logicalDevice, BUFFER_TYPE type, uint32_t dataCount, uint32_t alignment)
	{
		m_Size = dataCount * alignment;
		VkBufferCreateInfo		bufferCreateInfo = Initializers::Buffer::CreateInfo(type, m_Size);

		CHECK_API_SUCCESS(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &m_Buffer));

		VkMemoryRequirements	memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, m_Buffer, &memRequirements);

		if (!Device::m_Device->AllocateMemory(memRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Memory))
			std::cout << "Can't allocate memory for buffer" << std::endl; // TODO: change this for real logger

		CHECK_API_SUCCESS(vkBindBufferMemory(logicalDevice, m_Buffer, m_Memory, 0));
	}

	void			Destroy(const VkDevice logicalDevice)
	{
		if (m_Buffer != nullptr)
			vkDestroyBuffer(logicalDevice, m_Buffer, nullptr);
		if (m_Memory != nullptr)
			vkFreeMemory(logicalDevice, m_Memory, nullptr);
	}

	// static buffer
	void			UpdateData(const VkDevice logicalDevice, const std::vector<T> &data)
	{
		void	*mappedData;
		vkMapMemory(logicalDevice, m_Memory, 0, m_Size, 0, &mappedData);
		memcpy(mappedData, data.data(), static_cast<size_t>(m_Size));
		vkUnmapMemory(logicalDevice, m_Memory);
	}

	// dynamic buffer
	void			UpdateData(const VkDevice logicalDevice, const T *data)
	{
		void	*mappedData;
		vkMapMemory(logicalDevice, m_Memory, 0, m_Size, 0, &mappedData);
		memcpy(mappedData, data, static_cast<size_t>(m_Size));
		vkUnmapMemory(logicalDevice, m_Memory);
	}

	void			UpdateData(const VkDevice logicalDevice, const std::vector<std::vector<T>> &data)
	{
		void			*mappedData;
		vkMapMemory(logicalDevice, m_Memory, 0, m_Size, 0, &mappedData);

		const uint32_t	dataCount = static_cast<uint32_t>(data.size());
		for (uint32_t dataIndex = 0; dataIndex < dataCount; ++dataIndex)
		{
			const uint32_t	dataSize = static_cast<uint32_t>(data[dataIndex].size());
			memcpy((char*)(mappedData) + dataSize * dataIndex, data[dataIndex].data(), dataSize);
		}

		vkUnmapMemory(logicalDevice, m_Memory);
	}

	const VkBuffer	GetApiBuffer() const { return m_Buffer; }

private:
	VkBuffer		m_Buffer;
	VkDeviceMemory	m_Memory;

	uint64_t		m_Size;
}; // struct Buffer

//----------------------------------------------------------------

LIGHTLYY_END
