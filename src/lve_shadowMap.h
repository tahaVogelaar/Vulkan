#pragma once

#include "core/lve_device.hpp"
#include "core/lve_pipeline.hpp"
#include <lve_shadow_renderer.h>
#include "memory"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "lve_shadowMap.h"

namespace lve {
	struct PointLight {
		glm::mat4 proj;
		glm::vec3 position;
		float padding;
		glm::vec3 rotation;
		float padding2;
		float innerCone;
		float outerCone;
		float range;
		float intensity;
	};

	class ShadowMap {
	public:
		ShadowMap(LveDevice& device, VkExtent2D extent);
		~ShadowMap();

		ShadowMap(const ShadowMap &) = delete;
		ShadowMap &operator=(const ShadowMap &) = delete;

		void beginRender(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer);
		void endRender(VkCommandBuffer commandBuffer);

		VkImage getImage() const { return image; }
		VkImageView getImageView() const { return imageView; }
		VkSampler getSampler() const {return sampler; }
		VkExtent2D getExtent() const { return extent; }

	private:
		void create();

		VkDeviceMemory imageMemory;
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;

		LveDevice& lveDevice;
		VkExtent2D extent;
	};

	struct PointLightData {
		uint32_t index = 0;
		PointLight light;
		std::unique_ptr<ShadowMap> shadowMap;
		bool dirty = true;

		void create(LveDevice& device)
		{
		}

		void createShadowMap(LvePointShadowRenderer& shadowRenderer, LveDevice& device, VkExtent2D shadowExtent)
		{
			shadowMap = std::make_unique<ShadowMap>(device, shadowExtent);
			shadowRenderer.createFramebuffers(shadowMap->getImageView(), shadowMap->getExtent(), 1);
		}

		void update()
		{
			glm::mat4 lightView = glm::lookAt(light.position, light.position + light.rotation, glm::vec3(0,1,0));
			glm::mat4 lightProj = glm::perspective(glm::radians(70.f), 1.0f, .1f, 50.0f);
			light.proj = lightProj * lightView;
		}

		void setShadowEnable(bool enable, lve::LveDevice& device, VkDescriptorSet globalDescriptorSet)
		{
			if (enable)
				if (!shadowMap)
					std::cerr << "Shadow Map does not exist" << std::endl;

			VkDescriptorImageInfo imageInfo;
			imageInfo.imageView = shadowMap->getImageView();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.sampler = shadowMap->getSampler();

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.dstSet = globalDescriptorSet;
			write.dstBinding = 5;
			write.pImageInfo = &imageInfo;
			write.descriptorCount = 1;

			vkUpdateDescriptorSets(device.device(), 1, &write, 0, nullptr);
		}
	};
}