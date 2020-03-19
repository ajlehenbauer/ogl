// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <sstream>
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
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <string>
#include <iostream>
#include <math.h>

using namespace std;
typedef struct Vertex {
	float XYZW[4];
	float RGBA[4];
	void SetCoords(float *coords) {
		XYZW[0] = coords[0];
		XYZW[1] = coords[1];
		XYZW[2] = coords[2];
		XYZW[3] = coords[3];
	}
	void SetColor(float *color) {
		RGBA[0] = color[0];
		RGBA[1] = color[1];
		RGBA[2] = color[2];
		RGBA[3] = color[3];
	}
};

// ATTN: USE POINT STRUCTS FOR EASIER COMPUTATIONS
typedef struct point {
	float x, y, z;
	point(const float x = 0, const float y = 0, const float z = 0) : x(x), y(y), z(z) {};
	point(float *coords) : x(coords[0]), y(coords[1]), z(coords[2]) {};
	point operator -(const point& a)const {
		return point(x - a.x, y - a.y, z - a.z);
	}
	point operator +(const point& a)const {
		return point(x + a.x, y + a.y, z + a.z);
	}
	
	point operator *(const float& a)const {
		return point(x*a, y*a, z*a);
	}
	point operator /(const float& a)const {
		return point(x / a, y / a, z / a);
	}
	float mag(const point& a) {
		return (sqrt(pow(x - a.x, 2) + pow(y - a.y, 2) + pow(z - a.z, 2)));
	}
	float* toArray() {
		float array[] = { x, y, z, 1.0f };
		return array;
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], unsigned short[], size_t, size_t, int);
void createObjects(void);
void bezierPoints(void);
void pickVertex(void);
void moveVertex(void);
void drawScene(void);
void drawSecondVerts(void);
void cleanup(void);
void catCurve(void);
void drawCurve2(void);
void subDivide(Vertex[], Vertex[]);
static void mouseCallback(GLFWwindow*, int, int, int);
static void key_callback(GLFWwindow*, int, int, int, int);
// GLOBAL VARIABLES
GLFWwindow* window;
const GLuint window_width = 1024, window_height = 768;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

// ATTN: INCREASE THIS NUMBER AS YOU CREATE NEW OBJECTS
const GLuint NumObjects = 15;	// number of different "objects" to be drawn
GLuint VertexArrayId[NumObjects] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
GLuint VertexBufferId[NumObjects] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
GLuint IndexBufferId[NumObjects] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
size_t NumVert[NumObjects] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};

GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorArrayID;
GLuint pickingColorID;
GLuint LightID;

// Define objects
Vertex Vertices[] =
{
	{ { 0.8f, 2.0f, 0.0f, 1.0f },{ .1f, 1.0f, 1.0f, 1.0f } }, // 0
	{ { -0.8f, 2.0f, 0.0f, 1.0f },{ .2f, 1.0f, 1.0f, 1.0f } }, // 1
	{ { -1.2f, 0.8f, 0.0f, 1.0f },{ .3f, 1.0f, 1.0f, 1.0f } }, // 3
	{ { 0.0f, 0.0f, 0.0f, 1.0f },{ .4f, 1.0f, 1.0f, 1.0f } }, // 4
	{ { 1.2f, -0.8f, 0.0f, 1.0f },{ .5f, 1.0f, 1.0f, 1.0f } }, // 8
	{ { 0.8f, -2.0f, 0.0f, 1.0f },{ .6f, 1.0f, 1.0f, 1.0f } }, // 5
	{ { -0.8f, -2.0f, 0.0f, 1.0f },{ .7f, 1.0f, 1.0f, 1.0f } }, // 6
	{ { -1.2f, -0.8f, 0.0f, 1.0f },{ .8f, 1.0f, 1.0f, 1.0f } }, // 7
	{ { 0.0f, 0.0f, 0.0f, 1.0f },{ .9f, 1.0f, 1.0f, 1.0f } }, // 4
	{ { 1.2f, 0.8f, 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f, 1.0f } }, // 2
};

