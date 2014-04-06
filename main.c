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
#include "finger.h"

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

//Number of frames to wait before accepting another gesture
#define FINGER_SWITCH_TO		10

//Number of frames to get and store in static buffer
#define STATIC_IMAGE_FRAMES		5

//Drawing defines
#define DRAW_FINGER_NUM 		1 			//number of fingers to trigger a draw

//Brush size defines
#define BRUSH_FINGER_NUM		3 			//number of fingers to trigger brush change
#define BRUSH_SCALE				5 			//drawn size
#define BRUSH_SCALE_THUMB		3 			//drawn size for thumbnail
#define BRUSH_MAX_VAL			7 			//maximum brush size
#define BRUSH_MIN_VAL			1 			//minimum brush size

//Brush color defines
#define BRUSH_COLOR_FINGER_NUM	2 			//number of fingers to trigger brush color change
#define BRUSH_COLOR_MAX_VAL		10 			//total number of colours
#define BRUSH_COLOR_MIN_VAL		0 			//position of first colour

//Threshold values
#define THRESH_IMAGE			10 			//image thresholding value, not a pixel if less than this
#define THRESH_FINGER_SIZE		50			//finger size thresholding, not a finger if less than this
#define THRESH_ERASE			800 		//threshold to decide to erase screen or not

//A decent amount of time
#define DECENT_AMOUNT_OF_TIME	2 			//number in seconds which is an okay amount of time to wait

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
//Pixel buffer to hold unmodified camera data
//NOTE: For debugging only, deprecated
unsigned char pixels[DATA_LEN * RGBA_LEN];	//the data length is 4 times the size since the
											//values are RGBA, hence 4 values per pixel
//Pixel buffer to hold data to be output to 
//the screen. In this case, the colors are
//drawn onto this pixel buffer.
unsigned char pixels2[DATA_LEN * RGBA_LEN];

//image to subtract
unsigned char static_pixels[DATA_LEN];
//File pointer to camera feed. Typically /dev/video0
int fp;
//bool flag for camera thread. When 0, the thread
//will exit, otherwise it will continue running in a loop
int running = BOOL_FALSE;

#define CALIB_POINTS			3 		//number of points to use for calibration
//for calibration. Scaled with 352x288
POINT perfectPoints[CALIB_POINTS] ={
            { 50, 50 },			//Points were picked mostly
			{ 300, 120 },		//at random, they seem to
            { 200, 230 }		//calibrate well
};
//To store actual measured points when the
//user touches the screen
POINT actualPoints[CALIB_POINTS];
//Calibration matrix to convert captured points to
//effective points bounded by our window.
MATRIX calibMatrix;

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
void draw_square(unsigned char *data, int width, int height, struct pixel p, int size){
	int y0 = p.y, x0 = p.x;			//store initial x and y
	for (int y = 0; y < size; ++y){				//draw square of height: size
		p.y = y0 + y;							//move pixel to correct height
		for (int x = 0; x < size; ++x){			//draw square of width: size
			p.x = x0 + x;						//move pixel to correct width
			draw_pixel(data, width, height, p);	//draw the pixel
		}
	}
}

