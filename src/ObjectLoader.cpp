#include "ObjectLoader.h"

#include <algorithm>
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

std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

/*void Builder::loadGLTFModel(const tinygltf::Mesh& mesh, const tinygltf::Model& model)
{
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto &primitive : mesh.primitives)
        {
            const auto &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
            const auto &posView = model.bufferViews[posAccessor.bufferView];
            const auto &posBuffer = model.buffers[posView.buffer];
            const float *posData = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

            const float *normalData = nullptr;
            const float *uvData = nullptr;
            const float *colorData = nullptr;

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
            {
                const auto &normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                const auto &normalView = model.bufferViews[normalAccessor.bufferView];
                const auto &normalBuffer = model.buffers[normalView.buffer];
                normalData = reinterpret_cast<const float*>(&normalBuffer.data[normalView.byteOffset + normalAccessor.byteOffset]);
            }

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            {
                const auto &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                const auto &uvView = model.bufferViews[uvAccessor.bufferView];
                const auto &uvBuffer = model.buffers[uvView.buffer];
                uvData = reinterpret_cast<const float*>(&uvBuffer.data[uvView.byteOffset + uvAccessor.byteOffset]);
            }

            if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
            {
                const auto &colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
                const auto &colorView = model.bufferViews[colorAccessor.bufferView];
                const auto &colorBuffer = model.buffers[colorView.buffer];
                colorData = reinterpret_cast<const float*>(&colorBuffer.data[colorView.byteOffset + colorAccessor.byteOffset]);
            }

            // Process indices
            const auto &idxAccessor = model.accessors[primitive.indices];
            const auto &idxView = model.bufferViews[idxAccessor.bufferView];
            const auto &idxBuffer = model.buffers[idxView.buffer];

            size_t indexCount = idxAccessor.count;
            for (size_t i = 0; i < indexCount; ++i)
            {
                uint32_t idx;
                if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    idx = reinterpret_cast<const uint16_t*>(&idxBuffer.data[idxView.byteOffset + idxAccessor.byteOffset + i * 2])[0];
                else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    idx = reinterpret_cast<const uint32_t*>(&idxBuffer.data[idxView.byteOffset + idxAccessor.byteOffset + i * 4])[0];
                else
                    throw std::runtime_error("Unsupported index component type");

                Vertex vertex{};

                // Position
                vertex.position = {
                    posData[3 * idx + 0],
                    posData[3 * idx + 1],
                    posData[3 * idx + 2]
                };

                // Normal
                if (normalData)
                    vertex.normal = {
                        normalData[3 * idx + 0],
                        normalData[3 * idx + 1],
                        normalData[3 * idx + 2]
                    };

                // UV
                if (uvData)
                    vertex.uv = {
                        uvData[2 * idx + 0],
                        uvData[2 * idx + 1]
                    };

                // Color
                if (colorData)
                    vertex.color = {
                        colorData[3 * idx + 0],
                        colorData[3 * idx + 1],
                        colorData[3 * idx + 2]
                    };

                // Add unique vertex
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
}*/

void LoaderObject::loadMesh(unsigned int indMesh)
{
    // Get all accessor indices
    unsigned int posAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["POSITION"];
    unsigned int normalAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["NORMAL"];
    unsigned int texAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TEXCOORD_0"];
    unsigned int indAccInd = JSON["meshes"][indMesh]["primitives"][0]["indices"];

    // Use accessor indices to get all vertices components
    std::vector<float> posVec = getFloats(JSON["accessors"][posAccInd]);
    std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
    std::vector<float> normalVec = getFloats(JSON["accessors"][normalAccInd]);
    std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
    std::vector<float> texVec = getFloats(JSON["accessors"][texAccInd]);
    std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

	Builder builder;
    // Combine all the vertex components and also get the indices and textures
    builder.vertices = assembleVertices(positions, normals, texUVs);
    builder.indices = getIndices(JSON["accessors"][indAccInd]);
    //std::vector<Texture> textures = getTextures();
    builders.push_back(builder);
}


void LoaderObject::loadScene(const char* file)
{
	std::string text = get_file_contents(file);
	JSON = json::parse(text);

	// Get the binary data
	this->file = file;
	data = getData();

	// Traverse all nodes
	traverseNode(0);

}

void LoaderObject::traverseNode(unsigned int nextNode, glm::mat4 matrix)
{
	// Current node
	json node = JSON["nodes"][nextNode];

	glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
	if (node.find("translation") != node.end())
	{
		float transValues[3];
		for (unsigned int i = 0; i < node["translation"].size(); i++)
			transValues[i] = (node["translation"][i]);
		translation = make_vec3(transValues);
	}
	// Get quaternion if it exists
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	if (node.find("rotation") != node.end())
	{
		float rotValues[4] =
		{
			node["rotation"][3],
			node["rotation"][0],
			node["rotation"][1],
			node["rotation"][2]
		};
		rotation = make_quat(rotValues);
	}
	// Get scale if it exists
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	if (node.find("scale") != node.end())
	{
		float scaleValues[3];
		for (unsigned int i = 0; i < node["scale"].size(); i++)
			scaleValues[i] = (node["scale"][i]);
		scale = glm::make_vec3(scaleValues);
	}


	if (node.find("mesh") != node.end())
	{
		loadMesh(node["mesh"]);
	std::cout << "load mesh\n";
	}
	else
	std::cout << "no load mesh\n";

	if (node.find("children") != node.end())
	{
		for (unsigned int i = 0; i < node["children"].size(); i++)
			traverseNode(node["children"][i]);
	}
}


