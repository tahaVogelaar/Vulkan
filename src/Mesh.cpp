#include "Mesh.h"

#include "lve_utils.hpp"

// libs
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_set>

RenderBucket::RenderBucket(lve::LveDevice &device, uint32_t MAX_DRAW, lve::LveBuffer& objectSSBO) :
	lveDevice(device), objectSSBO(objectSSBO), MAX_DRAW(MAX_DRAW)
{
	// create draw buffer
	uint32_t commandSize = sizeof(VkDrawIndexedIndirectCommand);
	VkDeviceSize bufferSize = commandSize * MAX_DRAW;

	drawCommandsBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		commandSize,
		MAX_DRAW,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
}

void RenderBucket::loadMeshes(std::vector<Builder>& builders)
{
	this->builder = builders;
	drawCommands.clear();
	vertices.clear();
	indices.clear();
	builder.clear();

	uint32_t indexCount = 0;
	OBJECT_TYPES = 0;

	for (auto i: builder[0].vertices) vertices.push_back(i);

	for (auto i: builder[0].indices) indices.push_back(i);
	OBJECT_TYPES++;
	indexCount++;

	createVertexBuffers(vertices);
	createIndexBuffers(indices);

	stagingBuffer = std::make_unique<lve::LveBuffer>(
	lveDevice,
	sizeof(VkDrawIndexedIndirectCommand) * OBJECT_TYPES,
	1,
	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
);
	stagingBuffer->map();
}

void RenderBucket::render(VkCommandBuffer commandBuffer)
{
	VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexedIndirect(
		commandBuffer,
		drawCommandsBuffer->getBuffer(),
		0,
		static_cast<uint32_t>(drawCommands.size()),
		sizeof(VkDrawIndexedIndirectCommand));
}

void RenderBucket::update(double deltaTime, lve::LveBuffer& objectSSBOA)
{
	// stuff
	drawCommands.clear();
	drawCommands.reserve(OBJECT_TYPES);
	sortedBucket.clear();
	sortedBucket.reserve(bucket.size());

	for (uint32_t i = 0; i < OBJECT_TYPES; i++)
		objectTypeIndex[i] = 0;

	for (uint32_t i = 0; i < bucket.size(); i++)
	{
		if (!deadList[i]) continue;
		objectTypeIndex[bucket[i].materialId]++;
		sortedBucket.push_back(bucket[i]);
	}

	std::sort(sortedBucket.begin(), sortedBucket.end(),
		[](const Object& a, const Object& b){ return a.materialId < b.materialId; });

	uint32_t firstIndex = 0;
	int32_t vertexOffset = 0;
	uint32_t runningBaseInstance = 0;
	for (int i = 0; i < OBJECT_TYPES; i++)
	{
		uint32_t instancesForThisMaterial = objectTypeIndex[i];

		VkDrawIndexedIndirectCommand cmd{
			static_cast<uint32_t>(builder[i].indices.size()),   // indexCount
			instancesForThisMaterial,                  // instanceCount
			firstIndex,                                // firstIndex
			vertexOffset,                              // vertexOffset
			runningBaseInstance                         // firstInstance
		};
		drawCommands.push_back(cmd);

		// offsets
		firstIndex += static_cast<uint32_t>(builder[i].indices.size());
		vertexOffset += static_cast<int32_t>(builder[i].vertices.size());
		runningBaseInstance += instancesForThisMaterial;
	}

	updateSSBO(objectSSBOA);
    createDrawCommand();
}

void RenderBucket::updateSSBO(lve::LveBuffer& objectSSBOA)
{

	if (bucket.empty()) return;
	if (objectSSBOA.getMappedMemory() == nullptr)
	{
		objectSSBOA.map();
	}

	objectSSBOA.writeToBuffer(sortedBucket.data(), VK_WHOLE_SIZE);
	objectSSBOA.flush(VK_WHOLE_SIZE);


	for (size_t i = 0; i < bucket.size(); i++) {
		//if (bucket[i].dirty)
		//{
		//objectSSBOA.writeToIndex(&bucket[i], i);
		//objectSSBOA.flush(alignedSize, i * alignedSize);
		//	bucket[i].dirty = false;
		//}
	}

}
 //