//* Function: Get Static Image
//* Description: waits a couple frames by filling the static pixels buffer.
//This buffer is then subtracted from all further images captured
//* Input: nothing
//* Returns: nothing
void get_static_image(){
	//read a couple frames to get a good image
	for (int i = 0; i < STATIC_IMAGE_FRAMES; ++i)
	{
		//read image from camera
		read(fp, static_pixels, DATA_LEN);
	}
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
void perform_DSP(){
	static int calibrate = BOOL_TRUE;		//-bool flag to indicate if calibrating or not
	struct Centroid *centroids;				//-centroid array. assumed to be of size MAX_CENTROIDS because of get_centroids
	static int touch = BOOL_FALSE;			//-integer count of how many fingers are touching the screen
	static int fingerup = BOOL_FALSE;		//-bool flag set when fingers are lifted off the screen
	static int color_i = BRUSH_COLOR_MIN_VAL;//-selected colour from the 10 available
	static int brush_size = BRUSH_MIN_VAL;	//-selected brush size. Scaled using BRUSH_SCALE.
	static int touch_timeout = 0;			//-timeout counter used to wait between gestures
	struct Centroid last_centroid;			//-centroid to store the last point before fingers are lifted from the screen
	POINT point1, point2;					//-point1 is the raw point data and point2 is after it has been run
											//through the calibration matrix.
	
	//---------FIND CENTROIDS-------------
	//Threshold image
	threshold(buffer, DATA_WIDTH, DATA_HEIGHT, THRESH_IMAGE);

	//calculate centroids. returns MAX_CENTROIDS centroids always
	centroids = get_centroids(buffer, DATA_WIDTH, DATA_HEIGHT);

	//---------INTERPRET DATA-------------
    //get number of fingers touching screen
    int new_touch = 0;
    for (int i = 0; i < MAX_CENTROIDS; ++i)
    {
    	//finger found
    	if (centroids[i].size > THRESH_FINGER_SIZE)
    	{
    		new_touch++;	//increment number of fingers touching
    	}
    }
    //if fingers are touching
	if (new_touch)
	{
		touch = new_touch;				//new_touch isn't needed anymore, so store in touch
		last_centroid = centroids[0];	//update the last touched centroid
	}else{
		//if finger just came off
		if (touch == BOOL_TRUE)					//on falling edge of touch
			fingerup = BOOL_TRUE;				//fingers just came off
		touch = BOOL_FALSE;						//no fingers
	}
    //if calibrating
    if (calibrate)
    {
    	//-------------CALIBRATION SEQUENCE-----------------
    	//blank the screen if calibrating
    	struct pixel back_color = COLOR_DARK_GREY;	//background color
		for (int i = 0; i < DATA_LEN; ++i)
	    {
			//dark grey
			pixels2[i*RGBA_LEN+RGBA_R] = back_color.r;	//set red value
			pixels2[i*RGBA_LEN+RGBA_G] = back_color.g;	//set green value
			pixels2[i*RGBA_LEN+RGBA_B] = back_color.b;	//set blue value
			pixels2[i*RGBA_LEN+RGBA_A] = SHORT_MAX;	//set alpha
			buffer[i] = 0;				//clear buffer
	    }
    	//set calibration point to current ideal point
		calib_color.x = perfectPoints[calibrate-1].x;
		calib_color.y = perfectPoints[calibrate-1].y;
		//draw a square for calibration of smallest possible size
		draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, calib_color, BRUSH_SCALE);
		if (fingerup)	//on finger raise
		{
			//set actual calibration points to the last finger that touched the display
			actualPoints[calibrate-1].x = last_centroid.x;
			actualPoints[calibrate-1].y = last_centroid.y;
			calibrate++;			//increment up to number of calibration points
			//if done calibration
			if (calibrate > CALIB_POINTS)
			{
				//create a calibration matrix by comparing expected points to actual points
				setCalibrationMatrix(&perfectPoints[0], &actualPoints[0], &calibMatrix);
				//clear pixels
				for (int i = 0; i < DATA_LEN * RGBA_LEN; ++i)
				{
					pixels2[i] = 0;
				}
				//done calibration
				calibrate = BOOL_FALSE;
			}
			fingerup = BOOL_FALSE;
		}
	//if calibration is done and we have centroids
    }else{
    	//--------------DRAWING MODE (after calibration)----------------
    	//blank out brush indicator
	    draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, black, BRUSH_MAX_VAL*BRUSH_SCALE_THUMB);
    	finger_colors[color_i].x = 0;	//draw brush indicator at top left
		finger_colors[color_i].y = 0;
		//draw brush indicator
		draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, finger_colors[color_i], brush_size*BRUSH_SCALE_THUMB);
	    if (touch == DRAW_FINGER_NUM)	//one finger means draw
	    {
			//if decently sized, draw as a red pixel
			if (centroids[0].size > THRESH_FINGER_SIZE)
			{
				point2.x = centroids[0].x;
				point2.y = centroids[0].y;
				getDisplayPoint(&point1, &point2, &calibMatrix);
				finger_colors[color_i].x = point1.x;
				finger_colors[color_i].y = point1.y;
				draw_square(pixels2, DATA_WIDTH, DATA_HEIGHT, finger_colors[color_i], brush_size*BRUSH_SCALE);
			}
	    }else if(touch == BRUSH_COLOR_FINGER_NUM){	//two fingers means change color
	    	color_i++;
	    	if (color_i >= BRUSH_COLOR_MAX_VAL)		//loop back to first colour
	    	{
	    		color_i = BRUSH_COLOR_MIN_VAL;
	    	}
	    	touch_timeout = FINGER_SWITCH_TO;		//refill timeout
	    }else if (touch == BRUSH_FINGER_NUM){	//three fingers means change brush size
	    	brush_size++;
	    	if (brush_size > BRUSH_MAX_VAL)			//loop back to first size
	    	{
	    		brush_size = BRUSH_MIN_VAL;	//set to smallest brush value
	    	}
	    	touch_timeout = FINGER_SWITCH_TO;		//refill timeout
	    }
    	touch_timeout--;
    	if (touch_timeout < 0)		//bring timeout to zero and keep it there
    	{
    		touch_timeout = 0;
    	}
    }
    //---------------CHECK FOR ERASE GESTURE----------------
	//if big mass detected, clear screen
	for (int i = 0; i < MAX_CENTROIDS; ++i)
	{
		if (centroids[i].size > THRESH_ERASE)
		{
			//clear all pixels
			for (int j = 0; j < DATA_LEN * RGBA_LEN; ++j)
			{
				pixels2[j] = 0;
			}
			//wait a decent amount of time
			sleep(DECENT_AMOUNT_OF_TIME);
		}
	}
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

