/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	The Algorithms used originate from a base knowledge
*	of ECE 440 and ECE 442
*
*	Captures RAW Bayer Format Data from an IR USB Camera
*	And applies clusting/centroid calc to pixels to find
*	center positions of "finger blobs"
*
*	This will be modified to run on the DE2
*
*	type make to build, ./dsp to run
*	This was developed on Ubuntu 13.10
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <pthread.h>

#include "dsp.h"
#include "calibrate.h"
#include "socket.h"

//Size of camera image
#define DATA_WIDTH	352
#define DATA_HEIGHT	288
#define DATA_LEN	DATA_WIDTH*DATA_HEIGHT

//For DSP, byte image definitions
#define SHORT_MAX	255
#define SHORT_MIN	0
#define RGBA_LEN	4
#define RGBA_R		0
#define RGBA_G		1
#define RGBA_B		2
#define RGBA_A		3

//boolean values
#define BOOL_FALSE	0 		//not truth value
#define BOOL_TRUE	1 		//truth value

#define BRUSH_COLOR_MAX_VAL		10 			//total number of colours
#define BRUSH_SIZE				5 			//drawn size

//Color definitions
#define COLOR_TURQUOISE	{SHORT_MIN, SHORT_MAX, SHORT_MAX, 0, 0}
#define COLOR_BLACK		{0, 0, 0, 0, 0}
#define COLOR_DARK_GREY	{20, 20, 20, 0, 0}
#define JUNGLE_PALETTE1	{55, 24, 21, 0, 0}	//taken from http://media-cache-ec0.pinimg.com/736x/1f/d8/8a/1fd88ac2fdc296cce6b1791322ed4223.jpg
#define JUNGLE_PALETTE2	{91, 24, 16, 0, 0}
#define JUNGLE_PALETTE3	{198, 52, 39, 0, 0}
#define JUNGLE_PALETTE4	{255, 112, 98, 0, 0}
#define JUNGLE_PALETTE5	{255, 67, 3, 0, 0}
#define NATURE_PALETTE1	{75, 42, 27, 0, 0}	//taken from http://media-cache-ec0.pinimg.com/236x/b1/4f/3f/b14f3faeceef31ccad2eb0fea03adff4.jpg
#define NATURE_PALETTE2	{147, 87, 37, 0, 0}
#define NATURE_PALETTE3	{93, 66, 21, 0, 0}
#define NATURE_PALETTE4	{234, 186, 84, 0, 0}

//Drawing macros
#define TEX_TOP_LEFT			0.0f, 0.0f
#define TEX_TOP_RIGHT			1.0f, 0.0f
#define TEX_BOT_RIGHT			1.0f, 1.0f
#define TEX_BOT_LEFT			0.0f, 1.0f

//structure to contain pixel values
//for drawing
struct pixel
{
	int r, g, b;
	int x, y;
};

//Color used for calibration
struct pixel calib_color = COLOR_TURQUOISE;
//colors used for drawing
struct pixel black = COLOR_BLACK;
struct pixel finger_colors[BRUSH_COLOR_MAX_VAL] = {
	NATURE_PALETTE1,
	NATURE_PALETTE2,
	NATURE_PALETTE3,
	NATURE_PALETTE4,
	COLOR_BLACK,
	JUNGLE_PALETTE1,
	JUNGLE_PALETTE2,
	JUNGLE_PALETTE3,
	JUNGLE_PALETTE4,
	JUNGLE_PALETTE5,
};

//texture to hold output pixel buffer
GLuint texture[1];
//raw bayer pixel buffer, read from camera
unsigned char buffer[DATA_LEN];
//Pixel buffer to hold data to be output to 
//the screen. In this case, the colors are
//drawn onto this pixel buffer.
unsigned char pixels2[DATA_LEN * RGBA_LEN];

//image to subtract
unsigned char static_pixels[DATA_LEN];
//File pointer to camera feed. Typically /dev/video0
int fp;

