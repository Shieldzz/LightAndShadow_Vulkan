#include "Window.h"

#include "Device.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Window::Window()
:	m_ApiWindow(nullptr),
	m_PreviousMousePosition({ 0.f, 0.f })
{
	m_CameraX = 0.f;
	m_CameraY = 0.f;
}

//----------------------------------------------------------------

Window::~Window()
{
	if (m_ApiWindow != nullptr)
		glfwDestroyWindow(m_ApiWindow);

	m_ApiWindow = nullptr;

	glfwTerminate();
}

//----------------------------------------------------------------

bool	Window::Setup(const char* applicationName)
{
	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_ApiWindow = glfwCreateWindow(1280, 720, applicationName, NULL, NULL); // TODO: manage window size and resize
	if (m_ApiWindow == nullptr)
	{
		glfwTerminate();
		return false;
	}

	return true;
}

//----------------------------------------------------------------

void	Window::RetrieveInputs()
{
	// TODO: make correct class Input
	double		mouseX;
	double		mouseY;
	glfwGetCursorPos(m_ApiWindow, &mouseX, &mouseY);

	glm::mat4	camView = m_Camera->GetView();
	glm::vec3	right = glm::vec3(camView[0][0], camView[0][1], camView[0][2]);
	right.z *= -1.f;
	glm::vec3	forward = glm::vec3(camView[2][0], camView[2][1], camView[2][2]);
	forward.z *= -1.f;

	const float	deltaTime = Device::m_Device->GetDeltaTime();

	if (glfwGetKey(m_ApiWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		m_Camera->Translate(-right * (2.f * deltaTime));
	}
	if (glfwGetKey(m_ApiWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		m_Camera->Translate(right * (2.f * deltaTime));
	}
	if (glfwGetKey(m_ApiWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		m_Camera->Translate(forward * (2.f * deltaTime));
	}
	if (glfwGetKey(m_ApiWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		m_Camera->Translate(-forward * (2.f * deltaTime));
	}
	if (glfwGetKey(m_ApiWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		m_Camera->Translate({ 0.f, 2.f * deltaTime, 0.f });
	}
	if (glfwGetKey(m_ApiWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		m_Camera->Translate({ 0.f, -2.f * deltaTime, 0.f });
	}

	if (glfwGetMouseButton(m_ApiWindow, 1) == GLFW_PRESS)
	{
		glm::vec2	mouseDir = glm::vec2(mouseX, mouseY) - m_PreviousMousePosition;

		m_CameraX += -mouseDir.x * deltaTime * 0.25f;
		m_CameraY += -mouseDir.y * deltaTime * 0.25f;

		glm::vec3 eulerAngles = glm::vec3(m_CameraY, 0.f, m_CameraX);
		m_Camera->SetRotation(glm::toQuat(glm::orientate3(eulerAngles)));
	}

	m_PreviousMousePosition = { static_cast<float>(mouseX), static_cast<float>(mouseY) };

	glfwPollEvents();
}

//----------------------------------------------------------------

bool	Window::IsClosed() const
{
	return glfwWindowShouldClose(m_ApiWindow);
}

//----------------------------------------------------------------

HWND	Window::GetWindowHandle() const
{
	return glfwGetWin32Window(m_ApiWindow);
}

//----------------------------------------------------------------

LIGHTLYY_END