//* Function: Read From Camera
//* Description: Runs on a seperate thread from the graphics to capture video
//feed from the camera at a constant rate. This is so the camera feed doesn't
//lag behind because of slow UI.
//* Input: NULL
//* Returns: NULL
void *read_from_camera(void *args){
	while (running){	//while program is told to run
	    read(fp, buffer, DATA_LEN);		//read camera feed
	    //subtract static image
	    for (int i = 0; i < DATA_LEN; ++i)
	    {
	    	if (buffer[i] > static_pixels[i])
	    	{
	    		buffer[i] = buffer[i] - static_pixels[i];
	    	}else{		//if underflow, assign 0 so as to not get wonky results
	    		buffer[i] = 0;
	    	}
	    }
	    //copy to pixels buffer in case we want to debug in the future
	    for (int i = 0; i < DATA_LEN; ++i)
	    {
			pixels[i*RGBA_LEN+RGBA_R] = buffer[i];	//set red value
			pixels[i*RGBA_LEN+RGBA_G] = buffer[i];	//set green value
			pixels[i*RGBA_LEN+RGBA_B] = buffer[i];	//set blue value
			pixels[i*RGBA_LEN+RGBA_A] = SHORT_MAX;	//set alpha to 100%
	    }
		//tex_mutex = 0;
	    perform_DSP();
	}
	return NULL;
}


//GLFW Window Error Callback function
void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}
//GLFW Callback for close button on window
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

//Finds out how many camers we have (1 or 2)
int get_camera_num(){
	size_t len;
	char* buf = NULL;
	FILE* cam_pointer = popen("ls /dev/ | grep [Vv]ideo1", "r");
	getline(&buf, &len, cam_pointer);
	if (buf != NULL)
	{
		return 2;
	}
	return 1;
	pclose(cam_pointer);
}

int main(int argc, char const *argv[])
{
	running = 1;
	//Init GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	//set error callback
	glfwSetErrorCallback(error_callback);
	//get all screens
	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	//get size of last screen
	int widthMM, heightMM;
	glfwGetMonitorPhysicalSize(monitors[count-1], &widthMM, &heightMM);
	//make fullscreen window from last monitor
	GLFWwindow* window;
	if (monitors != NULL)
	{
		window = glfwCreateWindow(640, 480, "Webcam DSP", NULL, NULL);
	}else{
		window = glfwCreateWindow(640, 480, "Webcam DSP", NULL, NULL);
	}
	if (!window)
	{
	    glfwTerminate();
	    exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	int cam_num = get_camera_num();

	//Open IR Camera
	if (cam_num == 1)
	{
		fp = open("/dev/video0", O_RDONLY);
	}else{
		fp = open("/dev/video1", O_RDONLY);
	}
	
	get_static_image();

	float ratio;
	int width, height;

	//make pthread
	pthread_t cam_thread;
	pthread_create(&cam_thread, NULL, read_from_camera, NULL);

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
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, texture[1]);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( -ratio, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( ratio, 1.0f, 0.0);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( ratio,-1.0f, 0.0);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( -ratio, -1.0f, 0.0);
		glEnd();
		//glDisable(GL_TEXTURE_2D);

		glfwSetKeyCallback(window, key_callback);

	    glfwSwapBuffers(window);
	    glfwPollEvents();
	}
	printf("done\n");
	running = 0;
	pthread_join(cam_thread, NULL);
	close(fp);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}