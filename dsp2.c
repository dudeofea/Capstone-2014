#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <GLFW/glfw3.h>

#define BOUNDS 1.0f

#define DATA_LEN	352*288

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

struct Centroid
{
	float x;
	float y;
	int size;
};

GLuint texture[2];
unsigned char buffer[DATA_LEN];
unsigned char pixels[DATA_LEN * 4];
unsigned char pixels2[DATA_LEN * 4];
int fp;

void copy_to_pixels(){
    read(fp, buffer, DATA_LEN);
    for (int i = 0; i < DATA_LEN; ++i)
    {
		pixels[i*4+0] = buffer[i];
		pixels[i*4+1] = buffer[i];
		pixels[i*4+2] = buffer[i];
		pixels[i*4+3] = 255;
    }
}

//erode using a cross pattern matrix
void erode_cross(unsigned char *data, int width, int height){
	unsigned char new_data[width*height];
	memset(new_data, 0, width*height);
	for (int y = 1; y < height - 1; ++y)
	{
		for (int x = 1; x < width - 1; ++x)
		{
			if( data[width*(y-1)+(x)] > 0 &&
				data[width*(y)+(x+1)] > 0 &&
				data[width*(y+1)+(x)] > 0 &&
				data[width*(y)+(x-1)] > 0 &&
				data[width*(y)+(x)] > 0){
				new_data[width*(y)+(x)] = 255;
			}else{
				new_data[width*(y)+(x)] = 0;
			}
		}
	}
	memcpy(data, new_data, width*height);
}

struct Centroid* get_centroids(unsigned char* data, int width, int height){
	int prev_val = 0;
	int cent_index = 1;
	int new_index[255];
	static struct Centroid centroids[255];
	for (int i = 0; i < 255; ++i)
	{
		new_index[i] = i;
		centroids[i].x = 0;
		centroids[i].y = 0;
		centroids[i].size = 0;
	}
	//first pass, find pixel groups
	for (int y = 1; y < height; ++y)
	{
		for (int x = 1; x < width - 1; ++x)
		{
			if (data[width*(y)+(x)] != prev_val)
			{
				//THIS IS WHAT HAPPENS WHEN WORLDS COLLIDE!!!
				if (data[width*(y-1)+(x)] > 0 && data[width*(y)+(x-1)] > 0 && 
					data[width*(y-1)+(x)] != data[width*(y)+(x-1)])
				{
					//store lowest value
					if(new_index[data[width*(y)+(x-1)]] < new_index[data[width*(y-1)+(x)]]){
						new_index[data[width*(y-1)+(x)]] = new_index[data[width*(y)+(x-1)]];
					}else{
						new_index[data[width*(y)+(x-1)]] = new_index[data[width*(y-1)+(x)]];
					}
				}
				//pixel mass above
				if (data[width*(y-1)+(x)] > 0)
				{
					data[width*(y)+(x)] = new_index[data[width*(y-1)+(x)]];
				}
				else if (data[width*(y)+(x)] > 0)
				{
					//check previous pixel for pixel mass
					if (prev_val > 0)
					{
						data[width*(y)+(x)] = new_index[prev_val];
					}
					//beginning of pixel mass
					else
					{
						data[width*(y)+(x)] = new_index[cent_index];
					}
				}
				//end of pixel mass
				else
				{
					cent_index++;
				}
			}
			prev_val = data[width*(y)+(x)];
		}
	}
	//second pass, calc centroids
	int val = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			val = width*y+x;
			data[val] = new_index[data[val]];
			if (data[val] > 0)
			{
				centroids[data[val]].x += x;
				centroids[data[val]].y += y;
				centroids[data[val]].size++;
			}
		}
	}
	//normalize centroids
	for (int i = 0; i < 255; ++i)
	{
		if (centroids[i].size > 0)
		{
			centroids[i].x /= centroids[i].size;
			centroids[i].y /= centroids[i].size;
		}
	}
	//TODO: sort the centroids
	return centroids;
}

void perform_DSP(){
	//threshold
	for (int i = 0; i < DATA_LEN; ++i)
	{
		if(buffer[i] > 128){
			buffer[i] = 255;
		}else{
			buffer[i] = 0;
		}
	}
	//perform morphological erosion (computer only)
	//erode_cross(buffer, 352, 288);
	//erode_cross(buffer, 352, 288);
	//calculate centroids
	struct Centroid *centroids = get_centroids(buffer, 352, 288);
	//load data into pixels as greyscale
	for (int i = 0; i < DATA_LEN; ++i)
    {
    	/*if (buffer[i] == 0)
    	{
    		pixels2[i*4+0] = 0;
			pixels2[i*4+1] = 0;
			pixels2[i*4+2] = 0;
    	}else if (buffer[i] == 1)
    	{
    		pixels2[i*4+0] = 100;
			pixels2[i*4+1] = 50;
			pixels2[i*4+2] = 50;
    	}else if (buffer[i] == 2)
    	{
    		pixels2[i*4+0] = 50;
			pixels2[i*4+1] = 100;
			pixels2[i*4+2] = 50;
    	}
    	else if (buffer[i] == 3)
    	{
    		pixels2[i*4+0] = 50;
			pixels2[i*4+1] = 50;
			pixels2[i*4+2] = 100;
    	}
    	else if (buffer[i] == 4)
    	{
    		pixels2[i*4+0] = 100;
			pixels2[i*4+1] = 100;
			pixels2[i*4+2] = 50;
    	}
    	else if (buffer[i] == 5)
    	{
    		pixels2[i*4+0] = 50;
			pixels2[i*4+1] = 100;
			pixels2[i*4+2] = 100;
    	}
    	else if (buffer[i] > 6)
    	{
    		pixels2[i*4+0] = 100;
			pixels2[i*4+1] = 100;
			pixels2[i*4+2] = 100;
    	}*/
		if (buffer[i] > 0)
		{
			pixels2[i*4+0] = buffer[i] * 7;
			pixels2[i*4+1] = 10;
			pixels2[i*4+2] = 255;
		}else{
			pixels2[i*4+0] = 0;
			pixels2[i*4+1] = 0;
			pixels2[i*4+2] = 0;
		}
		pixels2[i*4+3] = 255;
    }
    for (int i = 0; i < 255; ++i)
    {
    	//if decently size, draw as a red pixel
    	if (centroids[i].size > 20)
    	{
    		//printf("cent: %f %f\n", centroids[i].y, centroids[i].x);
    		int val = (int)(352*centroids[i].y+centroids[i].x);
    		pixels2[val*4+0] = 255;
			pixels2[val*4+1] = 0;
			pixels2[val*4+2] = 0;
			pixels2[val*4+3] = 255;
    	}
    }
}

void setup_textures(){
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, 352, 288, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
	glEnable(GL_TEXTURE_2D);

	//perform DSP
	perform_DSP();

	glGenTextures(1, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, 352, 288, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels2);
	glEnable(GL_TEXTURE_2D);
}

void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char const *argv[])
{
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
	fp = open("/dev/video0", O_RDONLY);
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

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-ratio, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.0f,-1.0f, 0.0);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-ratio, -1.0f, 0.0);
		glEnd();

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
	return 0;
}