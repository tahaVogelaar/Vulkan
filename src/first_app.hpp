#pragma once

#include "lve_shadowMap.h"
#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_renderer.hpp"
#include "lve_window.hpp"
#include "Mesh.h"
#include "keyboard_movement_controller.hpp"
#include "lve_frame_info.hpp"
#include "Texture.h"
#include "entt.hpp"

// std
#include <memory>
#include <vector>

#include "simple_render_system.hpp"
#include "lve_shadowMap.h"
#include "lve_shadow_renderer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imconfig.h"
#include "imgui_internal.h"
#include "imstb_rectpack.h"
#include "imstb_textedit.h"
#include "imstb_truetype.h"

namespace lve {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		uint32_t MAX_OBJECT_COUNT = 32;
		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp &) = delete;
		FirstApp &operator=(const FirstApp &) = delete;

		void run();

	private:
		// loop
		void update(VkCommandBuffer &commandBuffer);
		void updateShadow();
		VkCommandBuffer startFrame();
		void render(VkCommandBuffer &commandBuffer);
		void renderImGui(VkCommandBuffer &commandBuffer);
		void endFrame(VkCommandBuffer &commandBuffer);

		// buid process
		void buildPreDescriptor();
		void buildDescriptors();
		void build();
		void initImGui();
		void loadGameObjects();

		// run time functions
		entt::entity createObject();

		LveWindow lveWindow{WIDTH, HEIGHT, "Vulkan Tutorial"};
		LveDevice lveDevice{lveWindow};
		LveRenderer lveRenderer{lveWindow, lveDevice};
		std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
		std::unique_ptr<LveDescriptorSetLayout> globalSetLayout;
		std::string simpleVert = "/home/taha/CLionProjects/untitled4/shaders/shader.vert", simpleFrag = "/home/taha/CLionProjects/untitled4/shaders/shader.frag",
		shadowVert = "/home/taha/CLionProjects/untitled4/shaders/shadow.vert", shadowFrag = "/home/taha/CLionProjects/untitled4/shaders/shadow.frag";



		// stuff
		Camera camera;
		GlobalUbo ubo;
		IndirectDraw indirectDraw{lveDevice, entities, MAX_OBJECT_COUNT};

		// objects
		std::vector<PointLightData> pointLigts;
		entt::registry entities;
		uint32_t objectCount = 0;
		std::unique_ptr<LvePointShadowRenderer> pointShadowRenderer;
		VkExtent2D shadowExtent{2024, 2024};

		// textures
		std::vector<std::unique_ptr<VulkanTexture> > textures;

		// imgui editor
		bool enableEditor = false;
		entt::entity currentEntity = entt::null, pastCurrentEntity = entt::null;

		// note: order of declarations matters
		std::unique_ptr<LveDescriptorPool> globalPool{}, ssobPool{};
		std::vector<VkDescriptorSet> globalDescriptorSets{LveSwapChain::MAX_FRAMES_IN_FLIGHT};
		std::vector<std::unique_ptr<LveBuffer> > uboBuffers{LveSwapChain::MAX_FRAMES_IN_FLIGHT};
		std::unique_ptr<LveBuffer> drawBuffers;
		std::unique_ptr<LveBuffer> pointLightBuffer;

		double lastTime = 0, currentTime = 0, lastUpdate1 = 0, deltaTime = 0;
		float aspect;
		int frameIndex = 0;
	};
} // namespace lve
