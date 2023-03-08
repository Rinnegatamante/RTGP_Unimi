#version 410 core
out vec3 FragColor;

in vec2 vTexcoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;
uniform samplerCube skybox;

uniform vec3 kernel[256];

// SSDO Configuration
uniform int kernelSize;
uniform float radius;
uniform float bias;

// Tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1200.0/4.0, 900.0/4.0); 

uniform mat4 projectionMatrix;
uniform mat4 invViewMatrix;

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
		
		// Get skybox light color
		vec4 skyboxDirection = invViewMatrix * vec4(samplePos - fragPos, 1.0f);
		skyboxDirection /= skyboxDirection.w; // Normalize the value
		vec3 skyboxColor = texture(skybox, skyboxDirection.xyz).rgb;
		
		// Range check and accumulation (Range check is introduced to avoid very far surfaces to alter sampled position AO factor)
		if (sampleDepth < samplePos.z + bias)
			occlusion += skyboxColor * dot(normal, normalize(samplePos - fragPos));	   
	}
	occlusion /= kernelSize;
	
	FragColor = occlusion;
}