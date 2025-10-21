/*
* Vulkan framebuffer class
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <algorithm>
#include <iterator>
#include <vector>
#include "vulkan/vulkan.h"
#include "lve_device.hpp"

namespace lve
{
	/**
	* @brief Encapsulates a single frame buffer attachment
	*/
	struct FramebufferAttachment
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkImageSubresourceRange subresourceRange;
		VkAttachmentDescription description;

		/**
		* @brief Returns true if the attachment has a depth component
		*/
		bool hasDepth()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment has a stencil component
		*/
		bool hasStencil()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment is a depth and/or stencil attachment
		*/
		bool isDepthStencil()
		{
			return(hasDepth() || hasStencil());
		}

	};

	/**
	* @brief Describes the attributes of an attachment to be created
	*/
	struct AttachmentCreateInfo
	{
		uint32_t width, height;
		uint32_t layerCount;
		VkFormat format;
		VkImageUsageFlags usage;
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
	};

	/**
	* @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	*/
	struct Framebuffer
	{
	private:
		LveDevice *vulkanDevice;
	public:
		uint32_t width, height;
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
		VkSampler sampler;
		std::vector<lve::FramebufferAttachment> attachments;

		/**
		* Default constructor
		*
		* @param vulkanDevice Pointer to a valid VulkanDevice
		*/
		Framebuffer(LveDevice *vulkanDevice)
		{
			assert(vulkanDevice);
			this->vulkanDevice = vulkanDevice;
		}

		/**
		* Destroy and free Vulkan resources used for the framebuffer and all of its attachments
		*/
		~Framebuffer()
		{
			assert(vulkanDevice);
			for (auto attachment : attachments)
			{
				vkDestroyImage(vulkanDevice->device(), attachment.image, nullptr);
				vkDestroyImageView(vulkanDevice->device(), attachment.view, nullptr);
				vkFreeMemory(vulkanDevice->device(), attachment.memory, nullptr);
			}
			vkDestroySampler(vulkanDevice->device(), sampler, nullptr);
			vkDestroyRenderPass(vulkanDevice->device(), renderPass, nullptr);
			vkDestroyFramebuffer(vulkanDevice->device(), framebuffer, nullptr);
		}

		/**
		* Add a new attachment described by createinfo to the framebuffer's attachment list
		*
		* @param createinfo Structure that specifies the framebuffer to be constructed
		*
		* @return Index of the new attachment
		*/
		uint32_t addAttachment(AttachmentCreateInfo createinfo)
		{
			FramebufferAttachment attachment;

			attachment.format = createinfo.format;

			VkImageAspectFlags aspectMask = 0;

			// Select aspect mask and layout depending on usage

			// Color attachment
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			// Depth (and/or stencil) attachment
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (attachment.hasDepth())
				{
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil())
				{
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			assert(aspectMask > 0);

			// custom
			VkImageCreateInfo imageCreateInfo {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			// custom

			VkImageCreateInfo image = imageCreateInfo;
			image.imageType = VK_IMAGE_TYPE_2D;
			image.format = createinfo.format;
			image.extent.width = createinfo.width;
			image.extent.height = createinfo.height;
			image.extent.depth = 1;
			image.mipLevels = 1;
			image.arrayLayers = createinfo.layerCount;
			image.samples = createinfo.imageSampleCount;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.usage = createinfo.usage;


			// custom
			VkMemoryAllocateInfo memAllocInfo {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			// custom

			VkMemoryAllocateInfo memAlloc = memAllocInfo;
			VkMemoryRequirements memReqs;

			// Create image for this attachment
			vkCreateImage(vulkanDevice->device(), &image, nullptr, &attachment.image);
			vkGetImageMemoryRequirements(vulkanDevice->device(), attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = vulkanDevice->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(vulkanDevice->device(), &memAlloc, nullptr, &attachment.memory);
			vkBindImageMemory(vulkanDevice->device(), attachment.image, attachment.memory, 0);

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;
			attachment.subresourceRange.levelCount = 1;
			attachment.subresourceRange.layerCount = createinfo.layerCount;


			// custom
			VkImageViewCreateInfo imageViewCreateInfo {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			VkImageViewCreateInfo imageView = imageViewCreateInfo;
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageView.format = createinfo.format;
			imageView.subresourceRange = attachment.subresourceRange;
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
			imageView.image = attachment.image;
			vkCreateImageView(vulkanDevice->device(), &imageView, nullptr, &attachment.view);

			// Fill attachment description
			attachment.description = {};
			attachment.description.samples = createinfo.imageSampleCount;
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.format = createinfo.format;
			attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			// Final layout
			// If not, final layout depends on attachment type
			if (attachment.hasDepth() || attachment.hasStencil())
			{
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		* Creates a default sampler for sampling from any of the framebuffer attachments
		* Applications are free to create their own samplers for different use cases
		*
		* @param magFilter Magnification filter for lookups
		* @param minFilter Minification filter for lookups
		* @param adressMode Addressing mode for the U,V and W coordinates
		*
		* @return VkResult for the sampler creation
		*/
		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode)
		{
			// custom
			VkSamplerCreateInfo samplerCreateInfo {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;

			VkSamplerCreateInfo samplerInfo = samplerCreateInfo;
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = adressMode;
			samplerInfo.addressModeV = adressMode;
			samplerInfo.addressModeW = adressMode;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			return vkCreateSampler(vulkanDevice->device(), &samplerInfo, nullptr, &sampler);
		}

		/**
		* Creates a default render pass setup with one sub pass
		*
		* @return VK_SUCCESS if all resources have been created successfully
		*/
		VkResult createRenderPass()
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments)
			{
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			std::vector<VkAttachmentReference> colorReferences;
			VkAttachmentReference depthReference = {};
			bool hasDepth = false;
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto& attachment : attachments)
			{
				if (attachment.isDepthStencil())
				{
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				}
				else
				{
					colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor)
			{
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth)
			{
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 4> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;


			dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].dstSubpass = 0;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			dependencies[2].srcSubpass = 0;
			dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			dependencies[3].srcSubpass = 0;
			dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			// Create render pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 4;
			renderPassInfo.pDependencies = dependencies.data();
			vkCreateRenderPass(vulkanDevice->device(), &renderPassInfo, nullptr, &renderPass);

			std::vector<VkImageView> attachmentViews;
			for (auto& attachment : attachments)
			{
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto& attachment : attachments)
			{
				if (attachment.subresourceRange.layerCount > maxLayers)
				{
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;
			vkCreateFramebuffer(vulkanDevice->device(), &framebufferInfo, nullptr, &framebuffer);

			return VK_SUCCESS;
		}

		VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool, VkRect2D renderArea, std::vector<VkClearValue> clearValues)
		{
			// Allocate a primary command buffer
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer;
			vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

			// Begin command buffer recording
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			// Setup render pass begin info
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = framebuffer;
			renderPassInfo.renderArea = renderArea;
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			return commandBuffer;
		}

		/**
		* End the render pass and finish recording the command buffer
		*/
		void endCommandBuffer(VkCommandBuffer commandBuffer)
		{
			vkCmdEndRenderPass(commandBuffer);
			vkEndCommandBuffer(commandBuffer);
		}
	};

}