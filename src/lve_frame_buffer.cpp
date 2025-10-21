#include "lve_frame_buffer.h"
#include <stdexcept>

namespace lve {
	LveFrameBuffer::LveFrameBuffer(LveSwapChain &swapChain, LveDevice &device, ConstructionMode mode,
									VkExtent2D extent) : renderPass(VK_NULL_HANDLE), swapChain(swapChain),
														device(device)
	{
		constructMode(mode, extent);
		create(mode, extent);
	}

	LveFrameBuffer::~LveFrameBuffer()
	{
		// Destroy render pass
		if (renderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(device.device(), renderPass, nullptr);
		}
	}

	VkCommandBuffer LveFrameBuffer::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.getCommandPool(); // <-- you need a command pool in LveDevice
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // good for single-use buffers

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		return commandBuffer;
	}

	void LveFrameBuffer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device.graphicsQueue()); // wait until finished (blocking)

		vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &commandBuffer);
	}


	void LveFrameBuffer::constructMode(ConstructionMode mode, VkExtent2D extent)
	{
		if (mode == ConstructionMode::Color || mode == ConstructionMode::ColorAndDepth)
		{
			// Color attachment
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = swapChain.getSwapChainImageFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			attachments.push_back(colorAttachment);
			usages.push_back(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}

		if (mode == ConstructionMode::Depth || mode == ConstructionMode::DepthStencil || mode ==
			ConstructionMode::ColorAndDepth)
		{
			// Depth attachment
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = swapChain.findDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments.push_back(depthAttachment);
			usages.push_back(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}
	}


	void LveFrameBuffer::create(ConstructionMode mode, VkExtent2D extent)
	{
		this->extent = extent;

		// Create images and views
		for (size_t i = 0; i < attachments.size(); i++)
		{
			auto &desc = attachments[i];
			auto usage = usages[i];

			// (1) Create VkImage
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = desc.format;
			imageInfo.extent.width = extent.width;
			imageInfo.extent.height = extent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = desc.samples;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = usage;

			VkImage image;
			if (vkCreateImage(device.device(), &imageInfo, nullptr, &image) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer image!");
			}

			// Allocate memory (simplified, use VMA in production)
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(device.device(), image, &memRequirements);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits,
															VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkDeviceMemory imageMemory;
			if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate framebuffer image memory!");
			}
			vkBindImageMemory(device.device(), image, imageMemory, 0);

			// (2) Create image view
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = desc.format;
			viewInfo.subresourceRange.aspectMask =
					(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
						? VK_IMAGE_ASPECT_DEPTH_BIT
						: VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer image view!");
			}
		}

		// Create render pass
		createRenderPass();
	}

	void LveFrameBuffer::createFramebuffers()
	{
		VkExtent2D swapChainExtent = extent;
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
		framebufferInfo.pAttachments = imageViews.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&frameBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}


	void LveFrameBuffer::createRenderPass()
	{
		std::vector<VkAttachmentReference> colorRefs;
		VkAttachmentReference depthRef{};
		bool hasDepth = false;

		for (size_t i = 0; i < attachments.size(); i++)
		{
			auto &desc = attachments[i];
			if (desc.finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				depthRef.attachment = static_cast<uint32_t>(i);
				depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				hasDepth = true;
			} else
			{
				VkAttachmentReference ref{};
				ref.attachment = static_cast<uint32_t>(i);
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorRefs.push_back(ref);
			}
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();
		subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer render pass!");
		}
	}
} // namespace lve
