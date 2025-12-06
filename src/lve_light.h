#pragma once
#include "core/lve_device.hpp"
#include "core/lve_buffer.hpp"
#include "lve_shadowMap.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace lve {
	class LveSpotLight {
	public:
		LveSpotLight(LveDevice& device, const PointLightData& data);
		~LveSpotLight();

		LveSpotLight(const LveSpotLight &) = delete;
		LveSpotLight &operator=(const LveSpotLight &) = delete;

		// settings
		PointLightData data;
		bool enableShadow = false;
		std::unique_ptr<ShadowMap> shadowMap;

		void updateLight();
		void updateShadowMap();

	private:
		void createBuffer();
		void createShadowMap();

		// stuff
		std::unique_ptr<LveBuffer> pointLightBuffer;

		// other stuff
		LveDevice& device;
	};
}