//for calibration. Scaled with 352x288
POINT perfectPoints[3] ={
            { 50, 50 },
			{ 300, 120 },
            { 200, 230 }
};
POINT actualPoints[3];
MATRIX calibMatrix;

//dsp thread
pthread_t dsp_thread;
int dsp_running = 0;

//* Function: Draw Pixel
//* Description: Draws a pixel to the screen, if the specified
//point exists. Otherwise draws nothing.
//* Input: pixel buffer, buffer width/height, pixel struct containing
//info about what to draw where
//* Returns: nothing
void draw_pixel(unsigned char *data, int width, int height, struct pixel p){
	int val = width*p.y+p.x;	//get position in 1D array
	if (val < 0 || val >= width*height)		//if out of bounds
		return;
	data[val*RGBA_LEN+RGBA_R] = p.r;		//set red value
	data[val*RGBA_LEN+RGBA_G] = p.g;		//set green value
	data[val*RGBA_LEN+RGBA_B] = p.b;		//set blue value
	data[val*RGBA_LEN+RGBA_A] = SHORT_MAX;	//set alpha value to 100% (opaque)
}

//* Function: Draw Square
//* Description: Draws a square of specified size to the screen.
//* Input: pixel buffer, buffer width/height, pixel struct containing
//info about what to draw where, size of square
//* Returns: nothing
void draw_square(unsigned char *data, int width, int height, struct pixel p){
	int y0 = p.y, x0 = p.x;			//store initial x and y
	for (int y = 0; y < BRUSH_SIZE; ++y){				//draw square of height: size
		p.y = y0 + y;							//move pixel to correct height
		for (int x = 0; x < BRUSH_SIZE; ++x){			//draw square of width: size
			p.x = x0 + x;						//move pixel to correct width
			draw_pixel(data, width, height, p);	//draw the pixel
		}
	}
}

//* Function: Get Biggest Finger
//* Description: Draws a square of specified size to the screen.
//* Input: Centroid array of size MAX_CENTROIDS
//* Returns: nothing
struct Centroid get_biggest_finger(struct Centroid *cents){
	int index = 0;
	for (int i = 0; i < MAX_CENTROIDS; ++i)
	{
		if (cents[i].size > cents[index].size)
		{
			index = i;
		}
	}
	return cents[index];
}

//* Function: Perform DSP
//* Description: Performs a number of operations on the camera input
//such as thresholding/centroid calc. Then draw squares on the screen
//or switch color or brush size, or even clear the screen. Depending
//on the number of centroids and their size, a different gesture is
//detected and the corresponding action is performed. See below for
//more details
//* Input: nothing
//* Returns: nothing
void *perform_DSP(void *args){
	static int calibrate = 1;
	struct Centroid *centroids;
	static int touch = 0;
	static int fingerup = 0;
	struct Centroid last_centroid;
	POINT point1, point2;

	while(dsp_running){
		//calculate centroids
		centroids = get_fingers();
		
	    //if fingers are touching
		if (centroids[0].size > 0)
		{
			touch = 1;
			last_centroid = get_biggest_finger(centroids);
		}else{
			//if finger just came off
			if (touch == 1)
				fingerup = 1;
			touch = 0;
		}
	    //if calibrating
	    if (calibrate)
	    {
	    	//black out screen if calbrating
	    	struct pixel back_color = COLOR_DARK_GREY;	//background color
			for (int i = 0; i < DATA_LEN; ++i)
		    {
				//dark grey
				pixels2[i*RGBA_LEN+RGBA_R] = back_color.r;	//set red value
				pixels2[i*RGBA_LEN+RGBA_G] = back_color.g;	//set green value
				pixels2[i*RGBA_LEN+RGBA_B] = back_color.b;	//set blue value
				pixels2[i*RGBA_LEN+RGBA_A] = SHORT_MAX;	//set alpha
				pixels2[i*4+3] = SHORT_MAX;
		    }
	    	//draw calibration point
			calib_color.x = perfectPoints[calibrate-1].x;
			calib_color.y = perfectPoints[calibrate-1].y;
			draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, calib_color);
			if (fingerup)
			{
				//set actual calibration points to the last finger that touched the display
				actualPoints[calibrate-1].x = last_centroid.x;
				actualPoints[calibrate-1].y = last_centroid.y;
				calibrate++;			//increment up to number of calibration points
				//if done calibration
				if (calibrate > 3)
				{
					//create calibration matrix
					setCalibrationMatrix(&perfectPoints[0], &actualPoints[0], &calibMatrix);
					//clear pixels
					for (int i = 0; i < DATA_LEN * 4; ++i)
					{
						pixels2[i] = 0;
					}
					//done calibration
					calibrate = 0;
				}
				fingerup = 0;
			}
	    }else{
		    if (centroids != NULL)
		    {
		    	for (int i = 0; i < 10; ++i)
				{
					//if decently sized, draw as a red pixel
					if (centroids[i].size > 10)
					{
						point2.x = centroids[i].x;
						point2.y = centroids[i].y;
						getDisplayPoint(&point1, &point2, &calibMatrix);
						finger_colors[0].x = point1.x;
						finger_colors[0].y = point1.y;
						draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, finger_colors[0]);
					}
				}
		    }
		}
	}
	return NULL;
}

