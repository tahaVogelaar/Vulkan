#pragma once

#include  <memory>

#include "core/lve_device.hpp"
#include "core/lve_pipeline.hpp"
#include "core/lve_swap_chain.hpp"
#include "lve_frame_buffer.h"

class FullScreenQuat {
public:
	FullScreenQuat(lve::LveDevice& device, lve::LveSwapChain& swapchain, VkExtent2D extent, VkDescriptorSetLayout globalSetLayout, std::string& vertShader, std::string& fragShader);
	//~FullScreenQuat();
	void draw(VkCommandBuffer cmd, int index, VkDescriptorSet& a);

private:
	void createPipeline(std::string& vertShader, std::string& fragShader);
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

	lve::LveDevice& device;
	std::unique_ptr<lve::LvePipeline> pipeline;
	VkPipelineLayout pipelineLayout;
	lve::LveSwapChain& swapchain;
};