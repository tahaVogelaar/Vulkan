#pragma once
#include "core/lve_device.hpp"
#include "core/lve_pipeline.hpp"

#include <memory>
#include <unordered_map>

namespace lve {
	struct Extent2DHash {
		size_t operator()(const VkExtent2D &extent) const noexcept
		{
			// Simple and effective hash
			return (static_cast<size_t>(extent.width) << 32) ^ static_cast<size_t>(extent.height);
		}
	};

	struct Extent2DEqual {
		bool operator()(const VkExtent2D& a, const VkExtent2D& b) const {
			return a.width == b.width && a.height == b.height;
		}
	};

	class LvePointShadowRenderer {
	public:
		LvePointShadowRenderer(LveDevice &device, const std::string &vertShaderPath, const std::string &fragShaderPath, VkDescriptorSetLayout globalSetLayout,
			std::vector<VkVertexInputBindingDescription> (*BindingDescriptions)(),
			std::vector<VkVertexInputAttributeDescription> (*AttributeDescriptions)()
			);

		~LvePointShadowRenderer();

		LvePointShadowRenderer(const LvePointShadowRenderer &) = delete;

		LvePointShadowRenderer &operator=(const LvePointShadowRenderer &) = delete;

		VkFramebuffer createFramebuffers(VkImageView imageView, VkExtent2D extent, uint32_t layers);
		VkFramebuffer getFramebuffer(VkExtent2D extent);

		VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
		VkPipeline getPipeline() { return pipeline->getPipeline(); }
		VkRenderPass getRenderPass() { return renderPass; }

		void bindPipeline(VkCommandBuffer commandBuffer)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
		}

	private:
		void createRenderPass();

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

		void createPipeline(const std::string &vertShaderPath, const std::string &fragShaderPath);

		// framebuffer held by resolution with ShadowMapSize
		std::unordered_map<VkExtent2D, VkFramebuffer, Extent2DHash, Extent2DEqual> framebuffers;

		// single renderPass
		VkRenderPass renderPass;

		// pipeline stuff
		std::unique_ptr<LvePipeline> pipeline;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		LveDevice &device;
	};
}
