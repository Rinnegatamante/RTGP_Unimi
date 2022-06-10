#version 410 core
out float FragColor;

in vec2 vTexcoords;

uniform sampler2D SSAOtex;

void main() 
{
	vec2 texelSize = 1.0 / vec2(textureSize(SSAOtex, 0));
	float result = 0.0;
	for (int x = -2; x < 2; ++x) 
	{
		for (int y = -2; y < 2; ++y) 
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(SSAOtex, vTexcoords + offset).r;
		}
	}
	FragColor = result / (4.0 * 4.0);
}
