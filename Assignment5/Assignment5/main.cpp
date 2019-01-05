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
#include <irrKlang.h>

#define MESH_PATH "Assets/"
#define SHADER_PATH "Shaders/"
#pragma comment(lib, "irrKlang.lib") // link with irrKlang.dll

using namespace std;
using namespace irrklang;

#pragma region SimpleTypes
typedef struct {
	size_t mPointCount = 0;
	vector<glm::vec3> mVertices;
	vector<glm::vec3> mNormals;
	vector<glm::vec2> mTextureCoords;
	glm::vec3 mDiffuse;
	glm::vec3 mAmbient;
	glm::vec3 mSpecular;
	float mShininess;
	float mOpacity;
} MeshData;

typedef struct {
	size_t mMeshCount = 0;
	size_t mTotalPoints = 0;
	vector<MeshData> mMeshes;
}ModelData;

#pragma endregion SimpleTypes

//Function signatures...
void KeyCallback(unsigned char, int, int);
void MouseCallback(int, int);
void drawMesh(ModelData &, unsigned int &);
void KeyReleaseCallback(unsigned char, int, int);

//GLOBALS...
unsigned int shelbyRedVAO = 0;
unsigned int shelbyBlueVAO = 0;
unsigned int shelbyYellowVAO = 0;
unsigned int shelbyBlackVAO = 0;
unsigned int wallVAO = 0;
unsigned int lampVAO = 0;
unsigned int floorVAO = 0;
int WIDTH = 1600;
int HEIGHT = 900;
ISoundEngine* soundEngine;

ModelData shelbyRedMeshData, shelbyBlueMeshData, shelbyYellowMeshData, shelbyBlackMeshData, wallMeshData;
float deltaTime;
static DWORD last_time = 0;
int mainWindowID;
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool firstMouse = true;

//Lighting stuff
//glm::vec3 pointLightPos(0.0f, 0.0f, 0.0f);
glm::vec3 pointLightPositions[] = {
	glm::vec3(-11.5f, 0.0f, -24.5f),
	glm::vec3(-11.5f, 0.0f, -7.0f),
	glm::vec3(8.5f, 0.0f, -7.0f),
	glm::vec3(8.5f, 0.0f, -24.5f),
	glm::vec3(-1.5f, 0.0f, -24.5f),
	glm::vec3(-1.5f, 0.0f, -7.0f),
	glm::vec3(8.5f, 0.0f, -16.5f),
	glm::vec3(-11.5f, 0.0f, -16.5f),
};

glm::vec3 spotLightPos[] = { 
	glm::vec3(-6.5f, 2.0f, -11.5f),
	glm::vec3(3.5f, 2.0f, -11.5f),
	glm::vec3(-6.5f, 2.0f, -19.5f),
	glm::vec3(3.5f, 2.0f, -19.5f),
};

