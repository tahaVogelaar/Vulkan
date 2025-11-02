#include "coreRenderer.h"

RenderSyncSystem::RenderSyncSystem(RenderBucket &bucket, lve::LveDevice &device,
									lve::LveDescriptorSetLayout& descriptor) : renderBucket(bucket),
	device(device)
{
	pointShadowRenderer = std::make_unique<lve::LvePointShadowRenderer>(device, shadowVert, shadowFrag,
																		descriptor.getDescriptorSetLayout(),
																		RenderBucket::getBindingDescriptionsShadow,
																		RenderBucket::getAttributeDescriptionsShadow);
}

void RenderSyncSystem::renderImGuiWindow(VkCommandBuffer commandBuffer, int WIDTH, int HEIGHT)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// object list

	ImGui::Begin("Objects", nullptr,
				ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
	ImGui::SetWindowPos(ImVec2(0, 0));

	if (ImGui::Button("Create Object", ImVec2(WIDTH / 4. - 15, 20)))
		createObject();

	int count = 0;
	auto view = registry.view<TransformComponent>();
	for (auto [entity, transform]: view.each())
	{
		std::string objName = "Object " + std::to_string(static_cast<unsigned long long>(entity));
		if (ImGui::Button(objName.c_str(), ImVec2(WIDTH / 4.f - 15, 20)))
		{
			if (currentEntity == entity)
				enableEditor = !enableEditor;
			else
			{
				currentEntity = entity;
				enableEditor = true;
			}
		}
		count++;
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

void RenderSyncSystem::deleteObject(entt::entity entity)
{
	auto &data = registry.get<DefaultObjectData>(currentEntity);
	renderBucket.deleteInstance(data.handle);
	registry.destroy(entity);
}


void RenderSyncSystem::createObject()
{
	entt::entity e = registry.create();
	TransformComponent& a = registry.emplace<TransformComponent>(e);
	registry.emplace<MeshComponent>(e, 0);

	BucketSendData data{};
	data.model = a.mat4();
	data.materialId = 0;
	data.entity = static_cast<entt::entity>(e);
	data.parent = static_cast<entt::entity>(entt::null);
	Handle handle = renderBucket.addInstance(data);

	registry.emplace<DefaultObjectData>(e, entt::null, handle, false);
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

void RenderSyncSystem::updateTransforms()
{
	auto view = registry.view<TransformComponent, DefaultObjectData>();

	// Find root entities (no parent)
	for (auto [entity, transform, meta]: view.each())
	{
		if (meta.parent == entt::null)
		{
			updateTransformRecursive(entity, glm::mat4(1.0f), true);

			auto &mesh = registry.get<MeshComponent>(entity);
			renderBucket.get(meta.handle)->model = transform.worldMatrix;
			renderBucket.get(meta.handle)->materialId = mesh.materialId;
		}
	}
}

void RenderSyncSystem::updateTransformRecursive(entt::entity entity, const glm::mat4 &parentMatrix, bool parentDirty)
{
	auto &transform = registry.get<TransformComponent>(entity);

	bool dirty = transform.dirty || parentDirty;

	if (dirty)
	{
		transform.worldMatrix = parentMatrix * transform.mat4();
		transform.dirty = false;
	}

	// Update children
	auto view = registry.view<DefaultObjectData>();
	for (auto [child, cmeta]: view.each())
	{
		if (cmeta.parent == entity)
		{
			updateTransformRecursive(child, transform.worldMatrix, dirty);
		}
	}
}
