#pragma once

#include "Object.h"

//----------------------------------------------------------------

LIGHTLYY_BEGIN

//----------------------------------------------------------------

struct LightData
{
	glm::vec4	m_Color;
	glm::vec4	m_Position;
	glm::vec4	m_Direction;

	float		m_Radius;
	float		m_Angle;
	float		m_Intensity;
	float		m_Attenuation;
	alignas (16)	uint32_t	m_Type;
};

//----------------------------------------------------------------

struct UBOLights
{
	LightData	m_lights[4];
	uint32_t	m_count;
};

//----------------------------------------------------------------

class Light : public Object
{
public:
	// Constructor
	Light(const std::string &name);
	Light(const float luminousFlow, const std::string &name, LightData data)
	:	Object(name)
	{
		m_Data = data;
		m_Direction = glm::vec4(0, 0, 1, 0); // origin dir
		m_LuminousFlow = luminousFlow;
		m_Type = static_cast<OBJECT_TYPE>(data.m_Type);
		SetPosition(data.m_Position);
	}
	~Light() override {};

	// getters
	glm::vec4		GetColor() const { return m_Data.m_Color; }
	float			GetRadius() const { return m_Data.m_Radius; };
	float			GetAngle() const { return m_Data.m_Angle; };
	int				GetType() const { return m_Data.m_Type; };
	float			GetAttenuation() const { return m_Data.m_Attenuation; };
	float			GetIntensity() const { return m_Data.m_Intensity; };
	LightData		GetData() {	return m_Data; };
	glm::vec4		GetDirection() const { return m_Direction; }

	// Setter
	virtual void	SetPosition(const glm::vec3 &position) override 
	{
		Object::SetPosition(position);

		m_Data.m_Position = glm::vec4(m_Position.x, m_Position.y, m_Position.z, 1);
	};

	virtual void	SetRotation(const glm::quat &rotation) override 
	{ 
		Object::SetRotation(rotation);

		m_Data.m_Direction = m_Rotation * m_Direction;
	};

	void	SetLuminousFlow(const float luminousFlow) { m_LuminousFlow = luminousFlow; };
	void	SetColor(glm::vec4 color) { m_Data.m_Color = color; }
	void	SetRadius(const float radius) { m_Data.m_Radius = radius; };
	void	SetAttenuation(const float attenuation) { m_Data.m_Attenuation = attenuation; };
	void	SetIntensity(const float intensity) { m_Data.m_Intensity = intensity; };
	void	SetAngle(const float angle) { m_Data.m_Angle = angle; };
	void	SetType(const int type) { m_Data.m_Type = type; };

protected:
	float		m_LuminousFlow; // lumen -> lm

	glm::vec4	m_Direction;
	LightData	m_Data;
}; // class Light

//----------------------------------------------------------------

LIGHTLYY_END