Shader lightingShader; 
Shader pointLightShader;
Shader texShader;
float lampVertices[] = {
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

float floorVertices[] = {
	 -9.0f, -9.0f, -9.0f,
	 9.0f, -9.0f, -9.0f,
	 9.0f, -9.0f,  9.0f,
	 9.0f, -9.0f,  9.0f,
	-9.0f, -9.0f,  9.0f,
	-9.0f, -9.0f, -9.0f,
};

#pragma region MESH LOADING
ModelData load_mesh(const char* file_name) {
	ModelData modelData;
	cout << file_name;
	const aiScene* scene = aiImportFile(
		file_name,
		 aiProcess_GenNormals | aiProcess_PreTransformVertices | aiProcess_Triangulate
	);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);
	printf("  %i animation\n", scene->mNumAnimations);

	modelData.mMeshCount = scene->mNumMeshes;
	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		MeshData meshData;
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("%i vertices in mesh\n", mesh->mNumVertices);
		meshData.mPointCount += mesh->mNumVertices;
		modelData.mTotalPoints += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				meshData.mVertices.push_back(glm::vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				meshData.mNormals.push_back(glm::vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				//cout << "ASDUASD  ILLI GILLI CHUM BOOM BADAMJINGA" << endl;
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				meshData.mTextureCoords.push_back(glm::vec2(vt->x, vt->y));
			}
		}

		//MATERIALS CODE!! NEW! V1.0
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		aiString texPath;	//filenames of textures
		glm::vec3 def = glm::vec3(1.0f);
		
		aiColor4D diffuse, specular, ambient;
		float shininess;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient))
			meshData.mAmbient = glm::vec3(ambient.r, ambient.g, ambient.b);
		else
			meshData.mAmbient = def;

		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
			meshData.mDiffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
		else
			meshData.mDiffuse = def;

		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular))
			meshData.mSpecular = glm::vec3(specular.r, specular.g, specular.b);
		else
			meshData.mSpecular = def;

		unsigned int max;
		if (AI_SUCCESS == aiGetMaterialFloatArray(material, AI_MATKEY_SHININESS, &shininess, &max))
			meshData.mShininess = shininess;
		else
			meshData.mShininess = 0.0;

		float opacity;
		if (AI_SUCCESS == aiGetMaterialFloatArray(material, AI_MATKEY_OPACITY, &opacity, &max))
			meshData.mOpacity = opacity;
		else
			meshData.mOpacity = 1.0;

		aiString name;
		if (AI_SUCCESS == aiGetMaterialString(material, AI_MATKEY_NAME, &name))
			cout << " NAME: " << name.C_Str() << "\t\n\n";	

		modelData.mMeshes.push_back(meshData);
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateLampBuffer() {

	GLuint pos = glGetAttribLocation(pointLightShader.ID, "position");
	glGenVertexArrays(1, &lampVAO);
	glBindVertexArray(lampVAO);
	unsigned int lampVBO = 0;
	//Now gen buffer objs
	glGenBuffers(1, &lampVBO);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(pos);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), NULL);
}

