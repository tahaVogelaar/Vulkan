#include "ObjectLoader.h"

#include <algorithm>
#include <array>
#include <array>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <cassert>
#include <cstring>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

void LoaderObject::loadScene(const std::filesystem::path& filePath) {
	namespace fg = fastgltf;
	// 1. Read the file into memory
	auto data = fg::GltfDataBuffer::FromPath(filePath);

	// 2. Create the parser
	fg::Parser parser;

	// 3. Parse depending on file type
	std::unique_ptr<fg::Expected<fg::Asset>> assetResult;
	if (fg::determineGltfFileType(data.get()) == fg::GltfType::GLB) {
		assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltfBinary(
			data.get(),
			filePath.parent_path(),
			fg::Options::LoadExternalBuffers,
			fg::Category::All
		));
	} else {
		assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltf(
			data.get(),
			filePath.parent_path(),
			fg::Options::LoadExternalBuffers,
			fg::Category::All
		));
	}

	if (!assetResult) {
		std::cerr << "Error loading glTF: " << fg::to_underlying(assetResult->error()) << "\n";
		return;
	}

	fg::Asset& asset = assetResult->get();

	// 4. Get the default scene
	if (asset.scenes.empty()) {
		std::cerr << "No scenes found!\n";
		return;
	}

	const fg::Scene& scene = asset.scenes[asset.defaultScene.value_or(0)];

	// 5. Traverse all root nodes
	for (auto nodeIndex : scene.nodeIndices) {
		const fg::Node& node = asset.nodes[nodeIndex];
		processNode(asset, node);
	}
}

const std::byte* getBufferData(const fastgltf::Asset& asset, const fastgltf::BufferView& view) {
	const fastgltf::Buffer& buffer = asset.buffers[view.bufferIndex];

	if (std::holds_alternative<fastgltf::sources::Vector>(buffer.data)) {
		return reinterpret_cast<const std::byte*>(
			std::get<fastgltf::sources::Vector>(buffer.data).bytes.data()
		) + view.byteOffset;
	}
	if (std::holds_alternative<fastgltf::sources::Array>(buffer.data)) {
		return reinterpret_cast<const std::byte*>(
			std::get<fastgltf::sources::Array>(buffer.data).bytes.data()
		) + view.byteOffset;
	}

	throw std::runtime_error("Unsupported buffer source type.");
}

void LoaderObject::processNode(fastgltf::Asset& asset, const fastgltf::Node& node) {
	if (node.meshIndex.has_value()) {
		const fastgltf::Mesh& mesh = asset.meshes[node.meshIndex.value()];
		processMesh(asset, mesh);
	}

	// Traverse child nodes
	for (auto childIdx : node.children) {
		processNode(asset, asset.nodes[childIdx]);
	}
}


void LoaderObject::processMesh(fastgltf::Asset &asset, const fastgltf::Mesh &mesh)
{
	uint32_t ID = 0;
	Builder builder;
	for (const auto& prim : mesh.primitives)
	{
		Vertex v{};

		// --- Positions ---
		if (auto it = prim.findAttribute("POSITION"); it != prim.attributes.end()) {
			const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
			const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;

			const size_t stride = view.byteStride.value_or(
				fastgltf::getElementByteSize(accessor.type, accessor.componentType)
			);

			for (size_t i = 0; i < accessor.count; ++i) {
				const float* f = reinterpret_cast<const float*>(base + i * stride);
				Vertex vert{};
				vert.position = glm::vec3(f[0], f[1], f[2]);
				builder.vertices.push_back(vert);
			}
		}

		// --- Normals ---
		if (auto it = prim.findAttribute("NORMAL"); it != prim.attributes.end()) {
			const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
			const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;

			const size_t stride = view.byteStride.value_or(
				fastgltf::getElementByteSize(accessor.type, accessor.componentType)
			);

			for (size_t i = 0; i < accessor.count; ++i) {
				const float* f = reinterpret_cast<const float*>(base + i * stride);
				builder.vertices[i].normal = glm::vec3(f[0], f[1], f[2]);
			}
		}

		// --- UVs ---
		if (auto it = prim.findAttribute("TEXCOORD_0"); it != prim.attributes.end()) {
			const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
			const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;

			const size_t stride = view.byteStride.value_or(
				fastgltf::getElementByteSize(accessor.type, accessor.componentType)
			);

			for (size_t i = 0; i < accessor.count; ++i) {
				const float* f = reinterpret_cast<const float*>(base + i * stride);
				builder.vertices[i].uv = glm::vec2(f[0], f[1]);
			}
		}

		// --- Colors (optional) ---
		if (auto it = prim.findAttribute("COLOR_0"); it != prim.attributes.end()) {
			const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
			const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;

			const size_t stride = view.byteStride.value_or(
				fastgltf::getElementByteSize(accessor.type, accessor.componentType)
			);

			for (size_t i = 0; i < accessor.count; ++i) {
				const float* f = reinterpret_cast<const float*>(base + i * stride);
				builder.vertices[i].color = glm::vec4(f[0], f[1], f[2], f[3]);
			}
		}

		// --- Indices ---
		if (prim.indicesAccessor.has_value()) {
			const fastgltf::Accessor& accessor = asset.accessors[prim.indicesAccessor.value()];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
			const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;

			for (size_t i = 0; i < accessor.count; ++i) {
				uint32_t index = 0;
				switch (accessor.componentType) {
					case fastgltf::ComponentType::UnsignedByte:
						index = *(reinterpret_cast<const uint8_t*>(base) + i);
						break;
					case fastgltf::ComponentType::UnsignedShort:
						index = *(reinterpret_cast<const uint16_t*>(base) + i * sizeof(uint16_t));
						break;
					case fastgltf::ComponentType::UnsignedInt:
						index = *(reinterpret_cast<const uint32_t*>(base) + i * sizeof(uint32_t));
						break;
					default:
						throw std::runtime_error("Unsupported index type");
				}
				builder.indices.push_back(index);
			}
		}
	}

	builder.ID = ID++;
	if (!builders.empty())
	{
		builder.vertexOffset = builders[builders.size() - 1].vertices.size() + builders[builders.size() - 1].vertexOffset;
		builder.indexOffset = builders[builders.size() - 1].indices.size() + builders[builders.size() - 1].indexOffset;
	}
	else
	{
		builder.vertexOffset = 0;
		builder.indexOffset = 0;
	}
	builders.push_back(builder);
}