unsigned short Indices[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};
unsigned short secondIndices[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

const size_t IndexCount = sizeof(Indices) / sizeof(unsigned short);
// ATTN: DON'T FORGET TO INCREASE THE ARRAY SIZE IN THE PICKING VERTEX SHADER WHEN YOU ADD MORE PICKING COLORS
float pickingColor[IndexCount] = { 0 / 255.0f, 1 / 255.0f, 2 / 255.0f,
3 / 255.0f, 4 / 255.0f, 5 / 255.0f,
6 / 255.0f, 7 / 255.0f, 8 / 255.0f, 9 / 255.0f };




// ATTN: ADD YOU PER-OBJECT GLOBAL ARRAY DEFINITIONS HERE
float blue[] = { .0f,1.f,1.f,1.f };
float red[] = { 1.f,0.f,0.f,1.f };
float yellow[] = { 1.f,1.f,0.f,1.f };
int currentSub=0;
bool showBez;
bool showCat;
bool moveZ;
bool singleV;
Vertex secondView[10];
Vertex cachedVertex;
Vertex sub1[20];
unsigned short sub1Ind[20];
Vertex sub2[40];
unsigned short sub2Ind[40];
Vertex sub3[80];
unsigned short sub3Ind[80];
Vertex sub4[160];
unsigned short sub4Ind[160];
Vertex sub5[320];
unsigned short sub5Ind[320];
Vertex bezPoints[40];
unsigned short bezPointInd[40];
Vertex bezEval[100];
unsigned short bezEvalInd[100];
Vertex catPoints[40];
unsigned short catPointInd[40];
Vertex catEval[100];
unsigned short catEvalInd[100];

Vertex bezPoints2[40];
unsigned short bezPointInd2[40];
Vertex bezEval2[100];
unsigned short bezEvalInd2[100];
Vertex catPoints2[40];
unsigned short catPointInd2[40];
Vertex catEval2[100];
unsigned short catEvalInd2[100];

point evalBezPoint(point a, point b, point c, point d, float t) {
	point e = a * t + b * (1 - t);
	point f = b * t + c * (1 - t);
	point g = c * t + d * (1 - t);

	point h = e * t + f * (1 - t);
	point i = f * t + g * (1 - t);

	return h * t + i * (1 - t);
}


void evalCat(void) {
	float red[] = { 1.0f,.0f,.0f,1.0f };
	for (int i = 0; i < 100; i++) {
		catEvalInd[i] = i;
		catEvalInd2[i] = i;
	}
	
	for (int i = 0; i < 10; i++) {
		point a = catPoints[i * 4].XYZW;
		point b = catPoints[i * 4 + 1].XYZW;
		point c = catPoints[i * 4 + 2].XYZW;
		point d = catPoints[i * 4 + 3].XYZW;

		for (int j = 0; j < 10; j++) {
			
			catEval[i * 10 + j].SetCoords(evalBezPoint(d, c, b, a, float(j) / 10).toArray());
			catEval[i * 10 + j].SetColor(yellow);
		}

	}
	cout << bezEval[10].XYZW[0]<<", "<< bezEval[10].XYZW[1]<<endl;
}
void evalBez() {
	float red[] = { 1.0f,.0f,.0f,1.0f };
	for (int i = 0; i < 100; i++) {
		bezEvalInd[i] = i;
		bezEvalInd2[i] = i;
	}

	for (int i = 0; i < 10; i++) {
		point a = bezPoints[i * 4].XYZW;
		point b = bezPoints[i * 4 + 1].XYZW;
		point c = bezPoints[i * 4 + 2].XYZW;
		point d = bezPoints[i * 4 + 3].XYZW;

		for (int j = 0; j < 10; j++) {

			bezEval[i * 10 + j].SetCoords(evalBezPoint(d, c, b, a, float(j) / 10).toArray());
			bezEval[i * 10 + j].SetColor(red);
		}

	}
}
void bezierPoints() {
	for (int i = 0; i < 40; i++) {
		
		bezPointInd[i] = i;
		bezPointInd2[i] = i;
		
	}

	for (int i = 0; i < 10; i++) {
		point a = Vertices[i].XYZW;
		point b;
		if (i < 9) {
			b = Vertices[i + 1].XYZW;
		}
		
		if (i == 9) {
			b = Vertices[0].XYZW;
		}
		
		point c = ((a * 2) + b) / 3;

		bezPoints[i * 4 + 1].SetCoords(c.toArray());
		c = (a + (b * 2)) / 3;
		bezPoints[i * 4 + 2].SetCoords(c.toArray());
	}
	
	for (int i = 1; i < 10; i++) {
		point a = bezPoints[i * 4 - 2].XYZW;
		point b = bezPoints[i * 4 + 1].XYZW;
		
		bezPoints[i * 4].SetCoords(((a + b) / 2).toArray());
		bezPoints[i * 4 - 1].SetCoords(((a + b) / 2).toArray());
		
	}
	point a = point(bezPoints[1].XYZW);
	point b = point(bezPoints[38].XYZW);
	bezPoints[0].SetCoords(((a + b) / 2).toArray());
	bezPoints[39] = bezPoints[0];
	
	for (int i = 0; i < 40; i++) {
		float yellow[] = { 1.f,1.f,.0f,1.0f };
		bezPoints[i].SetColor(yellow);
		if (bezPoints[i].XYZW[0]==0) {
		//cout << i << endl;
		}
		
	}
	
	evalBez();
}




void createObjects(void)
{
	
	for (int i = 0; i++; i < 40) {
		bezPointInd[i] = i;
	}
	for (int i = 0; i < 40; i++) {
		catPointInd[i] = i;
		catPointInd2[i] = i;
	}
	for (int i = 0; i < 20; i++) {
		sub1Ind[i] = i;
	}
	for (int i = 0; i < 40; i++)
	{
		sub2Ind[i] = i;
	}
	for (int i = 0; i < 80; i++)
	{
		sub3Ind[i] = i;
	}
	for (int i = 0; i < 160; i++)
	{
		sub4Ind[i] = i;
		
	}
	
	for (int i = 0; i < 320; i++)
	{
		sub5Ind[i] = i;
	}
	
	// ATTN: DERIVE YOUR NEW OBJECTS HERE:
	// each has one vertices {pos;color} and one indices array (no picking needed here)
	
	for (int i = 0; i < 10; i++) {
		point b = Vertices[i].XYZW;
		point a;
		if (i == 0) {
			
			 a = Vertices[9].XYZW;

		}
		else {
			 a = Vertices[i - 1].XYZW;			
		}
		point c = ((a * 4) + (b * 4)) / 8;
		sub1[2 * i].SetCoords(c.toArray());
		sub1[2 * i].SetColor(blue);

		if (i != 10) {
			point b = Vertices[i].XYZW;
			point c = Vertices[i + 1].XYZW;
			 
			if (i == 0) {
				a = Vertices[9].XYZW;
			}
			else {
				a = Vertices[i - 1].XYZW;	
			}
			point d = (a + (b * 6) + c)/8;
			sub1[2*i+1].SetCoords(d.toArray());
			sub1[2 * i+1].SetColor(blue);
		}

	}
	point a = Vertices[8].XYZW;
	point b = Vertices[9].XYZW;
	point c = Vertices[0].XYZW;
	point d = (a + (b * 6) + c) / 8;
	sub1[19].SetCoords(d.toArray());
	sub1[19].SetColor(Vertices[0].RGBA);
	subDivide(sub1, sub2);
	subDivide(sub2, sub3);
	subDivide(sub3, sub4);
	subDivide(sub4, sub5);
	//cout << sub2[0].XYZW[0] << ", " << sub2[0].XYZW[1] << endl;
	bezierPoints();
	catCurve();
}
void subDivide(Vertex arr1[], Vertex arr2[]) {
	
	int sizeA = *(&arr1 +1) - arr1;
	sizeA = 10*(sizeA/10);
	if (sizeA == 170) {
		sizeA = 160;
	}
	//cout << sizeA << endl;
	int sizeB = sizeA*2;
	

	//cout << sizeA <<endl;
	for (int i = 0; i < sizeA; i++) {
		point b = arr1[i].XYZW;
		point a;
		if (i == 0) {

			a = arr1[sizeA-1].XYZW;

		}
		else {
			a = arr1[i - 1].XYZW;
		}
		point c = ((a * 4) + (b * 4)) / 8;
		arr2[2 * i].SetCoords(c.toArray());
		arr2[2 * i].SetColor(blue);

		if (i != sizeA) {
			point b = arr1[i].XYZW;
			point c = arr1[i + 1].XYZW;

			if (i == 0) {
				a = arr1[sizeA-1].XYZW;
			}
			else {
				a = arr1[i - 1].XYZW;
			}
			point d = (a + (b * 6) + c) / 8;
			arr2[2 * i + 1].SetCoords(d.toArray());
			arr2[2 * i + 1].SetColor(arr1[0].RGBA);
		}

	}
	point a = arr1[sizeA-2].XYZW;
	point b = arr1[sizeA-1].XYZW;
	point c = arr1[0].XYZW;
	point d = (a + (b * 6) + c) / 8;
	arr2[sizeB-1].SetCoords(d.toArray());
	arr2[sizeB-1].SetColor(arr1[0].RGBA);
	
}
void catCurve(void) {
	point a;
	point b;
	float m;
	point currVert;
	point nextVert;
	point prevVert;
	point sNextVert;
	float d;
	for (int i = 0; i < 9; i++) {
		
		catPoints[i * 4] = Vertices[i];
		catPoints[i * 4 + 3] = Vertices[i+1];
		if (i == 0) {
			currVert = Vertices[i].XYZW;
			nextVert = Vertices[i + 1].XYZW;
			sNextVert = Vertices[i + 2].XYZW;
			prevVert = Vertices[9].XYZW;
			a = prevVert;
			b = nextVert;
			d = currVert.mag(nextVert)/3;
			m = b.mag(a);
			catPoints[i * 4 + 1].SetCoords( ( ( ( (b-a)/m )*d)+currVert ).toArray() );
			a = currVert;
			b = sNextVert;
			d = currVert.mag(nextVert) / 3;
			m = b.mag(a);
			catPoints[i * 4 + 2].SetCoords( (nextVert+( ( (a - b) / m)*d)  ).toArray());

		}
		else if (i == 8) {
			currVert = Vertices[i].XYZW;
			nextVert = Vertices[i + 1].XYZW;
			sNextVert = Vertices[0].XYZW;
			prevVert = Vertices[i-1].XYZW;
			a = prevVert;
			b = nextVert;
			d = currVert.mag(nextVert) / 3;
			m = b.mag(a);
			catPoints[i * 4 + 1].SetCoords(((((b - a) / m)*d) + currVert).toArray());
			a = currVert;
			b = sNextVert;
			d = currVert.mag(nextVert) / 3;
			m = b.mag(a);
			catPoints[i * 4 + 2].SetCoords((nextVert + (((a - b) / m)*d)).toArray());
		}
		else {
			currVert = Vertices[i].XYZW;
			nextVert = Vertices[i + 1].XYZW;
			sNextVert = Vertices[i + 2].XYZW;
			prevVert = Vertices[i-1].XYZW;
			a = prevVert;
			b = nextVert;
			d = currVert.mag(nextVert) / 3;
			m = b.mag(a);
			catPoints[i * 4 + 1].SetCoords(((((b - a) / m)*d) + currVert).toArray());
			a = currVert;
			b = sNextVert;
			m = b.mag(a);
			catPoints[i * 4 + 2].SetCoords((nextVert + (((a - b) / m)*d)).toArray());
		}
	}
	currVert = Vertices[9].XYZW;
	nextVert = Vertices[0].XYZW;
	sNextVert = Vertices[1].XYZW;
	prevVert = Vertices[8].XYZW;
	catPoints[36] = Vertices[9];
	catPoints[39] = Vertices[0];


	a = prevVert;
	b = nextVert;
	d = currVert.mag(nextVert) / 3;
	m = b.mag(a);
	
	catPoints[37].SetCoords(((((b - a) / m)*d) + currVert).toArray());
	a = currVert;
	b = sNextVert;
	d = currVert.mag(nextVert) / 3;
	m = b.mag(a);
	catPoints[38].SetCoords(((((a - b) / m)*d) + nextVert).toArray());
	
	for (int i = 0; i < 40; i++) {
		catPoints[i].SetColor(red);
		
	}
	evalCat();
}
void drawScene(void)
{
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(programID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		glEnable(GL_PROGRAM_POINT_SIZE);

		glBindVertexArray(VertexArrayId[0]);	// draw Vertices
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);	// update buffer data
																			//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
		glDrawElements(GL_POINTS, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
		glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
		// ATTN: OTHER BINDING AND DRAWING COMMANDS GO HERE, one set per object:
		//glBindVertexArray(VertexArrayId[<x>]); etc etc
		
		if (currentSub == 1) {
			//subdiv 1
			glBindVertexArray(VertexArrayId[1]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[1]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sub1), sub1);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[1], GL_UNSIGNED_SHORT, (void*)0);
			

		}
		if (currentSub == 2) {
			//subdiv 2
			glBindVertexArray(VertexArrayId[4]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[4]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sub2), sub2);	// update buffer data
																		//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[4], GL_UNSIGNED_SHORT, (void*)0);

		}
		if (currentSub == 3) {
			//subdiv 3
			glBindVertexArray(VertexArrayId[5]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[5]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sub3), sub3);	// update buffer data
																		//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[5], GL_UNSIGNED_SHORT, (void*)0);

		}
		if (currentSub == 4) {
			//subdiv 3
			glBindVertexArray(VertexArrayId[6]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[6]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sub4), sub4);	// update buffer data
																		//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[6], GL_UNSIGNED_SHORT, (void*)0);

		}
		if (currentSub == 5) {
			//subdiv 3
			glBindVertexArray(VertexArrayId[7]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[7]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sub5), sub5);	// update buffer data
																		//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[7], GL_UNSIGNED_SHORT, (void*)0);

		}
		
		if (showBez) {
			//bezier control points
			glBindVertexArray(VertexArrayId[2]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[2]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bezPoints), bezPoints);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[2], GL_UNSIGNED_SHORT, (void*)0);

			//bezier lines
			glBindVertexArray(VertexArrayId[3]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[3]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bezEval), bezEval);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_LINE_LOOP, NumVert[3], GL_UNSIGNED_SHORT, (void*)0);



			if (!singleV) {
				//bezier control points
				glBindVertexArray(VertexArrayId[11]);	// draw Vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[11]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bezPoints2), bezPoints2);	// update buffer data
																					//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
				glDrawElements(GL_POINTS, NumVert[11], GL_UNSIGNED_SHORT, (void*)0);

				//bezier lines
				glBindVertexArray(VertexArrayId[12]);	// draw Vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[12]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bezEval2), bezEval2);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
				glDrawElements(GL_LINE_LOOP, NumVert[12], GL_UNSIGNED_SHORT, (void*)0);
			}
			
		}
		if (showCat) {
			//cat control points
			glBindVertexArray(VertexArrayId[8]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[8]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(catPoints), catPoints);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[8], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_LINE_LOOP, NumVert[8], GL_UNSIGNED_SHORT, (void*)0);

			//cat lines
			//cat control points
			glBindVertexArray(VertexArrayId[9]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[9]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(catEval), catEval);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_LINE_LOOP, NumVert[9], GL_UNSIGNED_SHORT, (void*)0);
			if (!singleV) {
				//cat control points
				glBindVertexArray(VertexArrayId[13]);	// draw Vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[13]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(catPoints2), catPoints2);	// update buffer data
																					//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
				glDrawElements(GL_POINTS, NumVert[13], GL_UNSIGNED_SHORT, (void*)0);
				glDrawElements(GL_LINE_LOOP, NumVert[13], GL_UNSIGNED_SHORT, (void*)0);

				//cat lines
				//cat control points
				glBindVertexArray(VertexArrayId[14]);	// draw Vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[14]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(catEval2), catEval2);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
				glDrawElements(GL_LINE_LOOP, NumVert[14], GL_UNSIGNED_SHORT, (void*)0);
			}
		}
		if (!singleV) {
			//cat control points
			glBindVertexArray(VertexArrayId[10]);	// draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[10]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(secondView), secondView);	// update buffer data
																				//glDrawElements(GL_LINE_LOOP, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_POINTS, NumVert[10], GL_UNSIGNED_SHORT, (void*)0);
			glDrawElements(GL_LINE_LOOP, NumVert[10], GL_UNSIGNED_SHORT, (void*)0);
			
		}
		
		
		
		glBindVertexArray(0);
		
	}
	glUseProgram(0);
	// Draw GUI
	//TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void pickVertex(void)
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
		glUniform1fv(pickingColorArrayID, NumVert[0], pickingColor);	// here we pass in the picking marker array

																		// Draw the ponts
		glEnable(GL_PROGRAM_POINT_SIZE);
		glBindVertexArray(VertexArrayId[0]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);	// update buffer data
		glDrawElements(GL_POINTS, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);

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
	
	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}
