#include "Mesh.h"

#include "lve_utils.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

IndirectDraw::IndirectDraw(lve::LveDevice &device, entt::registry &entities, uint32_t MAX_DRAW) : lveDevice(device),
	entities(entities), MAX_DRAW(MAX_DRAW)
{
	// create draw buffer
	uint32_t commandSize = sizeof(VkDrawIndexedIndirectCommand);
	VkDeviceSize bufferSize = commandSize * MAX_DRAW;

	drawCommandsBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		commandSize,
		MAX_DRAW,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
}

void IndirectDraw::createMeshes(const std::vector<std::string> &files)
{
	drawCommands.clear();
	vertices.clear();
	indices.clear();
	builder.clear();

	uint32_t indicesCount = 0;

	for (std::string const file: files)
	{
		Builder b;
		b.loadModel(file);
		b.id = indexCount;

		for (auto i: b.vertices)
			vertices.push_back(i);

		for (auto i: b.indices)
			indices.push_back(i);

		builder.push_back(b);
		indexCount++;
	}

	createVertexBuffers(vertices);
	createIndexBuffers(indices);
}

void IndirectDraw::render(VkCommandBuffer commandBuffer)
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

void IndirectDraw::update(double deltaTime, lve::LveBuffer& drawBuffer)
{
	drawCommands.clear();

	std::map<uint32_t, uint32_t> indexMap;
	int count = 0;
	auto view = entities.view<Object, TransformComponent>();
	for (auto [entity, obj, transform]: view.each())
	{
		indexMap[obj.materialId]++;
		count++;
	}

	// sraw command vector

	uint32_t firstIndex = 0;
	int32_t vertexOffset = 0;
	drawCommands.reserve(indexMap.size());
	for (auto& b : builder)
	{
		VkDrawIndexedIndirectCommand cmd{};
		cmd.indexCount = static_cast<uint32_t>(b.indices.size());
		cmd.instanceCount = b.id;
		cmd.firstIndex = firstIndex;
		cmd.vertexOffset = vertexOffset;
		cmd.firstInstance = 0;

		drawCommands.push_back(cmd);

		// advance offsets
		firstIndex   += static_cast<uint32_t>(b.indices.size());
		vertexOffset += static_cast<int32_t>(b.vertices.size());
	}

	// ssbo
	std::vector<Object> instances;
	instances.reserve(view.size_hint());

	for (auto [entity, obj, transform] : view.each()) {
		instances.push_back({
			transform.mat4(),
			obj.materialId
		});
	}

	// sort ssbo
	std::sort(instances.begin(), instances.end(),
	[](const Object& a, const Object& b) {
		return a.materialId < b.materialId;
	});

	drawBuffer.writeToBuffer(instances.data(), sizeof(Object) * instances.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	createDrawCommand();
}

void IndirectDraw::createDrawCommand()
{
	uint32_t commandCount = drawCommands.size();
	VkDeviceSize bufferSize = drawCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
	uint32_t commandSize = sizeof(VkDrawIndexedIndirectCommand);

	lve::LveBuffer stagingBuffer{
		lveDevice,
		sizeof(VkDrawIndexedIndirectCommand),
		static_cast<uint32_t>(drawCommands.size()),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void *) drawCommands.data());

	// Copy to persistent GPU buffer
	lveDevice.copyBuffer(stagingBuffer.getBuffer(), drawCommandsBuffer->getBuffer(), bufferSize);
}

void IndirectDraw::ensureBufferCapacity(uint32_t requiredCommandCount)
{
	size_t requiredSize = requiredCommandCount * sizeof(VkDrawIndexedIndirectCommand);
	size_t currBufferSize = MAX_DRAW;

	// If current buffer is big enough, do nothing
	if (requiredSize <= currBufferSize)
		return;

	// Decide new size (e.g. double it to avoid frequent reallocations)
	size_t newSize = std::max(requiredSize, currBufferSize * 2);

	// 1. Create new buffer
	auto newBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		sizeof(VkDrawIndexedIndirectCommand),
		static_cast<uint32_t>(newSize / sizeof(VkDrawIndexedIndirectCommand)),
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// 2. Copy old data (if any)
	if (drawCommandsBuffer)
	{
		lveDevice.copyBuffer(
			drawCommandsBuffer->getBuffer(),
			newBuffer->getBuffer(),
			currBufferSize
		);
	}

	// 3. Replace the old buffer
	drawCommandsBuffer = std::move(newBuffer);
	currBufferSize = newSize;
}


void IndirectDraw::createVertexBuffers(const std::vector<Vertex> &vertices)
{
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
	stagingBuffer.writeToBuffer((void *) vertices.data());

	vertexBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void IndirectDraw::createIndexBuffers(const std::vector<uint32_t> &indices)
{
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
	stagingBuffer.writeToBuffer((void *) indices.data());

	indexBuffer = std::make_unique<lve::LveBuffer>(
		lveDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}


namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(Vertex const &vertex) const
		{
			size_t seed = 0;
			lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}; // namespace std


void Builder::loadModel(const std::string &filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto &shape: shapes)
	{
		for (const auto &index: shape.mesh.indices)
		{
			Vertex vertex{};

			if (index.vertex_index >= 0)
			{
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

			if (index.normal_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

glm::mat4 TransformComponent::mat4()
{
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

std::vector<VkVertexInputBindingDescription> IndirectDraw::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> IndirectDraw::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

	return attributeDescriptions;
}

std::vector<VkVertexInputBindingDescription> IndirectDraw::getBindingDescriptionsShadow()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> IndirectDraw::getAttributeDescriptionsShadow()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});

	return attributeDescriptions;
}
