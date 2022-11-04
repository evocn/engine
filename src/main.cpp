// Author: Alex Hartford
// Program: engine
// File: main
// Date: November 2022

#include <iostream>
#include <time.h>
#include <unordered_map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;
using namespace glm;

#define PI 3.14159265

// NOTE(Alex): Global State!
const unsigned int SCREEN_WIDTH = 1300;
const unsigned int SCREEN_HEIGHT = 750;
const unsigned int TEXT_SIZE = 16;

const float default_scale = 1.0f;
const float default_view = 1.414f;
const float default_selection = 0.5f;

#include "camera.h"
Camera camera(vec3(0.0f, 0.0f, 0.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool  firstMouse = true;

char  levelName[128] = "";

float        deltaTime = 0.0f;
float        lastFrame = 0.0f;
unsigned int frameCount = 0;
const float  y_offset = 1.0f;

bool gDONTCULL = false;
int drawnObjects;

enum ShaderTypes
{
    MATERIAL,
    TEXTURE
};

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shine;
};

struct FogSystem
{
    float maxDistance;
    float minDistance;
    vec4 color;
};

// Project Headers
#include "shader.h"
#include "model.h"
#include "manager.h"
#include "object.h"
#include "light.h"
#include "level.h"
#include "text.h"
#include "skybox.h"
#include "terrain.h"
#include "particles.h"
#include "boundary.h"
#include "water.h"
#include "sound.h"
#include "sun.h"

using namespace std;
using namespace glm;

void processInput(GLFWwindow *window, vector<Object> *objects, vector<Sound *> *sounds);

Level lvl;

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

float randFloat()
{
    float r = rand() / static_cast<float>(RAND_MAX);
    return r;
}

float randCoord()
{
    return (randFloat() * 220.0f) - 100.0f;
}

float randCoordDes()
{
    return (randFloat() * 440.0f) - 200.0f;
}

float randRange(float min, float max)
{
    return ((randFloat() * (max - min)) + min);
}

int main(void)
{
    srand(time(NULL));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "engine", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD.\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    /* Manage OpenGL State */
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Text Rendering */
    TextRenderer Text = TextRenderer(SCREEN_WIDTH, SCREEN_HEIGHT);
    Text.Load("../resources/verdanab.ttf", TEXT_SIZE);

    // Manager Object. Loads Shaders, Models.
    Manager m;

    vector<Object>  objects;
    vector<Light>   lights;
    vector<Emitter> emitters;
    vector<Sound *> sounds;
    vector<bool>    discoveredNotes;
    int discoveredCount = 1;

    // Sounds
    Sound pickup = Sound("../resources/audio/pickup2.mp3", 1.0f, false);
    Sound hmm = Sound("../resources/audio/hmm.wav", 1.0f, false);
    Sound rock = Sound("../resources/audio/desert.wav", vec3(25, 0, 0), 1.0f, 5.0f, 2.0f, 50.0f, true, false);
    Sound music = Sound("../resources/audio/BGM/愛にできることはまだあるかい.mp3", 0.1f, true);
    Sound streetMusic = Sound("../resources/audio/BGM/Sunrise.mp3", 0.3f, true);
    Sound sadMusic = Sound("../resources/audio/BGM/street.mp3", 0.3f, true);
    Sound forestMusic1 = Sound("../resources/audio/BGM/Forest_1_calm.mp3", 0.1f, true);
    Sound forestMusic2 = Sound("../resources/audio/BGM/Forest_1_dynamic.mp3", 0.1f, true);
    Sound desertMusic = Sound("../resources/audio/BGM/Desert.mp3", 0.3f, true);
    Sound alert = Sound("../resources/audio/alert.wav", 1.0f, false);
    Sound walk = Sound("../resources/audio/step.wav", 0.3f, false);
    Sound EVA = Sound("../resources/audio/congrats.wav", 1.0f, false);
    EVA.isLooping = false;
    Sound fire = Sound("../resources/audio/fire.mp3", vec3(-13.6, -3.799, 10.2), 1.0f, 7.0f, 1.0f, 10.0f, true, false);
    Sound whisper = Sound("../resources/audio/whisper.wav", 1.0f, false);
    Sound waterWalk = Sound("../resources/audio/waterWalk.wav", 0.3f, false);

    //Ambient Sounds
    Sound fogAmb = Sound("../resources/audio/wind.wav", 0.3f, true);
    Sound forestAmb = Sound("../resources/audio/bird.wav", vec3(0, 0, 0), 1.0f, 1.0f, 2.0f, 50.0f, true, false);
    Sound desertAmb = Sound("../resources/audio/fog.wav", vec3(0, 0, 0), 1.0f, 1.0f, 2.0f, 50.0f, true, false);

    //Talking Sounds
    Sound deep1 = Sound("../resources/audio/Talking/deep1.wav", 1.0f, false);
    Sound deep2 = Sound("../resources/audio/Talking/deep2.wav", 1.0f, false);
    Sound deep3 = Sound("../resources/audio/Talking/deep3.wav", 1.0f, false);
    Sound mid1 = Sound("../resources/audio/Talking/mid1.wav", 1.0f, false);
    Sound mid2 = Sound("../resources/audio/Talking/mid2.wav", 1.0f, false);
    Sound mid3 = Sound("../resources/audio/Talking/mid3.wav", 1.0f, false);
    Sound high1 = Sound("../resources/audio/Talking/high1.wav", 1.0f, false);
    Sound high2 = Sound("../resources/audio/Talking/high2.wav", 1.0f, false);
    Sound high3 = Sound("../resources/audio/Talking/high3.wav", 1.0f, false);
    Sound high4 = Sound("../resources/audio/Talking/high4.wav", 1.0f, false);
    Sound underwater1 = Sound("../resources/audio/Talking/underwater.wav", 1.0f, false);
    Sound underwater2 = Sound("../resources/audio/Talking/underwater2.wav", 1.0f, false);

    Sound laugh = Sound("../resources/audio/laugh.wav", 1.0f, false);

    sounds.push_back(&walk); // 0
    sounds.push_back(&walk); // 1
    sounds.push_back(&pickup); // 2
    sounds.push_back(&hmm); // 3
    sounds.push_back(&rock); // 4
    sounds.push_back(&fogAmb); // 5
    sounds.push_back(&music); // 6
    sounds.push_back(&alert); // 7
    sounds.push_back(&EVA); // 8
    sounds.push_back(&forestAmb); // 9
    sounds.push_back(&fire); // 10
    sounds.push_back(&whisper); // 11
    sounds.push_back(&deep1); // 12
    sounds.push_back(&deep2); // 13
    sounds.push_back(&deep3); // 14
    sounds.push_back(&mid1); // 15
    sounds.push_back(&mid2); // 16
    sounds.push_back(&mid3); // 17
    sounds.push_back(&high1);// 18 
    sounds.push_back(&high2);// 19
    sounds.push_back(&high3); // 20
    sounds.push_back(&high4); // 21
    sounds.push_back(&underwater1); // 22
    sounds.push_back(&underwater2); // 23 
    sounds.push_back(&laugh); // 24


    Skybox skybox;
    stbi_set_flip_vertically_on_load(false);
    // Default value.
    skybox.init("../resources/skyboxes/sunsky/", false);
    float skyboxMaskAmount = 0.0f;

    Terrain terrain;
    // Default value.
    terrain.init("../resources/heightmaps/lake.jpeg", 16.0f, 16.0f,
                 vec3(0.676, 0.691, 0.484),
                 vec3(0.459, 0.525, 0.275),
                 vec3(0.25, 0.129, 0.000));

    Sun sun = Sun(&m.models.sphere); 

    // Default value
    FogSystem fog = {200.0f, 15.0f, vec4(0.4f, 0.4f, 0.4f, 1.0f)};

    Boundary bound;
    bound.init(vec3(1.0f, 1.0f, 1.0f), -5.0f, terrain.width / 2.0f, 0.0f);

	lvl.LoadLevel("../levels/forest.txt", &objects, &lights, &sun,
				  &emitters, &fog, &skybox, &terrain, &bound);
    Frustum frustum;

    Water water;
    water.gpuSetup();

    // Editor Settings
    bool showObjectEditor = true;
    bool showParticleEditor = false;
    bool showLightEditor = false;
    bool showFogEditor = false;
    bool showShadowEditor = false;
    bool showTerrainEditor = false;
    bool showSkyboxEditor = false;
    bool showBoundaryEditor = false;
    bool showNoteEditor = false;
    bool showSoundEditor = false;
    bool showSunEditor = false;

    bool snapToTerrain = true;

    bool drawTerrain = true;
    bool drawSkybox = true;
    bool drawPointLights = true;
    bool drawParticles = true;

    char skyboxPath[128] = "";
    char terrainPath[128] = "";
    char object_id[3] = "";

    int selectedObject = 0;
    int interactingObject = 0;
    int selectedLight = 0;
    int selectedParticle = 0;
    int selectedSound = 0;

    while (!glfwWindowShouldClose(window))
    {
        fogAmb.updateSound();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        ++frameCount;

        drawnObjects = 0;

        // Input Resolution
        glfwPollEvents();
        processInput(window, &objects, &sounds);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);

		// reset viewport for actual Drawing.
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		mat4 projection = camera.GetProjectionMatrix();
		mat4 view = camera.GetViewMatrix();
		mat4 model;

		frustum.ExtractVFPlanes(projection, view);

		m.DrawAllModels(m.shaders.textureShader, &objects, &lights, &sun.dirLight,
			&fog, &frustum);

		if(drawTerrain)
		{
			terrain.Draw(m.shaders.terrainShader, &lights, &sun.dirLight, &fog);
		}

		// Render Skybox
		if (drawSkybox)
		{
			skybox.Draw(m.shaders.skyboxShader, skyboxMaskAmount);
		}

		// Render Sun
		sun.Draw(m.shaders.sunShader);

		// Render Point Lights
		m.shaders.lightShader.bind();
		{
			m.shaders.lightShader.setMat4("projection", projection);
			m.shaders.lightShader.setMat4("view", view);
			vec3 selectedColor = vec3(1.0f, 0.0f, 0.0f);

			if (drawPointLights)
			{
				for (int i = 0; i < lights.size(); ++i)
				{
					if(selectedLight == i)
					{
						m.shaders.lightShader.setVec3("lightColor", selectedColor);
					}
					else
					{
						m.shaders.lightShader.setVec3("lightColor", lights[i].color);
					}
					model = mat4(1.0f);
					float z_adj = lights[i].position.z;
					if (lights[i].position.x < 0) z_adj -= 0.05f;
					else z_adj += 0.1f;
					model = translate(model, vec3(lights[i].position.x, lights[i].position.y, z_adj));
					model = scale(model, vec3(0.15f, 0.075f, 0.075f));
					m.shaders.lightShader.setMat4("model", model);
					m.models.cube.Draw(m.shaders.lightShader);
				}
			}
		}
		m.shaders.lightShader.unbind();

		if (strcmp(lvl.currentLevel.c_str(), "../levels/forest.txt") == 0) {
			water.Draw(m.shaders.waterShader, deltaTime);
		}

		if (strcmp(lvl.currentLevel.c_str(), "../levels/street.txt") == 0) {
			water.height = -18.5f;
			water.color = vec4(0.15f, 0.15, 0.10f, 0.7f);
			water.Draw(m.shaders.waterShader, deltaTime);
		}

		if(drawParticles)
		{
			bound.height = -7.0f;
			for (int i = 0; i < emitters.size(); ++i)
			{
				emitters[i].Draw(m.shaders.particleShader, 
								 deltaTime, 
								 bound.width, 
								 bound.height,
								 0.0f); // Offset.
			}
		}

		// Render Note
		float minDistance = FLT_MAX;

		bound.DrawWall(m.shaders.boundaryShader, &m.models.cylinder);

        /* Present Render */
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}

float objectDis(vec3 curPos, vec3 objectPos)
{
    return sqrt(pow(curPos.x - objectPos.x, 2) + pow(curPos.z - objectPos.z, 2));
}

void processInput(GLFWwindow *window, vector<Object> *objects, vector<Sound *> *sounds)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos;
    float ypos;

	xpos = static_cast<float>(xposIn);
	ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	printf("Click!\n");
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
