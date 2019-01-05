
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

static const char* pVS = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																 \n\
                                                                              \n\
                                                                               \n\
void main()                                                                     \n\
{                                                                                \n\
    gl_Position = vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);  \n\
	color = vColor;							\n\
}";

// Fragment Shader 1
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS1 = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
in vec4 color;															\n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
FragColor = vec4(color.r, color.g, color.b, 1.0);					\n\
}";

// Fragment Shader 2
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to yellow (format: R, G, B, A).
static const char* pFS2 = "                                                \n\
#version 330                                                               \n\
                                                                           \n\
out vec4 FragColor;                                                        \n\
                                                                           \n\
void main()                                                                \n\
{                                                                          \n\
	FragColor = vec4(1.0, 1.0, 0.0, 1.0);								   \n\
}";

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
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

GLuint CompileShaders(int type)	//type = 0 for RBG and 1 for Yellow shader
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramRGB = glCreateProgram();
	GLuint shaderProgramYellow = glCreateProgram();
	if (shaderProgramRGB == 0 || shaderProgramYellow == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	AddShader(shaderProgramRGB, pVS, GL_VERTEX_SHADER);
	AddShader(shaderProgramRGB, pFS1, GL_FRAGMENT_SHADER);

	AddShader(shaderProgramYellow, pVS, GL_VERTEX_SHADER);
	AddShader(shaderProgramYellow, pFS2, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramRGB);
	glLinkProgram(shaderProgramYellow);

	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramRGB, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramRGB, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glGetProgramiv(shaderProgramYellow, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramYellow, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramRGB);
	glValidateProgram(shaderProgramYellow);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramRGB, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramRGB, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	glGetProgramiv(shaderProgramYellow, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramYellow, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
		//glUseProgram(shaderProgramRGB);

	return (type == 1 ? shaderProgramYellow : shaderProgramRGB);
}


#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBuffer(GLfloat triangle1[], GLfloat triangle2[], GLfloat colors[]) {
	GLuint shaderProgramRGB = CompileShaders(0);
	GLuint shaderProgramYellow = CompileShaders(1);

	glClear(GL_COLOR_BUFFER_BIT);
	GLuint numVertices = 3;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO[2];
	glGenBuffers(2, VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 

	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramRGB, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramRGB, "vColor");

	//RGB Triangle
	glUseProgram(shaderProgramRGB);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * 7 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(GLfloat), triangle2);	//Adding geometry data
	glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), numVertices * 4 * sizeof(GLfloat), colors);
	glEnableVertexAttribArray(positionID);
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices * 3 * sizeof(GLfloat)));
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Yellow Triangle.... (similar as above)
	glUseProgram(shaderProgramYellow);
	GLuint positionID2 = glGetAttribLocation(shaderProgramYellow, "vPosition");
	GLuint colorID2 = glGetAttribLocation(shaderProgramYellow, "vColor");
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * 7 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(GLfloat), triangle1);	//Adding geometry data
	glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), numVertices * 4 * sizeof(GLfloat), colors);
	glEnableVertexAttribArray(positionID2);
	glVertexAttribPointer(positionID2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorID2);
	glVertexAttribPointer(colorID2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices * 3 * sizeof(GLfloat)));
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glutSwapBuffers();
}

void linkCurrentBuffertoShader(GLuint shaderProgramID) {

	// Have to enable this

}
#pragma endregion VBO_FUNCTIONS


void display() {

	// NB: Make the call to draw the geometry in the currently activated vertex buffer. This is where the GPU starts to work!
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//glutSwapBuffers();
}


void init()
{
	// Create 6 vertices that make up a triangle that fits on the viewport 
	GLfloat triangle1[] = {
		// first triangle
		 0.0f,  0.0f, 0.0f,  // top right
		 -0.5f, 1.0f, 0.0f,  // bottom right
		-1.0f,  0.0f, 0.0f,  // top left
	};

	GLfloat triangle2[] = {
		// second triangle
	 0.0f, 0.0f, 0.0f,  // bottom right
	 0.5f, 1.0f, 0.0f,  // bottom left
	 1.0f,  0.0f, 0.0f   // top left
	};

	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = {
			0.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f, 1.0f
	};
	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(triangle1, triangle2, colors);
	// Link the current buffer to the shader
	//linkCurrentBuffertoShader(shaderProgramID);
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Hello Assignment 1");
	// Tell glut where the display function is
	//glutDisplayFunc(display);

	 // A call to glewInit() must be done after glut is initialized!
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