//* Function: Setup Textures
//* Description: creates a texture using output pixels generated from another
//thread (pixels2). Then it stores this into a texture.
//* Input: nothing
//* Returns: nothing
void setup_textures(){
	glEnable(GL_TEXTURE_2D);					//enable 2d texturing
	glBindTexture(GL_TEXTURE_2D, texture[1]);	//bind the global texture variable to our texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	//set magnification filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	//set minification filter
	//assign pixels to texture in RGBA format
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels2);
}

//* Function: Error Callback
//* Description: Callback if GLFW messed up. Copied from first GLFW tutorial.
//* Input: nothing
//* Returns: nothing
void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

//* Function: Key Callback
//* Description: Callback if GLFW detects a window close or escape button. 
//Copied from first GLFW tutorial.
//* Input: nothing
//* Returns: nothing
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);	//close program
}

//* Function: Main
//* Description: Displays a drawing application using a touchscreen as input
//and draws pixels to the screen according to the user. Uses the DE2 through
//a socket server to do some of the DSP.
//* Input: nothing
//* Returns: 0 or error code
int main(int argc, char const *argv[])
{
	//Init GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	//set error callback
	glfwSetErrorCallback(error_callback);

	//make GLFW window
	GLFWwindow* window = glfwCreateWindow(640, 480, "Webcam DSP", NULL, NULL);
	if (!window)
	{
	    glfwTerminate();
	    exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	//set callback for keys and buttons on window
	glfwSetKeyCallback(window, key_callback);

	float ratio;
	int width, height;

	//Init DE2
	de2_init();

	//bool flag for dsp thread. When 0, the thread
	//will exit, otherwise it will continue running in a loop
	dsp_running = BOOL_TRUE;
	pthread_create(&dsp_thread, NULL, perform_DSP, NULL);

	//generate texture
	glGenTextures(1, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float) height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();

		//make texture
		setup_textures();

		//Draw Processed Data
		glBegin(GL_QUADS);	//draw quads
			glTexCoord2f(TEX_TOP_LEFT); glVertex3f( -ratio, 1.0f, 0.0f);		//top left vertex
			glTexCoord2f(TEX_TOP_RIGHT); glVertex3f( ratio, 1.0f, 0.0);			//top right vertex
			glTexCoord2f(TEX_BOT_RIGHT); glVertex3f( ratio,-1.0f, 0.0);			//bottom right vertex
			glTexCoord2f(TEX_BOT_LEFT); glVertex3f( -ratio, -1.0f, 0.0);		//bottom left vertex
		glEnd();

	    glfwSwapBuffers(window);
	    glfwPollEvents();
	}
	//kill dsp
	dsp_running = BOOL_FALSE;
	pthread_join(dsp_thread, NULL);
	de2_close();
	//kill GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
	printf("done\n");
	return 0;
}