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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <pthread.h>

#include "dsp.h"

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

GLuint texture[2];
unsigned char buffer[DATA_LEN];
unsigned char pixels[DATA_LEN * 4];
unsigned char pixels2[DATA_LEN * 4];
int fp;
int tex_mutex = 0;
int running = 0;

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

void perform_DSP(){
	//Threshold image
	threshold(buffer, DATA_WIDTH, DATA_HEIGHT, SHORT_MAX / 7);
	clip_edges(buffer, DATA_WIDTH, DATA_HEIGHT, LEFT, 30);
	//perform morphological erosion (computer only)
	//erode_cross(buffer, 352, 288);
	//erode_cross(buffer, 352, 288);

	//calculate centroids
	struct Centroid *centroids = get_centroids(buffer, DATA_WIDTH, DATA_HEIGHT);

	//load data into pixels as greyscale
	for (int i = 0; i < DATA_LEN; ++i)
    {
		/*if (buffer[i] > 0)
		{
			//blue
			pixels2[i*4+0] = buffer[i] * 7;
			pixels2[i*4+1] = 10;
			pixels2[i*4+2] = SHORT_MAX;
		}else{
			//black
			pixels2[i*4+0] = 0;
			pixels2[i*4+1] = 0;
			pixels2[i*4+2] = 0;
		}*/
		pixels2[i*4+3] = SHORT_MAX;
		buffer[i] = 0;
    }
    struct pixel white = {SHORT_MAX, SHORT_MIN, SHORT_MIN, 0, 0};
    if (centroids != NULL)
    {
    	for (int i = 0; i < 255; ++i)
		{
			//if decently sized, draw as a red pixel
			if (centroids[i].size > 0)
			{
				//printf("cent: %f %f\n", centroids[i].y, centroids[i].x);
				//int val = 352*(int)centroids[i].y+(int)centroids[i].x;
				white.x = 352 - centroids[i].x;
				white.y = centroids[i].y;
				draw_circle(pixels2, 352, 288, white);
				//pixels2[val*4+0] = SHORT_MAX;
				//pixels2[val*4+1] = SHORT_MAX;
				//pixels2[val*4+2] = SHORT_MAX;
				//pixels2[val*4+3] = SHORT_MAX;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
}

void *read_from_camera(void *args){
	while (running){
		while(tex_mutex){ ; }
		tex_mutex = 1;
	    read(fp, buffer, DATA_LEN);
	    for (int i = 0; i < DATA_LEN; ++i)
	    {
			pixels[i*4+0] = buffer[i];
			pixels[i*4+1] = buffer[i];
			pixels[i*4+2] = buffer[i];
			pixels[i*4+3] = SHORT_MAX;
	    }
	    //perform DSP
		perform_DSP();
	    tex_mutex = 0;
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

int main(int argc, char const *argv[])
{
	running = 1;
	//Init GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(error_callback);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Webcam DSP", NULL, NULL);
	if (!window)
	{
	    glfwTerminate();
	    exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	//Open IR Camera
	fp = open("/dev/video0", O_RDONLY);

	float ratio;
	int width, height;

	glfwGetFramebufferSize(window, &width, &height);
	ratio = width / (float) height;

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glfwSetKeyCallback(window, key_callback);

	glGenTextures(1, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	//make pthread
	pthread_t cam_thread;
	pthread_create(&cam_thread, NULL, read_from_camera, NULL);

	while (!glfwWindowShouldClose(window))
	{
		while(tex_mutex){ ; }
		tex_mutex = 1;

		//make texture
		setup_textures();

		//Draw Processed Data
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( -ratio, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( ratio, 1.0f, 0.0);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( ratio,-1.0f, 0.0);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( -ratio, -1.0f, 0.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);

	    glfwSwapBuffers(window);
	    glfwPollEvents();

	    glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float) height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
	    tex_mutex = 0;
	}
	printf("done\n");
	running = 0;
	pthread_join(cam_thread, NULL);
	close(fp);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}