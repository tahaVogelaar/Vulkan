#pragma once
#include "core/lve_device.hpp"
#include "core/lve_swap_chain.hpp"
#include <array>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace lve {

	class LveFrameBuffer {
	public:
		enum ConstructionMode { Color = 0, Depth = 1, Stencil = 2, DepthStencil = 3, ColorAndDepth = 4 };

		LveFrameBuffer(LveSwapChain& swapChain, LveDevice& device, ConstructionMode mode, VkExtent2D extent);
		LveFrameBuffer(LveSwapChain& swapChain, LveDevice& device);

		~LveFrameBuffer();

		LveFrameBuffer(const LveFrameBuffer &) = delete;

		LveFrameBuffer &operator=(const LveFrameBuffer &) = delete;

		bool isOperational() const {return enabled;}

		void create(ConstructionMode mode, VkExtent2D extent);

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void createFramebuffers();

	//private:
		void constructMode(ConstructionMode mode, VkExtent2D extent);

		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		ConstructionMode createInfo;
		std::vector<VkImageView> imageViews;
		std::vector<VkImage> images;
		VkExtent2D extent;
		std::vector<VkFormat> formats;
		std::vector<VkImageUsageFlags> usages;
		std::vector<VkAttachmentDescription> attachments;
		LveDevice& device;
		LveSwapChain& swapChain;
		bool enabled = false;

		void createRenderPass();
	};
}
