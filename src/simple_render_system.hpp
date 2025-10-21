#pragma once

#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace lve {
	class SimpleRenderSystem {
	public:
		enum PipelineType {
			default_pipeline,
			shadow_pipeline
		};

		SimpleRenderSystem(
			LveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
			const std::string &vertShaderPath, const std::string &fragShaderPath, PipelineType type);

		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem &) = delete;

		SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

		VkPipeline getPipeline() const { return lvePipeline->getPipeline(); };

		VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

		void createPipeline(VkRenderPass renderPass, const std::string &vertShaderPath,
							const std::string &fragShaderPath, PipelineType type);

		void createShadowPipeline(VkRenderPass renderPass, const std::string &vertShaderPath,
						const std::string &fragShaderPath);

		LveDevice &lveDevice;

		std::unique_ptr<LvePipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
	};
} // namespace lve