void RenderBucket::createDrawCommand()
{
	VkDeviceSize bufferSize = OBJECT_TYPES * sizeof(VkDrawIndexedIndirectCommand);

	stagingBuffer->writeToBuffer((void *) drawCommands.data());

	// Copy to persistent GPU buffer
	lveDevice.copyBuffer(stagingBuffer->getBuffer(), drawCommandsBuffer->getBuffer(), bufferSize);

}

void RenderBucket::deleteInstance(Handle h)
{
	if (h.index >= bucket.size()) return;
	generations[h.index]++; // invalidate handle
	freeList.push_back(h.index);
	deadList[h.index] = false;
}

Handle RenderBucket::addInstance( BucketSendData& item) {
	Object o{ .model = item.model, .materialId = item.materialId };
	CpuObject c{ .entity = item.entity, .parent = item.parent };

	uint32_t index = 0;
	if (!freeList.empty()) {
		index = freeList.back();
		freeList.pop_back();
		// reuse slot: overwrite both vectors
		bucket[index] = o;
		deadList[index] = true;
		cpuBucket[index] = c;
		// generation stays as-is (the handle returned must use current generation)
	} else {
		index = static_cast<uint32_t>(bucket.size());
		bucket.push_back(o);
		cpuBucket.push_back(c);
		generations.push_back(0); // new slot generation starts at 0
		deadList.push_back(true);
	}

	return Handle{ index, generations[index] };
}


void RenderBucket::ensureBufferCapacity(uint32_t requiredCommandCount)
{
	const VkDeviceSize commandSize = sizeof(VkDrawIndexedIndirectCommand);
	VkDeviceSize requiredBytes = static_cast<VkDeviceSize>(requiredCommandCount) * commandSize;
	VkDeviceSize currBytes = static_cast<VkDeviceSize>(MAX_DRAW) * commandSize; // MAX_DRAW stores number of commands capacity

	if (requiredBytes <= currBytes) return;

	// New capacity: double the command count or just enough to hold required
	uint32_t newCommandCapacity = static_cast<uint32_t>(std::max(requiredBytes / commandSize, currBytes / commandSize * 2));
	if (newCommandCapacity == 0) newCommandCapacity = requiredCommandCount;

	auto newBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		static_cast<uint32_t>(commandSize),
		newCommandCapacity,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// Copy old data (if any) — copy old byte size
	if (drawCommandsBuffer)
	{
		lveDevice.copyBuffer(
			drawCommandsBuffer->getBuffer(),
			newBuffer->getBuffer(),
			currBytes
		);
	}

	// swap and update capacity
	drawCommandsBuffer = std::move(newBuffer);
	MAX_DRAW = newCommandCapacity;
	std::cout << "MAX DRAW: " << MAX_DRAW << '\n';
}






void RenderBucket::createVertexBuffers(const std::vector<Vertex> &vertices)
{
	uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	uint32_t vertexSize = sizeof(vertices[0]);
	VkDeviceSize bufferSize = vertexSize * vertexCount;

	lve::LveBuffer stagingBuffer{
		lveDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void *) vertices.data());

	vertexBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void RenderBucket::createIndexBuffers(const std::vector<uint32_t> &indices)
{
	uint32_t indexCount = static_cast<uint32_t>(indices.size());

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
	stagingBuffer.writeToBuffer((void *) indices.data());

	indexBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}


glm::mat4 TransformComponent::mat4()
{
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	dirty = false;
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
		{translation.x, translation.y, translation.z, 1.0f}
	};
}

glm::mat3 TransformComponent::normalMatrix()
{
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

std::vector<VkVertexInputBindingDescription> RenderBucket::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> RenderBucket::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

	return attributeDescriptions;
}

std::vector<VkVertexInputBindingDescription> RenderBucket::getBindingDescriptionsShadow()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> RenderBucket::getAttributeDescriptionsShadow()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});

	return attributeDescriptions;
}
