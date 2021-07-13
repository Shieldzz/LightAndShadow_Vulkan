#pragma once

//----------------------------------------------------------------

#include "Initializers.h"

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

#include "Camera.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

class Window
{
public:
	Window();
	~Window();

	bool	Setup(const char* applicationName);

	void	RetrieveInputs();

	bool	IsClosed() const;

	const GLFWwindow*	GetApiWindow() const { return m_ApiWindow; }
	HWND				GetWindowHandle() const;

	Camera	*m_Camera;

private:
	GLFWwindow	*m_ApiWindow;

	glm::vec2	m_PreviousMousePosition;

	float		m_CameraX;
	float		m_CameraY;
};

//----------------------------------------------------------------

LIGHTLYY_END
