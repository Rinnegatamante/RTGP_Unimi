#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

out vec2 vTexcoords;

void main() {
	vTexcoords = texcoord;
	gl_Position = vec4(position, 1.0f);
}
