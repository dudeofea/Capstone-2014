#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <GLFW/glfw3.h>

#include "paint.h"


#define DATA_WIDTH		480
#define DATA_HEIGHT		640
#define DATA_LEN		DATA_WIDTH*DATA_HEIGHT

GLuint texture[1];
unsigned char pixels[DATA_LEN * 4];

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
	Color white = {255, 255, 255};
	for (int y = 0; y < DATA_HEIGHT; ++y)
	{
		for (int x = 0; x < DATA_WIDTH; ++x)
		{
			set_pixel(x, y, white);
		}
	}
	//Init GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(error_callback);
	GLFWwindow* window = glfwCreateWindow(DATA_WIDTH, DATA_HEIGHT, "Drawing App", NULL, NULL);
	if (!window)
	{
	    glfwTerminate();
	    exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	while(update_screen(window)){
		sleep(10);
	}
	return 0;
}

//returns less than 0 if you need to shutdown
int update_screen(GLFWwindow* window){
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
	glDisable(GL_TEXTURE_2D);

    glfwSwapBuffers(window);
    glfwPollEvents();

    return !glfwWindowShouldClose(window);
}

//set a pixel to a color
void set_pixel(int x, int y, Color a){
	//if out of bounds
	if(x < 0 || x >= DATA_WIDTH || y < 0 || y >= DATA_HEIGHT)
		return;
	int i = (y*DATA_WIDTH+x)*4;
	pixels[i+0] = a.r;
	pixels[i+1] = a.g;
	pixels[i+2] = a.b;
	pixels[i+3] = 255;
}

//Recreate textures
void setup_textures(){
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, DATA_WIDTH, DATA_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
	glEnable(GL_TEXTURE_2D);
}