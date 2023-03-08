#version 410 core

out vec4 colorFrag;

// interpolated texture coordinates
in vec3 interp_UVW;

// texture sampler for the cube map
uniform samplerCube tCube;
uniform sampler2D gPosition;

void main()
{
	float depth = texture(gPosition, gl_FragCoord.xy / vec2(1200, 900)).z;
	if (depth > 0.099)
		colorFrag = texture(tCube, interp_UVW);
    else
		discard;
}
