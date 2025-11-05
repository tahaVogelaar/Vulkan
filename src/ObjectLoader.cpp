#include "ObjectLoader.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <cassert>
#include <cstring>

void LoaderObject::loadScene(std::string& path)
{
	const aiScene* scene = importer.ReadFile(
		path.c_str(),
		aiProcess_Triangulate |            // convert all faces to triangles
		aiProcess_FlipUVs |                // flip UVs if needed
		aiProcess_CalcTangentSpace         // compute tangents if you want normal mapping
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
		return;
	}

	materials.resize(scene->mNumMaterials);
	processNode(scene->mRootNode, scene);
}

void LoaderObject::processNode(aiNode* node, const aiScene* scene) {
	// Process all meshes in this node
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene);
	}

	// Recurse into children
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene);
	}
}

void LoaderObject::processMesh(aiMesh* mesh, const aiScene* scene) {
	dataStructureStuffIdk a;
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	// Set offsets BEFORE adding vertices/indices
	a.vertexOffset = static_cast<uint32_t>(vertices.size());
	a.indexOffset  = static_cast<uint32_t>(indices.size());

	// Process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

		if (mesh->HasNormals())
		    vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

		if (mesh->mTextureCoords[0])
		    vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
		else
		    vertex.uv = {0.0f, 0.0f};

		if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
		    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
		    vertices.push_back(vertex);
		    a.vertexCount++;
		}
		indices.push_back(uniqueVertices[vertex]);
		a.indexCount++;
	}


	imBadAtNamingStuff.push_back(a);
}