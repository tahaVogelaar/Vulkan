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
#include <glm/gtc/type_ptr.hpp>

LoaderObject::LoaderObject(TextureHandler& loader) : loader(loader)
{
	Material empty;
	empty.baseColor = glm::vec4(1);
	empty.color = -1;
	empty.roughMetal = -1;
	empty.normal = -1;


}


void LoaderObject::loadScene(const std::vector<std::string>& filePath)
{
	namespace fg = fastgltf;

	for (const std::filesystem::path path : filePath)
	{
		lastMeshSize = meshes.size();
		lastMatSize = loader.getMaterials().size();
        lastTexSize  = loader.getTextures().size(); // ADD THIS
		// 1. Read the file into memory
		auto data = fg::GltfDataBuffer::FromPath(path);

		// 2. Create the parser
		fg::Parser parser;
		fastgltf::Options options = fastgltf::Options::LoadExternalBuffers;

		// 3. Parse depending on file type
		std::unique_ptr<fg::Expected<fg::Asset>> assetResult;
		if (fg::determineGltfFileType(data.get()) == fg::GltfType::GLB) {
			assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltfBinary(
				data.get(),
				path.parent_path(),
				options,
				fg::Category::All
			));
		} else {
			assetResult = std::make_unique<fg::Expected<fg::Asset>>(parser.loadGltf(
				data.get(),
				path.parent_path(),
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
		// 5. load textures
		loadAllTextures(asset, path.parent_path());

		loadAllPrims(asset);
		const fg::Scene& scene = asset.scenes[asset.defaultScene.value_or(0)];

		int32_t indx = createStruct(path.filename().c_str());
		// Traverse all root nodes
		for (auto nodeIndex : scene.nodeIndices) {
			const fg::Node& node = asset.nodes[nodeIndex];
			int32_t childIndex = processNode(asset, node, indx);
			structures[indx].childeren.push_back(childIndex);
		}
	}
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

int32_t LoaderObject::createStruct(const char* name, const int32_t parent)
{
	Structure s;
	s.parent = parent;
	s.name = name;
	structures.push_back(s);
	structures[structures.size() - 1].index = structures.size() - 1;
	return structures.size() - 1;
}

int32_t LoaderObject::processNode(fastgltf::Asset& asset, const fastgltf::Node& node, int32_t parentStructure) {
	glm::mat4 localTransform{1.0f};
	uint32_t index = createStruct(node.name.c_str(), parentStructure);

	// get matrix
	const auto mat = fastgltf::getTransformMatrix(node);
	localTransform = glm::make_mat4(mat.data());

	// get mesh
	if (node.meshIndex.has_value())
		structures[index].meshID = lastMeshSize + node.meshIndex.value();
	else
		structures[index].meshID = -1;

	// Traverse child nodes
	for (auto childIdx : node.children) {
		uint32_t childIndex = processNode(asset, asset.nodes[childIdx], index);
		structures[index].childeren.push_back(childIndex);
	}

	// apply transform
	if (parentStructure != -1)
		structures[index].transform.mat4( structures[parentStructure].transform.worldMatrix * localTransform);
	else
		structures[index].transform.mat4(static_cast<glm::mat4>(localTransform));
	structures[index].name = node.name;

	return index;
}

void LoaderObject::loadAllPrims(fastgltf::Asset &asset)
{
	uint32_t meshCount = asset.meshes.size();

	for (size_t i = 0; i < meshCount; ++i)
	{
		size_t indexBase = 0;
		size_t primSize = asset.meshes[i].primitives.size();

		Mesh mesh;
		mesh.MeshID = lastMeshSize + i;

		for (size_t j = 0; j < primSize; ++j)
		{
			Primitive primitive;

			fastgltf::Primitive& prim = asset.meshes[i].primitives[j];

			// Determine vertex count for this primitive (from POSITION accessor)
			size_t primVertexCount = 0;
			if (auto it = prim.findAttribute("POSITION"); it != prim.attributes.end()) {
				const fastgltf::Accessor& acc = asset.accessors[it->accessorIndex];
				primVertexCount = acc.count;
			}
			else continue; // no positions? skip primitive (can't render)

			// temp storage for this primitive
			std::vector<Vertex> primVerts;
			primVerts.resize(primVertexCount);


			// --- Positions (required) ---
	        const fastgltf::Accessor& accessor = asset.accessors[prim.findAttribute("POSITION")->accessorIndex];
	        size_t idx = 0;
	        fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 v) {
	            primVerts[idx++].position = glm::vec3(v[0], v[1], v[2]);
	        });

	        // --- Normals (optional) ---
	        if (auto it = prim.findAttribute("NORMAL"); it != prim.attributes.end()) {
	            const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
	            size_t idx = 0;
	            fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 v) {
	                primVerts[idx++].normal = glm::vec3(v[0], v[1], v[2]);
	            });
	        } else {
	            // fill with default normal (e.g. up) or zero
	            for (size_t k = 0; k < primVertexCount; ++k) primVerts[k].normal = glm::vec3(0.0f, 1.0f, 0.0f);
	        }

			bool hasUV = false;
	        // --- UVs (optional) ---
	        if (auto it = prim.findAttribute("TEXCOORD_0"); it != prim.attributes.end())
	        	{
	            const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
	            size_t idx = 0;
	            fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, accessor, [&](fastgltf::math::fvec2 v) {
	                primVerts[idx++].uv = glm::vec2(v[0], v[1]);
	            });

	        	hasUV = true;
	        }
			else
			{
	            for (size_t k = 0; k < primVertexCount; ++k) primVerts[k].uv = glm::vec2(0.0f, 0.0f);
	        }

			// --- Indices ---
			std::vector<uint32_t> primIndices;
			if (prim.indicesAccessor.has_value())
			{
				auto &accessor = asset.accessors[prim.indicesAccessor.value()];
				primIndices.resize(accessor.count);
				uint32_t idx = 0;
				// Use fastgltf iterateAccessor with the actual component type if needed.
				fastgltf::iterateAccessor<uint32_t>(asset, accessor, [&](uint32_t index) {
					primIndices[idx++] = index; // local
				});
			} else {
				primIndices.resize(primVertexCount);
				for (uint32_t k = 0; k < (uint32_t)primVertexCount; ++k) primIndices[k] = k;
			}

			// --- TANGENT HANDLING ---
		bool hasTangent = false;

		if (auto it = prim.findAttribute("TANGENT"); it != prim.attributes.end())
		{
		    const fastgltf::Accessor& accessor = asset.accessors[it->accessorIndex];
		    size_t idx = 0;

		    fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, accessor, [&](fastgltf::math::fvec4 v) {
		        // store xyz and w separately
		        primVerts[idx].tangent = glm::vec4(v[0], v[1], v[2], v[3]);
		        idx++;
		    });

		    hasTangent = true;
		}

		// Fallback: compute tangent & w from positions + UVs
		if (!hasTangent && hasUV)
		{
		    // initialize tangents
		    for (auto& v : primVerts)
		    {
		        v.tangent = glm::vec4(0.0f);
		        v.bitangent = glm::vec3(0.0f);
		    }

		    for (size_t k = 0; k < primIndices.size(); k += 3)
		    {
		        uint32_t i0 = primIndices[k + 0];
		        uint32_t i1 = primIndices[k + 1];
		        uint32_t i2 = primIndices[k + 2];

		        const glm::vec3& p0 = primVerts[i0].position;
		        const glm::vec3& p1 = primVerts[i1].position;
		        const glm::vec3& p2 = primVerts[i2].position;

		        const glm::vec2& uv0 = primVerts[i0].uv;
		        const glm::vec2& uv1 = primVerts[i1].uv;
		        const glm::vec2& uv2 = primVerts[i2].uv;

		        glm::vec3 edge1 = p1 - p0;
		        glm::vec3 edge2 = p2 - p0;

		        glm::vec2 dUV1 = uv1 - uv0;
		        glm::vec2 dUV2 = uv2 - uv0;

		        float f = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
		        if (fabs(f) < 1e-8f) f = 1.0f; else f = 1.0f / f;

		        glm::vec4 T = glm::vec4(f * (edge1 * dUV2.y - edge2 * dUV1.y), 1);
		        glm::vec3 B = f * (edge2 * dUV1.x - edge1 * dUV2.x);

		        // accumulate
		        primVerts[i0].tangent += T;
		        primVerts[i1].tangent += T;
		        primVerts[i2].tangent += T;

		        primVerts[i0].bitangent += B;
		        primVerts[i1].bitangent += B;
		        primVerts[i2].bitangent += B;
		    }

		    // normalize & compute handedness
		    for (auto& v : primVerts)
		    {
		        glm::vec3 N = glm::normalize(v.normal);
		        glm::vec3 T = glm::normalize(v.tangent);
		        glm::vec3 B = glm::normalize(v.bitangent);

		        // tangent.w = handedness
		        float w = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
		        v.tangent = glm::vec4(T, w);

		        // bitangent is reconstructed in shader, no need to store B
		    }
		}
		// If mesh has no UVs and no tangents, fallback:
		if (!hasTangent && !hasUV)
		{
		    for (auto& v : primVerts)
		    {
		        v.tangent = glm::vec4(1,0,0, 1);
		        v.bitangent = glm::vec3(0,1,0); // optional, mostly for debug
		    }
		}

			primitive.indices = primIndices; // local indices
			primitive.vertices = primVerts;  // local vertices

			primitive.matID = prim.materialIndex.has_value() ? (prim.materialIndex.value() + lastMatSize) : -1;

			primitive.primitiveID = primitives.size();
			primitives.push_back(primitive);
			mesh.primitives.push_back(primitive.primitiveID);
		}

		meshes.push_back(mesh);
	}
}

