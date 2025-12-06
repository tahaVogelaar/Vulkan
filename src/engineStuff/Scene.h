#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "entt.hpp"
#include <vulkan/vulkan.h>
#include "../core/lve_buffer.hpp"
#include "../coreRenderer.h"
#include "../lve_light.h"
#include "../core/lve_descriptors.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imconfig.h"
#include "imgui_internal.h"
#include "imstb_rectpack.h"
#include "imstb_textedit.h"
#include "imstb_truetype.h"

struct SubMesh {
	Handle handle;
	int32_t primitiveID = -1;
	int32_t matID = -1;
};

struct MeshComponent {
	std::vector<SubMesh> subMeshes;
};

struct DefaultObjectData {
	entt::entity parent = entt::null;
	std::vector<entt::entity> children;
	bool dirty = true;
};

class RenderSyncSystem {
public:
	RenderSyncSystem(RenderBucket& bucket, lve::LveDevice& device, lve::LveDescriptorSetLayout& descriptor, LoaderObject& objectLoader);
	~RenderSyncSystem() = default;
	RenderSyncSystem(const RenderSyncSystem &) = delete;
	RenderSyncSystem &operator=(const RenderSyncSystem &) = delete;

	void renderImGuiWindow(VkCommandBuffer commandBuffer, int WIDTH, int HEIGHT);
	void syncToRenderBucket(VkDescriptorSet& descriptor);

	// get stuff
	entt::registry& getRegistery() {return registry; }
	lve::LvePointShadowRenderer& getPointShadowRenderer() const {return *pointShadowRenderer; }

	uint32_t getLightCount() const { return pointLightsList.size(); }

private:
	void updateTransformRecursive(entt::entity entity, VkDescriptorSet& descriptor, const glm::mat4& parentMatrix, bool parentDirty);

	// imgui
	void drawChildren(entt::entity entity);
	entt::entity createObject(entt::entity parent, int32_t structure = -1);
	void addPointLightComponent(entt::entity entity);
	void removeLightComponent(entt::entity entity);
	void deleteObject(entt::entity entity);

	// main stuff
	entt::registry registry;
	RenderBucket& renderBucket;
	lve::LveDevice& device;
	LoaderObject& objectLoader;

	// point light
	uint32_t LIGHT_COUNT = 16;
	std::unique_ptr<lve::LveBuffer> pointLightBuffer;
	std::vector<entt::entity> pointLightEntitys;
	std::vector<lve::PointLight> pointLightsList;
	std::unique_ptr<lve::LvePointShadowRenderer> pointShadowRenderer;
	std::string shadowVert = "/home/taha/CLionProjects/untitled4/shaders/shadow.vert", shadowFrag = "/home/taha/CLionProjects/untitled4/shaders/shadow.frag";

	bool enableEditor = false;
	entt::entity currentEntity = entt::null, pastCurrentEntity = entt::null;
};