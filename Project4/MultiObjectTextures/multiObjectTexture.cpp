//Multi-Object, Multi-Texture Example
//Stephen J. Guy, 2025 

//This example demonstrates:
// Loading multiple models (a cube and a knot)
// Using multiple textures (wood and brick)
// Instancing (the teapot is drawn in two locations)
// Continuous keyboard input - arrows (moves knot up/down/left/right continuous when being held)
// Keyboard modifiers - shift (up/down arrows move knot in/out of screen when shift is pressed)
// Single key events - pressing 'c' changes color of a random teapot
// Mixing textures and colors for models
// Phong lighting
// Binding multiple textures to one shader

const char* INSTRUCTIONS = 
"***************\n"
"Objective: Find your way out of the maze by collecting keys to unlock doors.\n"
"\n"
"Important keys:\n\n"
"Up/down/left/right - Moves the player.\n"
"M - Toggles the map, click again to change back.\n"
"***************\n"
;

//Mac OS build: g++ multiObjectTexture.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL3 -framework OpenGL -o MultiObjTest
//Linux build:  clang++ -Wall multiObjectTexture.cpp -xc glad/glad.c -F ~/Library/Frameworks -framework SDL3 -Wl,-rpath,$HOME/Library/Frameworks && ./a.out 

#include "map.h"
#include "model.h"
#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL3/SDL.h>
 #include <SDL3/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

struct Texture{
    GLuint wood;
	GLuint brick;
	GLuint checkered;
};

int screenWidth = 800; 
int screenHeight = 600;  
float timePast = 0;
float lastTime = 0;

//SJG: Store the object coordinates
//You should have a representation for the state of each object
float objx=0, objy=0, objz=0;
float colR=1, colG=1, colB=1;

glm::vec3 characterPos;
bool charInitialized = false;
bool moveUp = false, moveDown = false, moveLeft = false, moveRight = false;

glm::vec3 cameraPos, cameraFront, cameraUp, cameraRight;
float pitch = 0.0f;    
float yaw = 90.0f;   
float mouseSensitivity = 0.1f;

bool DEBUG_ON = true;
bool mapView = false;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

//srand(time(NULL));
float rand01(){
	return rand()/(float)RAND_MAX;
}

void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts);
void renderMap(Map& map, int shaderProgram, Model& model, Texture& textures);
void playableCharacter(Map& map, float dt);
void updateCamera();

