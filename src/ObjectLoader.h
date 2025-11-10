#pragma once

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Texture.h>
#include "lve_utils.hpp"
#include <filesystem>

#include "fastgltf/include/core.hpp"
#include <fastgltf/include/tools.hpp>
#include <fastgltf/include/util.hpp>
#include <fastgltf/include/types.hpp>

#include <vector>

struct Vertex {
	glm::vec3 position{0, 0, 0};
	glm::vec3 color{1, 1, 1};
	glm::vec3 normal{0, 0, 0};
	glm::vec2 uv{0, 0};
	Vertex() = default;
	Vertex(glm::vec3& p, glm::vec3& c, glm::vec3& n, glm::vec2& UV) :
	position(p), normal(n), color(c), uv(UV)
	{

	}

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	bool operator==(const Vertex& other) const {
		return position == other.position &&
			   color == other.color &&
			   normal == other.normal &&
			   uv == other.uv;
	}
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
			seed ^= hashVec3(vertex.color);
			seed ^= hashVec3(vertex.normal);
			seed ^= hashVec2(vertex.uv);
			return seed;
		}
	};
}

struct Builder {
	uint32_t ID = 0;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	int32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
};

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

struct Structure {
	int32_t ID = -1;
	int32_t index = -1, parent = -1;
	std::vector<int32_t> childeren;
	TransformComponent transform;
	std::string name;
};

struct TextureLoader {
	std::string path;
	int index;
};

class LoaderObject {
public:
	LoaderObject() = default;
	~LoaderObject() = default;

	void loadASingleObject();
	void loadScene(const std::filesystem::path& filePath);

	std::vector<Builder>& getBuilders() { return builders; }
	std::vector<Structure>& getStructures() {return structures; }

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
	std::vector<Builder> builders;
	std::vector<Structure> structures;

	// Loads a single mesh by its index
	int32_t processMesh(fastgltf::Asset &asset, const fastgltf::Mesh &mesh);
	// Traverses a node recursively, so it essentially traverses all connected nodes
	int32_t processNode(fastgltf::Asset& asset, const fastgltf::Node& node, int32_t parentStructure);
};