void generateFloor() {
	GLuint vertexPos = glGetAttribLocation(lightingShader.ID, "aPos");
	GLuint normalPos = glGetAttribLocation(lightingShader.ID, "aNormal");

	glGenVertexArrays(1, &floorVAO);
	glBindVertexArray(floorVAO);
	unsigned int vertexVBO = 0;
	unsigned int normalVBO = 0;
	//Now gen buffer objs
	glGenBuffers(1, &vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(vertexPos);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glVertexAttribPointer(vertexPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);	//CHECK HERE MAYBE!!!!!!!!!!!!!!!!!!!!!!!

	glGenBuffers(1, &normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(normalPos);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glVertexAttribPointer(normalPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

//Args MESH_NAME = Path of Mesh, VAO = destination VAO to store car's data.
ModelData generateObjectBufferData(string MESH_NAME, unsigned int &VAO) {
	//Load model...
	ModelData carMeshData = load_mesh(MESH_NAME.c_str());

	GLuint vertexPos = glGetAttribLocation(lightingShader.ID, "aPos");
	GLuint normalPos = glGetAttribLocation(lightingShader.ID, "aNormal");
	//GLuint texPos = glGetAttribLocation(lightingShader.ID, "aTexCoords"); //=======================================

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int vertexVBO = 0;	//For vertices
	unsigned int normalVBO = 0;	//For normals

	//Now gen buffer objs
	int offset = 0;	//For subbing data in buffer...
	glGenBuffers(1, &vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);	//FOR VERTICES
	glBufferData(GL_ARRAY_BUFFER, carMeshData.mTotalPoints * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
	//offset += carMeshData.mMeshes[0].mPointCount;
	for (int i = 0; i < carMeshData.mMeshCount; i++) {
		int size = carMeshData.mMeshes[i].mPointCount;
		glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(glm::vec3), size * sizeof(glm::vec3),
			&carMeshData.mMeshes[i].mVertices[0]);
		offset += size;
	}
	glEnableVertexAttribArray(vertexPos);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glVertexAttribPointer(vertexPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	offset = 0;
	glGenBuffers(1, &normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);	//FOR NORMALS
	glBufferData(GL_ARRAY_BUFFER, carMeshData.mTotalPoints * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
	for (int i = 0; i < carMeshData.mMeshCount; i++) {
		int size = carMeshData.mMeshes[i].mPointCount;
		glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(glm::vec3), size * sizeof(glm::vec3),
			&carMeshData.mMeshes[i].mNormals[0]);
		offset += size;
	}
	glEnableVertexAttribArray(normalPos);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glVertexAttribPointer(normalPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	/*
	//Adding texture coordinates....
	offset = 0;
	glGenBuffers(1, &texVBO);
	glBindBuffer(GL_ARRAY_BUFFER, texVBO);
	glBufferData(GL_ARRAY_BUFFER, carMeshData.mTotalPoints * sizeof(glm::vec2), NULL, GL_STATIC_DRAW);
	for (int i = 0; i < carMeshData.mMeshCount; i++) {
		int size = carMeshData.mMeshes[i].mPointCount;
		glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(glm::vec2), size * sizeof(glm::vec2),
			&carMeshData.mMeshes[i].mTextureCoords[0]);
		offset += size;
	}
	glEnableVertexAttribArray(texPos);
	glBindBuffer(GL_ARRAY_BUFFER, texVBO);
	glVertexAttribPointer(texPos, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	*/
	return carMeshData;
}
#pragma endregion VBO_FUNCTIONS

void display() {
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Lighting stuff...
	lightingShader.use();
	lightingShader.setVec3("viewPos", camera.position);

	glm::vec3 lightColor = glm::vec3(1.0f);
	glm::vec3 diffuseColor = lightColor * glm::vec3(0.75f);
	glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
	glm::vec3 specularColor = glm::vec3(1.0f);
	float shininess;
	
	//Point Lights
#pragma region PointLights
	lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
	lightingShader.setVec3("pointLights[0].ambient", ambientColor);
	lightingShader.setVec3("pointLights[0].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[0].specular", specularColor);
	lightingShader.setFloat("pointLights[0].constant", 1.0f);
	lightingShader.setFloat("pointLights[0].linear", 0.09);
	lightingShader.setFloat("pointLights[0].quadratic", 0.032);

	lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
	lightingShader.setVec3("pointLights[1].ambient", ambientColor);
	lightingShader.setVec3("pointLights[1].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[1].specular", specularColor);
	lightingShader.setFloat("pointLights[1].constant", 1.0f);
	lightingShader.setFloat("pointLights[1].linear", 0.09);
	lightingShader.setFloat("pointLights[1].quadratic", 0.032);

	lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
	lightingShader.setVec3("pointLights[2].ambient", ambientColor);
	lightingShader.setVec3("pointLights[2].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[2].specular", specularColor);
	lightingShader.setFloat("pointLights[2].constant", 1.0f);
	lightingShader.setFloat("pointLights[2].linear", 0.09);
	lightingShader.setFloat("pointLights[2].quadratic", 0.032);

	lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
	lightingShader.setVec3("pointLights[3].ambient", ambientColor);
	lightingShader.setVec3("pointLights[3].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[3].specular", specularColor);
	lightingShader.setFloat("pointLights[3].constant", 1.0f);
	lightingShader.setFloat("pointLights[3].linear", 0.09);
	lightingShader.setFloat("pointLights[3].quadratic", 0.032);

	lightingShader.setVec3("pointLights[4].position", pointLightPositions[4]);
	lightingShader.setVec3("pointLights[4].ambient", ambientColor);
	lightingShader.setVec3("pointLights[4].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[4].specular", specularColor);
	lightingShader.setFloat("pointLights[4].constant", 1.0f);
	lightingShader.setFloat("pointLights[4].linear", 0.09);
	lightingShader.setFloat("pointLights[4].quadratic", 0.032);

	lightingShader.setVec3("pointLights[5].position", pointLightPositions[5]);
	lightingShader.setVec3("pointLights[5].ambient", ambientColor);
	lightingShader.setVec3("pointLights[5].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[5].specular", specularColor);
	lightingShader.setFloat("pointLights[5].constant", 1.0f);
	lightingShader.setFloat("pointLights[5].linear", 0.09);
	lightingShader.setFloat("pointLights[5].quadratic", 0.032);

	lightingShader.setVec3("pointLights[6].position", pointLightPositions[6]);
	lightingShader.setVec3("pointLights[6].ambient", ambientColor);
	lightingShader.setVec3("pointLights[6].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[6].specular", specularColor);
	lightingShader.setFloat("pointLights[6].constant", 1.0f);
	lightingShader.setFloat("pointLights[6].linear", 0.09);
	lightingShader.setFloat("pointLights[6].quadratic", 0.032);

	lightingShader.setVec3("pointLights[7].position", pointLightPositions[7]);
	lightingShader.setVec3("pointLights[7].ambient", ambientColor);
	lightingShader.setVec3("pointLights[7].diffuse", diffuseColor);
	lightingShader.setVec3("pointLights[7].specular", specularColor);
	lightingShader.setFloat("pointLights[7].constant", 1.0f);
	lightingShader.setFloat("pointLights[7].linear", 0.09);
	lightingShader.setFloat("pointLights[7].quadratic", 0.032);
#pragma endregion

#pragma region SpotLights
	//SpotLight 1
	lightingShader.setVec3("spotLights[0].position", spotLightPos[0]);
	lightingShader.setVec3("spotLights[0].direction", glm::vec3(0.0f, -1.0f, 0.0f));
	lightingShader.setVec3("spotLights[0].ambient", 0.9f, 0.9f, 0.9f);
	lightingShader.setVec3("spotLights[0].diffuse", diffuseColor);
	lightingShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
	lightingShader.setFloat("spotLights[0].constant", 1.0f);
	lightingShader.setFloat("spotLights[0].linear", 0.09);
	lightingShader.setFloat("spotLights[0].quadratic", 0.032);
	lightingShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(30.0f)));
	lightingShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(60.0f)));

	//Spotlight 2
	lightingShader.setVec3("spotLights[1].position", spotLightPos[1]);
	lightingShader.setVec3("spotLights[1].direction", glm::vec3(0.0f, -1.0f, 0.0f));
	lightingShader.setVec3("spotLights[1].ambient", 0.9f, 0.9f, 0.9f);
	lightingShader.setVec3("spotLights[1].diffuse", diffuseColor);
	lightingShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
	lightingShader.setFloat("spotLights[1].constant", 1.0f);
	lightingShader.setFloat("spotLights[1].linear", 0.09);
	lightingShader.setFloat("spotLights[1].quadratic", 0.032);
	lightingShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(30.0f)));
	lightingShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(60.0f)));

	//Spotlight 3
	lightingShader.setVec3("spotLights[2].position", spotLightPos[2]);
	lightingShader.setVec3("spotLights[2].direction", glm::vec3(0.0f, -1.0f, 0.0f));
	lightingShader.setVec3("spotLights[2].ambient", 0.9f, 0.9f, 0.9f);
	lightingShader.setVec3("spotLights[2].diffuse", diffuseColor);
	lightingShader.setVec3("spotLights[2].specular", 1.0f, 1.0f, 1.0f);
	lightingShader.setFloat("spotLights[2].constant", 1.0f);
	lightingShader.setFloat("spotLights[2].linear", 0.09);
	lightingShader.setFloat("spotLights[2].quadratic", 0.032);
	lightingShader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(30.0f)));
	lightingShader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(60.0f)));

	//Spotlight 4
	lightingShader.setVec3("spotLights[3].position", spotLightPos[3]);
	lightingShader.setVec3("spotLights[3].direction", glm::vec3(0.0f, -1.0f, 0.0f));
	lightingShader.setVec3("spotLights[3].ambient", 0.9f, 0.9f, 0.9f);
	lightingShader.setVec3("spotLights[3].diffuse", diffuseColor);
	lightingShader.setVec3("spotLights[3].specular", 1.0f, 1.0f, 1.0f);
	lightingShader.setFloat("spotLights[3].constant", 1.0f);
	lightingShader.setFloat("spotLights[3].linear", 0.09);
	lightingShader.setFloat("spotLights[3].quadratic", 0.032);
	lightingShader.setFloat("spotLights[3].cutOff", glm::cos(glm::radians(30.0f)));
	lightingShader.setFloat("spotLights[3].outerCutOff", glm::cos(glm::radians(60.0f)));
