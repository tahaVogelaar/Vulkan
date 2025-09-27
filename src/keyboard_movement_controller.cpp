#include "keyboard_movement_controller.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <iostream>
#include <limits>

namespace lve {
	void Camera::update(GLFWwindow *window, float dt, GlobalUbo &ubo)
	{
		glm::vec3 rotationRad = glm::radians(ubo.rotation);

		// Calculate forward, right, and up directions from rotation
		glm::vec3 forward = glm::vec3(
			cos(rotationRad.y) * cos(rotationRad.x),
			sin(rotationRad.x),
			sin(rotationRad.y) * cos(rotationRad.x)
		);
		forward = glm::normalize(forward);

		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));


		if (glfwGetMouseButton(window, 1))
		{
			int xSize, ySize;
			double xpos, ypos;

			glfwGetWindowSize(window, &xSize, &ySize);
			glfwGetCursorPos(window, &xpos, &ypos);
			glfwSetCursorPos(window, xSize / 2, ySize / 2);
			double chanceX = xpos - xSize / 2;
			double chanceY = ypos - ySize / 2;

			ubo.rotation.y += chanceX * 10;
			ubo.rotation.x += -chanceY * 10;

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		} else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// clamp camera rotation
		if (ubo.rotation.x > 89.f) ubo.rotation.x = 89.f;
		if (ubo.rotation.x < -89.f) ubo.rotation.x = -89.f;


		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
			ubo.camPos += forward * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
			ubo.camPos -= forward * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
			ubo.camPos -= right * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
			ubo.camPos += right * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
			ubo.camPos += up * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
			ubo.camPos -= up * moveSpeed * dt;
		}

		glm::mat4 rot = glm::mat4(1.0f);
		rot = glm::rotate(rot, glm::radians(ubo.rotation.y), glm::vec3(0.f, 1.f, 0.f)); // yaw
		rot = glm::rotate(rot, glm::radians(ubo.rotation.x), glm::vec3(1.f, 0.f, 0.f)); // pitch

		glm::vec3 direction = glm::normalize(rot * glm::vec4(0.f, 0.f, -1.f, 0.f));
		ubo.view = glm::lookAt(ubo.camPos, ubo.camPos + direction, glm::vec3(0.f, 1.f, 0.f));

		ubo.proj = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
		ubo.proj[1][1] *= -1; // Flip Y for Vulkan

		// --- Combined matrix
		ubo.projView = ubo.proj * ubo.view;

	}
} // namespace lve
