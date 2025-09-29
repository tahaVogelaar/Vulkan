#include "first_app.hpp"
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
				.setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
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
			currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;
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
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = std::make_unique<LveBuffer>(
				lveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();

			drawBuffers[i] = std::make_unique<LveBuffer>(
				lveDevice, sizeof(Object) * 1, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			drawBuffers[i]->map();
		}

		auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
				.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							VK_SHADER_STAGE_ALL)
				.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							VK_SHADER_STAGE_ALL)
				.build();

		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto drawBufferInfo = drawBuffers[i]->descriptorInfo();
			LveDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeBuffer(1, &drawBufferInfo)
					.build(globalDescriptorSets[i]);
		}

		simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
			lveDevice, lveRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		);
	}

	void FirstApp::update(VkCommandBuffer &commandBuffer, FrameInfo &frameInfo)
	{
		camera.update(lveWindow.getGLFWwindow(), static_cast<float>(deltaTime), ubo);
		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();

		// update title
		if (currentTime - lastUpdate1 > 0.5)
		{
			double fps = 1. / (deltaTime);
			std::ostringstream title;
			title << "FPS: " << std::fixed << std::setprecision(1) << fps;
			lveWindow.setName(std::to_string(fps));
			lastUpdate1 = currentTime;
		}
	}

	void FirstApp::render(VkCommandBuffer &commandBuffer, FrameInfo &frameInfo)
	{
		lveRenderer.beginSwapChainRenderPass(commandBuffer);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, simpleRenderSystem->getPipeline());
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			simpleRenderSystem->getPipelineLayout(),
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		indirectDraw.render(commandBuffer);

		lveRenderer.endSwapChainRenderPass(commandBuffer);
		lveRenderer.endFrame();
	}

	void FirstApp::loadGameObjects()
	{
		std::vector<std::string> files;
		files.emplace_back("/home/taha/CLionProjects/untitled4/models/smooth_vase.obj");
		files.emplace_back("/home/taha/CLionProjects/untitled4/models/colored_cube.obj");
		indirectDraw.createMeshes(files);

		std::vector<Object> obj;
		obj.push_back(Object(glm::mat4(1), 0));
		obj.push_back(Object(glm::mat4(1), 1));
		indirectDraw.createObjects(obj);
	}
} // namespace lve
