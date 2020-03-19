// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sstream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
using namespace glm;

// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

typedef struct {
	float XYZW[4];
	float RGBA[4];
} Vertex;

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //UNCOMMENT IF ON MAC

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Anthony Lehenbauer(99543343).", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
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
	TwWindowSize(1024, 768);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	std::string message;
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &message, NULL);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	//glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	glm::mat4 ProjectionMatrix = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	glm::mat4 ViewMatrix = glm::lookAt(
		glm::vec3(0, 0, -5), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);

	
	// Define points
	Vertex Vertices[] =
	{
		{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 0

		{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // 1

		{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }, // 2

		{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 3
		

		{ { .0f, 1.42f, 0.0f, 1.0f },{ 1.0f, 1.0f, 0.0f, 1.0f } }, // 4
		
		{ { -1.42f, .0f, 0.0f, 1.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } }, // 5

		{ { .0f, -1.42f, 0.0f, 1.0f },{ 1.0f, 0.0f, 1.0f, 1.0f } }, // 6

		{ { 1.42f, .0f, 0.0f, 1.0f },{ .1f, .1f, .1f, 1.0f } } // 7
		
	};

	unsigned short Indices[] = {
		0, 1, 2, 3, 4, 5, 6, 7
	};

	const size_t BufferSize = sizeof(Vertices);
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].XYZW);
	const size_t IndexCount = sizeof(Indices) / sizeof(unsigned short);

	float pickingColor[IndexCount] = { 0 / 255.0f, 1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f, 5 / 255.0f, 6 / 255.0f, 7 / 255.0f };		// set this procedurally for greater number of points

	// Create Vertex Array Object
	GLuint VertexArrayId;
	glGenVertexArrays(1, &VertexArrayId);
	glBindVertexArray(VertexArrayId);

	// Create and load Buffer for vertex data
	GLuint VertexBufferId;
	glGenBuffers(1, &VertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, BufferSize, Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	GLuint IndexBufferId;
	glGenBuffers(1, &IndexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
	
	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("p1_StandardShading.vertexshader", "p1_StandardShading.fragmentshader");
	GLuint pickingProgramID = LoadShaders("p1_Picking.vertexshader", "p1_Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	GLuint pickingColorArrayID = glGetUniformLocation(pickingProgramID, "PickingColorArray");
	GLuint pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	
	do{
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}
		
		// update vertex buffer each frame so that shaders can use updated vertex data (HINT: useful when dragging, changing color)
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);	
		glBufferSubData(GL_ARRAY_BUFFER, 0, BufferSize, Vertices);	// update buffer data

		// PICKING IS DONE HERE
		// (Instead of picking each frame if the mouse button is down, 
		// you should probably only check if the mouse button was just released)
		
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)){

			// Clear the screen in white
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(pickingProgramID);
			{
				// Only the positions are needed (not the colors)
				glEnableVertexAttribArray(0);

				glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

				// Send our transformation to the currently bound shader, in the "MVP" uniform
				glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

				glUniform1fv(pickingColorArrayID, IndexCount, pickingColor);	// here we pass in the picking marker array

				// 1rst attribute buffer : vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
				// Assign vertex attributes
				glVertexAttribPointer(
					0,                  // attribute
					4,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					VertexSize,         // stride
					(void*)0            // array buffer offset
					);

				// Index buffer
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);

				// Draw the ponts
				glEnable(GL_PROGRAM_POINT_SIZE);
				glDrawElements(
					GL_POINTS,      // mode
					IndexCount,    // count
					GL_UNSIGNED_SHORT,   // type
					(void*)0           // element array buffer offset
					);

				glDisableVertexAttribArray(0);

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
				glReadPixels(xpos, 768-ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top
				
				float wx, wy;
				wx = -(xpos - 512) * 3 / 512;
				wy = -(ypos - 384) * 3 / 384;
				// Convert the color back to an integer ID
				int pickedID = data[0];
				//1024, 768
				if (pickedID == 255){ // Full white, must be the background !
					message = "background";
				}
				else {
					std::ostringstream oss;

					oss << "point " << pickedID;

					message = oss.str();

				}
				
				



				if (pickedID == 0) {
					
					Vertices[0] = {{ wx,wy,0.0f,1.0f }, { .5f, 0.0f, 0.0f, 1.0f }};
				}
				else if (pickedID == 1) {
					Vertices[1] = { { wx,wy,0.0f,1.0f },{ 0.0f, .5f, 0.0f, 1.0f } };
				}
				else if (pickedID == 2) {
					Vertices[2] = { { wx,wy,0.0f,1.0f },{ .5f, 0.0f, .9f, 1.0f } };
				}
				else if (pickedID == 3) {
					Vertices[3] = { { wx,wy,0.0f,1.0f },{ .5f, .5f, .5f, 1.0f } };
				}
				else if (pickedID == 4) {
					Vertices[4] = { { wx,wy,0.0f,1.0f },{ .5f, .5f, .0f, 1.0f } };
				}
				else if (pickedID == 5) {
					Vertices[5] = { { wx,wy,0.0f,1.0f },{ .0f, .5f, .5f, 1.0f } };
				}
				else if (pickedID == 6) {
					Vertices[6] = { { wx,wy,0.0f,1.0f },{ .5f, .0f, .5f, 1.0f } };
				}
				else if (pickedID == 7) {
					Vertices[7] = { { wx,wy,0.0f,1.0f },{ .5f, .5f, .5f, 1.0f } };
				}
				
		
			}
			// Uncomment these lines to see the picking shader in effect
			//glfwSwapBuffers(window);
			//continue; // skips the normal rendering
		}
		else {




			

		}


		// Dark blue background
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
		// Re-clear the screen for real rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Use our shader
		glUseProgram(programID);
		{
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			glm::vec3 lightPos = glm::vec3(4, 4, 4);
			glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);


			// 1rst attribute buffer : vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
			glVertexAttribPointer(
				0,                  // attribute
				4,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				VertexSize,         // stride
				(void*)0            // array buffer offset
				);

			// 2nd attribute buffer : colors
			glVertexAttribPointer(
				1,                  // attribute
				4,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				VertexSize,         // stride
				(GLvoid*)RgbOffset  // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);
			
			// Draw the triangles !
			glEnable(GL_PROGRAM_POINT_SIZE);
			glDrawElements(
				GL_POINTS,      // mode
				IndexCount,    // count
				GL_UNSIGNED_SHORT,   // type
				(void*)0           // element array buffer offset
				);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
		}
		// Draw GUI
		TwDraw();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &VertexBufferId);
	glDeleteBuffers(1, &IndexBufferId);
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);
	glDeleteVertexArrays(1, &VertexArrayId);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

