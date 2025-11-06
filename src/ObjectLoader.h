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
#include "json.hpp"
#include "fastgltf/core.hpp"

#include <vector>

using json = nlohmann::json;

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

/*struct Structure {
	uint32_t ID = 0;
	Structure* parent;
	std::vector<Structure> childeren;
	TransformComponent transform;
};*/

struct TextureLoader {
	std::string path;
	int index;
};

class LoaderObject {
public:
	LoaderObject() = default;
	~LoaderObject() = default;

	void loadASingleObject();
	void loadScene(const char* file);
	std::vector<Builder>& getBuilders() { return builders; }
private:
	// for gpu memory
	std::vector<Builder> builders;
	uint32_t totalVertexOffset = 0, totalIndexOffset = 0;

	std::vector<Material> materials;
	std::vector<VulkanTexture> textures;
	const char* file;
	std::vector<unsigned char> data;

	json JSON;

	// Loads a single mesh by its index
	void loadMesh(unsigned int indMesh);

	// Traverses a node recursively, so it essentially traverses all connected nodes
	void traverseNode(unsigned int nextNode, glm::mat4 matrix = glm::mat4(1.0f));

	// Gets the binary data from a file
	std::vector<unsigned char> getData();
	// Interprets the binary data into floats, indices, and textures
	std::vector<float> getFloats(json accessor);
	std::vector<uint32_t> getIndices(json accessor);
	//std::vector<Texture> getTextures();

	// Helps with the assembly from above by grouping floats
	std::vector<Vertex> assembleVertices
	(
		std::vector<glm::vec3> positions,
		std::vector<glm::vec3> normals,
		std::vector<glm::vec2> texUVs
	);

	glm::vec3 make_vec3(float a[3])
	{
		return glm::vec3(a[0], a[1], a[2]);
	}

	glm::quat make_quat(float a[4])
	{
		return glm::quat(a[0], a[1], a[2], a[3]);
	}

	std::vector<glm::vec2> groupFloatsVec2(std::vector<float> floatVec);
	std::vector<glm::vec3> groupFloatsVec3(std::vector<float> floatVec);
	std::vector<glm::vec4> groupFloatsVec4(std::vector<float> floatVec);
};