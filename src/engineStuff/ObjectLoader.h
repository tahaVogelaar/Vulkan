#pragma once

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <engineStuff/Texture.h>
#include "utils/engineUtilTools.h"

#include "fastgltf/include/core.hpp"
#include <fastgltf/include/tools.hpp>
#include <fastgltf/include/util.hpp>
#include <fastgltf/include/types.hpp>
#include "stb_image.h"

#include <filesystem>
#include <vector>


struct Vertex {
	glm::vec3 position{0, 0, 0};
	glm::vec3 normal{0, 0, 0};
	glm::vec2 uv{0, 0};
	glm::vec4 tangent{1, 0, 0, 1};
	glm::vec3 bitangent{0, 1, 0};
	Vertex() = default;
	Vertex(glm::vec3& p, glm::vec3& n, glm::vec2& UV) :
	position(p), normal(n), uv(UV)
	{

	}

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	bool operator==(const Vertex& other) const {
		return position == other.position &&
			   normal == other.normal &&
			   uv == other.uv;
	}
};

struct TransformComponent {
	glm::vec3 translation{};
	glm::quat quat{};
	glm::vec3 scale{1.f, 1.f, 1.f};
	glm::mat4 worldMatrix{1.0f};
	bool dirty = true;

	// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
	// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
	// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	glm::mat4 mat4();
	void mat4(glm::mat4 &matrix);
	void mat4(glm::mat4 matrix);

	glm::mat3 normalMatrix();
};

inline size_t hashVec3(const glm::vec3 &v) {
	size_t seed = 0;
	lve::hashCombine(seed, v.x, v.y, v.z);
	return seed;
}

inline size_t hashVec2(const glm::vec2 &v) {
	size_t seed = 0;
	lve::hashCombine(seed, v.x, v.y);
	return seed;
}
namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(Vertex const &vertex) const {
			size_t seed = 0;
			seed ^= hashVec3(vertex.position);
			seed ^= hashVec3(vertex.normal);
			seed ^= hashVec2(vertex.uv);
			return seed;
		}
	};
}

// holds the ID of its primitive, and it's material
// it includes the rendering parameters for indirect drawing
struct Primitive {
	uint32_t primitiveID = 0, matID = 0;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	int32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
};

// has its own ID and the primitives it holds
struct Mesh {
	uint32_t MeshID = 0;
	std::vector<uint32_t> primitives;
};

// holds mesh and its children
// used to load entire models
struct Structure {
	int32_t meshID = -1;
	int32_t index = -1, parent = -1;
	std::vector<int32_t> childeren;
	TransformComponent transform;
	std::string name;
};

class LoaderObject {
public:
	LoaderObject(TextureHandler& loader);
	~LoaderObject() = default;
	LoaderObject(const LoaderObject &) = delete;
	LoaderObject &operator=(const LoaderObject &) = delete;

	void loadScene(const std::vector<std::string>& filePath);
	int32_t createStruct(const char* name, const int32_t parent = -1);

	std::vector<Primitive>& getPrimitives() { return primitives; }
	std::vector<Structure>& getStructures() {return structures; }
	std::vector<Mesh>& getMeshes() { return meshes; }
	TextureHandler& getLoader() {return loader;}

	static glm::vec3 mat4ToPosition(const glm::mat4& matrix);
	static glm::vec3 mat4ToRotation(const glm::mat4& matrix);
	static glm::vec3 mat4ToRotation(const glm::mat4& matrix, const glm::vec3& scale);
	static glm::vec3 mat4ToScale(const glm::mat4& matrix);
	static glm::vec3 mat4ToScale(const glm::mat4& matrix, const glm::vec3& scale);
	static glm::quat mat4ToQuaternion(const glm::mat4& matrix);
	static glm::quat mat4ToQuaternion(const glm::mat4& matrix, const glm::vec3& scale);
	static glm::vec3 quaternionToVec3(const glm::quat& v);
	static glm::quat vec3ToQuaternion(const glm::vec3& v);
private:
	// for gpu memory
	std::vector<Structure> structures;
	std::vector<Mesh> meshes;
	std::vector<Primitive> primitives;
	TextureHandler& loader;

	// temp
	uint32_t lastMeshSize, lastTexSize, lastMatSize;
	// Loads a single mesh by its index
	int32_t processMesh(fastgltf::Asset &asset, const fastgltf::Mesh &mesh);
	// Traverses a node recursively, so it essentially traverses all connected nodes
	int32_t processNode(fastgltf::Asset& asset, const fastgltf::Node& node, int32_t parentStructure);

	void loadAllPrims(fastgltf::Asset &asset);
	void loadAllTextures(fastgltf::Asset &asset, const std::filesystem::path& baseDir);
	const std::byte* getBufferData(const fastgltf::Asset& asset, const fastgltf::BufferView& view);

	static inline void computeTriangleTangent(
	const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
	const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2,
	glm::vec3& tangent,
	glm::vec3& bitangent)
	{
		glm::vec3 edge1 = p1 - p0;
		glm::vec3 edge2 = p2 - p0;

		glm::vec2 dUV1 = uv1 - uv0;
		glm::vec2 dUV2 = uv2 - uv0;

		float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);

		tangent = f * (edge1 * dUV2.y - edge2 * dUV1.y);
		bitangent = f * (edge2 * dUV1.x - edge1 * dUV2.x);
	}
};