#pragma once

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_renderer.hpp"
#include "lve_window.hpp"
#include "Mesh.h"
#include "keyboard_movement_controller.hpp"
#include "lve_frame_info.hpp"

// std
#include <memory>
#include <vector>

namespace lve {

class FirstApp {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  FirstApp();
  ~FirstApp();

  FirstApp(const FirstApp &) = delete;
  FirstApp &operator=(const FirstApp &) = delete;

  void build();
  void update(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo);
  void render(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo);
  void run();

private:
  void loadGameObjects();

  LveWindow lveWindow{WIDTH, HEIGHT, "Vulkan Tutorial"};
  LveDevice lveDevice{lveWindow};
  LveRenderer lveRenderer{lveWindow, lveDevice};
  Camera camera;
  GlobalUbo ubo;

  IndirectDraw indirectDraw{lveDevice};

  // note: order of declarations matters
  std::unique_ptr<LveDescriptorPool> globalPool{}, ssobPool{};
  std::vector<VkDescriptorSet> globalDescriptorSets{LveSwapChain::MAX_FRAMES_IN_FLIGHT};
  std::vector<std::unique_ptr<LveBuffer>> uboBuffers{LveSwapChain::MAX_FRAMES_IN_FLIGHT};
  std::vector<std::unique_ptr<LveBuffer> > drawBuffers{LveSwapChain::MAX_FRAMES_IN_FLIGHT};

  double lastTime = 0, currTime = 0, lastUpdate1 = 0, deltaTime = 0;
  float aspect;
  int frameIndex = 0;
};
} // namespace lve