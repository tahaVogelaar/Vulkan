#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "entt.hpp"
#include <vulkan/vulkan.h>
#include "lve_buffer.hpp"
#include "Mesh.h"
#include "lve_light.h"
#include "lve_descriptors.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imconfig.h"
#include "imgui_internal.h"
#include "imstb_rectpack.h"
#include "imstb_textedit.h"
#include "imstb_truetype.h"

struct InstanceData {
	glm::mat4 model;
	uint32_t materialId;
	uint32_t _pad[3];
};

struct MeshComponent {
	uint32_t materialId = 0;
};

struct DefaultObjectData {
	entt::entity parent = entt::null;
	std::vector<entt::entity> children;
	Handle handle;
	bool dirty = true;
};

class RenderSyncSystem {
public:
	RenderSyncSystem(RenderBucket& bucket, lve::LveDevice& device, lve::LveDescriptorSetLayout& descriptor);
	~RenderSyncSystem() = default;

	void renderImGuiWindow(VkCommandBuffer commandBuffer, int WIDTH, int HEIGHT);
	void syncToRenderBucket(VkDescriptorSet& descriptor, lve::LveBuffer& buffer);
	void updateAllTransforms();

	// get stuff
	entt::registry& getRegistery() {return registry; }
	lve::LvePointShadowRenderer& getPointShadowRenderer() const {return *pointShadowRenderer; }
	lve::PointLightData& getLightIndex(uint32_t index) { return *pointLigts[index]; }
	std::vector<lve::PointLightData*>& getPointLights() {return pointLigts;}

private:
	void updateTransformRecursive(entt::entity entity, const glm::mat4& parentMatrix, bool parentDirty);

	// imgui
	void drawChildren(entt::entity entity);
	void createObject(entt::entity parent);
	void addLightComponent(entt::entity entity);
	void removeLightComponent(entt::entity entity);
	void deleteObject(entt::entity entity);

	// main stuff
	entt::registry registry;
	RenderBucket& renderBucket;
	lve::LveDevice& device;

	// dirty bucket
	std::vector<entt::entity> dirtyLight;

	// light
	std::vector<lve::PointLightData*> pointLigts;
	std::unique_ptr<lve::LvePointShadowRenderer> pointShadowRenderer;
	std::string shadowVert = "/home/taha/CLionProjects/untitled4/shaders/shadow.vert", shadowFrag = "/home/taha/CLionProjects/untitled4/shaders/shadow.frag";


	bool enableEditor = false;
	entt::entity currentEntity = entt::null, pastCurrentEntity = entt::null;
};