void doubleView(void) {
	point a;
	singleV = false;
	for (int i = 0; i < 10; i++) {

		a = Vertices[i].XYZW;
		a = a / 2;
		a.y = a.y + 1.5;
		Vertices[i].SetCoords(a.toArray());

	}
	drawSecondVerts();
	createObjects();
	drawCurve2();
}
void drawSecondVerts() {
	point a;
	point b;
	for (int i = 0; i < 10; i++) {

		a = Vertices[i].XYZW;
		a.y = a.y - 3;
		b = a;
		a.x = a.z;
		a.z = b.x;
		secondView[i].SetCoords(a.toArray());
		secondView[i].SetColor(blue);
	}
}
void drawCurve2(void) {
	point a;
	point b;
	for (int i = 0; i < 40; i++) {

		a = bezPoints[i].XYZW;
		a.y = a.y - 3;
		b = a;
		a.x = a.z;
		a.z = b.x;
		bezPoints2[i].SetCoords(a.toArray());
		bezPoints2[i].SetColor(yellow);
	}
	for (int i = 0; i < 100; i++) {

		a = bezEval[i].XYZW;
		a.y = a.y - 3;
		b = a;
		a.x = a.z;
		a.z = b.x;
		bezEval2[i].SetCoords(a.toArray());
		bezEval2[i].SetColor(red);
	}
	for (int i = 0; i < 40; i++) {

		a = catPoints[i].XYZW;
		a.y = a.y - 3;
		b = a;
		a.x = a.z;
		a.z = b.x;
		catPoints2[i].SetCoords(a.toArray());
		catPoints2[i].SetColor(red);
	}
	for (int i = 0; i < 100; i++) {

		a = catEval[i].XYZW;
		a.y = a.y - 3;
		b = a;
		a.x = a.z;
		a.z = b.x;
		catEval2[i].SetCoords(a.toArray());
		catEval2[i].SetColor(yellow);
	}
}
void singleView(void) {
	point a;
	singleV = true;
	for (int i = 0; i < 10; i++) {

		a = Vertices[i].XYZW;		
		a.y = a.y - 1.5;
		a = a * 2;
		Vertices[i].SetCoords(a.toArray());

	}
}

