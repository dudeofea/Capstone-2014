/*
*	Digital Signal Processing Functions
*	These functions accept an 8bpp image which is either
*	Thresholded or manipulated as a binary image.
*	
*	See cluster.png to see an example of centroid calculation
*/
#include <stdio.h>
#include <string.h>

struct Centroid
{
	float x;
	float y;
	int size;
};

void erode_cross(unsigned char *data, int width, int height);
struct Centroid* get_centroids(unsigned char* data, int width, int height);
void threshold(unsigned char *data, int width, int height, int thresh);