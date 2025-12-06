#include "fullScreenQuat.h"
#include <iostream>

FullScreenQuat::FullScreenQuat(lve::LveDevice &device, lve::LveSwapChain& swapchain, VkExtent2D extent, VkDescriptorSetLayout globalSetLayout, std::string& vertShader, std::string& fragShader) :
device(device), swapchain(swapchain)
{
	createPipelineLayout(globalSetLayout);
	createPipeline(vertShader, fragShader);
}

void FullScreenQuat::draw(VkCommandBuffer cmd, int index, VkDescriptorSet& a)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0,
		1,
		&a,
		0,
		nullptr
	);
	vkCmdDraw(cmd, 3, 1, 0, 0);
}


void FullScreenQuat::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
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

void FullScreenQuat::createPipeline(std::string& vertShader, std::string& fragShader)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	lve::PipelineConfigInfo pipelineConfig{};
	lve::LvePipeline::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.renderPass = swapchain.getRenderPass();
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<lve::LvePipeline>(
		device,
		vertShader,
		fragShader,
		pipelineConfig,
		false);
}