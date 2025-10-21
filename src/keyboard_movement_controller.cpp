#include "keyboard_movement_controller.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <iostream>
#include <limits>
#include <optional>

#include "lve_camera.hpp"


namespace lve {
	void Camera::update(GLFWwindow *window, float dt, GlobalUbo &ubo)
	{
		glm::vec3 position = ubo.camPos, rotation = ubo.rotation;

		if (glfwGetMouseButton(window, 1)) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			double dx, dy;
			static double lastX = 0, lastY = 0;

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			firstMouse = false;
			if (firstMouse) {
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			dx = xpos - lastX;
			dy = ypos - lastY;
			lastX = xpos;
			lastY = ypos;

			rotation.y += dx * lookSpeed;
			rotation.x += dy * lookSpeed;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		// arrow for rotation
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS)
			rotation.x -= lookSpeed * .3;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS)
			rotation.x += lookSpeed * .3;
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS)
			rotation.y += lookSpeed * .3;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS)
			rotation.y -= lookSpeed * .3;

		// clamp rotation
		float yaw = rotation.y;
		float pitch = rotation.x;

		// Clamp pitch to avoid flipping
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 newForward;
		newForward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newForward.y = sin(glm::radians(pitch));
		newForward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		glm::vec3 forward = glm::normalize(newForward);
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
		{
			position += forward * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
		{
			position -= forward * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
		{
			position -= right * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
		{
			position += right * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
		{
			position -= up * moveSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
		{
			position += up * moveSpeed * dt;
		}

		LveCamera lveCam;

		int WX, WY;
		glfwGetWindowSize(window, &WY, &WX);
		aspect = static_cast<float>(WY) / static_cast<float>(WX);

		lveCam.setPerspectiveProjection(fov, aspect, nearPlane, farPlane);
		lveCam.setViewDirection(position, forward, up);
		ubo.proj = lveCam.getProjection();
		ubo.view = lveCam.getView();
		ubo.projView = ubo.proj * ubo.view;
		ubo.camPos = position;

		ubo.rotation = rotation;
		ubo.forward = forward;
	}
};	// namespace lve
