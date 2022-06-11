/*
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


// Std. Includes
#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
	#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
	#error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/camera.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// dimensions of application's window
GLuint screenWidth = 1200, screenHeight = 900;

// the rendering steps used in the application
enum render_passes{ SHADOWMAP, RENDER};

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// in this application, we have isolated the models rendering using a function, which will be called in each rendering step
void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, Model &bunnyModel);

// Function to draw a fullscreen quad
void DrawQuad();

// load image from disk and create an OpenGL texture
GLint LoadTexture(const char* path);

// we initialize an array of booleans for each keyboard key
bool keys[1024];

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
bool spinning = false;
// boolean to show/hide final ambient occlusion buffer values
bool show_occlusion = false;

// View matrix: the camera moves, so we just set to indentity now
glm::mat4 view = glm::mat4(1.0f);

// Angle FOV of our camera
GLfloat FOV = 45.0f;

// Model and Normal transformation matrices for the objects in the scene: we set to identity
glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
glm::mat3 sphereNormalMatrix = glm::mat3(1.0f);
glm::mat4 cubeModelMatrix = glm::mat4(1.0f);
glm::mat3 cubeNormalMatrix = glm::mat3(1.0f);
glm::mat4 bunnyModelMatrix = glm::mat4(1.0f);
glm::mat3 bunnyNormalMatrix = glm::mat3(1.0f);

// we create a camera. We pass the initial position as a paramenter to the constructor. The last boolean tells if we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_FALSE);

// light configuration
glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, -2.0f);
glm::vec3 lightColor = glm::vec3(0.2f, 0.8f, 0.2f);
GLfloat linearAttenuation = 0.09f;
GLfloat quadraticAttenuation = 0.032f;

// Configuration for the SSAO kernel samples
int kernelSize = 64; // Kernel Size for CryEngine2 derivates techniques
float kernelRadius = 0.5f; // Hemisphere/Sphere radius for the kernel
float kernelBias = 0.025f; // Kernel bias for CryEngine2 derivates techniques and Angle bias for HBAO
int numDirections = 16; // Number of different directions displacements to use for HBAO
int numSteps = 4; // Number of steps to perform per each direction for HBAO

// vector for the textures IDs
vector<GLint> textureID;

// UV repetitions
GLfloat repeat = 1.0;

// Flag that enables/disables additional blurring pass for the generated AO buffer
bool have_blur = true;

// Available ambient occlusion modes
enum {
	NO_SSAO,
	CRYENGINE2_AO,
	CRYENGINE2_AO_RECONSTR,
	STARCRAFT2_AO,
	STARCRAFT2_AO_RECONSTR,
	HBAO,
	SSAO_MODES_NUM
};
const char *techniqueNames[] = {
	"No Ambient Occlusion",
	"CryEngine 2 AO",
	"CryEngine 2 AO with Depth Resolve",
	"StarCraft II AO",
	"StarCraft II AO with Depth Resolve",
	"Horizon Based Ambient Occlusion (HBAO)",
};

// G Buffer buffers
enum {
	POSITION,
	NORMALS,
	ALBEDO,
	DEPTH_BUFFER,
	SSAO_BUFFER,
	FINAL_SSAO_BUFFER,
	GBUFFER_BUFFERS_NUM
};
const char *gbufferNames[] = {
	"Positions",
	"Normals",
	"Albedo",
	"Depth Buffer",
	"SSAO",
	"SSAO blurred"
};
GLuint gPosition, gNormal, gAlbedo, SSAOColorBuffer, SSAOColorBufferBlurred;
GLuint gbuffers[GBUFFER_BUFFERS_NUM];

// Currently active SSAO mode
GLint ssao_mode = CRYENGINE2_AO;
GLboolean camera_mode = GL_TRUE;

// Functions used to generate samples for our SSAO techniques
void generateSamplesForCryEngine(std::vector<glm::vec3>& SSAOKernel) { // CryEngine 2 AO generator (Sphere around a point)
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	SSAOKernel.clear();
	for (unsigned int i = 0; i < kernelSize; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / float(kernelSize);

		// Scale samples so that they're more aligned to the center of the kernel
		scale = (scale * scale) * 0.9f + 0.1f;
		sample *= scale;
		SSAOKernel.push_back(sample);
	}
}
void generateSamplesForStarCraft(std::vector<glm::vec3>& SSAOKernel) { // StarCraft II AO generator (Oriented Hemisphere considering point normal)
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	SSAOKernel.clear();
	for (unsigned int i = 0; i < kernelSize; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)); // By sampling Z only in [0, 1] range, we effectively sample inside an hemisphere
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / float(kernelSize);

		// Scale samples so that they're more aligned to center of kernel
		/*scale = (scale * scale) * 0.9f + 0.1f;
		sample *= scale;*/
		SSAOKernel.push_back(sample);
	}
}

