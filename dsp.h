/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	Digital Signal Processing Functions
*	These functions accept an 8bpp image which is either
*	Thresholded or manipulated as a binary image.
*	
*	See cluster.png to see an example of centroid calculation
*/
#include <stdio.h>
#include <string.h>
#include <math.h>

struct Centroid
{
	float x;
	float y;
	int size;
};

struct Circle
{
	float x;	//Center X
	float y;	//Center Y
	float r;	//Radius
};

#define TOP		0
#define RIGHT	1
#define BOTTOM	2
#define LEFT	3

struct Circle get_circle(struct Centroid a, struct Centroid b, struct Centroid c);
struct Centroid line_intersect(struct Centroid a1, struct Centroid a2, struct Centroid b1, struct Centroid b2);
void erode_cross(unsigned char *data, int width, int height);
struct Centroid* get_centroids(unsigned char* data, int width, int height);
void threshold(unsigned char *data, int width, int height, int thresh);
void clip_edges(unsigned char *data, int width, int height, int edge, int size);