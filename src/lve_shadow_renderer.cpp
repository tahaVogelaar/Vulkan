#include "lve_shadow_renderer.h"
#include <array>
#include <iostream>

namespace lve {
	LvePointShadowRenderer::LvePointShadowRenderer(LveDevice &device, const std::string &vertShaderPath, const std::string &fragShaderPath, VkDescriptorSetLayout globalSetLayout,
		std::vector<VkVertexInputBindingDescription> (*BindingDescriptions)(),
		std::vector<VkVertexInputAttributeDescription> (*AttributeDescriptions)()
		) :
	device(device)
	{
		createRenderPass();
		createPipelineLayout(globalSetLayout);
		createPipeline(vertShaderPath, fragShaderPath);
	}

	LvePointShadowRenderer::~LvePointShadowRenderer()
	{
		for (auto f : framebuffers)
			vkDestroyFramebuffer(device.device(), f.second, nullptr);
		vkDestroyRenderPass(device.device(), renderPass, nullptr);
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	VkFramebuffer LvePointShadowRenderer::createFramebuffers(VkImageView imageView, VkExtent2D extent, uint32_t layers)
	{
		auto it = framebuffers.find(extent);
		if (it != framebuffers.end())
		{
			std::cout << "frame buffer with the size of: " << extent.width << " x " << extent.height << " already exists\n";
			return it->second;
		}

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = &imageView;
		fbInfo.width = extent.width;
		fbInfo.height = extent.height;
		fbInfo.layers = layers;

		VkFramebuffer framebuffer;
		if (vkCreateFramebuffer(device.device(), &fbInfo, nullptr, &framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shadow framebuffer!");
		}

		std::cout << "frame buffer with the size of: " << extent.width << " x " << extent.height << " created\n";
		framebuffers.emplace(std::make_pair(extent, framebuffer));
		return framebuffer;
	}

	VkFramebuffer LvePointShadowRenderer::getFramebuffer(VkExtent2D extent)
	{
		auto it = framebuffers.find(extent);
		if (it != framebuffers.end())
			return it->second;

		return VK_NULL_HANDLE; // always return something
	}

	void LvePointShadowRenderer::createRenderPass()
	{
		VkAttachmentReference depthRef{};
		depthRef.attachment = 0;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pDepthStencilAttachment = &depthRef;

		// ---- DEPTH-ONLY RENDER PASS ----
		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = device.findDepthFormat();
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth at beginning of the render pass
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// We will read from depth, so it's important to store the depth attachment results
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// We don't care about initial layout of the attachment
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// Attachment will be transitioned to shader read at render pass end

		// dependencies
		std::array<VkSubpassDependency, 2> dependencies{};

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create renderpass
		VkRenderPassCreateInfo rpInfo{};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpInfo.attachmentCount = 1;
		rpInfo.pAttachments = &attachmentDescription;
		rpInfo.subpassCount = 1;
		rpInfo.pSubpasses = &subpass;

		rpInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		rpInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(device.device(), &rpInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shadow map render pass!");
		}
	}


	void LvePointShadowRenderer::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void LvePointShadowRenderer::createPipeline(const std::string &vertShaderPath, const std::string &fragShaderPath)
	{
		PipelineConfigInfo pipelineConfig{};
		LvePipeline::shadowPipelineConfigInfo(pipelineConfig);

		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;

		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<LvePipeline>(
			device,
			vertShaderPath,
			fragShaderPath,
			pipelineConfig,
			RenderBucket::getBindingDescriptionsShadow,
			RenderBucket::getAttributeDescriptionsShadow);
	}
}
