// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include "Shader.h"

#define MESH_PATH "Assets/"
#define SHADER_PATH "Shaders/"

using namespace std;

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	vector<glm::vec3> mVertices;
	vector<glm::vec3> mNormals;
	vector<glm::vec2> mTextureCoords;
	vector<aiMaterial> Materials;
} ModelData;
#pragma endregion SimpleTypes

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TextCoords;
};

//Function signatures...
void KeyCallback(unsigned char, int, int);
void MouseCallback(int, int);
//void DoMovement();

//GLOBALS...
GLuint shaderProgramID, lampShaderID, lightingShaderID;
unsigned int carsVAO = 0;
unsigned int otherObjectsVAO = 0;
unsigned int cubeVAO = 0;
int WIDTH = 1920;
int HEIGHT = 1080;
ModelData carMeshData;
ModelData objectMeshData;
glm::mat4 global1;
float deltaTime;
static DWORD last_time = 0;
int mainWindowID;
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool firstMouse = true;

//Lighting stuff
glm::vec3 lightPos(0.0f, 2.0f, -7.0f);

#pragma region MESH LOADING
ModelData load_mesh(const char* file_name) {
	ModelData modelData;
	cout << file_name;
	const aiScene* scene = aiImportFile(
		file_name,
		aiProcess_PreTransformVertices | aiProcess_GenNormals
	);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);
	printf("  %i animation\n", scene->mNumAnimations);


	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(glm::vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(glm::vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(glm::vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}

		}

		//MATERIALS CODE!! NEW! V1.0
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		aiString texPath;	//filenames of textures

		aiColor3D diffuse, specular, ambient;
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

void CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	lampShaderID = glCreateProgram();
	lightingShaderID = glCreateProgram();

	if (shaderProgramID == 0 || lampShaderID == 0 || lightingShaderID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	AddShader(lampShaderID, "Shaders/lampVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(lampShaderID, "Shaders/lampFragmentShader.txt", GL_FRAGMENT_SHADER);

	AddShader(lightingShaderID, "Shaders/lightingVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(lightingShaderID, "Shaders/lightingFragmentShader.txt", GL_FRAGMENT_SHADER);


	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };

	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << shaderProgramID << " " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glLinkProgram(lampShaderID);
	glGetProgramiv(lampShaderID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(lampShaderID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << lampShaderID << " " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glLinkProgram(lightingShaderID);
	glGetProgramiv(lightingShaderID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(lightingShaderID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << lightingShaderID << " " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(lampShaderID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(lampShaderID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glValidateProgram(lightingShaderID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(lightingShaderID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Finally, use the linked shader program, using shader1 by default
	glUseProgram(shaderProgramID);
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBuffer(string MESH_NAME) {
	objectMeshData = load_mesh(MESH_NAME.c_str());
	GLuint loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	GLuint loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	GLuint loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenVertexArrays(1, &otherObjectsVAO);

	//FOR SPHERE
	glBindVertexArray(otherObjectsVAO);
	unsigned int objectVBO1 = 0;
	unsigned int objectVBO2 = 0;
	//Now gen buffer objs
	glGenBuffers(1, &objectVBO1);
	glBindBuffer(GL_ARRAY_BUFFER, objectVBO1);
	glBufferData(GL_ARRAY_BUFFER, objectMeshData.mPointCount * sizeof(glm::vec3), &objectMeshData.mVertices[0], GL_STATIC_DRAW);
	glGenBuffers(1, &objectVBO2);
	glBindBuffer(GL_ARRAY_BUFFER, objectVBO2);	//FOR NORMALS
	glBufferData(GL_ARRAY_BUFFER, objectMeshData.mPointCount * sizeof(glm::vec3), &objectMeshData.mNormals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, objectVBO1);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, objectVBO2);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void generateCubeBuffer() {
	GLuint loc1 = glGetAttribLocation(lampShaderID, "vertex_position");
	cout << " LIGHT CUBE ATTRIB LOC1: " << loc1 << endl;
	float vertices[] = {
		-0.25f, -0.25f, -0.25f,
		 0.25f, -0.25f, -0.25f,
		 0.25f,  0.25f, -0.25f,
		 0.25f,  0.25f, -0.25f,
		-0.25f,  0.25f, -0.25f,
		-0.25f, -0.25f, -0.25f,

		-0.25f, -0.25f,  0.25f,
		 0.25f, -0.25f,  0.25f,
		 0.25f,  0.25f,  0.25f,
		 0.25f,  0.25f,  0.25f,
		-0.25f,  0.25f,  0.25f,
		-0.25f, -0.25f,  0.25f,

		-0.25f,  0.25f,  0.25f,
		-0.25f,  0.25f, -0.25f,
		-0.25f, -0.25f, -0.25f,
		-0.25f, -0.25f, -0.25f,
		-0.25f, -0.25f,  0.25f,
		-0.25f,  0.25f,  0.25f,

		 0.25f,  0.25f,  0.25f,
		 0.25f,  0.25f, -0.25f,
		 0.25f, -0.25f, -0.25f,
		 0.25f, -0.25f, -0.25f,
		 0.25f, -0.25f,  0.25f,
		 0.25f,  0.25f,  0.25f,

		-0.25f, -0.25f, -0.25f,
		 0.25f, -0.25f, -0.25f,
		 0.25f, -0.25f,  0.25f,
		 0.25f, -0.25f,  0.25f,
		-0.25f, -0.25f,  0.25f,
		-0.25f, -0.25f, -0.25f,

		-0.25f,  0.25f, -0.25f,
		 0.25f,  0.25f, -0.25f,
		 0.25f,  0.25f,  0.25f,
		 0.25f,  0.25f,  0.25f,
		-0.25f,  0.25f,  0.25f,
		-0.25f,  0.25f, -0.25f,
	};

	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);
	unsigned int cubeVBO = 0;
	//Now gen buffer objs
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

}

void generateCarObjectBuffer(string MESH_NAME) {
	carMeshData = load_mesh(MESH_NAME.c_str());
	GLuint loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	GLuint loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	GLuint loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");
	cout << " CAR MESH ATTRIBS\t LOC1: " << loc1 << " LOC2: " << loc2 << endl;

	glGenVertexArrays(1, &carsVAO);
	glBindVertexArray(carsVAO);

	unsigned int carVBO1 = 0;	//For vertices
	unsigned int carVBO2 = 0;	//For normals

	//Now gen buffer objs
	glGenBuffers(1, &carVBO1);
	glBindBuffer(GL_ARRAY_BUFFER, carVBO1);	//FOR VERTICES
	glBufferData(GL_ARRAY_BUFFER, carMeshData.mPointCount * sizeof(glm::vec3), &carMeshData.mVertices[0], GL_STATIC_DRAW);
	glGenBuffers(1, &carVBO2);
	glBindBuffer(GL_ARRAY_BUFFER, carVBO2);	//FOR NORMALS
	glBufferData(GL_ARRAY_BUFFER, carMeshData.mPointCount * sizeof(glm::vec3), &carMeshData.mNormals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, carVBO1);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, carVBO2);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS

void display() {
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");
	//GLuint light_location = glGetAttribLocation(shaderProgramID, "lightPosition");
	//glUniform4f(light_location, 1, lightPos.x, lightPos.y, lightPos.z);

	//glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 persp_proj = glm::perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
	//view = glm::rotate(view, glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//view = translate(view, glm::vec3(0.0f, -3.0f, 0.0f));

	//swapShaders();
	//FINGER 1 SPHERE 1 (ROOT NODE)
	glm::mat4 global1 = glm::mat4(1.0f);
	global1 = glm::translate(global1, glm::vec3(-0.8f, -2.5f, -10.0f));
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(global1));
	glBindVertexArray(carsVAO);
	glDrawArrays(GL_QUADS, 0, carMeshData.mPointCount);

	glutSwapBuffers();
}

//Not really using much of this except postRedisplay()...
void updateScene() {
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	deltaTime = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	CompileShaders();

	string car1 = MESH_PATH;
	car1.append("CAR_COBRA/Shelby.obj");
	cout << "GENERATING CAR BUFFER...." << endl;
	generateCarObjectBuffer(car1);
	cout << "GENERATING CUBE BUFFER...." << endl;
	generateCubeBuffer();
}

//Keyboard controls handler
void KeyCallback(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':	//FORWARD
		camera.ProcessKeyboard(FORWARD, deltaTime);
		break;
	case 'a':	//LEFT
		camera.ProcessKeyboard(LEFT, deltaTime);
		break;
	case 's':	//BACKWARD
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		break;
	case 'd':	//RIGHT
		camera.ProcessKeyboard(RIGHT, deltaTime);
		break;
	case 'e':	//RIGHT
		camera.ProcessKeyboard(ROTATE_RIGHT, deltaTime);
		break;
	case 'q':	//RIGHT
		camera.ProcessKeyboard(ROTATE_LEFT, deltaTime);
		break;
	case 'r':	//RIGHT
		camera.ProcessKeyboard(LOOK_UP, deltaTime);
		break;
	case 'f':	//RIGHT
		camera.ProcessKeyboard(LOOK_DOWN, deltaTime);
		break;
	case 27:
		glutDestroyWindow(mainWindowID);
		exit(0);
		break;
	}
}

//Mouse movement handler
void MouseCallback(int x, int y) {
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xOffset = x - lastX;
	GLfloat yOffset = y - lastY;

	lastX = x;
	lastY = y;

	cout << "X OFFSET: " << xOffset << " Y OFFSET: " << yOffset << endl;

	camera.ProcessMouseMovement(xOffset, yOffset);
}

int main(int argc, char** argv) {
	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WIDTH, HEIGHT);
	mainWindowID = glutCreateWindow("Graphics Assignment 5");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(KeyCallback);
	//glutPassiveMotionFunc(MouseCallback);		//FIX AND ENABLE LATER PLS

	// A call to glewInit() must be done after glut is initialized!
	glewExperimental = GL_TRUE;
	GLenum res = glewInit();

	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}