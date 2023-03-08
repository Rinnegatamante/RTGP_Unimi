#version 410 core
out vec3 FragColor;

in vec2 vTexcoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;
uniform sampler2D lightTexture;

uniform vec3 kernel[256];

// SSDO Configuration
uniform int kernelSize;
uniform float radius;
uniform float bias;

// Tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1200.0/4.0, 900.0/4.0); 

uniform mat4 projectionMatrix;

void main()
{
	// get input for SSDO algorithm
	vec3 fragPos = texture(gPosition, vTexcoords).xyz;
	vec3 normal = texture(gNormal, vTexcoords).xyz;
	vec3 randomVec = texture(noiseTexture, vTexcoords * noiseScale).xyz;
	
	// Create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Iterate over the sample kernel and calculate occlusion factor
	vec3 occlusion = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < kernelSize; ++i) {
		// Get sample position
		vec3 samplePos = TBN * kernel[i]; // Get the sample to view space
		samplePos = fragPos + samplePos * radius; 
		
		// Project sample position (to get position on screen)
		vec4 offset = vec4(samplePos, 1.0f);
		offset = projectionMatrix * offset; // Get the offset to screen space
		offset.xyz /= offset.w; // Normalize the value
		offset.xyz = offset.xyz * 0.5f + 0.5f; // Get it in [0, 1] range
		
		// Get sample depth
		float sampleDepth = texture(gPosition, offset.xy).z;
		vec3 sampleNormal = texture(gNormal, offset.xy).xyz;
		vec3 sampleColor = texture(lightTexture, offset.xy).xyz;
		
		// Range check and accumulation (Range check is introduced to avoid very far surfaces to alter sampled position AO factor)
		if (sampleDepth >= samplePos.z + bias)
			occlusion += max(dot(sampleNormal, normalize(samplePos - fragPos)), 0.0) * sampleColor;	   
	}
	occlusion = (occlusion / kernelSize);
	
	FragColor = occlusion;
}