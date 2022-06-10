#version 410 core
out vec4 FragColor;

in vec2 vTexcoords;

uniform sampler2D image;

void main()
{			 
	// retrieve data from gbuffer
	float AmbientOcclusion = texture(image, vTexcoords).x;
	
	FragColor = vec4(AmbientOcclusion, AmbientOcclusion, AmbientOcclusion, 1.0f);
}