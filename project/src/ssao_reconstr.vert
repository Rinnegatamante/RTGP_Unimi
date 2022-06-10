#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

out vec2 vTexcoords;
out vec2 ViewRay;

uniform float gAspectRatio;
uniform float gTanFOV;

void main() {
	vTexcoords = texcoord;
	ViewRay.x = position.x * gAspectRatio * gTanFOV;
	ViewRay.y = position.y * gTanFOV;
	gl_Position = vec4(position, 1.0f);
}
