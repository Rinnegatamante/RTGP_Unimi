#version 410 core
out vec4 FragColor;

in vec2 vTexcoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D SSAO;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

void main()
{			 
	// retrieve data from gbuffer
	vec3 FragPos = texture(gPosition, vTexcoords).xyz;
	vec3 Normal = texture(gNormal, vTexcoords).xyz;
	vec3 Diffuse = texture(gAlbedo, vTexcoords).xyz;
	float AmbientOcclusion = texture(SSAO, vTexcoords).x;
	
	// Ambient coefficient
	vec3 ambient = vec3(0.3f * Diffuse * AmbientOcclusion);
	vec3 lighting  = ambient; 
	vec3 viewDir  = normalize(-FragPos);
	
	// Diffuse coefficient
	vec3 lightDir = normalize(lightPosition - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0f) * Diffuse * lightColor;
	
	// Specular coefficient
	vec3 halfwayDir = normalize(lightDir + viewDir);  
	float spec = pow(max(dot(Normal, halfwayDir), 0.0f), 8.0f);
	vec3 specular = lightColor * spec;
	
	// Attenuation (Linear and Quadratic)
	float distance = length(lightPosition - FragPos);
	float attenuation = 1.0f / (1.0f + linearAttenuation * distance + quadraticAttenuation * distance * distance);
	diffuse *= attenuation;
	specular *= attenuation;
	lighting += diffuse + specular;

	FragColor = vec4(lighting, 1.0f);
}
