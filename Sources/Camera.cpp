#include "Camera.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Camera::Camera(const float fov, const glm::vec2 &windowSize, const float near, const float far, const glm::vec3 &vectorUp)
:	Object("Camera")
{
	m_View = glm::lookAt(m_Position, glm::vec3(0.0f), vectorUp);

	m_Projection = glm::perspective(fov, windowSize.x / windowSize.y, near, far);
	m_Projection[1][1] *= -1; // invert Y axis
}

//----------------------------------------------------------------

void	Camera::Translate(const glm::vec3 &vector)
{
	m_Position += vector;

	UpdateView();
}

//----------------------------------------------------------------

void	Camera::UpdateView() 
{ 
	m_View = glm::lookAt(m_Position, m_Position - (m_Rotation * glm::vec3(0.0f, 0.0f, 1.0f)), m_Rotation * glm::vec3(0.0f, 1.0f, 0.0f));

	m_InvView = m_View;
	m_InvView[3] *= -1.f;
	m_InvView[3][3] = 1.f;
	m_InvView = glm::transpose(glm::inverse(m_InvView));
}

//----------------------------------------------------------------

LIGHTLYY_END
