#include "../lve_shadowMap.h"
#include <cassert>
#include <array>
#include <iostream>

namespace lve {
	ShadowMap::ShadowMap(LveDevice &device, VkExtent2D extent) : lveDevice(device), extent(extent)
	{
		create();
	}

	ShadowMap::~ShadowMap()
	{
		vkDestroyImage(lveDevice.device(), image, nullptr);
		vkDestroyImageView(lveDevice.device(), imageView, nullptr);
		vkDestroySampler(lveDevice.device(), sampler, nullptr);
	}

	void ShadowMap::beginRender(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = extent;

		VkClearValue clearValue{};
		clearValue.depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{{0, 0}, extent};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}


	void ShadowMap::endRender(VkCommandBuffer commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
	}

	void ShadowMap::create()
	{
		// sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

		// ---- IMAGE ----
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_D32_SFLOAT;
		imageInfo.extent = VkExtent3D{extent.width, extent.height, 1};
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if (vkCreateImage(lveDevice.device(), &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shadow map image!");
		}

		// memory
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(lveDevice.device(), image, &memRequirements);

		// allocate
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = lveDevice.findMemoryType(
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		if (vkAllocateMemory(lveDevice.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate shadow map memory!");
		}

		vkBindImageMemory(lveDevice.device(), image, imageMemory, 0);

		// ---- IMAGE VIEW ----
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = lveDevice.findDepthFormat();
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;       // start from first mip
		viewInfo.subresourceRange.levelCount = 1;         // only 1 mip
		viewInfo.subresourceRange.baseArrayLayer = 0;     // first layer
		viewInfo.subresourceRange.layerCount = 1;         // 1 layer

		if (vkCreateImageView(lveDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shadow map image view!");
		}

		std::cout << "Image view created" << std::endl;
		if (imageView == VK_NULL_HANDLE)
			std::cout << "Image view is NULL HANDLE" << std::endl;
	}
}
