#pragma once

#include "keyboard_movement_controller.hpp"

// lib
#include <vulkan/vulkan.h>

namespace lve {
struct FrameInfo {
  int frameIndex;
  float frameTime;
  VkCommandBuffer commandBuffer;
  Camera &camera;
  VkDescriptorSet globalDescriptorSet;
};
}  // namespace lve