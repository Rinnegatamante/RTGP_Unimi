#version 410 core
out float FragColor;

in vec2 vTexcoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;

uniform int numDirections; // Number of directions displacements to perform
uniform int numSteps; // Number of steps to perform per direction
uniform float sampleRadius; // Radius size to consider for our hemisphere

const float PI = 3.14159265f;

// Tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1200.0/4.0, 900.0/4.0); 

void main()
{
	// get input for HBAO algorithm
	vec3 fragPos = texture(gPosition, vTexcoords).xyz;
	vec3 normal = normalize(texture(gNormal, vTexcoords).xyz);
	vec3 randomVec = texture(noiseTexture, vTexcoords * noiseScale).xyz;
	
	// Rotation displacement per direction so that we perform a full circle sampling
	float deltaRot = 2.0 * PI / numDirections;
	float cosRot = cos(deltaRot);
	float sinRot = sin(deltaRot);
  
	// Create a 2D rotation matrix to rotate our sampling vector
	mat2 deltaRotationMatrix = mat2(cosRot, -sinRot, sinRot, cosRot);
	
	// Calculate tangent vector since perpendicular to  normal one
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	
	vec2 sampleDir = tangent.xy * (sampleRadius / (float(numDirections * numSteps) + 1.0));
  
	// Apply noise to the base sampling vector
	mat2 noiseMatrix = mat2(randomVec.x, -randomVec.y, randomVec.y,  randomVec.x);
	sampleDir = noiseMatrix * sampleDir;
  
	// Iterate over the directions and calculate occlusion factor
	float occlusion = 0.0;
	for (int i = 0; i < numDirections; ++i) {
		// Incrementally rotate sample direction
		sampleDir = deltaRotationMatrix * sampleDir;
		
		// Perform incremental sampling steps for each direction
		float oldAngle = 0.0f;
		for (int j = 0; j < numSteps; ++j) {
			// Displace from current fragment position towards the sample direction with some noise in between
			vec2 samplePos = vTexcoords + (randomVec.z + float(j)) * sampleDir;
			vec3 sampleViewPos = texture(gPosition, samplePos).xyz; // Get view space position for the sampled position
			vec3 sampleDiff = (sampleViewPos - fragPos); // Calculate the vector going from fragment to sampled position
			float tangentAngle = (PI / 2.0) - acos(dot(normal, normalize(sampleDiff))); // Calculating angle between fragment and sampled position
			
			// Add ambient occlusion contribution factor only for samples further than current bias
			if (tangentAngle > oldAngle) {
				float value = sin(tangentAngle) - sin(oldAngle);
				occlusion += value;
				oldAngle = tangentAngle;
			}
		}
	}
  
	occlusion = 1.0 - occlusion / numDirections;
	occlusion = clamp(occlusion, 0.0, 1.0);
	FragColor = occlusion;
}
