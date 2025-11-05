#pragma once

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <Texture.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>

#include "lve_utils.hpp"


struct Vertex {
	glm::vec3 position{0, 0, 0};
	glm::vec3 color{1, 1, 1};
	glm::vec3 normal{0, 0, 0};
	glm::vec2 uv{0, 0};
	Vertex() = default;

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

struct dataStructureStuffIdk {
	int id = 0;
	uint32_t vertexCount = 0;
	int32_t vertexOffset = 0;
	uint32_t indexCount = 0;
	uint32_t indexOffset = 0;
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
	void loadScene(std::string& path);
	std::vector<Vertex>* getVertices() { return &vertices; };
	std::vector<uint32_t>* getIndices() { return &indices; };
	std::vector<dataStructureStuffIdk>* getOffsets() { return &imBadAtNamingStuff; };
private:
	void processMesh(aiMesh* mesh, const aiScene* scene);
	void processNode(aiNode* node, const aiScene* scene);

	// for gpu memory
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<dataStructureStuffIdk> imBadAtNamingStuff;
	std::vector<Material> materials;
	std::vector<VulkanTexture> textures;
	Assimp::Importer importer;
};