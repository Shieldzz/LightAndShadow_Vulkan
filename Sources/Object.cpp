#include "Object.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

Object::Object(const std::string &name)
:	m_Name(name)
{
	m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	m_Scale = glm::vec3(1.0f, 1.0f, 1.0f);

	m_EulerAngles = glm::vec3(0.f, 0.f, 0.f);

	m_Model = glm::mat4(1.f);
}

//----------------------------------------------------------------

void	Object::Translate(const glm::vec3 &vector)
{
	m_Position += vector;

	UpdateMatrix();
}

//----------------------------------------------------------------

void	Object::SetPosition(const glm::vec3 &position)
{
	m_Position = position;

	UpdateMatrix();
}

//----------------------------------------------------------------

void	Object::SetRotation(const glm::quat &rotation)
{
	m_Rotation = rotation;

	UpdateMatrix();
}

//----------------------------------------------------------------

void	Object::SetScale(const glm::vec3 &scale)
{
	m_Scale = scale;

	UpdateMatrix();
}

//----------------------------------------------------------------

void	Object::UpdateMatrix()
{
	m_Model = glm::mat4(1.f);
	m_Model = glm::scale(m_Model, m_Scale);
	m_Model = glm::translate(m_Model, m_Position);
	m_Model *= glm::toMat4(m_Rotation);
}

//----------------------------------------------------------------

LIGHTLYY_END
