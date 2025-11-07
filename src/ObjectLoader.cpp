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



	//Structure ss;
	//structures.push_back(ss);
	//uint32_t index = structures.size() - 1;
	//structures[index].index = index;
	//structures[index].parent = -1;
	//structures[index].transform = TransformComponent{};
	//structures[index].ID = -1;
	//structures[index].name = "skibidi";

	const fg::Scene& scene = asset.scenes[asset.defaultScene.value_or(0)];
	// Traverse all root nodes
	for (auto nodeIndex : scene.nodeIndices) {
		const fg::Node& node = asset.nodes[nodeIndex];
		int32_t childIndex = processNode(asset, node, -1);
		//structures[index].childeren.push_back(childIndex);
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

int32_t LoaderObject::processNode(fastgltf::Asset& asset, const fastgltf::Node& node, int32_t parentStructure) {
	glm::mat4 localTransform{1.0f};
	Structure s;
	structures.push_back(s);
	uint32_t index = structures.size() - 1;
	structures[index].index = index;
	structures[index].parent = parentStructure;

	switch (node.transform.index()) {
		case 1: { // Matrix
			const auto& mat = std::get<fastgltf::math::fmat4x4>(node.transform);
			localTransform = glm::make_mat4(mat.data());
			break;
		}
		case 2: { // TRS
			const auto& trs = std::get<fastgltf::TRS>(node.transform);

			glm::vec3 translation = glm::make_vec3(trs.translation.data());
			glm::quat rotation = glm::make_quat(trs.rotation.data()); // w,x,y,z order
			glm::vec3 scale = glm::make_vec3(trs.scale.data());

			localTransform =
				glm::translate(glm::mat4(1.0f), translation) *
				glm::mat4_cast(rotation) *
				glm::scale(glm::mat4(1.0f), scale);
			break;
		}
		case 0:
		default:
			// Identity
			localTransform = glm::mat4(1.0f);
			break;
	}

	if (node.meshIndex.has_value()) {
		const fastgltf::Mesh& mesh = asset.meshes[node.meshIndex.value()];
		structures[index].ID = processMesh(asset, mesh);
	}

	// Traverse child nodes
	for (auto childIdx : node.children) {
		uint32_t childIndex = processNode(asset, asset.nodes[childIdx], index);
		structures[index].childeren.push_back(childIndex);
	}


	structures[index].transform.translation = mat4ToPosition(localTransform);
	structures[index].transform.rotation = mat4ToRotation(localTransform);
	structures[index].transform.scale = mat4ToScale(localTransform);
	structures[index].name = node.name;
	return index;
}


int32_t LoaderObject::processMesh(fastgltf::Asset &asset, const fastgltf::Mesh &mesh)
{
	uint32_t ID = 0;
	Builder builder;
	for (const auto& prim : mesh.primitives)
	{
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
				Vertex vert;
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
			if (accessor.count > 0)
				for (size_t i = 0; i < accessor.count; ++i) {
					const float* f = reinterpret_cast<const float*>(base + i * stride);
					builder.vertices[i].color = glm::vec4(f[0], f[1], f[2], f[3]);
				}
		}

		// --- Indices ---
		if (prim.indicesAccessor.has_value()) {
			auto& accessor = asset.accessors[prim.indicesAccessor.value()];
			const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];

			const std::byte* bufferData = getBufferData(asset, view);
			const std::byte* base = bufferData + view.byteOffset + accessor.byteOffset;

			builder.indices.resize(accessor.count);
			uint32_t idx = 0;
			fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](std::uint32_t index) {
				builder.indices[idx++] = index;
			});
		}
	}


	builder.ID = builders.size();
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
	return builder.ID;
}





glm::vec3 LoaderObject::mat4ToPosition(const glm::mat4& matrix)
{
	return glm::vec3(matrix[3]);
}

glm::quat LoaderObject::mat4ToQuaternion(const glm::mat4& matrix)
{
	glm::vec3 scale = mat4ToScale(matrix);
	glm::mat3 rotMat;
	rotMat[0] = glm::vec3(matrix[0]) / scale.x;
	rotMat[1] = glm::vec3(matrix[1]) / scale.y;
	rotMat[2] = glm::vec3(matrix[2]) / scale.z;

	return glm::quat_cast(rotMat);
}

glm::quat LoaderObject::mat4ToQuaternion(const glm::mat4& matrix, const glm::vec3& scale)
{
	glm::mat3 rotMat;
	rotMat[0] = glm::vec3(matrix[0]) / scale.x;
	rotMat[1] = glm::vec3(matrix[1]) / scale.y;
	rotMat[2] = glm::vec3(matrix[2]) / scale.z;

	return glm::quat_cast(rotMat);
}

glm::vec3 LoaderObject::mat4ToRotation(const glm::mat4& matrix, const glm::vec3& scale)
{
	glm::mat3 rotMat;
	rotMat[0] = glm::vec3(matrix[0]) / scale.x;
	rotMat[1] = glm::vec3(matrix[1]) / scale.y;
	rotMat[2] = glm::vec3(matrix[2]) / scale.z;

	return quaternionToVec3(glm::quat_cast(rotMat));
}

glm::vec3 LoaderObject::mat4ToRotation(const glm::mat4& matrix)
{
	glm::vec3 scale = mat4ToScale(matrix);
	glm::mat3 rotMat;
	rotMat[0] = glm::vec3(matrix[0]) / scale.x;
	rotMat[1] = glm::vec3(matrix[1]) / scale.y;
	rotMat[2] = glm::vec3(matrix[2]) / scale.z;

	return quaternionToVec3(glm::quat_cast(rotMat));
}

glm::vec3 LoaderObject::mat4ToScale(const glm::mat4& matrix)
{
	return {
		glm::length(glm::vec3(matrix[0])),
		glm::length(glm::vec3(matrix[1])),
		glm::length(glm::vec3(matrix[2]))
	};
}

glm::vec3 LoaderObject::quaternionToVec3(const glm::quat& quaternion)
{
	glm::vec3 euler = glm::eulerAngles(quaternion); // radians
	return glm::degrees(euler);
}
glm::quat LoaderObject::vec3ToQuaternion(const glm::vec3& rotation)
{
	return glm::quat(rotation);
}