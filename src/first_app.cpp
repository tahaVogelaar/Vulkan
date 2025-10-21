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
#include <algorithm>
#include <random>

namespace lve {
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

				// bindless textures
				.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
				.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1000)

				.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
							1)

				.build();

		loadGameObjects();
		buildPreDescriptor();
		buildDescriptors();
		build();
		initImGui();
	}

	FirstApp::~FirstApp()
	{
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

			VkCommandBuffer commandBuffer = startFrame();
			frameIndex = lveRenderer.getFrameIndex();

			updateShadow();
			update(commandBuffer);
			render(commandBuffer);
			renderImGui(commandBuffer);

			endFrame(commandBuffer);
		}
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
				lveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		drawBuffers = std::make_unique<LveBuffer>(
			lveDevice, sizeof(Object) * MAX_OBJECT_COUNT, 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		drawBuffers->map();

		pointLightBuffer = std::make_unique<LveBuffer>(
			lveDevice, sizeof(PointLight) * 1, 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		pointLightBuffer->map();

		// create Texture
		std::vector<VkDescriptorImageInfo> imageInfos;
		for (int i = 0; i < textures.size(); i++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = textures[i]->getSampler();
			imageInfo.imageView = textures[i]->getImageView();
			imageInfo.imageLayout = textures[i]->getImageLayout();
			imageInfos.emplace_back(imageInfo);
		}

		// setup layout
		globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
				.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // global ubo
							VK_SHADER_STAGE_ALL)
				.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // draw object ssbo
							VK_SHADER_STAGE_ALL)
				.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // lights
							VK_SHADER_STAGE_ALL)
				.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
							static_cast<uint32_t>(imageInfos.size())) // textures

				.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // shadowmap
							VK_SHADER_STAGE_FRAGMENT_BIT)
				.build();


		// setup descriptor set
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto drawBufferInfo = drawBuffers->descriptorInfo();
			auto pointLightBufferInfo = pointLightBuffer->descriptorInfo();

			LveDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeBuffer(1, &drawBufferInfo)
					.writeBuffer(2, &pointLightBufferInfo)
					.writeImages(3, imageInfos.data(), static_cast<uint32_t>(imageInfos.size()))
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

		pointShadowRenderer = std::make_unique<LvePointShadowRenderer>(lveDevice, shadowVert, shadowFrag, globalSetLayout->getDescriptorSetLayout(),
			IndirectDraw::getBindingDescriptionsShadow, IndirectDraw::getAttributeDescriptionsShadow);

		PointLightData pointLightData;
		pointLightData.shadowMap = std::make_unique<ShadowMap>(lveDevice, shadowExtent);

		pointLightData.light.position = ubo.camPos;
		pointLightData.light.rotation = ubo.forward;

		pointLightData.light.innerCone = .95f;
		pointLightData.light.outerCone = .9f;
		pointLightData.light.intensity = 1;
		pointLightData.light.specular = 1;

		pointLightData.update();

		pointLigts.emplace_back(std::move(pointLightData));
		pointShadowRenderer->createFramebuffers(pointLigts[0].shadowMap->getImageView(), shadowExtent, 1);
	}

	VkCommandBuffer FirstApp::startFrame()
	{
		VkCommandBuffer commandBuffer = lveRenderer.beginFrame();
		lveRenderer.beginSwapChainRenderPass(commandBuffer);
		return commandBuffer;
	}

	void FirstApp::endFrame(VkCommandBuffer &commandBuffer)
	{
		lveRenderer.endSwapChainRenderPass(commandBuffer);
		lveRenderer.endFrame();
	}


	void FirstApp::update(VkCommandBuffer &commandBuffer)
	{
		camera.update(lveWindow.getGLFWwindow(), static_cast<float>(deltaTime), ubo);
		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();

		pointLightBuffer->writeToBuffer(&pointLigts[0].light, 1 * sizeof(PointLight));
		pointLightBuffer->flush();

		// update title
		if (currentTime - lastUpdate1 > 0.5)
		{
			double fps = 1. / (deltaTime);
			std::ostringstream title;
			title << "FPS: " << std::fixed << std::setprecision(1) << fps;
			lveWindow.setName(std::to_string(fps));
			lastUpdate1 = currentTime;
		}
		indirectDraw.update(deltaTime, *drawBuffers);
	}

	void FirstApp::updateShadow()
	{
		VkCommandBuffer cmd = lveDevice.beginSingleTimeCommands();
		pointShadowRenderer->bindPipeline(cmd);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pointShadowRenderer->getPipelineLayout(),
			0,
			1,
			&globalDescriptorSets[frameIndex],
			0,
			nullptr
		);

		pointLigts[0].shadowMap->beginRender(cmd, pointShadowRenderer->getRenderPass(), pointShadowRenderer->getFramebuffer(shadowExtent));
		indirectDraw.render(cmd);
		pointLigts[0].shadowMap->endRender(cmd);
		lveDevice.endSingleTimeCommands(cmd);

		VkDescriptorImageInfo imageInfo;
		imageInfo.imageView = pointLigts[0].shadowMap->getImageView();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.sampler = pointLigts[0].shadowMap->getSampler();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstSet = globalDescriptorSets[frameIndex];
		write.dstBinding = 4;
		write.pImageInfo = &imageInfo;
		write.descriptorCount = 1;

		vkUpdateDescriptorSets(lveDevice.device(), 1, &write, 0, nullptr);

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

		indirectDraw.render(commandBuffer);
	}

	void FirstApp::renderImGui(VkCommandBuffer &commandBuffer)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// object list

		ImGui::Begin("Objects", nullptr,
			ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
		ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
		ImGui::SetWindowPos(ImVec2(0, 0));

		if (ImGui::Button("Create Object", ImVec2(WIDTH / 4. - 15, 20)))
			createObject();

		int count = 0;
		auto view = entities.view<Object, TransformComponent>();
		for (auto [entity, obj, transform] : view.each())
		{
			std::string objName = "Object " + std::to_string(count);
			if (ImGui::Button(objName.c_str(), ImVec2(WIDTH / 4.f - 15, 20)))
			{
				if (currentEntity == entity)
					enableEditor = !enableEditor;
				else
				{
					currentEntity = entity;
					enableEditor = true;
				}
			}
			count++;
		}

		ImGui::End();

		if (enableEditor && currentEntity != entt::null)
		{
			ImGui::Begin("Properties", nullptr,
				ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
			ImGui::SetWindowPos(ImVec2(WIDTH - WIDTH / 4.f, 0));

			auto &obj = entities.get<Object>(currentEntity);
			auto &transform = entities.get<TransformComponent>(currentEntity);

			// Example editable properties
			ImGui::Text("Material ID: %d", obj.materialId);
			ImGui::DragFloat3("Position", &transform.translation.x, 0.1f);
			ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.1f);
			ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);

			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	entt::entity FirstApp::createObject()
	{
		entt::entity e = entities.create();
		entities.emplace<Object>(e);
		entities.emplace<TransformComponent>(e);
		return e;
	}

	void FirstApp::loadGameObjects()
	{
		std::vector<std::string> files;
		files.emplace_back("/home/taha/CLionProjects/untitled4/models/smooth_vase.obj");
		files.emplace_back("/home/taha/CLionProjects/untitled4/models/cube.obj");
		indirectDraw.createMeshes(files);

		files.clear();
		files.emplace_back("/home/taha/Pictures/Screenshots/Screenshot from 2025-09-21 14-39-26.png");
		files.emplace_back("/home/taha/Pictures/why-do-people-never-talk-about-fc4-v0-6cimycz1faif1.jpg");

		for (auto const &f: files)
			textures.push_back(std::make_unique<VulkanTexture>(lveDevice, f));
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
