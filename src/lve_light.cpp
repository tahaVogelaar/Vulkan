#include "lve_light.h"

namespace lve {
	LveSpotLight::LveSpotLight(LveDevice &device, const PointLightData &data) :
	device(device)
	{

	}

	void LveSpotLight::createShadowMap()
	{

	}

	void LveSpotLight::createBuffer()
	{
		pointLightBuffer = std::make_unique<LveBuffer>(
			device, sizeof(PointLightData), 1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		pointLightBuffer->map();
	}
}