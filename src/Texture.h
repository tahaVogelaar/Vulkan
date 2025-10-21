#pragma once
#include "lve_device.hpp"


class VulkanTexture {
public:
	VulkanTexture(lve::LveDevice& device, const std::string& filePath);
	VulkanTexture(lve::LveDevice& device, VkSampler sampler, const VkExtent3D& extent);
	~VulkanTexture();

	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture& operator=(const VulkanTexture&) = delete;

	VkSampler getSampler() {return sampler;}
	VkImageView getImageView() {return view;}
	VkImageLayout getImageLayout() {return layout;}
	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);


	lve::LveDevice& device;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
	VkFormat format;
	VkImageLayout layout;
};
