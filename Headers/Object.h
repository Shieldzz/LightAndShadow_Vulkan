#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"

#include "Utility.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

enum class OBJECT_TYPE
{
	Mesh = 0,
	DirectionalLight =  1,
	PointLight = 2,
	SpotLight = 3
};

//----------------------------------------------------------------

class Object
{
public:
	// Constructor
	Object(const std::string &name);
	virtual ~Object() {};

	virtual void		Translate(const glm::vec3 &vector);

	// Getter
	glm::vec3			GetPosition() const {	return m_Position; };
	glm::quat			GetRotation() const { return m_Rotation; };
	glm::vec3			GetScale()	const { return m_Scale; };

	glm::vec3			GetEulerAngles() const { return m_EulerAngles; }

	glm::mat4&			GetModel() { return m_Model; };

	const std::string&	GetName() const { return m_Name; }

	OBJECT_TYPE			GetType() const { return m_Type; }

	// Setter
	virtual void		SetPosition(const glm::vec3 &position);
	virtual void		SetRotation(const glm::quat &rotation);
	virtual void		SetScale(const glm::vec3 &scale);

	void				SetEulerAngles(const glm::vec3 &eulerAngles) { m_EulerAngles = eulerAngles; }

	void				SetName(std::string newName) { m_Name = newName; }

protected:
	void				UpdateMatrix();

	glm::vec3	m_Position;
	glm::quat	m_Rotation;
	glm::vec3	m_Scale;

	glm::vec3	m_EulerAngles;

	glm::mat4	m_Model;

	std::string	m_Name;

	OBJECT_TYPE	m_Type;
}; // class Object

//----------------------------------------------------------------

LIGHTLYY_END
