#ifndef _PAINT_H_
#define _PAINT_H_

typedef struct
{
	char r;
	char g;
	char b;
} Color;

int update_screen(GLFWwindow* window);
void set_pixel(int x, int y, Color a);
void setup_textures();

#endif