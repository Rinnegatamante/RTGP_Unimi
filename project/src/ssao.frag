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

uniform mat4 projectionMatrix;

void main()
{
	// get input for SSAO algorithm
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
		vec3 samplePos = TBN * kernel[i]; // Get the sample to camera space
		samplePos = fragPos + samplePos * radius; 
		
		// Project sample position (to sample texture) (to get position on screen/texture)
		vec4 offset = vec4(samplePos, 1.0f);
		offset = projectionMatrix * offset; // Get the offset to clip space
		offset.xyz /= offset.w; // Normalize the value
		offset.xyz = offset.xyz * 0.5f + 0.5f; // Get it in [0, 1] range
		
		// Get sample depth
		float sampleDepth = texture(gPosition, offset.xy).z;
		
		// Range check & accumulate
		float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;		   
	}
	occlusion = 1.0f - (occlusion / kernelSize);
	
	FragColor = occlusion;
}