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