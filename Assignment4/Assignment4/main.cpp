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

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

//GLM includes
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define MESH_NAME_SPHERE "quad_sphere.obj"
#define MESH_NAME_CYLINDER "cylinder.obj"

using namespace std;

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<glm::vec3> mVertices;
	std::vector<glm::vec3> mNormals;
	std::vector<glm::vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID;
GLuint shaderProgramID2;
int activeShader = 0;	//0 for shader1 and 1 for shader2

ModelData mesh_data_sphere;
ModelData mesh_data_cylinder;
unsigned int sphere_vao = 0;
unsigned int cylinder_vao = 0;
int width = 1920;
int height = 1080;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

//Globals for persistence of object data
glm::mat4 identity = glm::mat4(1.0f);
glm::mat4 initTransform = glm::mat4(1.0f);
glm::mat4 global1, global2, global3, global4;;	//FINGER 1
glm::mat4 f2_global1, f2_global2, f2_global3, f2_global4;	//FINGER 2
glm::mat4 f3_global1, f3_global2, f3_global3, f3_global4;	//FINGER 3
glm::mat4 r_gl1, r_gl2, r_gl3, f2_r_gl1, f2_r_gl2, f3_r_gl1, f3_r_gl3;	//FOR ROTATIONS

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
/*
string readShaderSource(const string& fileName)
{
	string temp;
	string output;
	ifstream inputfile(fileName);
	if (inputfile.is_open()) {
		while (getline(inputfile, temp))
			output.append(temp);
	}
	else
		cout << "Unable to access shader..." << endl;
	
	return output;
}
*/
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
	shaderProgramID2 = glCreateProgram();

	if (shaderProgramID == 0 || shaderProgramID2 == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	AddShader(shaderProgramID2, "Shaders/simpleBlueVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID2, "Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };

	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	glLinkProgram(shaderProgramID2);

	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glGetProgramiv(shaderProgramID2, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
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
	glValidateProgram(shaderProgramID2);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID2, GL_VALIDATE_STATUS, &Success);
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
void generateSphereBuffers() {
	mesh_data_sphere = load_mesh(MESH_NAME_SPHERE);
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenVertexArrays(1, &sphere_vao);
	
	//FOR SPHERE
	glBindVertexArray(sphere_vao);
	unsigned int sphere_vbo1 = 0;
	unsigned int sphere_vbo2 = 0;
	//Now gen buffer objs
	glGenBuffers(1, &sphere_vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo1);
	glBufferData(GL_ARRAY_BUFFER, mesh_data_sphere.mPointCount * sizeof(glm::vec3), &mesh_data_sphere.mVertices[0], GL_STATIC_DRAW);
	glGenBuffers(1, &sphere_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo2);	//FOR NORMALS
	glBufferData(GL_ARRAY_BUFFER, mesh_data_sphere.mPointCount * sizeof(glm::vec3), &mesh_data_sphere.mNormals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo1);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo2);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void generateCylinderBuffers() {
	mesh_data_cylinder = load_mesh(MESH_NAME_CYLINDER);
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenVertexArrays(1, &cylinder_vao);
	glBindVertexArray(cylinder_vao);
	unsigned int cylinder_vbo1 = 0;	//For vertices
	unsigned int cylinder_vbo2 = 0;	//For normals
	//Now gen buffer objs
	glGenBuffers(1, &cylinder_vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, cylinder_vbo1);
	glBufferData(GL_ARRAY_BUFFER, mesh_data_cylinder.mPointCount * sizeof(glm::vec3), &mesh_data_cylinder.mVertices[0], GL_STATIC_DRAW);
	glGenBuffers(1, &cylinder_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, cylinder_vbo2);	//FOR NORMALS
	glBufferData(GL_ARRAY_BUFFER, mesh_data_cylinder.mPointCount * sizeof(glm::vec3), &mesh_data_cylinder.mNormals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, cylinder_vbo1);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, cylinder_vbo2);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS

void swapShaders();

void display() {
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 persp_proj = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	view = glm::rotate(view, glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	view = translate(view, glm::vec3(8.0, 1.0, -30.0f));

	glm::mat4 temp = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 1.5f));	//Temp for scaling...

	swapShaders();
	//FINGER 1 SPHERE 1 (ROOT NODE)
	glm::mat4 local1 = global1;
	local1 = local1 * r_gl1;
	local1 = glm::translate(local1, glm::vec3(-10.0f, -10.0f, 0.0f));
	glm::mat4 global1 = local1;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(global1));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	glm::mat4 wrist = glm::mat4(1.0f);
	//extra *= temp;
	wrist *= global1;
	wrist = glm::rotate(wrist, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wrist *= temp;
	//extra = glm::translate(extra, glm::vec3(15.0f, -20.0f, 0.0f));
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(wrist));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	//Finger 1 cylinder 1 (SEGMENT 1)
	glm::mat4 local2 = global2;
	local2 = glm::rotate(local2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	local2 = glm::translate(local2, glm::vec3(0.0f, 0.0f, -6.0f));
	glm::mat4 global2 = local1 * local2 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(global2));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	swapShaders();
	//Finger 1 sphere 2	(SEGMENT 2)
	glm::mat4 local3 = global3;
	local3 = glm::rotate(local3, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 global3 = local1 * local2 * local3;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(global3));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	//Finger 1 cylinder 2
	glm::mat4 local4 = global4;
	local4 = glm::rotate(local4, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
	local4 = glm::translate(local4, glm::vec3(0.0f, 0.0f, -6.0f));
	glm::mat4 global4 = local1 * local2 * local3 * local4 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(global4));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	swapShaders();
	//Finger 2 sphere 1 (BASE - ATTACHED TO SPHERE 1 of finger 1, but can be attached to wrist cylinder as well....the linking 
	//wrist members wouldn't change things too much)
	glm::mat4 f2_local1 = f2_global1;
	//f2_local1 = f2_local1 * f2_r_gl1;
	f2_local1 = glm::translate(f2_local1, glm::vec3(3.0f, 0.0f, 0.0f));
	glm::mat4 f2_global1 = local1 * f2_local1;	//This var is ACTUALLY local. The real global var is OVERHADOWED here. Sorry for poor nomenclature..
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f2_global1));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	//Finger 2 cylinder 1 (SEGMENT 1)
	glm::mat4 f2_local2 = f2_global2;
	f2_local2 = glm::rotate(f2_local2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	f2_local2 = glm::translate(f2_local2, glm::vec3(0.0f, 0.0f, -6.0f));
	glm::mat4 f2_global2 = local1 * f2_local1 * f2_local2 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f2_global2));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	swapShaders();
	//Finger 2 sphere 2	(SEGMENT 2)
	glm::mat4 f2_local3 = f2_global3;
	f2_local3 = glm::rotate(f2_local3, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 f2_global3 = local1 * f2_local1 * f2_local2 * f2_local3;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f2_global3));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	//Finger 2 cylinder 2
	glm::mat4 f2_local4 = f2_global4;
	f2_local4 = glm::rotate(f2_local4, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	f2_local4 = glm::translate(f2_local4, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 f2_global4 = local1 * f2_local1 * f2_local2 * f2_local3 * f2_local4 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f2_global4));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	swapShaders();
	//Finger 3 sphere 3 (BASE of FINGER 3) - PARENT -> FINGER 1 SPHERE 1
	glm::mat4 f3_local1 = f3_global1;
	f3_local1 = glm::translate(f3_local1, glm::vec3(6.0f, 0.0f, 0.0f));
	glm::mat4 f3_global1 = local1 * f3_local1;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f3_global1));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	//Finger 3 cylinder 1 (SEGMENT 1)
	glm::mat4 f3_local2 = f3_global2;
	f3_local2 = glm::rotate(f3_local2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	f3_local2 = glm::translate(f3_local2, glm::vec3(0.0f, 0.0f, -6.0f));
	glm::mat4 f3_global2 = local1 * f3_local1 * f3_local2 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f3_global2));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);
	
	swapShaders();
	//Finger 3 sphere 2	(SEGMENT 2)
	glm::mat4 f3_local3 = f3_global3;
	f3_local3 = glm::rotate(f3_local3, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 f3_global3 = local1 * f3_local1 * f3_local2 * f3_local3;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f3_global3));
	glBindVertexArray(sphere_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_sphere.mPointCount);
	swapShaders();

	//Finger 3 cylinder 2
	glm::mat4 f3_local4 = f3_global4;
	f3_local4 = glm::rotate(f3_local4, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	f3_local4 = glm::translate(f3_local4, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 f3_global4 = local1 * f3_local1 * f3_local2 * f3_local3 * f3_local4 * temp;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(f3_global4));
	glBindVertexArray(cylinder_vao);
	glDrawArrays(GL_QUADS, 0, mesh_data_cylinder.mPointCount);

	glutSwapBuffers();
}

