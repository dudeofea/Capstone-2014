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
	int x;
	int y;
	int size;
};

struct Circle
{
	float x;	//Center X
	float y;	//Center Y
	float r;	//Radius
};

struct Circle get_circle(struct Centroid a, struct Centroid b, struct Centroid c);
struct Centroid line_intersect(struct Centroid a1, struct Centroid a2, struct Centroid b1, struct Centroid b2);
void erode_cross(unsigned char *data, int width, int height);
struct Centroid* get_centroids(unsigned char* data, int width, int height);
void threshold(unsigned char *data, int width, int height, int thresh);
void quantize(unsigned char *data, int width, int height, int qval);
int zero_length_encode(char *data, int data_len);
void zero_length_decode(char *buffer, unsigned char *data, int data_len);
unsigned char checksum(unsigned char *data, int len);