#include "first_app.hpp"
#include "core/lve_buffer.hpp"

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
#include <algorithm>
#include <random>

namespace lve {

	void FirstApp::loadGameObjects()
	{
		std::vector<std::string> files;
		//files.emplace_back("../models/abandoned_factory_remodelled__gameready/sigma.gltf");
		//files.emplace_back("../models/free_porsche_911_carrera_4s/scene.gltf");
		//files.emplace_back("../models/ccity_building_set_1/ccity_building_set_1.gltf");
		//files.emplace_back("../models/stuff/main_sponza/Untitled.gltf");
		//files.emplace_back("../models/stuff/pkg_a_curtains/NewSponza_Curtains_glTF.gltf");
		files.emplace_back("../models/DamagedHelmet.gltf");
		//files.emplace_back("../models/Sphere.gltf");
		renderBucket.getObjectLoader().loadScene(files);
		renderBucket.loadMeshes(renderBucket.getObjectLoader().getPrimitives());
	}

	void FirstApp::loadTexturesIntoDescriptor()
	{
		std::vector<std::unique_ptr<VulkanTexture>>& aa = renderBucket.getObjectLoader().getLoader().getTextures();
		uint32_t NUM_TEXTURES = aa.size();

		std::vector<VkDescriptorImageInfo> imageInfos(NUM_TEXTURES);
		for (int i = 0; i < NUM_TEXTURES; i++)
		{
			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].imageView = aa[i]->view;
			imageInfos[i].sampler = aa[i]->sampler;
		}

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstBinding = 4;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(NUM_TEXTURES);
		descriptorWrite.pImageInfo = imageInfos.data();

		for (size_t k = 0; k < LveSwapChain::MAX_FRAMES_IN_FLIGHT; ++k)
		{
			descriptorWrite.dstSet = globalDescriptorSets[k];
			vkUpdateDescriptorSets(lveDevice.device(), 1, &descriptorWrite, 0, nullptr);
		}

