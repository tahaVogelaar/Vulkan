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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

void LoaderObject::loadScene(const std::filesystem::path& filePath) {
	namespace fg = fastgltf;
	// 1. Read the file into memory
	auto data = fg::GltfDataBuffer::FromPath(filePath);

	// 2. Create the parser
	fg::Parser parser;
	fastgltf::Options options = fastgltf::Options::LoadExternalBuffers;

	// 3. Parse depending on file type
	std::unique_ptr<fg::Expected<fg::Asset>> assetResult;
	if (fg::determineGltfFileType(data.get()) == fg::GltfType::GLB) {
		assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltfBinary(
			data.get(),
			filePath.parent_path(),
			options,
			fg::Category::All
		));
	} else {
		assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltf(
			data.get(),
			filePath.parent_path(),
			options,
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
	// Traverse all root nodes
	for (auto nodeIndex : scene.nodeIndices) {
		const fg::Node& node = asset.nodes[nodeIndex];
		int32_t childIndex = processNode(asset, node, -1);
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

glm::vec3 mat4ToRotationRadians(const glm::mat4& matrix)
{
	glm::vec3 scale = LoaderObject::mat4ToScale(matrix);
	glm::mat3 rotMat;
	rotMat[0] = glm::vec3(matrix[0]) / scale.x;
	rotMat[1] = glm::vec3(matrix[1]) / scale.y;
	rotMat[2] = glm::vec3(matrix[2]) / scale.z;

	return glm::eulerAngles(glm::quat_cast(rotMat)); // radians directly
}


int32_t LoaderObject::processNode(fastgltf::Asset& asset, const fastgltf::Node& node, int32_t parentStructure) {
	glm::mat4 localTransform{1.0f};
	Structure s;
	structures.push_back(s);
	uint32_t index = structures.size() - 1;
	structures[index].index = index;
	structures[index].parent = parentStructure;

	const auto mat = fastgltf::getTransformMatrix(node);
	localTransform = glm::make_mat4(mat.data());


	if (node.meshIndex.has_value()) {
		const fastgltf::Mesh& mesh = asset.meshes[node.meshIndex.value()];
		structures[index].ID = processMesh(asset, mesh);
	}
	else
		structures[index].ID = -1;

	// Traverse child nodes
	for (auto childIdx : node.children) {
		uint32_t childIndex = processNode(asset, asset.nodes[childIdx], index);
		structures[index].childeren.push_back(childIndex);
	}


	structures[index].transform.translation = mat4ToPosition(localTransform);
	structures[index].transform.rotation = (mat4ToRotationRadians(localTransform));
	/*std::cout << structures[index].transform.rotation.x << ' ' <<
		structures[index].transform.rotation.y << ' ' <<
			structures[index].transform.rotation.z << '\n';*/
	structures[index].transform.scale = mat4ToScale(localTransform);
	structures[index].name = node.name;
	return index;
}


int32_t LoaderObject::processMesh(fastgltf::Asset &asset, const fastgltf::Mesh &mesh)
{
    uint32_t ID = 0;
    Builder builder;

    // keep track of per-mesh offsets for concatenation
    size_t vertexBase = 0;
    size_t indexBase = 0;

    for (const auto& prim : mesh.primitives)
    {
        // Determine vertex count for this primitive (from POSITION accessor)
        size_t primVertexCount = 0;
        if (auto it = prim.findAttribute("POSITION"); it != prim.attributes.end()) {
            const fastgltf::Accessor& acc = asset.accessors[it->accessorIndex];
            primVertexCount = acc.count;
        } else {
            // no positions? skip primitive (can't render)
            continue;
        }

        // temp storage for this primitive
        std::vector<Vertex> primVerts;
        primVerts.resize(primVertexCount);

        // --- Positions (required) ---
        {
            const fastgltf::Accessor& accessor = asset.accessors[prim.findAttribute("POSITION")->accessorIndex];
            size_t idx = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 v) {
                primVerts[idx++].position = glm::vec3(v[0], v[1], v[2]);
            });
        }

        // --- Normals (optional) ---
        if (auto it = prim.findAttribute("NORMAL"); it != prim.attributes.end()) {
            const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
            size_t idx = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 v) {
                primVerts[idx++].normal = glm::vec3(v[0], v[1], v[2]);
            });
        } else {
            // fill with default normal (e.g. up) or zero
            for (size_t i = 0; i < primVertexCount; ++i) primVerts[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // --- UVs (optional) ---
        if (auto it = prim.findAttribute("TEXCOORD_0"); it != prim.attributes.end()) {
            const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
            size_t idx = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, accessor, [&](fastgltf::math::fvec2 v) {
                primVerts[idx++].uv = glm::vec2(v[0], v[1]);
            });
        } else {
            for (size_t i = 0; i < primVertexCount; ++i) primVerts[i].uv = glm::vec2(0.0f, 0.0f);
        }

        // --- Colors (optional) ---
        if (auto it = prim.findAttribute("COLOR_0"); it != prim.attributes.end()) {
            const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
            const fastgltf::BufferView& view = asset.bufferViews[accessor.bufferViewIndex.value()];
            const std::byte* base = getBufferData(asset, view) + accessor.byteOffset;
            const size_t stride = view.byteStride.value_or(fastgltf::getElementByteSize(accessor.type, accessor.componentType));
            for (size_t i = 0; i < accessor.count; ++i) {
                const float* f = reinterpret_cast<const float*>(base + i * stride);
                primVerts[i].color = glm::vec4(f[0], f[1], f[2], (accessor.type == fastgltf::AccessorType::Vec4) ? f[3] : 1.0f);
            }
        } else {
            for (size_t i = 0; i < primVertexCount; ++i) primVerts[i].color = glm::vec4(1.0f);
        }

        // --- Indices (optional) ---
        std::vector<uint32_t> primIndices;
        if (prim.indicesAccessor.has_value()) {
            auto& accessor = asset.accessors[prim.indicesAccessor.value()];
            primIndices.resize(accessor.count);
            uint32_t idx = 0;
            fastgltf::iterateAccessor<uint32_t>(asset, accessor, [&](uint32_t index) {
                primIndices[idx++] = index;
            });

            // offset indices by vertexBase when appending
            for (auto &i : primIndices) i += static_cast<uint32_t>(vertexBase);
        } else {
            // no indices: create a trivial index list
            primIndices.resize(primVertexCount);
            for (uint32_t i = 0; i < (uint32_t)primVertexCount; ++i) primIndices[i] = vertexBase + i;
        }

        // append primVerts and primIndices into builder
        builder.vertices.insert(builder.vertices.end(), primVerts.begin(), primVerts.end());
        builder.indices.insert(builder.indices.end(), primIndices.begin(), primIndices.end());

        // update base counters
        vertexBase = builder.vertices.size();
        indexBase  = builder.indices.size();
    } // primitives

    // set offsets/ID then push builder
    builder.ID = static_cast<uint32_t>(builders.size());
    if (!builders.empty()) {
        // previous totals
        const auto &prev = builders.back();
        builder.vertexOffset = prev.vertices.size() + prev.vertexOffset;
        builder.indexOffset  = prev.indices.size()  + prev.indexOffset;
    } else {
        builder.vertexOffset = 0;
        builder.indexOffset  = 0;
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