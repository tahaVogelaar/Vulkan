#include "sunShadow.h"

SunObject::SunObject(lve::LveDevice& device) :
device(device)
{
	shadowMap = std::make_unique<lve::ShadowMap>(
		device, shadowResolution);
}

void SunObject::update(glm::mat4& invViewProj)
{
	// Find the main camera’s frustum corners in world space
	std::vector<glm::vec3> frustumCorners;
	for (int x = 0; x < 2; x++)
		for (int y = 0; y < 2; y++)
			for (int z = 0; z < 2; z++) {
				glm::vec4 corner = invViewProj * glm::vec4(2.0f*x - 1.0f, 2.0f*y - 1.0f, 2.0f*z - 1.0f, 1.0f);
				corner /= corner.w;
				frustumCorners.push_back(glm::vec3(corner));
			}

	// Find the frustum center
	glm::vec3 center(0.0f);
	for (auto& c : frustumCorners)
		center += c;
	center /= frustumCorners.size();

	// Find the frustum center
	glm::mat4 lightView = glm::lookAt(center - sunDir * distance, center, glm::vec3(0,1,0));

	// Transform frustum corners into light space
	glm::vec3 min(FLT_MAX), max(-FLT_MAX);
	for (auto& c : frustumCorners) {
		glm::vec4 trf = lightView * glm::vec4(c, 1.0);
		min = glm::min(min, glm::vec3(trf));
		max = glm::max(max, glm::vec3(trf));
	}
	glm::mat4 lightProj = glm::ortho(min.x, max.x, min.y, max.y, -max.z, -min.z);
	glm::mat4 lightSpaceMatrix = lightProj * lightView; // Light-space matrix
}

void SunObject::beginDraw()
{
	//shadowMap->beginRender();
}
