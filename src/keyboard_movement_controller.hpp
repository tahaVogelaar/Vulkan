#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "lve_window.hpp"

struct GlobalUbo {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 projView;
	alignas(16) glm::vec3 camPos;
	alignas(16) glm::vec3 rotation;
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
		};

		void update(GLFWwindow *window, float dt, GlobalUbo &ubo);

		KeyMappings keys{};
		float moveSpeed{3.f};
		float lookSpeed{0.1f};
		float fov = 70.f, aspect = 1.f, nearPlane = .1f, farPlane = 1000.f;
	};
} // namespace lve
