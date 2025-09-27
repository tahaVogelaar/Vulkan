#include "Mesh.h"

#include "lve_utils.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cassert>
#include <cstring>
#include <unordered_map>


void IndirectDraw::createDrawBuffers(const std::vector<std::string>& files) {
  builder.resize(files.size());
  int verticesCount = 0;
  uint32_t indicesCount = 0;

  for (const auto& file : files) {
    Builder b;
    b.loadModel(file);
    builder.push_back(b);

    for (auto i : b.vertices)
      vertices.push_back(i);

    for (auto i : b.indices)
      indices.push_back(i);

    drawCommands.push_back(VkDrawIndexedIndirectCommand{
      static_cast<uint32_t>(b.indices.size()),
      1,
      indicesCount,
      verticesCount,
      0});

    verticesCount += static_cast<int>(b.vertices.size());
    indicesCount += b.indices.size();
  }

  createVertexBuffers(vertices);
  createIndexBuffers(indices);


}


void IndirectDraw::render(VkCommandBuffer commandBuffer) {
  vkCmdDrawIndexedIndirect(commandBuffer, drawCommandsBuffer->getBuffer(), 0, static_cast<uint32_t>(drawCommands.size()), sizeof(VkDrawIndexedIndirectCommand));
}

void IndirectDraw::createDrawCommand() {
  uint32_t commandCount = drawCommands.size();
  VkDeviceSize bufferSize = drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
  uint32_t commandSize = sizeof(VkDrawIndexedIndirectCommand);

  lve::LveBuffer stagingBuffer{
  lveDevice,
    commandSize,
    commandCount,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)drawCommands.data());

  drawCommandsBuffer = std::make_unique<lve::LveBuffer>(
    lveDevice,
    commandSize,
    commandCount,
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  lveDevice.copyBuffer(stagingBuffer.getBuffer(), drawCommandsBuffer->getBuffer(), bufferSize);
}

void IndirectDraw::createVertexBuffers(const std::vector<Vertex> &vertices) {
  vertexCount = static_cast<uint32_t>(vertices.size());
  assert(vertexCount >= 3 && "Vertex count must be at least 3");
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
  uint32_t vertexSize = sizeof(vertices[0]);

  lve::LveBuffer stagingBuffer{
    lveDevice,
    vertexSize,
    vertexCount,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffer = std::make_unique<lve::LveBuffer>(
      lveDevice,
      vertexSize,
      vertexCount,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void IndirectDraw::createIndexBuffers(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());

  VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
  uint32_t indexSize = sizeof(indices[0]);

  lve::LveBuffer stagingBuffer{
    lveDevice,
    indexSize,
    indexCount,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffer = std::make_unique<lve::LveBuffer>(
      lveDevice,
      indexSize,
      indexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}




















namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const &vertex) const {
    size_t seed = 0;
    lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
    return seed;
  }
};
};  // namespace std



void Builder::loadModel(const std::string &filepath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  vertices.clear();
  indices.clear();

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      if (index.vertex_index >= 0) {
        vertex.position = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2],
      };

        vertex.color = {
          attrib.colors[3 * index.vertex_index + 0],
          attrib.colors[3 * index.vertex_index + 1],
          attrib.colors[3 * index.vertex_index + 2],
      };
      }

      if (index.normal_index >= 0) {
        vertex.normal = {
          attrib.normals[3 * index.normal_index + 0],
          attrib.normals[3 * index.normal_index + 1],
          attrib.normals[3 * index.normal_index + 2],
      };
      }

      if (index.texcoord_index >= 0) {
        vertex.uv = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          attrib.texcoords[2 * index.texcoord_index + 1],
      };
      }

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
}

glm::mat4 TransformComponent::mat4() {
  const float c3 = glm::cos(rotation.z);
  const float s3 = glm::sin(rotation.z);
  const float c2 = glm::cos(rotation.x);
  const float s2 = glm::sin(rotation.x);
  const float c1 = glm::cos(rotation.y);
  const float s1 = glm::sin(rotation.y);
  return glm::mat4{
          {
            scale.x * (c1 * c3 + s1 * s2 * s3),
            scale.x * (c2 * s3),
            scale.x * (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
          scale.y * (c3 * s1 * s2 - c1 * s3),
          scale.y * (c2 * c3),
          scale.y * (c1 * c3 * s2 + s1 * s3),
          0.0f,
      },
      {
        scale.z * (c2 * s1),
        scale.z * (-s2),
        scale.z * (c1 * c2),
        0.0f,
    },
    {translation.x, translation.y, translation.z, 1.0f}};
}

glm::mat3 TransformComponent::normalMatrix() {
  const float c3 = glm::cos(rotation.z);
  const float s3 = glm::sin(rotation.z);
  const float c2 = glm::cos(rotation.x);
  const float s2 = glm::sin(rotation.x);
  const float c1 = glm::cos(rotation.y);
  const float s1 = glm::sin(rotation.y);
  const glm::vec3 invScale = 1.0f / scale;

  return glm::mat3{
          {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1),
        },
        {
          invScale.y * (c3 * s1 * s2 - c1 * s3),
          invScale.y * (c2 * c3),
          invScale.y * (c1 * c3 * s2 + s1 * s3),
      },
      {
        invScale.z * (c2 * s1),
        invScale.z * (-s2),
        invScale.z * (c1 * c2),
    },
  };
}