int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//------------Test Different Maps----------------
	// Map map = parseMapFile("noDoors.txt");
	// Map map = parseMapFile("doorWithKey.txt");
	// Map map = parseMapFile("allDoorAndKeys.txt");
	Map map = parseMapFile("noLeaving.txt");
	//-----------------------------------------------
	


    //Print the version of SDL we are using (should be 3.x or higher)
    const int sdl_linked = SDL_GetVersion();
    printf("\nCompiled against SDL version %d.%d.%d ...\n", SDL_VERSIONNUM_MAJOR(SDL_VERSION), SDL_VERSIONNUM_MINOR(SDL_VERSION), SDL_VERSIONNUM_MICRO(SDL_VERSION));
    printf("Linking against SDL version %d.%d.%d.\n", SDL_VERSIONNUM_MAJOR(sdl_linked), SDL_VERSIONNUM_MINOR(sdl_linked), SDL_VERSIONNUM_MICRO(sdl_linked));

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (title, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", screenWidth, screenHeight, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_SetWindowRelativeMouseMode(window, 1); //1 = enabled

	
	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}
	//Here we will load two different model files 
	
	//Load Wall
	std::ifstream modelFile;
	modelFile.open("models/cube.txt");
	int numLines = 0;
	if (modelFile.is_open()){
		modelFile >> numLines;
	}
	float* model1 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model1[i];
	}
	printf("%d\n",numLines);
	int numVertsCube = numLines/8;
	modelFile.close();
	
	//Load Model 2
	modelFile.open("models/teapot.txt");
	numLines = 0;
	if (modelFile.is_open()){
		modelFile >> numLines;
	}
	float* model2 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model2[i];
	}
	printf("%d\n",numLines);
	int numVertsTeapot = numLines/8;
	modelFile.close();

	//Load Floor
	modelFile.open("models/floor.txt");
	numLines = 0;
	if (modelFile.is_open()) {
		modelFile >> numLines; 
	}

	float* model3 = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> model3[i];
	}
	int numVertsFloor = numLines / 8; 
	modelFile.close();

	modelFile.open("models/sphere.txt");
	numLines = 0;
	if (modelFile.is_open()) {
		modelFile >> numLines; 
	}
	float* model4 = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> model4[i];
	}
	int numVertsSphere = numLines / 8; 
	modelFile.close();

	
	//SJG: I load each model in a different array, then concatenate everything in one big array
	// This structure works, but there is room for improvement here. Eg., you should store the start
	// and end of each model a data structure or array somewhere.
	//Concatenate model arrays
	int totalNumVerts = numVertsCube + numVertsTeapot + numVertsFloor + numVertsSphere;
	float* modelData = new float[totalNumVerts * 8];
	std::copy(model1, model1 + numVertsCube * 8, modelData);
	std::copy(model2, model2 + numVertsTeapot * 8, modelData + numVertsCube * 8);
	std::copy(model3, model3 + numVertsFloor * 8, modelData + (numVertsCube + numVertsTeapot) * 8);
	std::copy(model4, model4 + numVertsSphere * 8, modelData + (numVertsCube + numVertsTeapot + numVertsFloor) * 8);

	int startVertCube = 0;
	int startVertTeapot = numVertsCube;
	int startVertFloor = numVertsCube + numVertsTeapot;
	int startVertSphere = numVertsCube + numVertsTeapot + numVertsFloor;

	Model models;
	models.startCube = startVertCube;
	models.countCube = numVertsCube;

	models.startFloor = startVertFloor;
	models.countFloor = numVertsFloor;

	models.startDoor = startVertCube;
	models.countDoor = numVertsCube;

	models.startKey = startVertTeapot;
	models.countKey = numVertsTeapot;

	models.startCharacter = startVertSphere;
	models.countCharacter = numVertsSphere;

	Texture textures;
	
	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex0;
    glGenTextures(1, &tex0);
	textures.wood = tex0;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //Load the texture into memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_DestroySurface(surface);
    //// End Allocate Texture ///////


	//// Allocate Texture 1 (Brick) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("brick.bmp");
	if (surface1==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex1;
    glGenTextures(1, &tex1);
	textures.brick = tex1;
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE1);
    
    glBindTexture(GL_TEXTURE_2D, tex1);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface1->w,surface1->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface1->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_DestroySurface(surface1);
	//// End Allocate Texture ///////


	//// Allocate Texture 2 (Checkered Pavement) ///////
	SDL_Surface* surface2 = SDL_LoadBMP("checkered.bmp");
	if (surface2==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex2;
    glGenTextures(1, &tex2);
	textures.checkered = tex2;
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //Load the texture into memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface2->w,surface2->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface2->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_DestroySurface(surface2);
    //// End Allocate Texture ///////

	
	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts*8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

	int texturedShader = InitShader("textured-Vertex.glsl", "textured-Fragment.glsl");	

	
	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);
	
	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");


	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	
                       	
	glEnable(GL_DEPTH_TEST);  

	printf("%s\n",INSTRUCTIONS);
	
	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue
			if (windowEvent.type == SDL_EVENT_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_EVENT_KEY_UP && windowEvent.key.key == SDLK_ESCAPE) 
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_EVENT_KEY_UP && windowEvent.key.key == SDLK_F){ //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}
			//move position based on bool values, use the bool values to do actual movement in playableCharacter()
			if (windowEvent.type == SDL_EVENT_KEY_DOWN) {
				if (windowEvent.key.key == SDLK_UP) {
					moveUp = true;
				}
				if (windowEvent.key.key == SDLK_DOWN) {
					moveDown = true;
				}
				if (windowEvent.key.key == SDLK_LEFT) {
					moveLeft = true;
				}
				if (windowEvent.key.key == SDLK_RIGHT) {
					moveRight = true;
				}
				//toggle map view
				if (windowEvent.key.key == SDLK_M) {
					mapView = !mapView;
				}
			}
			if (windowEvent.type == SDL_EVENT_KEY_UP) {
				if (windowEvent.key.key == SDLK_UP) {
					moveUp = false;
				}
				if (windowEvent.key.key == SDLK_DOWN) {
					moveDown = false;
				}
				if (windowEvent.key.key == SDLK_LEFT) {
					moveLeft = false;
				}
				if (windowEvent.key.key == SDLK_RIGHT) {
					moveRight = false;
				}
			}
			//Hangle mouse movement
			if (windowEvent.type == SDL_EVENT_MOUSE_MOTION) {
				float mouseWD = windowEvent.motion.xrel; //up-down mouse movement
				float mouseLR = windowEvent.motion.yrel; //left-right mouse movement
				mouseWD *= mouseSensitivity;
				mouseLR *= mouseSensitivity;
				yaw += mouseWD;
				pitch -= mouseLR;
			}
		}
		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);

		timePast = SDL_GetTicks()/1000.f; 
		float dt = timePast - lastTime;
    	lastTime = timePast;

		// playableCharacter(map, dt);
		cameraPos = characterPos + glm::vec3(0.0f, 0.5f, 0.0f);
		updateCamera();


		float mapWidth = map.width;
		float mapHeight = map.height;
		float distY = 1.5f * std::max(mapWidth, mapHeight);

		glm::mat4 view;
		glm::vec3 flashlightPos;
		glm::vec3 flashlightDir;
		float cutOff;
		//Change perspective (top-down or first-person) based on map toggle
		if (mapView) {
			view = glm::lookAt(
				glm::vec3(mapWidth/2.0f, distY, mapHeight/2.0f), 
				glm::vec3(mapWidth/2.0f, 0.0f, mapHeight/2.0f), 
				glm::vec3(0.f, 0.f, 1.f)                          
			);
			//remove flashlight in mapView
			flashlightPos = glm::vec3(0.0f);        
			flashlightDir = glm::vec3(0.0f, 0.0f, 1.0f); 
			cutOff = -1.0f;
		} else {
			view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
			//flashlight based on camera in first-person pov
			flashlightPos = glm::vec3(view * glm::vec4(cameraPos, 1.0f));
			flashlightDir = glm::normalize(glm::mat3(view) * cameraFront);
		}
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		//change the ambient amount depending on the gamemode (less in first-person, more in map view)
		float ambientAmount = mapView ? 0.5f : 0.2f; 
		glUniform1f(glGetUniformLocation(texturedShader, "ambient"), ambientAmount);

		//pass appropriate values to the shader
		glUniform3fv(glGetUniformLocation(texturedShader, "light.position"), 1, glm::value_ptr(flashlightPos));
		glUniform3fv(glGetUniformLocation(texturedShader, "light.direction"), 1, glm::value_ptr(flashlightDir));
		glUniform1f(glGetUniformLocation(texturedShader, "light.cutOff"), glm::cos(glm::radians(12.5f)));

		glm::mat4 proj = glm::perspective(glm::radians(45.0f), screenWidth / (float) screenHeight, 0.1f, 50.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glUniform1i(glGetUniformLocation(texturedShader, "tex2"), 2);

		glBindVertexArray(vao);
		// drawGeometry(texturedShader, startVertCube, numVertsCube, startVertKnot, numVertsKnot);
		renderMap(map, texturedShader, models, textures);
		playableCharacter(map, dt);
		SDL_GL_SwapWindow(window); //Double buffering
	}
	
    delete[] modelData;
    delete[] model1;
    delete[] model2;

	//Clean Up
	glDeleteProgram(texturedShader);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DestroyContext(context);
	SDL_Quit();
	return 0;
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}

