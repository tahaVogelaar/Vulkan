#pragma once

#include "lve_device.hpp"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <memory>
#include <string>
#include <vector>

namespace lve {
	class LveSwapChain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		LveSwapChain(LveDevice &deviceRef, VkExtent2D windowExtent);

		LveSwapChain(
			LveDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<LveSwapChain> previous);

		~LveSwapChain();

		LveSwapChain(const LveSwapChain &) = delete;
		LveSwapChain &operator=(const LveSwapChain &) = delete;

		void copyMainToSwapChain(size_t index);
		void mainPrepareRenderPass(size_t index);

		VkImageView getMainImageView(int index) { return mainImageViews[index]; }
		VkImage getMainImage(int index) { return mainImages[index]; }
		VkFormat getMainImageFormat() { return mainImageFormat; }

		VkImageView getSwapchainImageView(int index) { return swapChainImageViews[index]; }
		VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }

		VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
		VkRenderPass getRenderPass() { return renderPass; }

		VkImage getDepthImage(int index) { return depthImages[index]; }
		VkImageView getDepthImageView(int index) { return depthImageViews[index]; }
		VkSampler getDepthSampler() {return depthSampler;}

		size_t imageCount() { return swapChainImages.size(); }
		VkExtent2D getSwapChainExtent() { return swapChainExtent; }
		uint32_t width() { return swapChainExtent.width; }
		uint32_t height() { return swapChainExtent.height; }
		VkSwapchainKHR getSwapChain() { return swapChain; }

		float extentAspectRatio()
		{
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}

		VkFormat findDepthFormat();
		static VkFormat getImageFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }

		VkResult acquireNextImage(uint32_t *imageIndex);

		VkResult submitCommandBuffers(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		bool compareSwapFormats(const LveSwapChain &swapChain) const
		{
			return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
					swapChain.swapChainImageFormat == swapChainImageFormat;
		}

		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR> &availableFormats);

		static VkPresentModeKHR chooseSwapPresentMode(
			const std::vector<VkPresentModeKHR> &availablePresentModes);
	private:
		void init();

		void createMainResources();

		void createSwapChain();

		void createSwapChainImageViews();

		void createDepthResources();

		void createRenderPass();

		void createFramebuffers();

		void createSyncObjects();


		// Helper functions

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

		VkFormat mainImageFormat;
		VkFormat swapChainImageFormat;
		VkFormat swapChainDepthFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass;

		// depth
		std::vector<VkImage> depthImages;
		VkSampler depthSampler;
		std::vector<VkImageView> depthImageViews;
		std::vector<VkDeviceMemory> depthImageMemorys;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkImage> mainImages;
		std::vector<VkDeviceMemory> mainImageMemorys;
		std::vector<VkImageView> mainImageViews;

		LveDevice &device;
		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain;
		std::shared_ptr<LveSwapChain> oldSwapChain;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;

		std::vector<VkFence> computeInFlightFences;
		std::vector<VkSemaphore> computeFinishedSemaphores;

		size_t currentFrame = 0;
	};
} // namespace lve