void LoaderObject::loadAllTextures(fastgltf::Asset &asset, const std::filesystem::path& baseDir)
{
    for (size_t i = 0; i < asset.textures.size(); ++i)
    {
        const auto texture = asset.textures[i];
        const auto& image = asset.images[texture.imageIndex.value()];

        // External image path (URI)
    	if (auto* file = std::get_if<fastgltf::sources::URI>(&image.data))
    	{
    		std::string uri = file->uri.c_str();

    		std::filesystem::path texPath = baseDir / uri;
    		texPath = texPath.lexically_normal();

    		if (!std::filesystem::exists(texPath)) {
    			if (uri.rfind("./", 0) == 0) {
    				std::string stripped = uri.substr(2);
    				texPath = (baseDir / stripped).lexically_normal();
    			}
    		}

    		if (!std::filesystem::exists(texPath)) {
    			std::cerr << "Texture file not found: " << texPath
						  << " (uri: " << uri << ")\n";
    			continue;
    		}

    		std::string parent = texPath.parent_path().string();
    		std::string name   = texPath.stem().string();

    		std::filesystem::path ddsPath = parent + "/dds/" + (name + ".dds");

    		// If you want to force DDS fallback, ensure directory exists.
    		std::filesystem::create_directories(ddsPath.parent_path());

    		loader.addTexture(texPath);

    		continue;
    	}

        // Embedded in bufferView
    	if (auto* bv = std::get_if<fastgltf::sources::BufferView>(&image.data))
    	{
    		const auto& bufferView = asset.bufferViews[bv->bufferViewIndex];
    		size_t size = bufferView.byteLength;
    		if (size == 0) {
    			std::cerr << "Warning: bufferView texture " << i << " has size 0, skipping\n";
    			continue;
    		}
    		const std::byte* data = getBufferData(asset, bufferView) + bufferView.byteOffset;
    		loader.addTexture(data, size);
    		continue;
    	}
    }

	// load materials
	for (size_t i = 0; i < asset.materials.size(); ++i)
	{
		Material mat;
		const fastgltf::Material& mate = asset.materials[i];

		// color texture
		if (mate.pbrData.baseColorTexture.has_value())
			mat.color = static_cast<int16_t>(mate.pbrData.baseColorTexture.value().textureIndex + lastTexSize);
		else
			mat.color = -1;

		// matal rough texture
		if (mate.pbrData.metallicRoughnessTexture.has_value())
			mat.roughMetal = static_cast<int16_t>(mate.pbrData.metallicRoughnessTexture.value().textureIndex + lastTexSize);
		else
			mat.roughMetal = -1;

		// color texture
		if (mate.normalTexture.has_value())
			mat.normal = static_cast<int16_t>(mate.normalTexture.value().textureIndex + lastTexSize);
		else
			mat.normal = -1;

		// base color
		mat.baseColor = glm::vec4{
			mate.pbrData.baseColorFactor[0],
			mate.pbrData.baseColorFactor[1],
			mate.pbrData.baseColorFactor[2],
			mate.pbrData.baseColorFactor[3]
		};

		loader.addMaterial(mat);
	}
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
	//return glm::quat(glm::radians(rotation));
	return glm::quat(rotation);
}

const std::byte* LoaderObject::getBufferData(const fastgltf::Asset& asset, const fastgltf::BufferView& view) {
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