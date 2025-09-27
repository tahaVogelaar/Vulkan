#include "first_app.hpp"
#include "simple_render_system.hpp"
#include "lve_buffer.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

// std
#include <array>
#include <cassert>
#include <iomanip>
#include <stdexcept>

namespace lve {
	FirstApp::FirstApp()
	{
		globalPool = LveDescriptorPool::Builder(lveDevice)
				.setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
				.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							LveSwapChain::MAX_FRAMES_IN_FLIGHT)
				.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							LveSwapChain::MAX_FRAMES_IN_FLIGHT)

				.build();
		loadGameObjects();
	}

	FirstApp::~FirstApp()
	{
	}

	void FirstApp::run()
	{
		build();

		while (!lveWindow.shouldClose())
		{
			glfwPollEvents();
			currTime = glfwGetTime();
			deltaTime = currTime - lastTime;
			lastTime = currTime;
			aspect = lveRenderer.getAspectRatio();

			if (VkCommandBuffer commandBuffer = lveRenderer.beginFrame())
			{
				frameIndex = lveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex, static_cast<float>(deltaTime), commandBuffer, camera,
					globalDescriptorSets[frameIndex]
				};

				update(commandBuffer, frameInfo);
				render(commandBuffer, frameInfo);
			}
		}
	}

	void FirstApp::build()
	{
		for (int i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
		{
			uboBuffers[i] = std::make_unique<LveBuffer>(
				lveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();

			drawBuffers[i] = std::make_unique<LveBuffer>(
				lveDevice, sizeof(VkDrawIndexedIndirectCommand), 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			drawBuffers[i]->map();
		}

		auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
				.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							VK_SHADER_STAGE_VERTEX_BIT)
				.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							VK_SHADER_STAGE_VERTEX_BIT)
				.build();

		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			LveDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.build(globalDescriptorSets[i]);

			auto drawBufferInfo = drawBuffers[i]->descriptorInfo();
			LveDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(1, &drawBufferInfo)
					.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{
			lveDevice, lveRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		};
	}

	void FirstApp::update(VkCommandBuffer &commandBuffer, FrameInfo& frameInfo)
	{
		camera.update(lveWindow.getGLFWwindow(), static_cast<float>(deltaTime), ubo);
		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();

		// update title
		if (currTime - lastUpdate1 > 0.5)
		{
			double fps = 1. / (deltaTime);
			std::ostringstream title;
			title << "FPS: " << std::fixed << std::setprecision(1) << fps;
			lveWindow.setName(std::to_string(fps));
			lastUpdate1 = currTime;
		}
	}

	void FirstApp::render(VkCommandBuffer &commandBuffer, FrameInfo& frameInfo)
	{
		lveRenderer.beginSwapChainRenderPass(commandBuffer);

		indirectDraw.render(commandBuffer);

		lveRenderer.endSwapChainRenderPass(commandBuffer);
		lveRenderer.endFrame();
	}

	void FirstApp::loadGameObjects()
	{
		std::vector<std::string> files;
		files.emplace_back("/home/taha/CLionProjects/untitled4/models/smooth_vase.obj");

		indirectDraw.createDrawBuffers(files);
	}
} // namespace lve
