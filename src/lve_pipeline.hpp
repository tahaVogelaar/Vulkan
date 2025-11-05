#pragma once

#include "lve_device.hpp"
#include "shaderc/shaderc.hpp"

// std
#include <string>
#include <vector>

#include "Mesh.h"

namespace lve {
	struct PipelineConfigInfo {
		PipelineConfigInfo(const PipelineConfigInfo &) = delete;

		PipelineConfigInfo() = default;

		PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class LvePipeline {
	public:
		LvePipeline(
			LveDevice &device,
			const std::string &vertFilepath,
			const std::string &fragFilepath,
			const PipelineConfigInfo &configInfo,
			bool USEAA = true,
			std::vector<VkVertexInputBindingDescription> (*BindingDescriptions)() = RenderBucket::getBindingDescriptions,
			std::vector<VkVertexInputAttributeDescription> (*AttributeDescriptions)() = RenderBucket::getAttributeDescriptions);

		~LvePipeline();

		LvePipeline(const LvePipeline &) = delete;
		LvePipeline &operator=(const LvePipeline &) = delete;

		void bind(VkCommandBuffer commandBuffer);

		static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo, VkExtent2D extent = VkExtent2D{0, 0});
		static void shadowPipelineConfigInfo(PipelineConfigInfo &configInfo, VkExtent2D extent = VkExtent2D{0, 0});

		VkPipeline getPipeline() const {return graphicsPipeline; };

	private:
		static std::string readFile(const std::string &filepath);

		static std::vector<uint32_t> shaderToSpirV(std::string source,
													shaderc_shader_kind kind,
													const std::string &name,
													const std::string &entryPoint);

		void createGraphicsPipeline(
			const std::string &vertFilepath,
			const std::string &fragFilepath,
			const PipelineConfigInfo &configInfo,
			bool USEAA,
			std::vector<VkVertexInputBindingDescription> (*BindingDescriptions)(),
			std::vector<VkVertexInputAttributeDescription> (*AttributeDescriptions)());

		void createShaderModule(const std::vector<uint32_t> &code, VkShaderModule *shaderModule);

		LveDevice &lveDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
} // namespace lve