//Used OpenGL camera tutorial for reference
void updateCamera() {
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(dir);
    cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0,1,0)));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

void drawStartEnd(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts, const Texture& textures) {
    glUseProgram(shaderProgram);

	//load the checkered texture from checkered.bmp
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures.checkered);
    glUniform1i(uniTexID, 2);

	//position based on the map position
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(position.x, -1.0f, position.z));
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");    
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, modelStart, modelVerts);
}

void drawWall(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts, const Texture& textures) {
    glUseProgram(shaderProgram);

	//load the brick texture
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures.brick);
    glUniform1i(uniTexID, 1);

	//position the walls based on the map position
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, position);
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");    
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, modelStart, modelVerts);
}

void drawFloor(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts, const Texture& textures) {
    glUseProgram(shaderProgram);

	//load the wood texture
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.wood);
    glUniform1i(uniTexID, 0);

	//position the floor based on map position
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(position.x, -0.5f, position.z));
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, modelStart, modelVerts);
}

//Match the color of the keys with the door
glm::vec3 getColor(char c) {
	if (c == 'A' || c == 'a') {
		return glm::vec3(1.0f, .4f, .4f); 		//pink
	} else if (c == 'B' || c == 'b') {
		return glm::vec3(1.0f, 0.6f, 0.1f); 	//light yellow
	} else if (c == 'C'  || c == 'c') {
		return glm::vec3(1.0f, 1.0f, 0.3f); 	//light orange
	} else if (c == 'D'  || c == 'd') {
		return glm::vec3(.4f, 1.0f, .4f); 		//light green
	} else if (c == 'E'  || c == 'e') {
		return glm::vec3(.4f, .4f, 1.0f); 		//light blue
	} else {
		return glm::vec3(.0f, .0f, .0f); 		//black default
	}
}

