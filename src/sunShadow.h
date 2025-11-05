#include "lve_shadowMap.h"
#include "lve_frame_buffer.h"

class SunObject {
public:
	SunObject(lve::LveDevice& device);
	~SunObject();

	void update(glm::mat4& invViewProj);
	void beginDraw();
	void endDraw();

private:
	float distance = 50;
	glm::vec3 sunDir;

	std::unique_ptr<lve::LveFrameBuffer> frameBuffer;
	VkExtent2D shadowResolution{2024, 2024};
	std::unique_ptr<lve::ShadowMap> shadowMap;
	lve::LveDevice& device;
};