		std::vector<Material>& m = renderBucket.getObjectLoader().getLoader().getMaterials();
		uint32_t NUM_Mat = aa.size();
		materialBuffer->writeToBuffer(m.data());
		materialBuffer->flush();
	}


	void FirstApp::run()
	{
		while (!lveWindow.shouldClose())
		{
			// update stuff
			glfwPollEvents();
			currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;
			aspect = lveRenderer.getAspectRatio();
			WIDTH = static_cast<int>(lveWindow.getExtent().width);
			HEIGHT = static_cast<int>(lveWindow.getExtent().height);

			if (VkCommandBuffer commandBuffer = startFrame())
			{
				frameIndex = lveRenderer.getFrameIndex();

				update(commandBuffer);
				//updateShadow();
				render(commandBuffer);

				//VulkanTexture::transitionImageLayout(lveDevice, lveRenderer.getSwapChain()->getDepthImage(frameIndex),
				//	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

				//fullScreenQuat->draw(commandBuffer, frameIndex, globalDescriptorSets[frameIndex]);

				renderImGui(commandBuffer);


				lveRenderer.endSwapChainRenderPass(commandBuffer);
				//compute->run(lveRenderer.getSwapChain()->getMainImage(frameIndex), commandBuffer, lveWindow.getExtent());
				lveRenderer.endFrame();
				endFrame(commandBuffer);
			}
		}

		vkDeviceWaitIdle(lveDevice.device());
	}

	std::string formatFloat2(double value) {
		// round to 2 decimal places
		value = std::round(value * 100.0) / 100.0;
		std::ostringstream oss;
		oss << value;
		return oss.str(); // trailing zeros dropped automatically
	}

	std::string formatFloat5(double value) {
		// round to 2 decimal places
		value = std::round(value * 100000.0) / 100000.0;
		std::ostringstream oss;
		oss << value;
		return oss.str(); // trailing zeros dropped automatically
	}


	void FirstApp::update(VkCommandBuffer &commandBuffer)
	{
		camera.update(lveWindow.getGLFWwindow(), static_cast<float>(deltaTime), ubo);
		ubo.lightData.x = renderSyncSystem->getLightCount();
		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();
		// update title
		if (currentTime - lastUpdate1 > .5)
		{
			double fps = 1. / (deltaTime);
			std::string title;
			title += "fps: " + formatFloat2(fps);
			title += " ms: " + formatFloat5(deltaTime);
			lveWindow.setName(title.c_str());
			lastUpdate1 = currentTime;
		}

		renderSyncSystem->syncToRenderBucket(globalDescriptorSets[frameIndex]);
		renderBucket.update(deltaTime, *drawSSBO);
	}

	void FirstApp::updateShadow()
	{
		VkCommandBuffer cmd = lveDevice.beginSingleTimeCommands();
		renderSyncSystem->getPointShadowRenderer().bindPipeline(cmd);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			renderSyncSystem->getPointShadowRenderer().getPipelineLayout(),
			0,
			1,
			&globalDescriptorSets[frameIndex],
			0,
			nullptr
		);/*
		renderSyncSystem->getLightIndex(0).shadowMap->beginRender(cmd, renderSyncSystem->getPointShadowRenderer().getRenderPass(),
											renderSyncSystem->getPointShadowRenderer().getFramebuffer(shadowExtent));
		renderBucket.render(cmd);
		renderSyncSystem->getLightIndex(0).shadowMap->endRender(cmd);
		lveDevice.endSingleTimeCommands(cmd);
		renderSyncSystem->getLightIndex(0).updateDescriptorSet(lveDevice, globalDescriptorSets[frameIndex]);*/
	}

	void FirstApp::render(VkCommandBuffer &commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, simpleRenderSystem->getPipeline());
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			simpleRenderSystem->getPipelineLayout(),
			0,
			1,
			&globalDescriptorSets[frameIndex],
			0,
			nullptr);

		renderBucket.render(commandBuffer);
	}

	void FirstApp::renderImGui(VkCommandBuffer &commandBuffer)
	{
		renderSyncSystem->renderImGuiWindow(commandBuffer, WIDTH, HEIGHT);
	}




	// init
	FirstApp::FirstApp()
	{
		globalPool = LveDescriptorPool::Builder(lveDevice)
				.setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
				// global ubo
				.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							LveSwapChain::MAX_FRAMES_IN_FLIGHT)
				// game objects
				.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)

				// lights
				.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)

				// bindless m
				.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							256)
				.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)

				.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
							256)

				.build();

		loadGameObjects();
		buildPreDescriptor();
		buildDescriptors();
		loadTexturesIntoDescriptor();
		build();
		initImGui();
	}

	FirstApp::~FirstApp()
	{
	}

	void FirstApp::buildPreDescriptor()
	{
	}

	void FirstApp::buildDescriptors()
	{
		// setup infos
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = std::make_unique<LveBuffer>(
				lveDevice, sizeof(GlobalUbo), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		drawSSBO = std::make_unique<LveBuffer>(
			lveDevice, sizeof(InstanceData) * MAX_OBJECT_COUNT, 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		drawSSBO->map();

		// material buffer
		materialBuffer = std::make_unique<LveBuffer>(
			lveDevice, sizeof(Material) * 256, 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		materialBuffer->map();


		// create Texture
		std::vector<VkDescriptorImageInfo> imageInfos;

		// setup layout
		globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
				.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // global ubo
							VK_SHADER_STAGE_ALL)
				.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // draw object ssbo
							VK_SHADER_STAGE_ALL)
				.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // lights
							VK_SHADER_STAGE_ALL)
				// materials
				.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				// texture samplers
				.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
					256)
				.build();


		// setup descriptor set
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto drawBufferInfo = drawSSBO->descriptorInfo();
			//auto pointLightBufferInfo = pointLightBuffer->descriptorInfo();
			auto materialBufferInfo = materialBuffer->descriptorInfo();

			LveDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeBuffer(1, &drawBufferInfo)
					//.writeBuffer(2, &pointLightBufferInfo)
					.writeBuffer(3, &materialBufferInfo)
					.build(globalDescriptorSets[i]);
		}
	}

	void FirstApp::build()
	{
		simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
			lveDevice, lveRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout(),
			simpleVert, simpleFrag, SimpleRenderSystem::PipelineType::default_pipeline
		);

		renderSyncSystem = std::make_unique<RenderSyncSystem>(
			renderBucket, lveDevice, *globalSetLayout.get(), renderBucket.getObjectLoader());

		std::string AAa = "/home/taha/CLionProjects/untitled4/shaders/sky.vert";
		std::string AAAa = "/home/taha/CLionProjects/untitled4/shaders/sky.frag";
		fullScreenQuat = std::make_unique<FullScreenQuat>(
			lveDevice, *lveRenderer.getSwapChain(), VkExtent2D(WIDTH, HEIGHT),
			globalSetLayout->getDescriptorSetLayout(), AAa, AAAa
		 );

		std::string sor = "/home/taha/CLionProjects/untitled4/shaders/compute.glsl";
		compute = std::make_unique<Compute>(lveDevice, sor, *lveRenderer.getSwapChain());
	}

	VkCommandBuffer FirstApp::startFrame()
	{
		if (VkCommandBuffer commandBuffer = lveRenderer.beginFrame())
		{
			lveRenderer.beginSwapChainRenderPass(commandBuffer);
			return commandBuffer;
		}
		else
			return VK_NULL_HANDLE;
	}

	void FirstApp::endFrame(VkCommandBuffer &commandBuffer)
	{
	}


	void FirstApp::initImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		io.IniFilename = nullptr;
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(lveWindow.getGLFWwindow(), true);

		// Descriptor pool (as before)
		VkDescriptorPoolSize pool_sizes[] = {
			{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
			{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
			{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
		};
		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool;
		vkCreateDescriptorPool(lveDevice.device(), &pool_info, nullptr, &imguiPool);

		// Init info (new-style)
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.ApiVersion = VK_API_VERSION_1_3;
		init_info.Instance = lveDevice.getInstance();
		init_info.PhysicalDevice = lveDevice.getPhysicalDevice();
		init_info.Device = lveDevice.device();
		init_info.QueueFamily = lveDevice.findPhysicalQueueFamilies().graphicsFamily;
		init_info.Queue = lveDevice.graphicsQueue();
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = LveSwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.ImageCount = LveSwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.UseDynamicRendering = false; // or true if using KHR_dynamic_rendering

		// Pipeline info (main viewport)
		init_info.PipelineInfoMain.RenderPass = lveRenderer.getSwapChainRenderPass();
		init_info.PipelineInfoMain.Subpass = 0;
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info); // NOTE: no separate renderpass parameter any more
	}
} // namespace lve
