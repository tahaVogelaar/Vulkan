#pragma once

#include "lve_buffer.hpp"
#include "lve_device.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include "entt.hpp"
#include "ObjectLoader.h"

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 rotation{};
	glm::vec3 scale{1.f, 1.f, 1.f};
	glm::mat4 worldMatrix{1.0f};
	bool dirty = true;

	// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
	// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
	// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	glm::mat4 mat4();

	glm::mat3 normalMatrix();
};

struct Object {
	glm::mat4 model;
	uint32_t materialId;
	uint32_t _pad[3];
};

// i dont wanna explain this
struct CpuObject {
	entt::entity entity;
	entt::entity parent;
};

// send this to RenderBucket
struct BucketSendData {
	glm::mat4 model;
	uint32_t materialId;
	entt::entity entity;
	entt::entity parent;
};

struct Handle {
	uint32_t index;
	uint32_t generation;
};

class RenderBucket {
public:
	RenderBucket(lve::LveDevice &device, uint32_t MAX_DRAW, lve::LveBuffer& objectSSBO);
	void loadMeshes(std::vector<Builder>& builders);

	Handle addInstance(BucketSendData &item);
	void deleteInstance(Handle h);
	void updateSSBO(lve::LveBuffer& objectSSBOA);
	Object* get(const Handle& h) {
		if (h.index >= bucket.size()) return nullptr;
		if (generations[h.index] != h.generation) return nullptr; // stale handle
		return &bucket[h.index];
	}

	void update(double deltaTime, lve::LveBuffer& objectSSBO);
	void render(VkCommandBuffer commandBuffer);
	std::unique_ptr<lve::LveBuffer> stagingBuffer;

private:
	std::vector<Builder> builder;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::map<uint32_t, uint32_t> objectTypeIndex; // this is the batch it creates every
	std::vector<Object> bucket; // holds drawable objects (unsorted)
	std::vector<Object> sortedBucket; // sorts bucket every frame
	std::vector<CpuObject> cpuBucket;

	std::vector<bool> deadList;
	std::vector<uint32_t> generations; // stores how many times that slot has been reused
	std::vector<uint32_t> freeList; // holds indices of deleted slots that can be reused

	uint32_t MAX_DRAW;
	uint32_t OBJECT_TYPES = 2;

	std::vector<Vertex> totalVertex;
	std::vector<uint32_t> totalIndex;

	std::unique_ptr<lve::LveBuffer> vertexBuffer;
	std::unique_ptr<lve::LveBuffer> indexBuffer;
	std::vector<VkDrawIndexedIndirectCommand> drawCommands;
	std::unique_ptr<lve::LveBuffer> drawCommandsBuffer;

	void createVertexBuffers(const std::vector<Vertex> &vertices);
	void createIndexBuffers(const std::vector<uint32_t> &indices);
	void ensureBufferCapacity(uint32_t requiredCommandCount);
	void createDrawCommand();

	lve::LveDevice &lveDevice;
	lve::LveBuffer& objectSSBO;

public:
	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptionsShadow();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionsShadow();
};
