#pragma once

#include "Initializers.h"
#include "glm/glm.hpp"

#include "Object.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

#undef near
#undef far

//----------------------------------------------------------------

class Camera : public Object
{
public:
	// Constructor
	Camera(const float fov, const glm::vec2 &windowSize, const float near, const float far, const glm::vec3 &vectorUp);
	virtual ~Camera() {};

	virtual void	Translate(const glm::vec3 &vector) override;

	void			UpdateView();

	// Getter
	glm::mat4		GetView() const { return m_View; };
	glm::mat4		GetInvView() const { return m_InvView; }
	glm::mat4		GetProjection() const { return m_Projection; };

	// Setter
	virtual void	SetPosition(const glm::vec3 &position) override { m_Position = position; UpdateView(); };
	virtual void	SetRotation(const glm::quat &rotation) override { m_Rotation = rotation; UpdateView(); };
	virtual void	SetScale(const glm::vec3 &scale) override { m_Scale = scale; };

private:
	glm::mat4	m_View;
	glm::mat4	m_InvView;

	glm::mat4	m_Projection;
};

//----------------------------------------------------------------

LIGHTLYY_END