void drawDoor(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts, glm::vec3 color) {
	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, -1);

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glUniform3fv(uniColor, 1, glm::value_ptr(color));

	glm::mat4 model = glm::mat4(1);
	model = glm::translate(model, position);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); 
	glDrawArrays(GL_TRIANGLES, modelStart, modelVerts); 
}

void drawKey(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts, glm::vec3 color) {
	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, -1);

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glUniform3fv(uniColor, 1, glm::value_ptr(color));

	glm::mat4 model = glm::mat4(1);
	model = glm::translate(model, position);
	model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
	model = glm::rotate(model,timePast * glm::radians(90.0f),glm::vec3(0.0f, 1.0f, 1.0f));
	model = glm::rotate(model,timePast * glm::radians(45.0f),glm::vec3(1.0f, 0.0f, 0.0f));
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); 

	glDrawArrays(GL_TRIANGLES, modelStart, modelVerts); 
}

void keyPickup(glm::vec3 pos, Map& map) {
	//converting to map cords
	int newX = floor(pos.x);
	int newZ = floor(pos.z);
    int mapX = map.width - 1 - newX;
    int mapZ = map.height - 1 - newZ;
    char currTile = map.grid[mapZ][mapX];

	//Check to see if a key has been picked up by the player. If so, make it disappear
    if (currTile >= 'a' && currTile <= 'e') {
        int keyIndex = currTile - 'a';
        map.keyCollected[keyIndex] = true;
        map.grid[mapZ][mapX] = '0'; //remove key from map
    }
}

void doorUnlocked(glm::vec3 pos, Map& map) {
	int newX = floor(pos.x);
	int newZ = floor(pos.z);
    int mapX = map.width - 1 - newX;
    int mapZ = map.height - 1 - newZ;
    char currTile = map.grid[mapZ][mapX];
	//Check to see if a key has been collected for the door
	if (currTile >= 'A' && currTile <= 'E') {
        int keyIndex = currTile - 'A';
		if (map.keyCollected[keyIndex]) {
			map.grid[mapZ][mapX] = '0'; //remove door from map
		}
    }
}

void drawCharacter(glm::vec3 position, int shaderProgram, int modelStart, int modelVerts) {
	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, -1);

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 color(1.0f, 1.0f, 1.0f);
	glUniform3fv(uniColor, 1, glm::value_ptr(color));

	glm::mat4 model = glm::mat4(1);
	model = glm::translate(model, position);
	model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); 

	glDrawArrays(GL_TRIANGLES, modelStart, modelVerts); 
}
bool checkBlocked(glm::vec3& pos, int mapX, int mapZ, float radiusSqr){
	float minX = mapX - 0.5f;
    float maxX = mapX + 0.5f;
	float minZ = mapZ - 0.5f;
    float maxZ = mapZ + 0.5f;

	//calculate distance from center to nearest wall or door
    float closestX = glm::clamp(pos.x, minX, maxX);
    float closestZ = glm::clamp(pos.z, minZ, maxZ);
    float distX = pos.x - closestX;
    float distZ = pos.z - closestZ;
    float dist2 = distX * distX + distZ * distZ;

	//check if closest point is inside character radius
    return (dist2 < radiusSqr);
}
bool isBlocked(glm::vec3 pos, const Map& map) {
    float radius = 0.25f;
	float radiusSqr = radius*radius;

    //convert to map coordinates
    int newX = floor(pos.x);
    int newZ = floor(pos.z);

    //check all the neighbors of the character
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int mapX = newX + dx;
            int mapZ = newZ + dz;
			int col = map.width - 1 - mapX;
            int row = map.height - 1 - mapZ;

			//out of bounds
            if (col < 0 || col >= map.width || row < 0 || row >= map.height) continue;

            //check for overlaps (clipping) when a character meets a wall
			char currTile = map.grid[row][col];
            if (currTile == 'W' && checkBlocked(pos, mapX, mapZ, radiusSqr)) {
                return true;
			}
			//check for overlaps (clipping) when a character meets a locked door
			if (currTile >= 'A' && currTile <= 'E') {
                int keyIndex = currTile - 'A'; 
				//block door if the key has not been collected for it
				if (!map.keyCollected[keyIndex] && checkBlocked(pos, mapX, mapZ, radiusSqr)) {
					return true;
				}
			}
        }
    }
    return false;
}


