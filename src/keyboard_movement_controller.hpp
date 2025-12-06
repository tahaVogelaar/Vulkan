#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/lve_window.hpp"

struct GlobalUbo {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 projView;
	glm::vec4 camPos;
	glm::ivec4 lightData;
	glm::vec4 rotation;
	glm::vec4 forward;
};

namespace lve {
	class Camera {
	public:
		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_SPACE;
			int moveDown = GLFW_KEY_LEFT_CONTROL;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
		};

		void update(GLFWwindow *window, float dt, GlobalUbo &ubo);

		float normalSpeed = 2, fastSpeed = 10, currentSpeed = normalSpeed;
		KeyMappings keys{};
		float lookSpeed{0.1f};
		float fov = 70.f, aspect = 1.f, nearPlane = .1f, farPlane = 1000.f;
		bool firstMouse = true;
		double lastX, lastY;
	};
} // namespace lve
