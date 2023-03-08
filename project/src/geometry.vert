#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 vPosition;
out vec3 vNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

void main()
{
	vec4 viewPos = viewMatrix * modelMatrix * vec4(position, 1.0f);
	vPosition = viewPos.xyz; 
	
	vNormal = normalMatrix * normal;
	
	gl_Position = projectionMatrix * viewPos;
}
