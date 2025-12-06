#pragma once

#include "core/lve_buffer.hpp"
#include "core/lve_device.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include "entt.hpp"
#include "engineStuff/ObjectLoader.h"


// stuff you send to the gpu
struct InstanceData {
	glm::mat4 model;
	uint32_t primId;
	uint32_t materialID;
	uint32_t _pad[2];
};

// i dont wanna explain this
struct CpuObject {
	entt::entity entity;
	entt::entity parent;
};

// send this to RenderBucket
struct BucketSendData {
	glm::mat4 model;
	int32_t primitiveID;
	int32_t materialID;
	entt::entity entity;
	entt::entity parent;
};

	struct Handle {
		int32_t index = -1;
		int32_t generation = -1;

		bool operator==(const Handle &v) const {
			return (index == v.index && generation == v.generation);
		}
	};

class RenderBucket {
public:
	RenderBucket(lve::LveDevice &device, uint32_t MAX_DRAW, lve::LveBuffer& objectSSBO);
	RenderBucket(const RenderBucket &) = delete;
	RenderBucket &operator=(const RenderBucket &) = delete;

	void loadMeshes(std::vector<Primitive>& builders);
	Handle addInstance(BucketSendData &item);
	void deleteInstance(Handle h);
	void updateSSBO(lve::LveBuffer& objectSSBOA);
	InstanceData* get(const Handle& h) {
		if (h.index >= bucket.size()) return nullptr;
		if (generations[h.index] != h.generation) return nullptr; // stale handle
		return &bucket[h.index];
	}

	void update(double deltaTime, lve::LveBuffer& objectSSBO);
	void render(VkCommandBuffer commandBuffer);
	std::unique_ptr<lve::LveBuffer> stagingBuffer;

	TextureHandler& getTextureHandler() {return textureHandler; }
	LoaderObject& getObjectLoader() { return objectLoader; }

private:
	lve::LveDevice &lveDevice;
	lve::LveBuffer& objectSSBO;

	TextureHandler textureHandler{lveDevice};
	LoaderObject objectLoader{textureHandler};

	std::vector<Primitive> builder;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::unordered_map<uint32_t, uint32_t> objectTypeIndex; // this is the batch it creates every
	std::vector<InstanceData> bucket; // holds drawable objects (unsorted)
	std::vector<InstanceData> sortedBucket; // sorts bucket every frame
	std::vector<CpuObject> cpuBucket;

	std::vector<bool> deadList;
	std::vector<int32_t> generations; // stores how many times that slot has been reused
	std::vector<int32_t> freeList; // holds indices of deleted slots that can be reused

	uint32_t MAX_DRAW;
	uint32_t OBJECT_TYPES;

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

public:
	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptionsShadow();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionsShadow();
};