#pragma endregion

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 persp_proj = glm::perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);

	lightingShader.setMat4("view", view);
	lightingShader.setMat4("projection", persp_proj);
	
	//Finally draw cars/objects...
#pragma region CarDrawing
	//Drawing red Shelby first
	glm::mat4 shelbyRedModel = glm::mat4(1.0f);
	shelbyRedModel = glm::translate(shelbyRedModel, glm::vec3(-5.0f, -2.5f, -10.0f));
	shelbyRedModel = glm::rotate(shelbyRedModel, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightingShader.setMat4("model", shelbyRedModel);
	drawMesh(shelbyRedMeshData, shelbyRedVAO);

	//Drawing blue Shelby
	glm::mat4 shelbyBlueModel = shelbyRedModel;		//Blue shelby pos. according to red shelby. => HIERARCHY!
	shelbyBlueModel = glm::rotate(shelbyBlueModel, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shelbyBlueModel = glm::translate(shelbyBlueModel, glm::vec3(7.0f, 0.0f, 0.0f));
	shelbyBlueModel = glm::rotate(shelbyBlueModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightingShader.setMat4("model", shelbyBlueModel);
	drawMesh(shelbyBlueMeshData, shelbyBlueVAO);

	//Draw yellow shelby
	glm::mat4 shelbyYellowModel = shelbyRedModel;		//Blue shelby pos. according to red shelby. => HIERARCHY!
	shelbyYellowModel = glm::rotate(shelbyYellowModel, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shelbyYellowModel = glm::translate(shelbyYellowModel, glm::vec3(0.0f, 0.0f, -8.0f));
	shelbyYellowModel = glm::rotate(shelbyYellowModel, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightingShader.setMat4("model", shelbyYellowModel);
	drawMesh(shelbyYellowMeshData, shelbyYellowVAO);

	//Draw black shelby
	glm::mat4 shelbyBlackModel = shelbyBlueModel;		//Blue shelby pos. according to red shelby. => HIERARCHY!
	shelbyBlackModel = glm::rotate(shelbyBlackModel, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shelbyBlackModel = glm::translate(shelbyBlackModel, glm::vec3(0.0f, 0.0f, -8.0f));
	shelbyBlackModel = glm::rotate(shelbyBlackModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightingShader.setMat4("model", shelbyBlackModel);
	drawMesh(shelbyBlackMeshData, shelbyBlackVAO);
#pragma endregion

	//RIGHT WALL ==============================================================================
	glm::mat4 wallModel1 = glm::mat4(1.0f);
	//wallModel1 = glm::rotate(wallModel1, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel1 = glm::rotate(wallModel1, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel1 = glm::rotate(wallModel1, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel1 = glm::translate(wallModel1, glm::vec3(9.0f, -10.1f, -0.5f));
	wallModel1 = glm::scale(wallModel1, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel1);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel2 = glm::mat4(1.0f);
	wallModel2 = glm::rotate(wallModel2, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel2 = glm::rotate(wallModel2, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel2 = glm::translate(wallModel2, glm::vec3(16.03f, -10.1f, -0.5f));
	wallModel2 = glm::scale(wallModel2, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel2);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel3 = glm::mat4(1.0f);
	wallModel3 = glm::rotate(wallModel3, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel3 = glm::rotate(wallModel3, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel3 = glm::translate(wallModel3, glm::vec3(23.06f, -10.1f, -0.5f));
	wallModel3 = glm::scale(wallModel3, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel3);
	drawMesh(wallMeshData, wallVAO);
	//========================================================================================

	//LEFT WALL ==============================================================================
	glm::mat4 wallModel4 = glm::mat4(1.0f);
	wallModel4 = glm::rotate(wallModel4, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel4 = glm::rotate(wallModel4, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel4 = glm::translate(wallModel4, glm::vec3(9.0f, 13.1f, -0.5f));
	wallModel4 = glm::rotate(wallModel4, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	wallModel4 = glm::scale(wallModel4, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel4);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel5 = glm::mat4(1.0f);
	wallModel5 = glm::rotate(wallModel5, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel5 = glm::rotate(wallModel5, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel5 = glm::translate(wallModel5, glm::vec3(16.03f, 13.1f, -0.5f));
	wallModel5 = glm::rotate(wallModel5, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	wallModel5 = glm::scale(wallModel5, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel5);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel6 = glm::mat4(1.0f);
	wallModel6 = glm::rotate(wallModel6, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel6 = glm::rotate(wallModel6, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel6 = glm::translate(wallModel6, glm::vec3(23.06f, 13.1f, -0.5f));
	wallModel6 = glm::rotate(wallModel6, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	wallModel6 = glm::scale(wallModel6, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel6);
	drawMesh(wallMeshData, wallVAO);
	//========================================================================================

	//BACK WALL ==============================================================================
	glm::mat4 wallModel7 = glm::mat4(1.0f);
	wallModel7 = glm::rotate(wallModel7, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel7 = glm::rotate(wallModel7, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel7 = glm::translate(wallModel7, glm::vec3(26.1f, 13.1f, -0.5f));
	wallModel7 = glm::rotate(wallModel7, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel7 = glm::translate(wallModel7, glm::vec3(-3.4f, 0.0f, 0.0f));	//Final translation
	wallModel7 = glm::scale(wallModel7, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel7);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel8 = glm::mat4(1.0f);
	wallModel8 = glm::rotate(wallModel8, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel8 = glm::rotate(wallModel8, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel8 = glm::translate(wallModel8, glm::vec3(26.1f, 13.1f, -0.5f));
	wallModel8 = glm::rotate(wallModel8, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel8 = glm::translate(wallModel8, glm::vec3(-10.43f, 0.0f, 0.0f));	//Final Translation
	wallModel8 = glm::scale(wallModel8, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel8);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel9 = glm::mat4(1.0f);
	wallModel9 = glm::rotate(wallModel9, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel9 = glm::rotate(wallModel9, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel9 = glm::translate(wallModel9, glm::vec3(26.1f, 13.1f, -0.5f));
	wallModel9 = glm::rotate(wallModel9, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel9 = glm::translate(wallModel9, glm::vec3(-17.46f, 0.0f, 0.0f));	//Final Translation
	wallModel9 = glm::scale(wallModel9, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel9);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel10 = glm::mat4(1.0f);
	wallModel10 = glm::rotate(wallModel10, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel10 = glm::rotate(wallModel10, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel10 = glm::translate(wallModel10, glm::vec3(26.1f, 13.1f, -0.5f));
	wallModel10 = glm::rotate(wallModel10, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel10 = glm::translate(wallModel10, glm::vec3(-24.49f, 0.0f, 0.0f));	//Final Translation
	wallModel10 = glm::scale(wallModel10, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel10);
	drawMesh(wallMeshData, wallVAO);
	//========================================================================================
	
	//FRONT WALL ==============================================================================
	glm::mat4 wallModel11 = glm::mat4(1.0f);
	wallModel11 = glm::rotate(wallModel11, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel11 = glm::rotate(wallModel11, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel11 = glm::translate(wallModel11, glm::vec3(5.5f, -10.1f, -0.5f));
	wallModel11 = glm::rotate(wallModel11, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel11 = glm::scale(wallModel11, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel11);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel12 = glm::mat4(1.0f);
	wallModel12 = glm::rotate(wallModel12, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel12 = glm::rotate(wallModel12, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel12 = glm::translate(wallModel12, glm::vec3(5.5f, -3.07f, -0.5f));
	wallModel12 = glm::rotate(wallModel12, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel12 = glm::scale(wallModel12, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel12);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel13 = glm::mat4(1.0f);
	wallModel13 = glm::rotate(wallModel13, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel13 = glm::rotate(wallModel13, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel13 = glm::translate(wallModel13, glm::vec3(5.5f, 3.96f, -0.5f));
	wallModel13 = glm::rotate(wallModel13, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel13 = glm::scale(wallModel13, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel13);
	drawMesh(wallMeshData, wallVAO);

	glm::mat4 wallModel14 = glm::mat4(1.0f);
	wallModel14 = glm::rotate(wallModel14, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel14 = glm::rotate(wallModel14, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	wallModel14 = glm::translate(wallModel14, glm::vec3(5.5f, 10.99f, -0.5f));
	wallModel14 = glm::rotate(wallModel14, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	wallModel14 = glm::scale(wallModel14, glm::vec3(0.01f, 0.01f, 0.01f));
	lightingShader.setMat4("model", wallModel14);
	drawMesh(wallMeshData, wallVAO);


	//Draw the floor
	glm::mat4 floorModel = glm::mat4(1.0f);
	floorModel = glm::translate(floorModel, glm::vec3(-1.5f, 6.5f, -17.0f));
	floorModel = glm::scale(floorModel, glm::vec3(1.3f, 1.0f, 1.3f));
	glBindVertexArray(floorVAO);
	lightingShader.setMat4("model", floorModel);
	lightingShader.setVec3("material.ambient", glm::vec3(0.0f));
	lightingShader.setVec3("material.diffuse", glm::vec3(1.0f, 0.7f, 0.1f));
	lightingShader.setVec3("material.specular", glm::vec3(0.0f));
	lightingShader.setFloat("material.shininess", 10.0f);
	lightingShader.setFloat("material.opacity", 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//==============================================
	//Now we draw the lamps....
#pragma region lampDrawing
	pointLightShader.use();
	pointLightShader.setMat4("projection", persp_proj);
	pointLightShader.setMat4("view", view);
	//glm::mat4 lampModel1 = glm::mat4(1.0f);
	//lampModel1 = glm::translate(lampModel1, pointLightPos);
	////lampModel = glm::scale(lampModel, glm::vec3(20.0f, 20.0f, 20.0f));
	//pointLightShader.setMat4("model", lampModel1);
	glBindVertexArray(lampVAO);
	//glDrawArrays(GL_TRIANGLES, 0, 36);

	glm::mat4 lampModel2 = glm::mat4(1.0f);
	lampModel2 = glm::translate(lampModel2, spotLightPos[0]);
	lampModel2 = glm::scale(lampModel2, glm::vec3(0.3f, 0.3f, 0.3f));
	pointLightShader.setMat4("model", lampModel2);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glm::mat4 lampModel3 = glm::mat4(1.0f);
	lampModel3 = glm::translate(lampModel3, spotLightPos[1]);
	lampModel3 = glm::scale(lampModel3, glm::vec3(0.3f, 0.3f, 0.3f));
	pointLightShader.setMat4("model", lampModel3);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glm::mat4 lampModel4 = glm::mat4(1.0f);
	lampModel4 = glm::translate(lampModel4, spotLightPos[2]);
	lampModel4 = glm::scale(lampModel4, glm::vec3(0.3f, 0.3f, 0.3f));
	pointLightShader.setMat4("model", lampModel4);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glm::mat4 lampModel5 = glm::mat4(1.0f);
	lampModel5 = glm::translate(lampModel5, spotLightPos[3]);
	lampModel5 = glm::scale(lampModel5, glm::vec3(0.3f, 0.3f, 0.3f));
	pointLightShader.setMat4("model", lampModel5);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	//glm::mat4 tempModel = glm::mat4(1.0f);
	//for (int i = 0; i < pointLightPositions->length(); i++) {
	//	tempModel = glm::translate(tempModel, spotLightPos[i]);
	//	tempModel = glm::scale(tempModel, glm::vec3(0.4f, 0.4f, 0.4f));
	//	pointLightShader.setMat4("model", tempModel);
	//	glDrawArrays(GL_TRIANGLES, 0, 36);
	//}
#pragma endregion

	glutSwapBuffers();
}

void drawMesh(ModelData &model, unsigned int &VAO) {
	glBindVertexArray(VAO);
	int index = 0;
	for (int i = 0; i < model.mMeshCount; i++) {
		lightingShader.setVec3("material.ambient", model.mMeshes[i].mAmbient);
		lightingShader.setVec3("material.diffuse", model.mMeshes[i].mDiffuse);
		lightingShader.setVec3("material.specular", model.mMeshes[i].mSpecular);
		lightingShader.setFloat("material.shininess", model.mMeshes[i].mShininess);
		lightingShader.setFloat("material.opacity", model.mMeshes[i].mOpacity);

		glDrawArrays(GL_QUADS, index, model.mMeshes[i].mPointCount);
		index += model.mMeshes[i].mPointCount;
	}
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
	//Compile and link shaders..
	lightingShader.initializeShader("Shaders/ultraVertexShader.txt", "Shaders/newUltraFragShader.txt");
	pointLightShader.initializeShader("Shaders/lampVertexShader.txt", "Shaders/lampFragmentShader.txt");
	//texShader.initializeShader("Shaders/texVS.txt", "Shaders/texFS.txt");

	//Shelby Cobra Red
	string car1 = MESH_PATH;
	car1.append("CAR_COBRA/Shelby.obj");
	
	//Shelby Cobra Blue
	string car2 = MESH_PATH;
	car2.append("CAR_COBRA_BLUE/Shelby.obj");
	
	//Shelby Cobra Yellow
	string car3 = MESH_PATH;
	car3.append("CAR_COBRA_YELLOW/Shelby.obj");

	//Shelby Cobra Black
	string car4 = MESH_PATH;
	car4.append("CAR_COBRA_BLACK/Shelby.obj");

	//Wall
	string wall = MESH_PATH;
	wall.append("WALL/wall.obj");

	shelbyRedMeshData = generateObjectBufferData(car1, shelbyRedVAO);	
	shelbyBlueMeshData = generateObjectBufferData(car2, shelbyBlueVAO);
	shelbyYellowMeshData = generateObjectBufferData(car3, shelbyYellowVAO);
	shelbyBlackMeshData = generateObjectBufferData(car4, shelbyBlackVAO);
	wallMeshData = generateObjectBufferData(wall, wallVAO);
	generateLampBuffer();
	generateFloor();

	//Create sound engine
	soundEngine = createIrrKlangDevice();
	if (!soundEngine)
		cout << "Sound engine creation failed :(" << endl;
}

#pragma region Controls
//Keyboard controls handler
void KeyCallback(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':	//FORWARD
		camera.ProcessKeyboard(FORWARD, deltaTime);
		if (!soundEngine->isCurrentlyPlaying("Sounds/footsteps.mp3"))
			soundEngine->play2D("Sounds/footsteps.mp3");
		break;
	case 'a':	//LEFT
		camera.ProcessKeyboard(LEFT, deltaTime);
		if (!soundEngine->isCurrentlyPlaying("Sounds/footsteps.mp3"))
			soundEngine->play2D("Sounds/footsteps.mp3");
		break;
	case 's':	//BACKWARD
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (!soundEngine->isCurrentlyPlaying("Sounds/footsteps.mp3"))
			soundEngine->play2D("Sounds/footsteps.mp3");
		break;
	case 'd':	//RIGHT
		camera.ProcessKeyboard(RIGHT, deltaTime);
		if (!soundEngine->isCurrentlyPlaying("Sounds/footsteps.mp3"))
			soundEngine->play2D("Sounds/footsteps.mp3");
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
	case 'z':	//RIGHT
		camera.ProcessKeyboard(UP, deltaTime);
		break;
	case 'x':	//RIGHT
		camera.ProcessKeyboard(DOWN, deltaTime);
		break;
	case 27:
		glutDestroyWindow(mainWindowID);
		exit(0);
		break;
	}
}

void KeyReleaseCallback(unsigned char key, int x, int y) {
	switch (key) {
		case 'w':	//FORWARD
			soundEngine->stopAllSounds();
			break;
		case 'a':	//LEFT
			soundEngine->stopAllSounds();
			break;
		case 's':	//BACKWARD
			soundEngine->stopAllSounds();
			break;
		case 'd':	//RIGHT
			soundEngine->stopAllSounds();
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

	//lastX = x;
	//lastY = y;

	cout << "X OFFSET: " << xOffset << " Y OFFSET: " << yOffset << endl;

	camera.ProcessMouseMovement(xOffset, yOffset);
}
#pragma endregion Controls

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
	glutKeyboardUpFunc(KeyReleaseCallback);
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