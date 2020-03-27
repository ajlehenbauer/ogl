// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <stack>   
#include <sstream>
#include <iostream>
// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;
// Include AntTweakBar
//#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;

const int window_width = 1024, window_height = 768;

typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	void SetPosition(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float *color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float *coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void loadObject(char*, glm::vec4, Vertex * &, GLushort* &, int);
void createVAOs(Vertex[], GLushort[], int);
void createObjects(void);
void pickObject(void);
void renderScene(void);
void cleanup(void);
void lightScene(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);

// GLOBAL VARIABLES
GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;
glm::vec3 camPos;
GLuint gPickedIndex = -1;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

const GLuint NumObjects = 9;	// ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects] = { 0,1,2,3,4,5,6,7,8};
GLuint VertexBufferId[NumObjects] = { 0,1,2,3,4,5,6,7,8 };
GLuint IndexBufferId[NumObjects] = { 0,1,2,3,4,5,6,7,8 };

size_t NumIndices[NumObjects] = { 0,1,2,3,4,5,6,7,8 };
size_t VertexBufferSize[NumObjects] = { 0,1,2,3,4,5,6,7,8 };
size_t IndexBufferSize[NumObjects] = { 0,1,2,3,4,5,6,7,8 };
int numVertices[NumObjects];

GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;

GLint gX = 0.0;
GLint gZ = 0.0;


Vertex *bodyVertex;
Vertex *baseVertex;
Vertex *arm1Vertex;
Vertex *arm2Vertex;
Vertex *balljointVertex;
Vertex *penVertex;
Vertex *projectileVertex;

GLushort* penIdcs;
GLushort* bodyIdcs;
GLushort* arm1Idcs;
GLushort* arm2Idcs;
GLushort* baseIdcs;
GLushort* ballIdcs;
GLushort* projectileIdcs;
// animation control
bool animation = false;
GLfloat phi = 0.0;
int actions;
vec3 * bezier;
void loadObject(char* file, glm::vec4 color, Vertex * &out_Vertices, GLushort* &out_Indices, int ObjectId)
{
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, normals);

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetColor(&color[0]);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	numVertices[ObjectId] = indexed_vertices.size();
	NumIndices[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
	
}

void rotateSingle(float * &r,float x, float y, float a, float b, string d) {
	float theta;
	float alpha;
	if (d == "left") {
		theta = .1;
	}
	else if (d == "right") {
		theta = -.1;
	}
	else if (d == "up") {
		theta = .1;
	}
	else if (d == "down") {
		theta = -.1;
	}
	else {
		theta = 0;
	}
	
		r = new float[2];
		x = x - a;
		y = y - b;
		r[0] = ((x)* cos(theta) - (y)* sin(theta)) + a;
		r[1] = ((x)* sin(theta) + (y)* cos(theta)) + b;

}
void rotateY(float * &r, float x, float z, float a, float b, float theta) {
	r = new float[2];
	x = x - a;
	z = z - b;
	r[0] = ((x)* cos(theta) - (z)* sin(theta)) + a;
	r[1] = ((x)* sin(theta) + (z)* cos(theta)) + b;
}
void createObjects(void)
{
	//-- COORDINATE AXES --//
	Vertex CoordVerts[] =
	{
		{ { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } },
	};
	
	VertexBufferSize[0] = sizeof(CoordVerts);	// ATTN: this needs to be done for each hand-made object with the ObjectID (subscript)
	createVAOs(CoordVerts, NULL, 0);
	
	//-- GRID --//
	
	Vertex GridVerts[44];

	for (int i = 0; i < 11; i++) {
		float a[] = {i-5,0,5,1};
		
		GridVerts[i*2].SetPosition(a);
		
		a[2] = -5;

		GridVerts[i*2+1].SetPosition(a);
		
	}
	for (int i = 0; i < 11; i++) {
		float a[] = {5,0,i-5,1 };

		GridVerts[i * 2+22].SetPosition(a);

		a[0] = -5;

		GridVerts[i * 2 + 23].SetPosition(a);

	}
	for (int i = 0; i < 44; i++) {
		float d[] = { 0.0, 0.0, 1.0 };

		GridVerts[i].SetNormal(d);

		float c[] = { 1,1,1,1 };
		GridVerts[i].SetColor(c);
	}
	unsigned short ind[44];
	for (int i = 0; i < 44; i++) {
		ind[i] = i;
	}
	VertexBufferSize[1] = sizeof(GridVerts);	// ATTN: this needs to be done for each hand-made object with the ObjectID (subscript)
	createVAOs(GridVerts, ind, 1);
	//-- .OBJs --//



	// ATTN: load your models here
	Vertex* Verts;
	GLushort* Idcs;
	
	loadObject("body.obj", glm::vec4(1.0, 0, 0, 1.0), bodyVertex, bodyIdcs, 2);
	createVAOs(bodyVertex, bodyIdcs, 2);
	loadObject("base.obj", glm::vec4(1.0, 1.0, 0, 1.0), baseVertex, baseIdcs, 3);
	createVAOs(baseVertex, baseIdcs, 3);
	loadObject("arm1.obj", glm::vec4(1.0, 0, 1.0, 1.0), arm1Vertex, arm1Idcs, 4);
	createVAOs(arm1Vertex, arm1Idcs, 4);
	loadObject("balljoint.obj", glm::vec4(0, 1.0, 1.0, 1.0), balljointVertex, ballIdcs, 5);
	createVAOs(balljointVertex, ballIdcs, 5);
	loadObject("arm2.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), arm2Vertex, arm2Idcs, 6);
	createVAOs(arm2Vertex, arm2Idcs, 6);
	loadObject("pen.obj", glm::vec4(0, 0, 1.0, 1.0), penVertex, penIdcs, 7);
	createVAOs(penVertex, penIdcs, 7);
	loadObject("balljoint.obj", glm::vec4(0, 0, 1.0, 1.0), projectileVertex, projectileIdcs, 8);
	createVAOs(projectileVertex, projectileIdcs, 8);

}
void rotate(int objectNum, string d) {
	
	
	Vertex  origin;
	switch (objectNum) {
		case 2:
			origin = bodyVertex[numVertices[2]-1];
			break;

		case 4:
			origin = arm1Vertex[2];
			break;
		case 6:
			origin = arm2Vertex[numVertices[6]-1];
			break;
		case 7:
			origin = penVertex[numVertices[7]-1];
	}

	
	if (objectNum <= 7) {
		for (int i = 0; i < numVertices[7]; i++) {
			
			float* r;
			
			if (d == "left" || d == "right") {
				rotateSingle(r, penVertex[i].Position[0], penVertex[i].Position[2],
					origin.Position[0],
					origin.Position[2], d);
				penVertex[i].Position[0] = r[0];
				penVertex[i].Position[2] = r[1];
			}
			else {
				rotateSingle(r, penVertex[i].Position[0], penVertex[i].Position[1],
					origin.Position[0],
					origin.Position[1], d);
				penVertex[i].Position[0] = r[0];
				penVertex[i].Position[1] = r[1];
			}
			
		}
		createVAOs(penVertex, penIdcs, 7);
		
	}
	
	if (objectNum <= 6) {
		for (int i = 0; i < numVertices[6]; i++) {

			float* r;

			if (d == "left" || d == "right") {
				rotateSingle(r, arm2Vertex[i].Position[0], arm2Vertex[i].Position[2],
					origin.Position[0],
					origin.Position[2], d);
				arm2Vertex[i].Position[0] = r[0];
				arm2Vertex[i].Position[2] = r[1];
			}
			else {
				rotateSingle(r, arm2Vertex[i].Position[0], arm2Vertex[i].Position[1],
					origin.Position[0],
					origin.Position[1], d);
				arm2Vertex[i].Position[0] = r[0];
				arm2Vertex[i].Position[1] = r[1];
			}
		}
		createVAOs(arm2Vertex, arm2Idcs, 6);
	}
	if (objectNum <= 5) {
		for (int i = 0; i < numVertices[5]; i++) {

			float* r;

			if (d == "left" || d == "right") {
				rotateSingle(r, balljointVertex[i].Position[0], balljointVertex[i].Position[2],
					origin.Position[0],
					origin.Position[2], d);
				balljointVertex[i].Position[0] = r[0];
				balljointVertex[i].Position[2] = r[1];
			}
			else {
				rotateSingle(r, balljointVertex[i].Position[0], balljointVertex[i].Position[1],
					origin.Position[0],
					origin.Position[1], d);
				balljointVertex[i].Position[0] = r[0];
				balljointVertex[i].Position[1] = r[1];
			}
		}
		createVAOs(balljointVertex, ballIdcs, 5);
	}
	if (objectNum <= 4) {
		for (int i = 0; i < numVertices[4]; i++) {

			float* r;

			if (d == "left" || d == "right") {
				rotateSingle(r, arm1Vertex[i].Position[0], arm1Vertex[i].Position[2],
					origin.Position[0],
					origin.Position[2], d);
				arm1Vertex[i].Position[0] = r[0];
				arm1Vertex[i].Position[2] = r[1];
			}
			else {
				rotateSingle(r, arm1Vertex[i].Position[0], arm1Vertex[i].Position[1],
					origin.Position[0],
					origin.Position[1], d);
				arm1Vertex[i].Position[0] = r[0];
				arm1Vertex[i].Position[1] = r[1];
			}
		}
		
		createVAOs(arm1Vertex, arm1Idcs, 4);
	}
	
	if (objectNum <= 2) {
		for (int i = 0; i < numVertices[2]; i++) {

			float* r;

			if (d == "left" || d == "right") {
				rotateSingle(r, bodyVertex[i].Position[0], bodyVertex[i].Position[2],
					origin.Position[0],
					origin.Position[2], d);
				bodyVertex[i].Position[0] = r[0];
				bodyVertex[i].Position[2] = r[1];
			}
			else {
				rotateSingle(r, bodyVertex[i].Position[0], bodyVertex[i].Position[1],
					origin.Position[0],
					origin.Position[1], d);
				bodyVertex[i].Position[0] = r[0];
				bodyVertex[i].Position[1] = r[1];
			}
		}
		createVAOs(bodyVertex, bodyIdcs, 2);
	}



}
void translate(float x, float z) {

	for (int i = 0; i < numVertices[3]; i++) {
		baseVertex[i].Position[0] += x;
		baseVertex[i].Position[2] += z;
	}
	createVAOs(baseVertex, baseIdcs, 3);
	for (int i = 0; i < numVertices[2]; i++) {
		bodyVertex[i].Position[0] += x;
		bodyVertex[i].Position[2] += z;
	}
	createVAOs(bodyVertex, bodyIdcs, 2);
	for (int i = 0; i < numVertices[4]; i++) {
		arm1Vertex[i].Position[0] += x;
		arm1Vertex[i].Position[2] += z;
	}
	createVAOs(arm1Vertex, arm1Idcs, 4);
	for (int i = 0; i < numVertices[5]; i++) {
		balljointVertex[i].Position[0] += x;
		balljointVertex[i].Position[2] += z;
	}
	createVAOs(balljointVertex, ballIdcs, 5);
	for (int i = 0; i < numVertices[6]; i++) {
		arm2Vertex[i].Position[0] += x;
		arm2Vertex[i].Position[2] += z;
	}
	createVAOs(arm2Vertex, arm2Idcs, 6);
	for (int i = 0; i < numVertices[7]; i++) {
		penVertex[i].Position[0] += x;
		penVertex[i].Position[2] += z;
	}
	createVAOs(penVertex, penIdcs, 7);

}

void renderScene(void)
{
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!


	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	
	glUseProgram(programID);
	{
		glm::vec3 lightPos = camPos;
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		
		
		
		
		glBindVertexArray(VertexArrayId[0]);	// draw CoordAxes
		glDrawArrays(GL_LINES, 0, 6);
		
		glBindVertexArray(VertexArrayId[1]);	// draw CoordAxes
		
		glDrawArrays(GL_LINES, 0, 44);
		
		
		
		glBindVertexArray(VertexArrayId[2]);	// draw body
		
		
		glDrawElements(GL_TRIANGLE_STRIP, 500, GL_UNSIGNED_SHORT,0);
		
		glBindVertexArray(VertexArrayId[3]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[3], GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(VertexArrayId[4]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[4], GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(VertexArrayId[5]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[5], GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(VertexArrayId[6]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[6], GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(VertexArrayId[7]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[7], GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(VertexArrayId[8]);	// draw body
		glDrawElements(GL_TRIANGLE_STRIP, numVertices[8], GL_UNSIGNED_SHORT, 0);





		glBindVertexArray(0);

	}
	glUseProgram(0);
	// Draw GUI
	//TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void pickObject(void)
{
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		
		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		glBindVertexArray(0);

	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);
	
	if (gPickedIndex == 255){ // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}

	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}

int initWindow(void)
{
	camPos = { 10,10,10 };
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Lehenbauer,Anthony(ufid)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	/*
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);
	
	*/
	

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}
void moveCameraLR(bool left) {
	
	float x = camPos[0];
	float y = camPos[1];
	float z = camPos[2];

	
	vec3 v = {x-z/17.32,y,z+x/17.32};
	
	if (!left) {
		v = { x + z / 17.32,y,z - x / 17.32 };
	}
	camPos = v;
	vec3 up = { -v[0],v[1],-v[2] };
	// Camera matrix
	gViewMatrix = glm::lookAt(v,	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		up);	// up
}
void moveCameraZ(bool moveUp) {
	float x = camPos[0];
	float y = camPos[1];
	float z = camPos[2];
	float mag = 17;

	vec3 v = {x-x/mag,y+(mag-y)/ mag,z-z/ mag };
	
	if (!moveUp) {
		v = { x + x/ mag,y - (mag - y) / mag,z + z/ mag };
	}
	camPos = v;
	vec3 up = { -v[0],v[1],-v[2] };
	// Camera matrix
	gViewMatrix = glm::lookAt(v,	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		up);	// up
}
void initOpenGL(void)
{

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f),	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		glm::vec3(0.0, 1.0, 0.0));	// up

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	

	pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");
	
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	


	
	
	createObjects();
}
void createBezier(glm::vec3 * & points) {

	
	vec3 s = { penVertex[numVertices[7]-1].Position[0],penVertex[numVertices[7]-1].Position[1],penVertex[numVertices[7]-1].Position[2] };

	vec3 e = { penVertex[0].Position[0],penVertex[0].Position[1],penVertex[0].Position[2] };
	float d = glm::distance(s, e);
	glm::vec3 v = e-s;
	points = new vec3[4];
	points[0] = e;
	points[1] = e + v;
	vec3 a = { v[0],0,v[2] };
	points[2] = points[1]+a;
	points[3] = points[2] + a;
	points[3][1] = 0;
}
void moveToBezPoint(glm::vec3  points[], float t) {
	cout <<"t:"<< t << endl;
	vec3 a = points[0] * (1-t) + points[1] * t;
	vec3 b = points[1] * (1 - t) + points[2] * t;
	vec3 c = points[2] * (1 - t) + points[3] * t;
	vec3 d = a * (1 - t) + b * t;
	vec3 e = b * (1 - t) + c * t;
	vec3 f = d * (1 - t) + e * t;

	vec3 p = { projectileVertex[0].Position[0],projectileVertex[0].Position[1],projectileVertex[0].Position[2] };
	
	vec3 dist = f-p;

	for (int i = 0; i < numVertices[8]; i++) {
		projectileVertex[i].Position[0] += dist[0];
		projectileVertex[i].Position[1] += dist[1];
		projectileVertex[i].Position[2] += dist[2];

	}

	createVAOs(projectileVertex, projectileIdcs, 8);
	vec3 center = { arm1Vertex[2].Position[0], arm1Vertex[2].Position[1] , arm1Vertex[2].Position[2] };
	vec3 td = points[3] - center ;
	if (t >=1) {
		translate(td[0], td[2]);
	}

}
void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	
	
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);	//
	glBindVertexArray(VertexArrayId[ObjectId]);		//
	
	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	// Assign vertex es
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset); 
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)Normaloffset);

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
			);
	}
}

