#pragma once
#include <memory>
#include <unordered_map>
#include <glm/vec4.hpp>

#include "../core/lve_device.hpp"
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include "../ADFSCFEGAS.h"

class VulkanTexture {
public:

	VulkanTexture(lve::LveDevice& device, const std::string& filePath);
	//VulkanTexture(lve::LveDevice& device, const std::string& filePath, bool a);
	VulkanTexture(lve::LveDevice& device, VkSampler sampler, const VkExtent3D& extent);
	VulkanTexture(lve::LveDevice& device, const std::byte* dataV, size_t size);
	~VulkanTexture();

	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture& operator=(const VulkanTexture&) = delete;

	VkSampler getSampler() {return sampler;}
	VkImageView getImageView() {return view;}
	VkImageLayout getImageLayout() {return layout;}
	static void transitionImageLayout(lve::LveDevice& device, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);


	lve::LveDevice& device;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
	VkFormat format;
	VkImageLayout layout;
};

struct Material {
	int32_t color = -1; // texture index
	int32_t normal = -1; // texture index
	int32_t roughMetal = -1; // texture index
	int32_t padding; // texture index
	glm::vec4 baseColor{1, 1, 1, 1};
};

class TextureHandler {
public:
	TextureHandler(lve::LveDevice& device);
	TextureHandler(const TextureHandler &) = delete;
	TextureHandler &operator=(const TextureHandler &) = delete;

	std::vector<std::unique_ptr<VulkanTexture>>& getTextures() {return textures;}
	std::vector<Material>& getMaterials() {return materials;}
	void addTexture(const std::string& filePath);
	void addTexture(const std::byte* dataV, size_t size);
	void addMaterial(Material& mat);


private:
	std::vector<std::unique_ptr<VulkanTexture>> textures;
	std::vector<Material> materials;

	std::vector<bool> deadList;
	std::vector<uint32_t> generations; // stores how many times that slot has been reused
	std::vector<uint32_t> freeList; // holds indices of deleted slots that can be reused
	// std::unordered_map<uint32_t, std::unique_ptr<VulkanTexture>> tracker;
	lve::LveDevice& device;

};