void moveZVertex(void) {
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec4 vp = glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]);

	// retrieve your cursor position
	// get your world coordinates
	// move points
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	float wx, wy;
	wx = -(xpos - 512) * 4 / 512;
	wy = -(ypos - 384) * 3 / 384;
	
	float x, y, z;
	x = Vertices[gPickedIndex].XYZW[0];
	y = Vertices[gPickedIndex].XYZW[1];
	
	z = cachedVertex.XYZW[2] + y - wy;

	Vertices[gPickedIndex] = { { x,y,z,1.0f },{ .5f, 0.0f, 0.0f, 1.0f } };
	drawSecondVerts();
	drawCurve2();
	cout << z << endl;
}
// fill this function in!
void moveVertex(void)
{
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec4 vp = glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]);

	// retrieve your cursor position
	// get your world coordinates
	// move points
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	float wx, wy;
	wx = -(xpos - 512) * 4 / 512;
	wy = -(ypos - 384) * 3 / 384;
	
	Vertices[gPickedIndex] = { { wx,wy,0.0f,1.0f },{ .5f, 0.0f, 0.0f, 1.0f } };

	if (gPickedIndex == 255) { // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}
	drawSecondVerts();
}

int initWindow(void)
{
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // FOR MAC

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Lehenbauer,Anthony(99543343)", NULL, NULL);
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
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetKeyCallback(window, key_callback);
	return 0;
}

