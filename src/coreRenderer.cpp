#include "coreRenderer.h"
#include <iostream>

RenderSyncSystem::RenderSyncSystem(RenderBucket &bucket, lve::LveDevice &device,
									lve::LveDescriptorSetLayout& descriptor, LoaderObject& objectLoader) :
renderBucket(bucket), device(device), objectLoader(objectLoader)
{
	pointShadowRenderer = std::make_unique<lve::LvePointShadowRenderer>(device, shadowVert, shadowFrag,
																		descriptor.getDescriptorSetLayout(),
																		RenderBucket::getBindingDescriptionsShadow,
																		RenderBucket::getAttributeDescriptionsShadow);
}

void RenderSyncSystem::drawChildren(entt::entity entity)
{
	auto& data = registry.get<DefaultObjectData>(entity);
	std::string objName = "Object " + std::to_string((uint32_t)entity);

	ImGui::PushID((int)entity);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (data.children.empty())
		flags |= ImGuiTreeNodeFlags_Leaf;
	if (currentEntity == entity)
		flags |= ImGuiTreeNodeFlags_Selected;

	bool open = ImGui::TreeNodeEx(objName.c_str(), flags);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		currentEntity = entity;
		enableEditor = true;
	}

	ImGui::SameLine();
	std::string addBtn = "+##" + std::to_string((uint32_t)entity);
	if (ImGui::Button(addBtn.c_str()))
		createObject(entity);

	if (open)
	{
		for (auto child : data.children)
			if (child != entt::null)
				drawChildren(child);
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void RenderSyncSystem::renderImGuiWindow(VkCommandBuffer commandBuffer, int WIDTH, int HEIGHT)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Objects", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
	ImGui::SetWindowPos(ImVec2(0, 0));

	static bool menuOpen = false;
	if (ImGui::Button("Create Object", ImVec2(WIDTH / 4.f - 15, 20)))
		menuOpen = !menuOpen; // toggle open/close


	static int selectedItem = -1;
	if (menuOpen) {
		ImGui::BeginChild("MenuList", ImVec2(150, 0), true);

		for (size_t i = 0; i < objectLoader.getStructures().size(); i++)
		{
			if (objectLoader.getStructures()[i].parent != -1) continue;
			if (ImGui::Button(objectLoader.getStructures()[i].name.c_str()))
			{
				createObject(entt::null, objectLoader.getStructures()[i].index);
				menuOpen = false;
			}
		}

		ImGui::EndChild();
	}


	auto view = registry.view<DefaultObjectData>();
	for (auto [entity, data] : view.each())
	{
		if (data.parent != entt::null) continue;

		drawChildren(entity);
	}

	ImGui::End();

	if (enableEditor && currentEntity != entt::null)
	{
		ImGui::Begin("Properties", nullptr,
					ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse |
					ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
		ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
		ImGui::SetWindowPos(ImVec2(WIDTH - WIDTH / 4.f, 0));

		auto &data = registry.get<DefaultObjectData>(currentEntity);
		data.dirty = true;

		if (auto *transform = registry.try_get<TransformComponent>(currentEntity))
		{
			ImGui::Text("Transform");
			ImGui::DragFloat3("Position", &transform->translation.x, 0.1f);
			ImGui::DragFloat3("Rotation", &transform->rotation.x, 0.1f);
			ImGui::DragFloat3("Scale", &transform->scale.x, 0.1f);
			transform->dirty = true;
		}

		if (auto *mesh = registry.try_get<MeshComponent>(currentEntity))
		{
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Mesh ID");
			if (ImGui::Button("<", ImVec2(20, 20)) && mesh->materialId > 0)
				mesh->materialId--;
			ImGui::SameLine();
			ImGui::Text(std::to_string(mesh->materialId).c_str());
			ImGui::SameLine();
			if (ImGui::Button(">", ImVec2(20, 20)))
				mesh->materialId++;
		}

		if (auto *light = registry.try_get<lve::PointLightData>(currentEntity))
		{
			ImGui::PushID(2);
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Text("Light");
			ImGui::SameLine();
			if (ImGui::Button("-", ImVec2(20, 20)))
				removeLightComponent(currentEntity);

			ImGui::DragFloat3("Rotation", &light->light.rotation.x, 0.1f);
			ImGui::DragFloat("Inner Cone", &light->light.innerCone, 0.001f, 0, 1);
			ImGui::DragFloat("Outer Cone", &light->light.outerCone, 0.001f, 0, 1);
			ImGui::DragFloat("Intensity", &light->light.intensity, 0.001f, 0, 5);
			ImGui::DragFloat("Specular", &light->light.specular, 0.001f, 0, 5);

			light->update();
			dirtyLight.push_back(currentEntity);

			ImGui::PopID();
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::Button("Delete"))
		{
			deleteObject(currentEntity);
			enableEditor = false;
		}

		ImGui::BeginChild("ComponentList",
						ImVec2(), // size of the child region
						true, // border
						ImGuiWindowFlags_HorizontalScrollbar);

		if (!registry.try_get<lve::PointLightData>(currentEntity))
			if (ImGui::Selectable("Point Light"))
				addLightComponent(currentEntity);

		ImGui::EndChild();

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void RenderSyncSystem::deleteObject(entt::entity entity) {
	auto& data = registry.get<DefaultObjectData>(entity);

	// Remove self from parent’s children list
	if (data.parent != entt::null) {
		auto& parentData = registry.get<DefaultObjectData>(data.parent);
		parentData.children.erase(
			std::remove(parentData.children.begin(), parentData.children.end(), entity),
			parentData.children.end()
		);
	}

	// Recursively delete children
	for (auto child : data.children) {
		if (registry.valid(child))
			deleteObject(child);
	}

	renderBucket.deleteInstance(data.handle);
	registry.destroy(entity);
}


void RenderSyncSystem::createObject(entt::entity parent, int32_t object)
{
	entt::entity e = registry.create();
	TransformComponent& a = registry.emplace<TransformComponent>(e);
	registry.emplace<MeshComponent>(e, objectLoader.getStructures()[object].ID);

	BucketSendData data{};
	data.model = a.mat4();
	data.materialId = objectLoader.getStructures()[object].ID;
	data.entity = e;
	data.parent = parent;
	Handle handle = renderBucket.addInstance(data);

	DefaultObjectData aaAA{};
	aaAA.parent = parent;
	aaAA.handle = handle;
	aaAA.dirty = false;
	registry.emplace<DefaultObjectData>(e, aaAA);
	if (parent != entt::null)
		registry.get<DefaultObjectData>(parent).children.push_back(e);

	for (auto& i : objectLoader.getStructures()[object].childeren)
		createObject(e, i);
}

void RenderSyncSystem::addLightComponent(entt::entity entity)
{
	lve::PointLightData& light = registry.emplace<lve::PointLightData>(entity);

	light.light.innerCone = .95f;
	light.light.outerCone = .9f;
	light.light.intensity = 1;
	light.light.specular = 1;

	light.update();
	light.createShadowMap(*pointShadowRenderer, device, VkExtent2D{2024, 2024});
	pointLigts.push_back(&light);
}

void RenderSyncSystem::removeLightComponent(entt::entity entity)
{
	registry.remove<lve::PointLightData>(entity);
}


void RenderSyncSystem::syncToRenderBucket(VkDescriptorSet& descriptor, lve::LveBuffer& buffer)
{
	auto view = registry.view<DefaultObjectData>();
	for (auto [entity, data] : view.each())
	{
		if (!data.dirty) return;
		if (TransformComponent *transform = registry.try_get<TransformComponent>(currentEntity))
			if (transform->dirty)
				updateTransformRecursive(entity, transform->worldMatrix, true);

		if (lve::PointLightData *light = registry.try_get<lve::PointLightData>(currentEntity))
			if (light->dirty)
			{
				light->update();
				light->updateDescriptorSet(device, descriptor);
				light->updateBuffer(buffer);
			}
	}

	dirtyLight.clear();
}

void RenderSyncSystem::updateAllTransforms()
{
	auto view = registry.view<TransformComponent, DefaultObjectData>();

	// Find root entities (no parent)
	for (auto [entity, transform, meta]: view.each())
		if (meta.parent == entt::null)
			updateTransformRecursive(entity, glm::mat4(1.0f), true);
}

void RenderSyncSystem::updateTransformRecursive(entt::entity entity, const glm::mat4 &parentMatrix, bool parentDirty)
{
	auto &transform = registry.get<TransformComponent>(entity);
	auto& data = registry.get<DefaultObjectData>(entity);

	bool dirty = transform.dirty || parentDirty;

	if (dirty)
	{
		transform.worldMatrix = parentMatrix * transform.mat4();
		transform.dirty = false;

		auto &mesh = registry.get<MeshComponent>(entity);
		renderBucket.get(data.handle)->model = transform.worldMatrix;
		renderBucket.get(data.handle)->materialId = mesh.materialId;
	}

	// Update children
	for (auto child : data.children) {
		updateTransformRecursive(child, transform.worldMatrix, dirty);
	}
}
