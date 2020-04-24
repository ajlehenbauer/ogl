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
#include <common/texture.hpp>
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
	float UV[2];
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
	void SetUV(float *coords) {
		UV[0] = coords[0];
		UV[1] = coords[1];
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


Vertex *headVertex;
Vertex *baseVertex;
Vertex *arm1Vertex;
Vertex *arm2Vertex;
Vertex *balljointVertex;
Vertex *penVertex;
Vertex *projectileVertex;

GLushort* penIdcs;
GLushort* headIdcs;
GLushort* arm1Idcs;
GLushort* arm2Idcs;
GLushort* baseIdcs;
GLushort* ballIdcs;
GLushort* projectileIdcs;
// animation control


//TEXTURE STUFF
GLuint TextureProgramID;
// Get a handle for our "MVP" uniform
GLuint TextureMatrixID;
//GLuint Texture = loadBMP_custom("uvtemplate.bmp");
GLuint Texture;
// Get a handle for our "myTextureSampler" uniform
GLuint TextureID;
// Data read from the header of the BMP file

bool showTexture;

unsigned char texHeader[54]; // Each BMP file begins by a 54-bytes header
unsigned int texDataPos;     // Position in the file where the actual data begins
unsigned int texWidth, texHeight;
unsigned int imageSize;   // = width*height*3
// Actual RGB data
unsigned char * texData;
GLuint textureID;



bool animation = false;
GLfloat phi = 0.0;
int actions;
vec3 * bezier;

GLuint loadBMP(const char* imagepath) {
	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("Image could not be opened\n"); return 0; }
	if (fread(texHeader, 1, 54, file) != 54) { // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (texHeader[0] != 'B' || texHeader[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}
	texDataPos = *(int*)&(texHeader[0x0A]);
	imageSize = *(int*)&(texHeader[0x22]);
	texWidth = *(int*)&(texHeader[0x12]);
	texHeight = *(int*)&(texHeader[0x16]);
	if (imageSize == 0)    imageSize = texWidth * texHeight * 3; // 3 : one byte for each Red, Green and Blue component
	if (texDataPos == 0)   texDataPos = 54; // The BMP header is done that way

	// Create a buffer
	texData = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(texData, 1, imageSize, file);

	//Everything is in memory now, the file can be closed
	fclose(file);
}

void createTexture() {
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, texData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void loadObject(char* file, glm::vec4 color, Vertex * &out_Vertices, GLushort* &out_Indices, int ObjectId)
{
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, uvs, normals);

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;

	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetColor(&color[0]);
		out_Vertices[i].SetUV(&indexed_uvs[i].x);
		
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
	
	loadObject("newHeadTest.obj", glm::vec4(.5,.5, .5, 1.0), headVertex, headIdcs, 2);
	createVAOs(headVertex, headIdcs, 2);
	

}

void renderScene(void)
{
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!


	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
		
		
		if (!showTexture) {
			glBindVertexArray(VertexArrayId[2]);	// draw body


			glDrawElements(GL_TRIANGLES, 1200, GL_UNSIGNED_SHORT, 0);
		}
		

		

		glBindVertexArray(0);

	}
	if (showTexture) {
		glUseProgram(TextureProgramID);
		{
			glm::vec3 lightPos = camPos;
			glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
			glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
			glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(TextureMatrixID, 1, GL_FALSE, &MVP[0][0]);



			glBindVertexArray(VertexArrayId[2]);	// draw body

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			// Set our "myTextureSampler" sampler to use Texture Unit 0
			glUniform1i(TextureID, 0);

			glDrawElements(GL_TRIANGLES, 1200, GL_UNSIGNED_SHORT, 0);

			glBindVertexArray(0);
		}
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
		v = { x + x/ mag,y - (mag - y) / mag,z + (17-z)/ mag };
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

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
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
	

	TextureProgramID = LoadShaders("TextureShading.vertexshader", "TextureShading.fragmentshader");
	TextureMatrixID = glGetUniformLocation(TextureProgramID, "MVP");
	
	Texture = loadBMP_custom("Anthony.bmp");
	TextureID = glGetUniformLocation(TextureProgramID, "myTextureSampler");
	
	createObjects();
}


void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	
	
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;
	const size_t UVoffset = sizeof(Vertices[0].Normal) + Normaloffset;

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

	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)UVoffset);


	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal
	glEnableVertexAttribArray(3);	// texture



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
			
			
			
			break;

		case GLFW_KEY_RIGHT:
			if (actions == -1) {
				moveCameraLR(false);
			}
			
			break;
		case GLFW_KEY_UP:
			if (actions == -1) {
				moveCameraZ(true);
			}
			
			break;
		case GLFW_KEY_DOWN:
			if (actions == -1) {
				moveCameraZ(false);
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
		case GLFW_KEY_T:
			showTexture = !showTexture;
			break;
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
	showTexture = false;
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
		
		

		// DRAWING POINTS
		renderScene();


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}