void initOpenGL(void)
{
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	//glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

																			// Camera matrix
	gViewMatrix = glm::lookAt(
		glm::vec3(0, 0, -5), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Create and compile our GLSL program from the shaders
	programID = programID = LoadShaders("hw1bShade.vertexshader", "hw1bShade.fragmentshader");
	pickingProgramID = LoadShaders("hw1bPick.vertexshader", "hw1bPick.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorArrayID = glGetUniformLocation(pickingProgramID, "PickingColorArray");
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	createVAOs(Vertices, Indices, sizeof(Vertices), sizeof(Indices), 0);
	
	createObjects();
	bezierPoints();
	evalBez();
	createVAOs(sub1, sub1Ind, sizeof(sub1), sizeof(sub1Ind), 1);
	createVAOs(sub2, sub2Ind, sizeof(sub2), sizeof(sub2Ind), 4);
	createVAOs(sub3, sub3Ind, sizeof(sub3), sizeof(sub3Ind), 5);
	createVAOs(sub4, sub4Ind, sizeof(sub4), sizeof(sub4Ind), 6);
	createVAOs(sub5, sub5Ind, sizeof(sub5), sizeof(sub5Ind), 7);
	createVAOs(bezPoints, bezPointInd, sizeof(bezPoints), sizeof(bezPointInd), 2);
	createVAOs(bezEval, bezEvalInd, sizeof(bezEval), sizeof(bezEvalInd), 3);
	createVAOs(catPoints, catPointInd, sizeof(catPoints), sizeof(catPointInd), 8);
	createVAOs(catEval, catEvalInd, sizeof(catEval), sizeof(catEvalInd), 9);
	createVAOs(secondView, secondIndices, sizeof(secondView), sizeof(secondIndices), 10);

	createVAOs(bezPoints2, bezPointInd2, sizeof(bezPoints2), sizeof(bezPointInd2), 11);
	createVAOs(bezEval2, bezEvalInd2, sizeof(bezEval2), sizeof(bezEvalInd2), 12);
	createVAOs(catPoints2, catPointInd2, sizeof(catPoints2), sizeof(catPointInd2), 13);
	createVAOs(catEval2, catEvalInd2, sizeof(catEval2), sizeof(catEvalInd2), 14);
	

	// ATTN: create VAOs for each of the newly created objects here:
	// createVAOs(<fill this appropriately>);
	

}

void createVAOs(Vertex Vertices[], unsigned short Indices[], size_t BufferSize, size_t IdxBufferSize, int ObjectId) {

	NumVert[ObjectId] = IdxBufferSize / (sizeof GLubyte);

	GLenum ErrorCheckValue = glGetError();
	size_t VertexSize = sizeof(Vertices[0]);
	size_t RgbOffset = sizeof(Vertices[0].XYZW);

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, BufferSize, Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	glGenBuffers(1, &IndexBufferId[ObjectId]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IdxBufferSize, Indices, GL_STATIC_DRAW);

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color

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

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickVertex();
		cachedVertex = Vertices[gPickedIndex];
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		pickVertex();
		for (int i = 0; i < 10; i++) {
			
			Vertices[i].SetColor(blue);
		}
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		showBez = !showBez;
	}
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		currentSub++;
		if (currentSub > 5) {
			currentSub = 0;
		}
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		showCat = !showCat;
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		if (singleV) {
			doubleView();
		}
		else {
			singleView();
		}
		
	}
		
}


int main(void)
{
	singleV = true;
	showBez = false;
	showCat = false;
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
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1sec ago
											 // printf and reset
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// DRAGGING: move current (picked) vertex with cursor
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		{
			moveZVertex();
		}
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
		{
			moveVertex();
		}
		

		// DRAWING SCENE
		createObjects();	// re-evaluate curves in case vertices have been moved
		drawScene();


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}