//Not really using much of this except postRedisplay()...
void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}

//Function for swapping shader programs
void swapShaders() {
	if (activeShader == 0) {
		glUseProgram(shaderProgramID2);
		activeShader = 1;
	}
	else {
		glUseProgram(shaderProgramID);
		activeShader = 0;
	}
}

void init()
{
	// Set up the shaders
	CompileShaders();

	// load mesh into a vertex array objects
	generateSphereBuffers();
	generateCylinderBuffers();

	//finger 1
	global1 = glm::mat4(1.0f);
	global2 = glm::mat4(1.0f);
	global3 = glm::mat4(1.0f);
	global4 = glm::mat4(1.0f);

	//finger 2
	f2_global1 = glm::mat4(1.0f);
	f2_global2 = glm::mat4(1.0f);
	f2_global3 = glm::mat4(1.0f);
	f2_global3 = glm::mat4(1.0f);
	f2_global4 = glm::mat4(1.0f);

	//finger 3
	f3_global1 = glm::mat4(1.0f);
	f3_global2 = glm::mat4(1.0f);
	f3_global3 = glm::mat4(1.0f);
	f3_global4 = glm::mat4(1.0f);


	//Rotations...
	r_gl1 = glm::mat4(1.0f);
	r_gl2 = glm::mat4(1.0f);
	f2_r_gl2 = glm::mat4(1.0f);
	f2_r_gl2 = glm::mat4(1.0f);
	f3_r_gl1 = glm::mat4(1.0f);

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':
		//Rotate the finger 1 - segment 2
		global3 = glm::rotate(global3, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 'e':
		//Rotate the finger 2 - segment 2
		f2_global3 = glm::rotate(f2_global3, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 'r':
		//Rotate the finger 3 - segment 2
		f3_global3 = glm::rotate(f3_global3, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 's': //Rotate finger 1 - segment 1
		global2 = glm::rotate(global2, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 'd':	//Rotate finger 2 - segment 1
		f2_global2 = glm::rotate(f2_global2, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 'f':	//Rotate finger 2 - segment 1
		f3_global2 = glm::rotate(f3_global2, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	}
}

int main(int argc, char** argv) {
	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

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