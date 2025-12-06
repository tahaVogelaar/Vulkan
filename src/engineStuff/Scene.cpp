#include "Scene.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

RenderSyncSystem::RenderSyncSystem(RenderBucket &bucket, lve::LveDevice &device,
									lve::LveDescriptorSetLayout& descriptor, LoaderObject& objectLoader) :
renderBucket(bucket), device(device), objectLoader(objectLoader)
{
	pointShadowRenderer = std::make_unique<lve::LvePointShadowRenderer>(device, shadowVert, shadowFrag,
																		descriptor.getDescriptorSetLayout(),
																		RenderBucket::getBindingDescriptionsShadow,
																		RenderBucket::getAttributeDescriptionsShadow);

	pointLightBuffer = std::make_unique<lve::LveBuffer>(
		device, sizeof(lve::PointLight) * LIGHT_COUNT, 1,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	pointLightBuffer->map();
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

std::string formatFloat2(float value) {
	// round to 2 decimal places
	value = std::roundf(value * 100.0f) / 100.0f;
	std::ostringstream oss;
	oss << value;
	return oss.str(); // trailing zeros dropped automatically
}

void RenderSyncSystem::renderImGuiWindow(VkCommandBuffer commandBuffer, int WIDTH, int HEIGHT)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin(std::to_string(registry.view<DefaultObjectData>().size()).c_str(), nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize(ImVec2(WIDTH / 4.f, HEIGHT));
	ImGui::SetWindowPos(ImVec2(0, 0));

	static bool menuOpen = false;
	if (ImGui::Button("Create Object", ImVec2(WIDTH / 4.f - 15, 20)))
		menuOpen = !menuOpen; // toggle open/close


	static int selectedItem = -1;
	if (menuOpen)
	{
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

			// rotation
			glm::vec3 rotationDegrees = glm::degrees(glm::eulerAngles(transform->quat));
			if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotationDegrees), 1.0f))
			{
				glm::vec3 rotationRadians = glm::radians(rotationDegrees);
				transform->quat = glm::quat(rotationRadians);
			}

			ImGui::DragFloat3("Scale", &transform->scale.x, 0.1f);
			transform->dirty = true;
		}

		if (auto *light = registry.try_get<lve::PointLightData>(currentEntity))
		{
			data.dirty = true;
			light->dirty = true;
			ImGui::PushID(2);
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Text("Light");
			ImGui::SameLine();
			if (ImGui::Button("-", ImVec2(20, 20)))
				removeLightComponent(currentEntity);

			ImGui::DragFloat("Inner Cone", &light->light.innerCone, 0.001f, 0, light->light.outerCone);
			ImGui::DragFloat("Outer Cone", &light->light.outerCone, 0.001f, light->light.innerCone, 1);
			ImGui::DragFloat("Range", &light->light.range, 0.001f, 0, 200);
			ImGui::DragFloat("Intensity", &light->light.intensity, 0.001f, 0, 5);
			std::string posText = "Position:\n" + std::to_string(light->light.position.x) + '\n' + std::to_string(light->light.position.y) + '\n' + std::to_string(light->light.position.z);
			ImGui::Text(posText.c_str());

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
				addPointLightComponent(currentEntity);

		ImGui::EndChild();

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void RenderSyncSystem::deleteObject(entt::entity entity) {
	if (!registry.valid(entity))
		return;

	auto& data = registry.get<DefaultObjectData>(entity);

	// Copy children first, since we'll be modifying registry during recursion
	auto childrenCopy = data.children;

	// Remove self from parent’s child list
	if (data.parent != entt::null && registry.valid(data.parent)) {
		auto& parentData = registry.get<DefaultObjectData>(data.parent);
		parentData.children.erase(
			std::remove(parentData.children.begin(), parentData.children.end(), entity),
			parentData.children.end()
		);
	}

	// Recursively delete children (safe from modification)
	for (auto child : childrenCopy) {
		if (registry.valid(child))
			deleteObject(child);
	}

	// if entity has mesh, destroy it
	MeshComponent* mesh = registry.try_get<MeshComponent>(entity);
	if (mesh)
		for (int i = 0; i < mesh->subMeshes.size(); ++i)
			renderBucket.deleteInstance(mesh->subMeshes[i].handle);

	// Destroy self
	registry.destroy(entity);
}

entt::entity RenderSyncSystem::createObject(entt::entity parent, int32_t structure)
{
	// create entity
	entt::entity e = registry.create();

	// give parent entity
	Structure& struc = objectLoader.getStructures()[structure];
	DefaultObjectData& data = registry.emplace<DefaultObjectData>(e);
	data.parent = parent;

	// recursively create children
	for (auto& i : struc.childeren)
	{
		entt::entity c = createObject(e, i);
		data.children.push_back(c);
	}

	if (structure == -1) return e;

	// transform stuff
	TransformComponent& transform = registry.emplace<TransformComponent>(e);
	transform.translation = struc.transform.translation;
	transform.quat = struc.transform.quat;
	transform.scale = struc.transform.scale;
	transform.mat4();


	// create the mesh component and its sub meshes
	if (struc.meshID != -1)
	{
		MeshComponent& mesh = registry.emplace<MeshComponent>(e);

		// create the bucket and entity data
		auto prims = objectLoader.getMeshes()[struc.meshID].primitives;
		for (int i = 0; i < prims.size(); ++i)
		{
			Primitive& prim = objectLoader.getPrimitives()[prims[i]];

			// send data
			BucketSendData data{};
			data.primitiveID = prim.primitiveID;
			data.model = transform.worldMatrix;
			data.entity = e;
			data.parent = parent;
			data.materialID = prim.matID;
			Handle handle = renderBucket.addInstance(data);

			// create submesh
			SubMesh subMesh;
			subMesh.primitiveID = prim.primitiveID;
			subMesh.matID = prim.matID;
			subMesh.handle = handle;

			mesh.subMeshes.push_back(subMesh);
		}
	}

	return e;
}

void RenderSyncSystem::addPointLightComponent(entt::entity entity)
{
	lve::PointLightData& light = registry.emplace<lve::PointLightData>(entity);

	light.index = pointLightsList.size();
	light.light.innerCone = .95f;
	light.light.outerCone = .9f;
	light.light.intensity = 1.f;
	light.light.range = 100;

	light.create(device);
	light.update();
	//light.createShadowMap(*pointShadowRenderer, device, VkExtent2D{2024, 2024});

	pointLightEntitys.push_back(entity);
}

void RenderSyncSystem::removeLightComponent(entt::entity entity)
{
	registry.remove<lve::PointLightData>(entity);
}


void RenderSyncSystem::syncToRenderBucket(VkDescriptorSet& descriptor)
{
	pointLightsList.clear();

	auto view = registry.view<DefaultObjectData>();
	for (auto [entity, data] : view.each())
	{
		if (!data.dirty || data.parent != entt::null) continue;

		TransformComponent& transform = registry.get<TransformComponent>(entity);
		if (transform.dirty)
			updateTransformRecursive(entity, descriptor, transform.worldMatrix, true);

		if (lve::PointLightData *light = registry.try_get<lve::PointLightData>(entity))
			if (light->dirty)
			{
				light->light.position = transform.translation;
				light->light.rotation = LoaderObject::quaternionToVec3(transform.quat);
				light->update();
			}
	}

	if (pointLightsList.size() == 0) return;
	pointLightBuffer->writeToBuffer(pointLightsList.data(), sizeof(lve::PointLight));

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = pointLightBuffer->getBuffer();
	bufferInfo.range = sizeof(lve::PointLight) * pointLightsList.size();
	bufferInfo.offset = 0;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.dstSet = descriptor;
	write.dstBinding = 2;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device.device(), 1, &write, 0, nullptr);
}

void RenderSyncSystem::updateTransformRecursive(entt::entity entity, VkDescriptorSet& descriptor, const glm::mat4 &parentMatrix, bool parentDirty)
{
	auto &transform = registry.get<TransformComponent>(entity);
	auto& data = registry.get<DefaultObjectData>(entity);

	lve::PointLightData *light = registry.try_get<lve::PointLightData>(entity);
	if (light)
		pointLightsList.push_back(light->light);

	bool dirty = parentDirty || data.dirty || light;

	if (transform.dirty)
	{
		transform.worldMatrix = parentMatrix * transform.mat4();
		transform.dirty = false;

		if (MeshComponent *mesh = registry.try_get<MeshComponent>(entity))
		{
			for (int i = 0; i < mesh->subMeshes.size(); ++i)
			{
				renderBucket.get(mesh->subMeshes[i].handle)->model = transform.worldMatrix;
				renderBucket.get(mesh->subMeshes[i].handle)->primId = mesh->subMeshes[i].primitiveID;
				renderBucket.get(mesh->subMeshes[i].handle)->materialID = mesh->subMeshes[i].matID;
			}
		}

		// point light
	}
}