std::vector<unsigned char> LoaderObject::getData()
{
	// Create a place to store the raw text, and get the uri of the .bin file
	std::string bytesText;
	std::string uri = JSON["buffers"][0]["uri"];

	// Store raw text data into bytesText
	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
	bytesText = get_file_contents((fileDirectory + uri).c_str());

	// Transform the raw text data into bytes and put them in a vector
	std::vector<unsigned char> data(bytesText.begin(), bytesText.end());
	return data;
}

std::vector<float> LoaderObject::getFloats(json accessor)
{
	std::vector<float> floatVec;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 1);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	std::string type = accessor["type"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Interpret the type and store it into numPerVert
	unsigned int numPerVert;
	if (type == "SCALAR") numPerVert = 1;
	else if (type == "VEC2") numPerVert = 2;
	else if (type == "VEC3") numPerVert = 3;
	else if (type == "VEC4") numPerVert = 4;
	else throw std::invalid_argument("Type is invalid (not SCALAR, VEC2, VEC3, or VEC4)");

	// Go over all the bytes in the data at the correct place using the properties from above
	unsigned int beginningOfData = byteOffset + accByteOffset;
	unsigned int lengthOfData = count * 4 * numPerVert;
	for (unsigned int i = beginningOfData; i < beginningOfData + lengthOfData; i += 4)
	{
		unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
		float value;
		std::memcpy(&value, bytes, sizeof(float));
		floatVec.push_back(value);
	}

	return floatVec;
}

std::vector<GLuint> LoaderObject::getIndices(json accessor)
{
	std::vector<GLuint> indices;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 0);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	unsigned int componentType = accessor["componentType"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Get indices with regards to their type: unsigned int, unsigned short, or short
	unsigned int beginningOfData = byteOffset + accByteOffset;
	if (componentType == 5125)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 4; i += 4)
		{
			unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
			unsigned int value;
			std::memcpy(&value, bytes, sizeof(unsigned int));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5123)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = { data[i], data[i + 1] };
			unsigned short value;
			std::memcpy(&value, bytes, sizeof(unsigned short));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5122)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = { data[i], data[i + 1] };
			short value;
			std::memcpy(&value, bytes, sizeof(short));
			indices.push_back((GLuint)value);
		}
	}

	return indices;
}

/*std::vector<Texture> LoaderObject::getTextures()
{
	std::vector<Texture> textures;

	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

	// Go over all images
	for (unsigned int i = 0; i < JSON["images"].size(); i++)
	{
		// uri of current texture
		std::string texPath = JSON["images"][i]["uri"];

		// Check if the texture has already been loaded
		bool skip = false;
		for (unsigned int j = 0; j < loadedTexName.size(); j++)
		{
			if (loadedTexName[j] == texPath)
			{
				textures.push_back(loadedTex[j]);
				skip = true;
				break;
			}
		}

		// If the texture has been loaded, skip this
		if (!skip)
		{
			// Load diffuse texture
			if (texPath.find("baseColor") != std::string::npos)
			{
				Texture diffuse = Texture((fileDirectory + texPath).c_str(), "diffuse", loadedTex.size());
				textures.push_back(diffuse);
				loadedTex.push_back(diffuse);
				loadedTexName.push_back(texPath);
			}
			// Load specular texture
			else if (texPath.find("metallicRoughness") != std::string::npos)
			{
				Texture specular = Texture((fileDirectory + texPath).c_str(), "specular", loadedTex.size());
				textures.push_back(specular);
				loadedTex.push_back(specular);
				loadedTexName.push_back(texPath);
			}
		}
	}

	return textures;
}*/

std::vector<Vertex> LoaderObject::assembleVertices
(
	std::vector<glm::vec3> positions,
	std::vector<glm::vec3> normals,
	std::vector<glm::vec2> texUVs
)
{
	std::vector<Vertex> vertices;
	glm::vec3 enptyColor(1.0f, 1.0f, 1.0f);
	for (int i = 0; i < positions.size(); i++)
	{
		vertices.push_back
		(
			Vertex
			{
				positions[i],
				enptyColor,
				normals[i],
				texUVs[i]
			}
		);
	}
	return vertices;
}

std::vector<glm::vec2> LoaderObject::groupFloatsVec2(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 2;

	std::vector<glm::vec2> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec2(0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec3> LoaderObject::groupFloatsVec3(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 3;

	std::vector<glm::vec3> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec3(0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec4> LoaderObject::groupFloatsVec4(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 4;

	std::vector<glm::vec4> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec4(0, 0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}