#pragma once

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_game_object.hpp"
#include "lve_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace lve {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(
			LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem &) = delete;

		SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

		void renderGameObjects(FrameInfo &frameInfo);

		VkPipeline getPipeline() const { return lvePipeline->getPipeline(); };

		VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

		void createPipeline(VkRenderPass renderPass);

		LveDevice &lveDevice;

		std::unique_ptr<LvePipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
	};
} // namespace lve
