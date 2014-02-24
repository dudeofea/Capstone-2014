/*
*	Digital Signal Processing Functions
*	These functions accept an 8bpp image which is either
*	Thresholded or manipulated as a binary image.
*	
*	See cluster.png to see an example of centroid calculation
*/
#include "dsp.h"

//Create a binary image by thresholding
void threshold(unsigned char *data, int width, int height, int thresh){
	int data_len = width*height;
	for (int i = 0; i < data_len; ++i)
	{
		if(data[i] > thresh){
			data[i] = 255;
		}else{
			data[i] = 0;
		}
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

/*
* Pass in a binary array of pixel data and receive an array of centroids
*/
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
		for (int x = 0; x < width; ++x)
		{
			if (data[width*(y)+(x)] != prev_val)
			{
				//if pixel not zero
				if (data[width*(y)+(x)] > 0)
				{
					//check previous pixel for pixel mass
					if (prev_val > 0)
					{
						if (data[width*(y-1)+(x)] > 0){
							//fix past values
							for (int i = x; i >= 0; i--)
							{
								if (data[width*(y)+(i)] == 0)
									break;
								data[width*(y)+(i)] = data[width*(y-1)+(x)];
							}
							//cent_index--;
							new_index[prev_val] = data[width*(y-1)+(x)];
						}else{
							//printf("(%d, %d) PREV CHAIN\n", x, y);
							data[width*(y)+(x)] = new_index[prev_val];
						}
					}
					//THIS IS WHAT HAPPENS WHEN WORLDS COLLIDE!!!
					else if (data[width*(y-1)+(x)] > 0)
					{
						//printf("(%d, %d) COLLIDE 1\n", x, y);
						data[width*(y)+(x)] = data[width*(y-1)+(x)];
						//this basically doesn't count since it gets grouped
						//into another pixel mass
						cent_index--;
					}
					//beginning of pixel mass
					else
					{
						//printf("(%d, %d) NEW GROUP\n", x, y);
						data[width*(y)+(x)] = new_index[cent_index];
					}
				}
				//end of pixel mass
				else
				{
					//printf("(%d, %d) END GROUP\n", x, y);
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
				centroids[data[val] - 1].x += x;
				centroids[data[val] - 1].y += y;
				centroids[data[val] - 1].size++;
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