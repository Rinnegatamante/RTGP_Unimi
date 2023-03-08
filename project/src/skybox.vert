/*
17_skybox.vert: vertex shader for the visualization of the cube map as environment map

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/


#version 410 core

// vertex position in world coordinates
in vec3 position;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// texture coordinates for the environment map sampling (we use 3 coordinates because we are sampling in 3 dimensions)
out vec3 interp_UVW;

// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

void main()
{
		// in this case, we are not using the UV coordinates of the models, but we use the vertex position as 3D texture coordinates, in order to have a 1:1 mapping from the cube map and the cube used as "the world"
		interp_UVW = position;

		// we apply the transformations to the vertex
    vec4 pos = projectionMatrix * viewMatrix * vec4(position, 1.0);
		// we want to set the Z coordinate of the projected vertex at the maximum depth (i.e., we want Z to be equal to 1.0 after the projection divide)
		// -> we set Z equal to W (because in the projection divide, after clipping, all the components will be divided by W).
		// This means that, during the depth test, the fragments of the environment map will have maximum depth (see comments in the code of the main application)
		gl_Position = pos.xyww;
}