void playableCharacter(Map& map, float dt) {
    glm::vec3 newPos = characterPos;
	float delta = 3.0f * dt; //dt was calculated in the main function using timepast-lastTime and 3.0f represents speed
    
	glm::vec3 northSouth;
	glm::vec3 eastWest;
	if (mapView) { //if top-down view is toggled, lock the players movement to world axis and independent of camera
		northSouth = glm::vec3(.0f, .0f, 1.0f);
		eastWest = glm::vec3(-1.0f, .0f, .0f);
	} else { //otherwise, FP POV, movement dependent on camera direction
		northSouth = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
		eastWest = glm::normalize(glm::vec3(cameraRight.x, 0, cameraRight.z));
	}
	if (moveUp) {
		newPos += northSouth * delta;
	}
	if (moveDown) {
		newPos -= northSouth * delta;
	}
	if (moveRight) {
		newPos += eastWest * delta;
	}
	if (moveLeft) {
		newPos -= eastWest * delta;
	}
	//only update the character position if they did not collide with a wall or a locked door
    if (!isBlocked(newPos, map)) {
        characterPos = newPos;
		keyPickup(characterPos, map); //check if a key was picked up by the player
		doorUnlocked(characterPos, map); //check if door was unlocked by the player
    }
}


//render the map based on the map .txt file
void renderMap(Map& map, int shaderProgram, Model& models, Texture& texture) {
    float mapWidth = map.width;
	float mapHeight = map.height;

    //loop through each tile to render different components (e.g. wall, floor, door, key)
    for (int row = 0; row < mapHeight; row++) {
        for (int col = 0; col < mapWidth; col++) {
            char currTile = map.grid[row][col];
            float mapX = mapWidth - 1 - col; 
            float mapZ = mapHeight - 1 - row;

            glm::vec3 currPos(mapX, 0.f, mapZ);
			if (currTile == 'S' || currTile == 'G') {
				drawStartEnd(currPos, shaderProgram, models.startCube, models.countCube, texture);
			} else if (currTile == 'W') {
                drawWall(currPos, shaderProgram, models.startCube, models.countCube, texture);
            } else if (currTile == '0') { 
                drawFloor(currPos, shaderProgram, models.startFloor, models.countFloor, texture);
            } else if (currTile >= 'A' && currTile <= 'E') {
                glm::vec3 color = getColor(currTile);
                drawDoor(currPos, shaderProgram, models.startDoor, models.countDoor, color);
            } else if (currTile >= 'a' && currTile <= 'e') {
                glm::vec3 color = getColor(currTile);
                drawFloor(currPos, shaderProgram, models.startFloor, models.countFloor, texture);
    			drawKey(currPos, shaderProgram, models.startKey, models.countKey, color);
            }
        }
    }
    //initialize the character at the start
    if (!charInitialized) {
        float mapX = mapWidth - 1 - map.startX; 
        float mapZ = mapHeight - 1 - map.startZ;
        characterPos = glm::vec3(mapX, -0.5f, mapZ);
        charInitialized = true;
    }
    drawCharacter(characterPos, shaderProgram, models.startCharacter, models.countCharacter);
}
