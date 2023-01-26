#version 410 core
out float FragColor;

in vec2 vTexcoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;

uniform vec3 kernel[256];

// SSAO Configuration
uniform int kernelSize;
uniform float radius;
uniform float bias;

// Tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1200.0/4.0, 900.0/4.0); 

const float PI = 3.14159265f;

uniform mat4 projectionMatrix;

// Fast approximation for acos (Credits: https://stackoverflow.com/questions/3380628/fast-arc-cos-algorithm/3380723#3380723)
float fast_acos(float x) {
   return (-0.69813170079773212 * x * x - 0.87266462599716477) * x + 1.5707963267948966;
}

void main()
{
	// get input for UE4 AO algorithm
	vec3 fragPos = texture(gPosition, vTexcoords).xyz;
	vec3 normal = normalize(texture(gNormal, vTexcoords).xyz);
	vec3 randomVec = normalize(texture(noiseTexture, vTexcoords * noiseScale).xyz);
	
	// Create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0f;
	for (int i = 0; i < kernelSize; ++i) {
		// Get sample position
		vec3 samplePos = TBN * kernel[i]; // Get the sample to view space
		vec3 samplePos2 = fragPos - samplePos * radius;
		samplePos = fragPos + samplePos * radius;
		
		// Project sample position (to get position on screen)
		vec4 offset = vec4(samplePos, 1.0f);
		offset = projectionMatrix * offset; // Get the offset to screen space
		offset.xyz /= offset.w; // Normalize the value
		offset.xyz = offset.xyz * 0.5f + 0.5f; // Get it in [0, 1] range
		
		// Project second sample position (to get position on screen)
		vec4 offset2 = vec4(samplePos2, 1.0f);
		offset2 = projectionMatrix * offset2; // Get the offset to screen space
		offset2.xyz /= offset2.w; // Normalize the value
		offset2.xyz = offset2.xyz * 0.5f + 0.5f; // Get it in [0, 1] range
		
		vec3 v1 = texture(gPosition, offset.xy).xyz - fragPos;
		vec3 v2 = texture(gPosition, offset2.xy).xyz - fragPos;
		
		occlusion += max(fast_acos(dot(normalize(v1), normalize(v2))), 0.0f);		   
	}
	occlusion /= kernelSize * PI;
	
	FragColor = occlusion;
}
