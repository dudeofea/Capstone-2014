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

#define DATA_LEN	352*288
#define DATA_WIDTH	352
#define DATA_HEIGHT	288
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define SHORT_MAX	255
#define SHORT_MIN	0

struct pixel
{
	int r, g, b;
	int x, y;
};

struct pixel color1 = {SHORT_MIN, SHORT_MAX, SHORT_MAX, 0, 0};
struct pixel finger_colors[10] = {
	{SHORT_MAX, SHORT_MIN, SHORT_MAX, 0, 0},
	{SHORT_MIN, SHORT_MIN, SHORT_MAX, 0, 0},
	{SHORT_MIN, SHORT_MAX, SHORT_MIN, 0, 0},
	{SHORT_MAX, SHORT_MAX, SHORT_MAX, 0, 0},
	{SHORT_MIN, SHORT_MIN, SHORT_MIN, 0, 0},
	{30, 70, 120, 0, 0},
	{120, 30, 70, 0, 0},
	{120, 120, 70, 0, 0},
	{120, 30, 120, 0, 0},
	{70, 30, 30, 0, 0},
};
GLuint texture[1];
unsigned char buffer[DATA_LEN];
unsigned char pixels[DATA_LEN * 4];
unsigned char pixels2[DATA_LEN * 4];

//image to subtract
unsigned char static_pixels[DATA_LEN];
int fp;
int running = 0;

//for calibration. Scaled with 352x288
POINT perfectPoints[3] ={
            { 50, 50 },
			{ 300, 120 },
            { 200, 230 }
};
POINT actualPoints[3];
MATRIX calibMatrix;

void draw_pixel(unsigned char *data, int width, int height, struct pixel p){
	int val = width*p.y+p.x;
	if (val < 0 || val >= width*height)
		return;
	data[val*4+0] = p.r;
	data[val*4+1] = p.g;
	data[val*4+2] = p.b;
	data[val*4+3] = SHORT_MAX;
}

void draw_circle(unsigned char *data, int width, int height, struct pixel p){
	int y0 = p.y, x0 = p.x;
	for (int y = 0; y < 5; ++y){
		p.y = y0 + y;
		for (int x = 0; x < 5; ++x){
			p.x = x0 + x;
			draw_pixel(data, width, height, p);
		}
	}
}

void get_static_image(){
	//read a couple frames to get a good image
	for (int i = 0; i < 5; ++i)
	{
		read(fp, static_pixels, DATA_LEN);
	}
}

void perform_DSP(){
	static int calibrate = 1;
	struct Centroid *centroids;
	static int touch = 0;
	static int fingerup = 0;
	struct Centroid last_centroid;
	POINT point1, point2;
	
	//Threshold image
	threshold(buffer, DATA_WIDTH, DATA_HEIGHT, 4);

	//calculate centroids
	centroids = get_centroids(buffer, DATA_WIDTH, DATA_HEIGHT);

	//load data into pixels as greyscale
	for (int i = 0; i < DATA_LEN; ++i)
    {
    	if(calibrate){
			// if (buffer[i] > 0)
			// {
			// 	//blue
			// 	pixels2[i*4+0] = buffer[i] * 7;
			// 	pixels2[i*4+1] = 10;
			// 	pixels2[i*4+2] = SHORT_MAX;
			//}else{
				//black
				pixels2[i*4+0] = 20;
				pixels2[i*4+1] = 20;
				pixels2[i*4+2] = 20;
			//}
		}
		pixels2[i*4+3] = SHORT_MAX;
		buffer[i] = 0;
    }
    //if fingers are touching
	if (centroids[0].size > 0)
	{
		touch = 1;
		last_centroid = centroids[0];
	}else{
		//if finger just came off
		if (touch == 1)
			fingerup = 1;
		touch = 0;
	}
    //if calibrating
    if (calibrate)
    {
		color1.x = perfectPoints[calibrate-1].x;
		color1.y = perfectPoints[calibrate-1].y;
		draw_circle(pixels2, DATA_WIDTH, DATA_HEIGHT, color1);
		if (fingerup)
		{
			actualPoints[calibrate-1].x = last_centroid.x;
			actualPoints[calibrate-1].y = last_centroid.y;
			calibrate++;
			//if done calibration
			if (calibrate > 3)
			{
				setCalibrationMatrix(&perfectPoints[0], &actualPoints[0], &calibMatrix);
				//clear pixels
				for (int i = 0; i < DATA_LEN * 4; ++i)
				{
					pixels2[i] = 0;
				}
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
					getDisplayPoint(&point1, &point2, &calibMatrix) ;
					//printf("cent: %f %f\n", centroids[i].y, centroids[i].x);
					//int val = 352*(int)centroids[i].y+(int)centroids[i].x;
					finger_colors[0].x = point1.x;
					finger_colors[0].y = point1.y;
					draw_circle(pixels2, DATA_WIDTH, DATA_HEIGHT, finger_colors[0]);
				}
			}
	    }
	}
}

//Recreate textures
void setup_textures(){
	/*glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
	glEnable(GL_TEXTURE_2D);*/

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels2);
}

void *read_from_camera(void *args){
	while (running){
	    read(fp, buffer, DATA_LEN);
	    //subtract static image
	    for (int i = 0; i < DATA_LEN; ++i)
	    {
	    	if (buffer[i] > static_pixels[i])
	    	{
	    		buffer[i] = buffer[i] - static_pixels[i];
	    	}else{
	    		buffer[i] = 0;
	    	}
	    }
	    for (int i = 0; i < DATA_LEN; ++i)
	    {
			pixels[i*4+0] = buffer[i];
			pixels[i*4+1] = buffer[i];
			pixels[i*4+2] = buffer[i];
			pixels[i*4+3] = SHORT_MAX;
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