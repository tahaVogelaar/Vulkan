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
		glm::vec3 position = glm::vec3(ubo.camPos.x, ubo.camPos.y, ubo.camPos.z);
		glm::vec3 rotation = glm::vec3(ubo.rotation.x, ubo.rotation.y, ubo.rotation.z);

		if (glfwGetMouseButton(window, 1) && firstMouse)
		{
			int WX, WY; glfwGetWindowSize(window, &WY, &WX);
			double centerX = WY / 2;
			double centerY = WX / 2;

			glfwSetCursorPos(window, centerX, centerY);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			lastX = centerX;     // <<< IMPORTANT
			lastY = centerY;     // <<< IMPORTANT

			firstMouse = false;
			return;
		}

		else if (glfwGetMouseButton(window, 1)) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			double dx, dy;

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			dx = xpos - lastX;
			dy = ypos - lastY;
			lastX = xpos;
			lastY = ypos;

			rotation.y += dx * lookSpeed;
			rotation.x += dy * lookSpeed;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		if (glfwGetMouseButton(window, 1) == GLFW_RELEASE) firstMouse = true;

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
		float pitch = -rotation.x;

		// Clamp pitch to avoid flipping
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 newForward;
		newForward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newForward.y = sin(glm::radians(pitch));
		newForward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		glm::vec3 forward = glm::normalize(newForward);
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 up = glm::normalize(glm::cross(right, forward));


		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			currentSpeed = fastSpeed;
		else
			currentSpeed = normalSpeed;

		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
		{
			position += forward * currentSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
		{
			position -= forward * currentSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
		{
			position -= right * currentSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
		{
			position += right * currentSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
		{
			position += up * currentSpeed * dt;
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
		{
			position -= up * currentSpeed * dt;
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
		ubo.camPos = glm::vec4(position, 0);

		ubo.rotation = glm::vec4(rotation, 0);
		ubo.forward = glm::vec4(forward, 0);
	}
};	// namespace lve