void cleanup(void)
{
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	cout << actions << endl;
	// ATTN: MODIFY AS APPROPRIATE
	if (action == GLFW_REPEAT) {
		switch (key)
		{
		case GLFW_KEY_LEFT:
			if (actions == -1) {
				moveCameraLR(true);
			}
			else if (actions == 2) {
				translate(.1, 0);
			}
			else {
				rotate(actions,"left");
			}
			
			
			break;

		case GLFW_KEY_RIGHT:
			if (actions == -1) {
				moveCameraLR(false);
			}
			else if (actions == 2) {
				translate(-.1, 0);
			}
			else {
				rotate(actions, "right");
			}
			break;
		case GLFW_KEY_UP:
			if (actions == -1) {
				moveCameraZ(true);
			}
			else if (actions == 2) {
				translate(0, .1);
			}
			else {
				rotate(actions, "up");
			}
			break;
		case GLFW_KEY_DOWN:
			if (actions == -1) {
				moveCameraZ(false);
			}
			else if (actions == 2) {
				translate(0, -.1);
			}
			else {
				rotate(actions, "down");
			}
			break;
		
		default:
			break;
		}
	
	}
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_C:
			actions = -1;
			break;
		case GLFW_KEY_1:
			actions = 4;
			break;
		case GLFW_KEY_2:
			actions = 6;
			break;
		case GLFW_KEY_P:
			actions = 7;
			break;
		case GLFW_KEY_B:
			actions = 2;
			break;
		case GLFW_KEY_T:
			actions = 4;
			break;
		case GLFW_KEY_J:
			animation = true;
			createBezier(bezier);

		}
	}
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickObject();
	}
}

int main(void)
{
	actions = -1;
	// initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// initialize OpenGL pipeline
	initOpenGL();
	
	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		//// Measure speed
		//double currentTime = glfwGetTime();
		//nbFrames++;
		//if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
		//	// printf and reset
		//	printf("%f ms/frame\n", 1000.0 / double(nbFrames));
		//	nbFrames = 0;
		//	lastTime += 1.0;
		//}
		
		if (animation){
			
			vec3 * me;
		;
			createBezier(me);
			//cout << penVertex[0].Position[0] << ", " << penVertex[0].Position[1] << ", " << penVertex[0].Position[2] << endl;
			
			cout << projectileVertex[0].Position[0] << ", "<<projectileVertex[0].Position[1] << ", "<<projectileVertex[0].Position[2] << endl;
			moveToBezPoint(me, phi);
			
			
			phi += 0.01;
			if (phi > 1.01) {
				phi = 0;
				animation = false;
			}
		}

		// DRAWING POINTS
		renderScene();


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}