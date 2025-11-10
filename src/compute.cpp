#include "compute.h"

#include <random>

void Compute::run(VkImage targetImage, VkExtent2D extent)
{
    // 1. Allocate a one-time command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }

    // 2. Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 3. Transition main image to GENERAL for compute
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;  // previous render pass
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // wait for previous writes
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.image = targetImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    // 4. Bind compute pipeline and descriptor sets
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout,
        0,
        1,
        descriptorSets.data(),
        0,
        nullptr
    );

    // 5. Dispatch compute workgroups
    vkCmdDispatch(
        commandBuffer,
        (extent.width + 15) / 16,
        (extent.height + 15) / 16,
        1
    );

    // 6. Barrier back to COLOR_ATTACHMENT_OPTIMAL for later graphics / copy
    VkImageMemoryBarrier barrierToGraphics{};
    barrierToGraphics.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierToGraphics.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrierToGraphics.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrierToGraphics.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrierToGraphics.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    barrierToGraphics.image = targetImage;
    barrierToGraphics.subresourceRange = barrier.subresourceRange;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrierToGraphics
    );

    // 7. End command buffer
    vkEndCommandBuffer(commandBuffer);

    // 8. Submit command buffer with fence
    VkFence fence;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device.device(), &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.computeQueue(), 1, &submitInfo, fence);
    vkWaitForFences(device.device(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device.device(), fence, nullptr);

    // 9. Free command buffer
    vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &commandBuffer);
}



void Compute::update(size_t imageIndex)
{/*
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSets[imageIndex];
	descriptorWrite.dstBinding = 0; // your binding in the shader
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);*/
}


Compute::Compute(lve::LveDevice &device, std::string source, lve::LveSwapChain& swapChain) :
device(device), swapChain(swapChain)
{
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	std::vector<Particle> particles{PARTICLE_COUNT};
	for (auto& particle : particles) {
		float r = 0.25f * sqrt(rndDist(rndEngine));
		float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
		float x = r * cos(theta) * swapChain.getSwapChainExtent().height / swapChain.getSwapChainExtent().width;
		float y = r * sin(theta);
		particle.position = glm::vec2(x, y);
		particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
		particle.color = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
	}

	createDescriptorPool();
	createDescriptorSetLayout();
	createShaderModule(source);
	createPipelineLayout();
	createPipeline();
}

void Compute::createDescriptorPool()
{
	descriptorPool = lve::LveDescriptorPool::Builder(device)
		.setMaxSets(lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT)

		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT)
		//.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		//			lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT)
		.build();
}

void Compute::createDescriptorSetLayout()
{
	// create layout with 2 storage images (binding 0 = color, 1 = depth)
	setLayout = lve::LveDescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		//.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build();

	// write descriptor sets (this assumes LveDescriptorWriter::build allocates from descriptorPool)
	for (uint32_t i = 0; i < descriptorSets.size(); ++i) {
		std::array<VkDescriptorImageInfo, 2> imageInfos{};
		imageInfos[0].imageView = swapChain.getMainImageView(i);
		imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL; // must be transitioned before dispatch

		imageInfos[1].imageView = swapChain.getDepthImageView(i);
		imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		lve::LveDescriptorWriter(*setLayout, *descriptorPool)
			.writeImage(0, &imageInfos[0])
			//.writeImage(1, &imageInfos[1])
			.build(descriptorSets[i]); // build should allocate and write
	}
}



void Compute::createShaderModule(std::string source)
{
	std::string sourceCode = lve::LvePipeline::readFile(source);
	auto computeCode = lve::LvePipeline::shaderToSpirV(sourceCode, shaderc_compute_shader, "computeShader", "main");

	VkShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.codeSize = computeCode.size() * sizeof(uint32_t);
	shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(computeCode.data());

	if (vkCreateShaderModule(device.device(), &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline layout!");
}

void Compute::createPipeline()
{
	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName  = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage  = shaderStageInfo;
	pipelineInfo.layout = pipelineLayout;

	if (vkCreateComputePipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline layout!");
}

void Compute::createPipelineLayout()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{setLayout->getDescriptorSetLayout()};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void Compute::transitionImageToGeneral(VkImage image, VkImageAspectFlags aspect)
{
	VkCommandBuffer cmd = device.beginSingleTimeCommands(); // implement helper
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or current layout
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	device.endSingleTimeCommands(cmd); // submit & wait helper
}