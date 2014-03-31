/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	Captures RAW Bayer Format Data from an IR USB Camera
*	and offloads onto DE2 board via sockets. The DE2 then
*	returns a list of (up to 10) points which represent
*	finger positions
*
*	type make to build, ./dsp to run
*	This was developed on Ubuntu 13.10
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
//#include <GLFW/glfw3.h>

#include "socket.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define SHORT_MAX	255
#define SHORT_MIN	0

/*GLuint texture[1];
unsigned char buffer[DATA_LEN];
unsigned char pixels[DATA_LEN * 4];
unsigned char pixels2[DATA_LEN * 4];
int fp;*/

/*void perform_DSP(){
	//Threshold image
	threshold(buffer, DATA_WIDTH, DATA_HEIGHT, SHORT_MAX / 4);

	//perform morphological erosion (computer only)
	//erode_cross(buffer, 352, 288);
	//erode_cross(buffer, 352, 288);

	//calculate centroids
	struct Centroid *centroids = get_centroids(buffer, DATA_WIDTH, DATA_HEIGHT);

	//load data into pixels as greyscale
	for (int i = 0; i < DATA_LEN; ++i)
    {
		if (buffer[i] > 0)
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
		}
		pixels2[i*4+3] = SHORT_MAX;
		buffer[i] = 0;
    }
    if (centroids != NULL)
    {
    	for (int i = 0; i < 255; ++i)
		{
			//if decently sized, draw as a red pixel
			if (centroids[i].size > 0)
			{
				//printf("cent: %f %f\n", centroids[i].y, centroids[i].x);
				int val = 352*(int)centroids[i].y+(int)centroids[i].x;
				pixels2[val*4+0] = SHORT_MAX;
				pixels2[val*4+1] = SHORT_MAX;
				pixels2[val*4+2] = SHORT_MAX;
				pixels2[val*4+3] = SHORT_MAX;
			}
		}
    }
}

//Recreate textures
void setup_textures(){
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
	glEnable(GL_TEXTURE_2D);

	//perform DSP
	perform_DSP();

	glGenTextures(1, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels2);
	glEnable(GL_TEXTURE_2D);
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
}*/

int main(int argc, char const *argv[])
{
	de2_init();
	while(1)
		get_fingers();
	de2_close();
	/*//Init GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(error_callback);
	GLFWwindow* window = glfwCreateWindow(640*2, 480, "Webcam DSP", NULL, NULL);
	if (!window)
	{
	    glfwTerminate();
	    exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	//Open IR Camera
	fp = open("/dev/video1", O_RDONLY);

	glfwSetKeyCallback(window, key_callback);
	while (!glfwWindowShouldClose(window))
	{
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

        copy_to_pixels();
        setup_textures();

        //Draw Raw Camera Data
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-ratio, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.0f,-1.0f, 0.0);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-ratio, -1.0f, 0.0);
		glEnd();

		//Draw Processed Data
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( ratio, 1.0f, 0.0);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( ratio,-1.0f, 0.0);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.0f, -1.0f, 0.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);

	    glfwSwapBuffers(window);
	    glfwPollEvents();
	}

	close(fp);

	glfwDestroyWindow(window);
	glfwTerminate();
	*/
	return 0;
}