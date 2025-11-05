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
#include "coreRenderer.h"
#include "fullScreenQuat.h"
#include "ObjectLoader.h"


namespace lve {
	class FirstApp {
	public:
		int WIDTH = 800;
		int HEIGHT = 600;
		uint32_t MAX_OBJECT_COUNT = 512;
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

		LveWindow lveWindow{WIDTH, HEIGHT, "Vulkan Tutorial"};
		LveDevice lveDevice{lveWindow};
		LveRenderer lveRenderer{lveWindow, lveDevice};
		std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
		std::unique_ptr<LveDescriptorSetLayout> globalSetLayout;
		std::string simpleVert = "/home/taha/CLionProjects/untitled4/shaders/shader.vert", simpleFrag = "/home/taha/CLionProjects/untitled4/shaders/shader.frag";

		// stuff
		Camera camera;
		GlobalUbo ubo;
		std::unique_ptr<LveBuffer> drawSSBO; // ssbo
		RenderBucket renderBucket{lveDevice, MAX_OBJECT_COUNT, *drawSSBO};
		std::unique_ptr<RenderSyncSystem> renderSyncSystem;

		// light
		std::unique_ptr<LveBuffer> pointLightBuffer;
		VkExtent2D shadowExtent{2024, 2024};

		// a
		std::unique_ptr<FullScreenQuat> fullScreenQuat;
		//std::string skyVert = "/home/taha/CLionProjects/untitled4/shaders/sky.vert", skyFrag = "/home/taha/CLionProjects/untitled4/shaders/sky.frag";

		// objects
		uint32_t objectCount = 0;
		LoaderObject objectLoader;

		// textures
		std::vector<std::unique_ptr<Material> > textures;

		// note: order of declarations matters
		std::unique_ptr<LveDescriptorPool> globalPool{}, ssobPool{};
		std::vector<VkDescriptorSet> globalDescriptorSets{LveSwapChain::MAX_FRAMES_IN_FLIGHT};
		std::vector<std::unique_ptr<LveBuffer> > uboBuffers{LveSwapChain::MAX_FRAMES_IN_FLIGHT};

		double lastTime = 0, currentTime = 0, lastUpdate1 = 0, deltaTime = 0;
		float aspect;
		int frameIndex = 0;
	};
} // namespace lve
