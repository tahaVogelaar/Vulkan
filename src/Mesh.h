#pragma once

#include "lve_buffer.hpp"
#include "lve_device.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

struct Vertex {
  glm::vec3 position{};
  glm::vec3 color{};
  glm::vec3 normal{};
  glm::vec2 uv{};

  static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

  bool operator==(const Vertex &other) const {
    return position == other.position && color == other.color && normal == other.normal &&
           uv == other.uv;
  }
};

struct Builder {
  std::vector<Vertex> vertices{};
  std::vector<uint32_t> indices{};

  void loadModel(const std::string &filepath);
};

struct TransformComponent {
  glm::vec3 translation{};
  glm::vec3 scale{1.f, 1.f, 1.f};
  glm::vec3 rotation{};

  // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
  // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
  // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
  glm::mat4 mat4();

  glm::mat3 normalMatrix();
};

struct Object {
  TransformComponent transform{};
  int id = 0;
};

class IndirectDraw {
  public:
  IndirectDraw() = default;
  IndirectDraw(lve::LveDevice &device, const std::vector<std::string>& files);

  void render(VkCommandBuffer commandBuffer);

private:
  std::vector<Builder> builder;
  std::vector<Object> objects;

  std::unique_ptr<lve::LveBuffer> vertexBuffer;
  uint32_t vertexCount;
  std::unique_ptr<lve::LveBuffer> indexBuffer;
  uint32_t indexCount;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<VkDrawIndexedIndirectCommand> drawCommands;
  std::unique_ptr<lve::LveBuffer> drawCommandsBuffer;

  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  void createDrawCommand();

  lve::LveDevice &lveDevice;
};