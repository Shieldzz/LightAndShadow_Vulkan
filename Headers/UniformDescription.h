#pragma once

#include <tuple>

#include "Initializers.h"
#include "RenderHandle.h"
#include "Device.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

enum class DESCRIPTION_TYPE
{
	Sampler = 0,
	CombinedImageSampler,
	SampledImage,
	StorageImage,
	UniformTexelBuffer,
	StorageTexelBuffer,
	UniformBuffer,
	StorageBuffer,
	UniformBufferDynamic,
	StorageBufferDynamic,
	InputAttachment
};

//----------------------------------------------------------------

union DescriptionInfo
{
	VkDescriptorImageInfo	*m_DescImageInfo;
	VkDescriptorBufferInfo	*m_DescBufferInfo;
	VkBufferView			*m_BufferView;
}; // union DescriptionInfo

//----------------------------------------------------------------

struct UniformDescription
{
	UniformDescription(	const VkDevice logicalDevice,
						const std::vector<std::tuple<DESCRIPTION_TYPE, SHADER_STAGE>> &descriptions,
						const std::vector<DescriptionInfo> &descriptionsInfo,
						uint32_t pendingFrames,
						bool isOffscreen = false)
	{
		// TODO: move descriptor structs in initializers

		// create DescriptorSetLayoutBindings
		const uint32_t								descriptionsCount = static_cast<uint32_t>(descriptions.size());
		std::vector<VkDescriptorSetLayoutBinding>	descLayoutBindings;
		descLayoutBindings.resize(descriptionsCount);

		for (uint32_t descIndex = 0; descIndex < descriptionsCount; ++descIndex)
		{
			VkDescriptorSetLayoutBinding	descBinding = { };
			{
				descBinding.descriptorType = static_cast<VkDescriptorType>(std::get<0>(descriptions[descIndex]));
				descBinding.binding = descIndex;
				descBinding.stageFlags = static_cast<VkShaderStageFlags>(std::get<1>(descriptions[descIndex]));
				descBinding.descriptorCount = 1;
			}

			descLayoutBindings[descIndex] = descBinding;
		}

		// create DescriptorSetLayout
		VkDescriptorSetLayoutCreateInfo				descLayoutCreateInfo = { };
		{
			descLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descLayoutCreateInfo.bindingCount = descriptionsCount;
			descLayoutCreateInfo.pBindings = descLayoutBindings.data();
		}

		VkDescriptorSetLayout						descLayout;
		CHECK_API_SUCCESS(vkCreateDescriptorSetLayout(logicalDevice, &descLayoutCreateInfo, nullptr, &descLayout));

		// create DescriptorSets
		m_DescriptorLayouts = std::vector<VkDescriptorSetLayout>(pendingFrames, descLayout);

		VkDescriptorSetAllocateInfo descAllocateInfo = { };
		{
			descAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descAllocateInfo.descriptorPool = Device::m_Device->GetDescriptorPool();
			descAllocateInfo.descriptorSetCount = pendingFrames;
			descAllocateInfo.pSetLayouts = m_DescriptorLayouts.data();
		}

		m_Descriptors.resize(pendingFrames);
		CHECK_API_SUCCESS(vkAllocateDescriptorSets(logicalDevice, &descAllocateInfo, m_Descriptors.data()));

		// update DescriptorSets with WriteDescriptorSets
		VkWriteDescriptorSet writeDesc = { };
		{
			writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDesc.dstArrayElement = 0;
			writeDesc.descriptorCount = 1;
		}

		const uint32_t		descInfoCount = static_cast<uint32_t>(descriptionsInfo.size()) / pendingFrames;
		for (uint32_t frameIndex = 0; frameIndex < pendingFrames; ++frameIndex)
		{
			writeDesc.dstSet = m_Descriptors[frameIndex];

			std::vector<VkWriteDescriptorSet>	writeDescs;
			writeDescs.resize(descInfoCount);

			const uint32_t						infoIndex = (isOffscreen) ? frameIndex * descInfoCount : 0;

			for (uint32_t descIndex = 0; descIndex < descriptionsCount; ++descIndex)
			{
				writeDesc.dstBinding = descIndex;

				DESCRIPTION_TYPE	descType = std::get<0>(descriptions[descIndex]);
				writeDesc.descriptorType = static_cast<VkDescriptorType>(descType);

				switch (descType) // TODO: add error management
				{
					case DESCRIPTION_TYPE::Sampler:
					case DESCRIPTION_TYPE::CombinedImageSampler:
					case DESCRIPTION_TYPE::SampledImage:
					case DESCRIPTION_TYPE::StorageImage:
					case DESCRIPTION_TYPE::InputAttachment:
					{
						writeDesc.pImageInfo = descriptionsInfo[descIndex + infoIndex].m_DescImageInfo;
						break;
					}
					case DESCRIPTION_TYPE::UniformBuffer:
					case DESCRIPTION_TYPE::StorageBuffer:
					case DESCRIPTION_TYPE::UniformBufferDynamic:
					case DESCRIPTION_TYPE::StorageBufferDynamic:
					{
						writeDesc.pBufferInfo = descriptionsInfo[descIndex + infoIndex].m_DescBufferInfo;
						break;
					}
					case DESCRIPTION_TYPE::UniformTexelBuffer:
					case DESCRIPTION_TYPE::StorageTexelBuffer:
					{
						writeDesc.pTexelBufferView = descriptionsInfo[descIndex + infoIndex].m_BufferView;
						break;
					}
					default:
						break;
				}

				writeDescs[descIndex] = writeDesc;
			}

			vkUpdateDescriptorSets(logicalDevice, descriptionsCount, writeDescs.data(), 0, nullptr);
		}
	}

	const std::vector<VkDescriptorSet>&			GetDescriptors() const { return m_Descriptors; }
	const std::vector<VkDescriptorSetLayout>&	GetDescriptorLayouts() const { return m_DescriptorLayouts; }

private:
	std::vector<VkDescriptorSet>		m_Descriptors;
	std::vector<VkDescriptorSetLayout>	m_DescriptorLayouts;
}; // struct UniformDescription

//----------------------------------------------------------------

LIGHTLYY_END
