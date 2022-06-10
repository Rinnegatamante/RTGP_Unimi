#version 410 core
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in vec3 vNormal;

void main()
{	
	gNormal = normalize(vNormal);
	gAlbedo = vec3(0.95f);
}
