#version 410 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in vec3 vPosition;
in vec3 vNormal;

void main()
{	
	gPosition = vPosition;
	gNormal = normalize(vNormal);
	gAlbedo = vec3(0.95f);
}