/////////////////// MAIN function ///////////////////////
int main()
{
	// Initialization of OpenGL context using GLFW
	glfwInit();
	// We set OpenGL specifications required for this application
	// In this case: 4.1 Core
	// If not supported by your graphics HW, the context will not be created and the application will close
	// N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
	// in relation also to the values indicated in these GLFW commands
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// we set if the window is resizable
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// we create the application's window
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "SSAO Analysis", nullptr, nullptr);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// we put in relation the window and the callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// we disable the mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD tries to load the context set by GLFW
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	// we define the viewport dimensions
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	
	// we enable Z test
	glEnable(GL_DEPTH_TEST);

	//the "clear" color for the frame buffer
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

	// we create the Shader Programs for our scene
	Shader geometryReconstrPass("geometry_reconstr.vert", "geometry_reconstr.frag");
	Shader geometryPass("geometry.vert", "geometry.frag");
	Shader lightingReconstrPass("ssao_reconstr.vert", "lighting_reconstr.frag");
	Shader lightingPass("ssao.vert", "lighting.frag");
	Shader SSAOPass("ssao.vert", "ssao.frag");
	Shader SSAOReconstrPass("ssao_reconstr.vert", "ssao_reconstr.frag");
	Shader HBAOPass("ssao.vert", "hbao.frag");
	Shader blurPass("ssao.vert", "blur.frag");
	Shader simplePass("ssao.vert", "simple.frag");

	// we load the model(s) (code of Model class is in include/utils/model.h)
	Model cubeModel("../../models/cube.obj");
	Model sphereModel("../../models/sphere.obj");
	Model bunnyModel("../../models/bunny_lp.obj");
	
	// Create a full white texture to simulate absence of ambient occlusion
	GLuint gWhiteTex;
	glGenTextures(1, &gWhiteTex);
	glBindTexture(GL_TEXTURE_2D, gWhiteTex);
	uint32_t white = 0xFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
	
	// Creating the textures for the G-Buffer framebuffer (gPosition, gNormal, gAlbedo in the geometry fragment shader)
	glGenTextures(1, &gPosition);
	gbuffers[POSITION] = gPosition;
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr); // Float texture to ensure values are not clamped in [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenTextures(1, &gNormal);
	gbuffers[NORMALS] = gNormal;
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr); // Float texture to ensure values are not clamped in [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenTextures(1, &gAlbedo);
	gbuffers[ALBEDO] = gAlbedo;
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr); // Float texture to ensure values are not clamped in [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Create a depth buffer for our framebuffer
	GLuint gDepthBuffer;
	glGenTextures(1, &gDepthBuffer); // We create it as a texture instead of a renderbuffer so that we can use it for the depth resolve technique
	glBindTexture(GL_TEXTURE_2D, gDepthBuffer);
	gbuffers[DEPTH_BUFFER] = gDepthBuffer;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	// Creating the G-Buffer framebuffer and binding the previously created texture to it
	GLuint gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepthBuffer, 0);

	// Enabling drawing on all the attached buffers for the given framebuffer
	GLuint full_attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	GLuint reconstr_attachments[] = {GL_NONE, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};

	// Creating the texture for the framebuffer holding SSAO processing stage
	glGenTextures(1, &SSAOColorBuffer);
	gbuffers[SSAO_BUFFER] = SSAOColorBuffer;
	glBindTexture(GL_TEXTURE_2D, SSAOColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Create the framebuffer for SSAO processing stage and binding previously created texture as color buffer
	GLuint SSAOfbo;
	glGenFramebuffers(1, &SSAOfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOColorBuffer, 0);
	
	// Creating the texture for the framebuffer holding SSAO blurring stage
	glGenTextures(1, &SSAOColorBufferBlurred);
	gbuffers[FINAL_SSAO_BUFFER] = SSAOColorBufferBlurred;
	glBindTexture(GL_TEXTURE_2D, SSAOColorBufferBlurred);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Create the framebuffer for SSAO blurring stage and binding previously created texture as color buffer
	GLuint SSAOBlurFBO;
	glGenFramebuffers(1, &SSAOBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOBlurFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOColorBufferBlurred, 0);
	
	// Binding back to default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Generate the sample kernel required for SSAO processing
	std::vector<glm::vec3> SSAOKernel;
	generateSamplesForCryEngine(SSAOKernel);
	
	// Generate a noise texture required for SSAO processing holding random vectors to use as directions during AO calculation
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<glm::vec3> SSAONoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		SSAONoise.push_back(noise);
	}
	GLuint noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &SSAONoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	// Projection matrix of the camera: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(FOV, (float)screenWidth/(float)screenHeight, 0.1f, 50.0f);

	// Setting bindings for all the samplers and the projection matrices used in our shaders, and for kernel config for SSAO
	SSAOPass.Use();
	glUniform1i(glGetUniformLocation(SSAOPass.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(SSAOPass.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(SSAOPass.Program, "noiseTexture"), 2);
	glUniformMatrix4fv(glGetUniformLocation(SSAOPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	HBAOPass.Use();
	glUniform1i(glGetUniformLocation(HBAOPass.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(HBAOPass.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(HBAOPass.Program, "noiseTexture"), 2);
	SSAOReconstrPass.Use();
	glUniform1f(glGetUniformLocation(SSAOReconstrPass.Program, "gAspectRatio"), (float)screenWidth/(float)screenHeight);
	glUniform1f(glGetUniformLocation(SSAOReconstrPass.Program, "gTanFOV"), tan(FOV));
	glUniform1i(glGetUniformLocation(SSAOReconstrPass.Program, "gDepthMap"), 0);
	glUniform1i(glGetUniformLocation(SSAOReconstrPass.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(SSAOReconstrPass.Program, "noiseTexture"), 2);
	glUniformMatrix4fv(glGetUniformLocation(SSAOReconstrPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(SSAOReconstrPass.Program, "invProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	lightingPass.Use();
	glUniform1i(glGetUniformLocation(lightingPass.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingPass.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingPass.Program, "gAlbedo"), 2);
	glUniform1i(glGetUniformLocation(lightingPass.Program, "SSAO"), 3);
	glUniformMatrix4fv(glGetUniformLocation(lightingPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	lightingReconstrPass.Use();
	glUniform1f(glGetUniformLocation(lightingReconstrPass.Program, "gAspectRatio"), (float)screenWidth/(float)screenHeight);
	glUniform1f(glGetUniformLocation(lightingReconstrPass.Program, "gTanFOV"), tan(FOV));
	glUniform1i(glGetUniformLocation(lightingReconstrPass.Program, "gDepthMap"), 0);
	glUniform1i(glGetUniformLocation(lightingReconstrPass.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingReconstrPass.Program, "gAlbedo"), 2);
	glUniform1i(glGetUniformLocation(lightingReconstrPass.Program, "SSAO"), 3);
	glUniformMatrix4fv(glGetUniformLocation(lightingReconstrPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(lightingReconstrPass.Program, "invProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	blurPass.Use();
	glUniform1i(glGetUniformLocation(blurPass.Program, "SSAOtex"), 0);
	geometryPass.Use();
	glUniformMatrix4fv(glGetUniformLocation(geometryPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	geometryReconstrPass.Use();
	glUniformMatrix4fv(glGetUniformLocation(geometryReconstrPass.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	simplePass.Use();
	glUniform1i(glGetUniformLocation(simplePass.Program, "image"), 0);

	// Rendering loop: this code is executed at each frame
	int oldKernelSize = kernelSize;
	int old_ssao_mode = ssao_mode;
	int64_t numFrames = -1;
	GLfloat deltaTimeSum = 0.0f;
	GLfloat averageFrameTime = 0.0f;
	while(!glfwWindowShouldClose(window))
	{
		// Regenerate the kernel samples if its size changes or if the SSAO mode changes
		if (kernelSize != oldKernelSize || ssao_mode != old_ssao_mode) {
			switch (ssao_mode) {
			case CRYENGINE2_AO:
			case CRYENGINE2_AO_RECONSTR:
				generateSamplesForCryEngine(SSAOKernel);
				break;
			case STARCRAFT2_AO:
			case STARCRAFT2_AO_RECONSTR:
				generateSamplesForStarCraft(SSAOKernel);
				break;
			default:
				break;
			}
		}
		oldKernelSize = kernelSize;
		old_ssao_mode = ssao_mode;
		
		// Handling changes in input mode for the mouse
		glfwSetInputMode(window, GLFW_CURSOR, camera_mode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		// we determine the time passed from the beginning
		// and we calculate time difference between current frame rendering and the previous one
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		numFrames++;
		if (numFrames > 0) {
			deltaTimeSum += deltaTime;
			averageFrameTime = deltaTimeSum / (GLfloat)numFrames;
		}	
		lastFrame = currentFrame;

		// Check is an I/O event is happening
		glfwPollEvents();
		// we apply FPS camera movements
		apply_camera_movements();
		
		// we get the view matrix from the Camera class
		view = camera.GetViewMatrix();
		
		// if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
		if (spinning)
			orientationY+=(deltaTime*spin_speed);
		
		// we set the viewport for the final rendering step
		glViewport(0, 0, width, height);
		
		// STEP 1 - GEOMETRY PASS
		// Render the full scene data into our auxiliary G Buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		if (ssao_mode == CRYENGINE2_AO_RECONSTR || ssao_mode == STARCRAFT2_AO_RECONSTR) { // If we use CryEngine 2 AO derivatives with depth resolve, we need a different program
			// Using different geometry pass program if we want to reconstruct view positions instead of using G buffer to store them
			glDrawBuffers(3, reconstr_attachments);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			geometryReconstrPass.Use();
			glUniformMatrix4fv(glGetUniformLocation(geometryReconstrPass.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			RenderObjects(geometryReconstrPass, cubeModel, sphereModel, bunnyModel);
		} else {
			glDrawBuffers(3, full_attachments);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			geometryPass.Use();
			glUniformMatrix4fv(glGetUniformLocation(geometryPass.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			RenderObjects(geometryPass, cubeModel, sphereModel, bunnyModel);
		}
		
		if (ssao_mode != NO_SSAO) {
			// STEP 2 - SSAO Texture generation
			glBindFramebuffer(GL_FRAMEBUFFER, SSAOfbo);
			glClear(GL_COLOR_BUFFER_BIT);
			if (ssao_mode == CRYENGINE2_AO_RECONSTR || ssao_mode == STARCRAFT2_AO_RECONSTR) { // CryEngine2 derivates with Depth Resolve
				SSAOReconstrPass.Use();
				glUniform1i(glGetUniformLocation(SSAOReconstrPass.Program, "kernelSize"), kernelSize);
				glUniform1f(glGetUniformLocation(SSAOReconstrPass.Program, "radius"), kernelRadius);
				glUniform1f(glGetUniformLocation(SSAOReconstrPass.Program, "bias"), kernelBias);
				for (int i = 0; i < kernelSize; i++) {
					char binding[32];
					sprintf(binding, "kernel[%d]", i);
					glUniform3fv(glGetUniformLocation(SSAOReconstrPass.Program, binding), 1, glm::value_ptr(SSAOKernel[i]));
				}
			} else if (ssao_mode == HBAO) { // HBAO
				HBAOPass.Use();
				glUniform1i(glGetUniformLocation(HBAOPass.Program, "numDirections"), numDirections);
				glUniform1f(glGetUniformLocation(HBAOPass.Program, "sampleRadius"), kernelRadius);
				glUniform1i(glGetUniformLocation(HBAOPass.Program, "numSteps"), numSteps);
			} else { // CryEngine2 derivates
				SSAOPass.Use();
				glUniform1i(glGetUniformLocation(SSAOPass.Program, "kernelSize"), kernelSize);
				glUniform1f(glGetUniformLocation(SSAOPass.Program, "radius"), kernelRadius);
				glUniform1f(glGetUniformLocation(SSAOPass.Program, "bias"), kernelBias);
				for (int i = 0; i < kernelSize; i++) {
					char binding[32];
					sprintf(binding, "kernel[%d]", i);
					glUniform3fv(glGetUniformLocation(SSAOPass.Program, binding), 1, glm::value_ptr(SSAOKernel[i]));
				}
			}
			glActiveTexture(GL_TEXTURE0);
			if (ssao_mode == CRYENGINE2_AO_RECONSTR || ssao_mode == STARCRAFT2_AO_RECONSTR)
				glBindTexture(GL_TEXTURE_2D, gDepthBuffer); // With Depth Resolve, we pass the depth buffer and we reconstruct positions in fragment shader
			else
				glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			DrawQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			if (have_blur) {
				// STEP 3 - Blurring SSAO Texture to avoid noise
				glBindFramebuffer(GL_FRAMEBUFFER, SSAOBlurFBO);
				glClear(GL_COLOR_BUFFER_BIT);
				blurPass.Use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, SSAOColorBuffer);
				DrawQuad();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		if (show_occlusion && ssao_mode != NO_SSAO) {
			// STEP 4 - Show ambient occlusion buffer on screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			simplePass.Use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, have_blur ? SSAOColorBufferBlurred : SSAOColorBuffer);
			DrawQuad();
		} else {
			// STEP 4 - Deferred rendering for lighting with added SSAO
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (ssao_mode == CRYENGINE2_AO_RECONSTR || ssao_mode == STARCRAFT2_AO_RECONSTR) { // If we use CryEngine 2 AO derivatives with depth resolve, we need a different program
				lightingReconstrPass.Use();
				glm::vec3 lightPosView = view * glm::vec4(lightPos, 1.0);
				glUniform3fv(glGetUniformLocation(lightingReconstrPass.Program, "lightPosition"), 1, glm::value_ptr(lightPosView));
				glUniform3fv(glGetUniformLocation(lightingReconstrPass.Program, "lightColor"), 1, glm::value_ptr(lightColor));
				glUniform1f(glGetUniformLocation(lightingReconstrPass.Program, "linearAttenuation"), linearAttenuation);
				glUniform1f(glGetUniformLocation(lightingReconstrPass.Program, "quadraticAttenuation"), quadraticAttenuation);
			} else {
				lightingPass.Use();
				glm::vec3 lightPosView = view * glm::vec4(lightPos, 1.0);
				glUniform3fv(glGetUniformLocation(lightingPass.Program, "lightPosition"), 1, glm::value_ptr(lightPosView));
				glUniform3fv(glGetUniformLocation(lightingPass.Program, "lightColor"), 1, glm::value_ptr(lightColor));
				glUniform1f(glGetUniformLocation(lightingPass.Program, "linearAttenuation"), linearAttenuation);
				glUniform1f(glGetUniformLocation(lightingPass.Program, "quadraticAttenuation"), quadraticAttenuation);
			}
			glActiveTexture(GL_TEXTURE0);
			if (ssao_mode == CRYENGINE2_AO_RECONSTR || ssao_mode == STARCRAFT2_AO_RECONSTR)
				glBindTexture(GL_TEXTURE_2D, gDepthBuffer); // With Depth Resolve, we pass the depth buffer and we reconstruct positions in fragment shader
			else
				glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAlbedo);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, ssao_mode == NO_SSAO ? gWhiteTex : (have_blur ? SSAOColorBufferBlurred : SSAOColorBuffer));
			DrawQuad();
		}
		
		// Rendering dear ImGui UI only if in cursor mode
		if (!camera_mode) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Frame Info");
			ImGui::Text("Average Frame Time: %.06f s", averageFrameTime);
			ImGui::Text("Last Frame Time: %.06f s", deltaTime);
			if (ImGui::Button("Reset Counters")) {
				numFrames = -1;
				averageFrameTime = 0.0f;
				deltaTimeSum = 0.0f;
			}
			ImGui::End();
			ImGui::Begin("G Buffer Inspector");
			static int filter_idx = 0;
			if (ImGui::BeginCombo("##combo", gbufferNames[filter_idx])) {
				for (int n = 0; n < GBUFFER_BUFFERS_NUM; n++) {
					bool is_selected = filter_idx == n;
					if (ImGui::Selectable(gbufferNames[n], is_selected))
						filter_idx = n;
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::Image((void *)(intptr_t)gbuffers[filter_idx], ImVec2(400, 300), ImVec2(0, 1), ImVec2(1, 0)); // Flipping the image upside down
			ImGui::End();
			
			ImGui::Begin("Configurator");
			ImGui::Checkbox("Spin Models", &spinning);
			ImGui::Separator();
			static int mode_idx = 1;
			ImGui::Text("Ambient Occlusion Technique:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##combo", techniqueNames[mode_idx])) {
				for (int n = 0; n < SSAO_MODES_NUM; n++) {
					bool is_selected = mode_idx == n;
					if (ImGui::Selectable(techniqueNames[n], is_selected)) {
						mode_idx = n;
						ssao_mode = mode_idx;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::Checkbox("Perform Blur Pass", &have_blur);
			if (ssao_mode != HBAO) {
				ImGui::SliderInt("Kernel Size", &kernelSize, 8, 256);
				ImGui::SliderFloat("Kernel Radius", &kernelRadius, 0.1f, 2.0f);
				ImGui::SliderFloat("Kernel Bias", &kernelBias, 0.01f, 1.0f);
			} else {
				ImGui::SliderInt("Directions Number", &numDirections, 4, 128);
				ImGui::SliderFloat("Kernel Radius", &kernelRadius, 0.1f, 2.0f);
				ImGui::SliderInt("Per Step Samples Number", &numSteps, 2, 128);
				ImGui::SliderFloat("Angle Bias", &kernelBias, 0.01f, 1.0f);
			}
			ImGui::Separator();
			ImGui::Checkbox("Show AO Buffer Only", &show_occlusion);
			ImGui::End();
			
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		
		// Swapping back and front buffers
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	// when I exit from the graphics loop, it is because the application is closing
	// we delete the Shader Programs
	geometryPass.Delete();
	SSAOPass.Delete();
	blurPass.Delete();
	lightingPass.Delete();
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}

// Function to draw a fullscreen quad
void DrawQuad() {
	static GLuint quadVAO = 0;
	static GLuint quadVBO;
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions		// texcoords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

//////////////////////////////////////////
// We render the objects.
void RenderObjects(Shader &shader, Model &cubeModel, Model &sphereModel, Model &bunnyModel)
{
	// ROOM
	// we reset to identity at each frame
	glUniform1i(glGetUniformLocation(shader.Program, "invertedNormals"), 1); // Invert normals for our room model
	cubeModelMatrix = glm::mat4(1.0f);
	cubeNormalMatrix = glm::mat3(1.0f);
	cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 7.0f, 0.0f));
	cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(7.5f, 7.5f, 7.5f));
	cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
	glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));

	// we render the room
	cubeModel.Draw();
	
	// SPHERE
	// we reset to identity at each frame
	glUniform1i(glGetUniformLocation(shader.Program, "invertedNormals"), 0); // Back to standard normals calculation
	sphereModelMatrix = glm::mat4(1.0f);
	sphereNormalMatrix = glm::mat3(1.0f);
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 0.3f, 0.0f));
	sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
	sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
	glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));

	// we render the sphere
	sphereModel.Draw();
	
	// CUBE
	// we reset to identity at each frame
	cubeModelMatrix = glm::mat4(1.0f);
	cubeNormalMatrix = glm::mat3(1.0f);
	cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.3f, 0.0f));
	cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
	cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
	cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
	glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));

	// we render the cube
	cubeModel.Draw();

	// BUNNY
	// we reset to identity at each frame
	bunnyModelMatrix = glm::mat4(1.0f);
	bunnyNormalMatrix = glm::mat3(1.0f);
	bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(3.0f, 0.3f, 0.0f));
	bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
	bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
	bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
	glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

	// we render the bunny
	bunnyModel.Draw();
}

//////////////////////////////////////////
// we load the image from disk and we create an OpenGL texture
GLint LoadTexture(const char* path)
{
	GLuint textureImage;
	int w, h, channels;
	unsigned char* image;
	image = stbi_load(path, &w, &h, &channels, STBI_rgb);

	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;

	glGenTextures(1, &textureImage);
	glBindTexture(GL_TEXTURE_2D, textureImage);
	// 3 channels = RGB ; 4 channel = RGBA
	if (channels==3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	else if (channels==4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	// we set how to consider UVs outside [0,1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// we set the filtering for minification and magnification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);

	// we set the binding to 0 once we have finished
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureImage;
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
	if(keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if(keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if(keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if(keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	GLuint new_subroutine;

	// if ESC is pressed, we close the application
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// if P is pressed, we start/stop the animated rotation of models
	if(key == GLFW_KEY_P && action == GLFW_PRESS)
		spinning=!spinning;
	
	// if T is pressed, we switch SSAO mode
	if(key == GLFW_KEY_T && action == GLFW_PRESS)
		ssao_mode = (ssao_mode + 1) % SSAO_MODES_NUM;
	
	// if Z is pressed, we show ambient occlusion final texture fullscreen
	if(key == GLFW_KEY_Z && action == GLFW_PRESS)
		show_occlusion=!show_occlusion;
	
	// Switch between cursor and camera mode
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		camera_mode = !camera_mode;
	
	// we keep trace of the pressed keys
	// with this method, we can manage 2 keys pressed at the same time:
	// many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
	// using a boolean array, we can then check and manage all the keys pressed at the same time
	if(action == GLFW_PRESS)
		keys[key] = true;
	else if(action == GLFW_RELEASE)
		keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera_mode)
		return;
	
	// we move the camera view following the mouse cursor
	// we calculate the offset of the mouse cursor from the position in the last frame
	// when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
	if(firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// offset of mouse cursor position
	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	// the new position will be the previous one for the next frame
	lastX = xpos;
	lastY = ypos;

	// we pass the offset to the Camera class instance in order to update the rendering
	camera.ProcessMouseMovement(xoffset, yoffset);
}
