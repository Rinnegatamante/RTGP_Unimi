#version 410 core
out vec3 FragColor;

in vec2 vTexcoords;

uniform sampler2D lightTex;
uniform sampler2D directionalLightTex;
uniform sampler2D indirectLightTex;


void main()
{
    // Get input for SSDO algorithm
	vec3 baseLight = texture(lightTex, vTexcoords).rgb;
	vec3 directionalLight = texture(directionalLightTex, vTexcoords).rgb;
	vec3 indirectLight = texture(indirectLightTex, vTexcoords).rgb;

	FragColor = baseLight + directionalLight + indirectLight;
}
