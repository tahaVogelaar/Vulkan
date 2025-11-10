#pragma once
#include "lve_device.hpp"
#include "lve_buffer.hpp"
#include "lve_pipeline.hpp"
#include <glm/glm.hpp>
#include "lve_swap_chain.hpp"
#include "lve_descriptors.hpp"
#include <memory>

struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec3 color;
};

struct ComputeUbo {
	float deltaTime;
	glm::vec2 size;
	float fill;
};

class Compute {
public:
	Compute(lve::LveDevice& device, std::string source, lve::LveSwapChain& swapChain);
	~Compute() = default;

	Compute(const Compute &) = delete;
	Compute &operator=(const Compute &) = delete;

	// build
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createShaderModule(std::string source);
	void createPipeline();
	void createPipelineLayout();
	void transitionImageToGeneral(VkImage image, VkImageAspectFlags aspect);

	// run time
	void update(size_t imageIndex);
	void run(VkImage targetImage, VkExtent2D extent);

	uint32_t PARTICLE_COUNT = 50000;
	VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

	std::unique_ptr<lve::LveDescriptorPool> descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets{lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT};
	std::unique_ptr<lve::LveDescriptorSetLayout> setLayout;

	std::vector<VkImageView> imageView;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkShaderModule shaderModule;
	std::vector<std::unique_ptr<lve::LveBuffer>> buffer{lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT};

	lve::LveDevice& device;
	lve::LveSwapChain& swapChain;
};