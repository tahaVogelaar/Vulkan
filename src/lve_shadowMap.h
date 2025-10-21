#pragma once

#include "lve_device.hpp"
#include "lve_pipeline.hpp"
#include "memory"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace lve {
	struct PointLight {
		glm::mat4 proj;
		glm::vec3 position;
		glm::vec3 rotation;
		float innerCone;
		float outerCone;
		float range;
		float specular;
		float intensity;
		float filler;
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
		PointLight light;
		std::unique_ptr<ShadowMap> shadowMap;

		void update()
		{
			glm::mat4 lightView = glm::lookAt(light.position, light.position + light.rotation, glm::vec3(0,1,0));
			glm::mat4 lightProj = glm::perspective(glm::radians(70.f), 1.0f, .1f, 50.0f);
			light.proj = lightProj * lightView;
		}
	};
}