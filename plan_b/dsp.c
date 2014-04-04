/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	The Algorithms used originate from a base knowledge
*	of ECE 440 and ECE 442
*
*	Digital Signal Processing Functions
*	These functions accept an 8bpp image which is either
*	Thresholded or manipulated as a binary image.
*	
*	See cluster.png to see an example of centroid calculation
*/
#include "dsp.h"

#define SHORT_MAX	255
#define SHORT_MIN	0

//Calculates a circle based on 3 points
struct Circle get_circle(struct Centroid a, struct Centroid b, struct Centroid c){
	struct Circle circle;
	struct Centroid ab1_p, ab2_p, bc1_p, bc2_p;		//Perpendicular points used to create a bisector
	//First point in bisectors is the midpoint
	ab1_p.x = (a.x + b.x) / 2;
	ab1_p.y = (a.y + b.y) / 2;
	bc1_p.x = (b.x + c.x) / 2;
	bc1_p.y = (b.y + c.y) / 2;
	//Second point is calc'ed first points as ref
	ab2_p.x = ab1_p.x - (ab1_p.y - a.y);
	ab2_p.y = ab1_p.y + (ab1_p.x - a.x);
	bc2_p.x = bc1_p.x - (bc1_p.y - b.y);
	bc2_p.y = bc1_p.y + (bc1_p.x - b.x);
	struct Centroid center = line_intersect(ab1_p, ab2_p, bc1_p, bc2_p);
	circle.x = center.x;
	circle.y = center.y;
	float val1 = center.x - a.x;
	float val2 = center.y - a.y;
	circle.r = sqrt(val1 * val1 + val2 * val2);
	return circle;
}

//Get line intersection from 2 lines formed from Centroids
struct Centroid line_intersect(struct Centroid a1, struct Centroid a2, struct Centroid b1, struct Centroid b2){
	struct Centroid intersect;
	float slopeA = (a2.y - a1.y) / (a2.x - a1.x);
	float slopeB = (b2.y - b1.y) / (b2.x - b1.x);
	float offsetA = a1.y - slopeA * a1.x;
	float offsetB = b1.y - slopeB * b1.x;
	if (slopeA == slopeB)
	{
		//Either parallel or coincident
		//Both of which suck.
		intersect.x = 0;
		intersect.y = 0;
		return intersect;
	}
	intersect.x = (offsetB - offsetA) / (slopeA - slopeB);
	intersect.y = a1.y + slopeA * (intersect.x - a1.x);
	//printf("x: %f\n", intersect.x);
	//printf("y: %f\n", intersect.y);
	return intersect;
}

//Create a binary image by thresholding
void threshold(unsigned char *data, int width, int height, int thresh){
	int data_len = width*height;
	for (int i = 0; i < data_len; ++i)
	{
		if(data[i] > thresh){
			data[i] = SHORT_MAX;
		}else{
			data[i] = SHORT_MIN;
		}
	}
}

//Quantizes an image
void quantize(unsigned char *data, int width, int height, int qval){
	int data_len = width*height;
	int tmp;
	for (int i = 0; i < data_len; ++i)
	{
		tmp = round((float)data[i] / qval);
		data[i] = tmp * qval;
	}
}

unsigned char checksum(unsigned char *data, int data_len){
	unsigned char tmp = 0;
	for (int i = 0; i < data_len; ++i)
	{
		tmp += data[i];
	}
	return tmp;
}

//erode using a cross pattern matrix
void erode_cross(unsigned char *data, int width, int height){
	unsigned char new_data[width*height];
	memset(new_data, SHORT_MIN, width*height);
	for (int y = 1; y < height - 1; ++y)
	{
		for (int x = 1; x < width - 1; ++x)
		{
			//if
			if( data[width*(y-1)+(x)] > SHORT_MIN &&
				data[width*(y)+(x+1)] > SHORT_MIN &&
				data[width*(y+1)+(x)] > SHORT_MIN &&
				data[width*(y)+(x-1)] > SHORT_MIN &&
				data[width*(y)+(x)] > SHORT_MIN){
				new_data[width*(y)+(x)] = SHORT_MAX;
			}else{
				new_data[width*(y)+(x)] = SHORT_MIN;
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
	int cent_index = SHORT_MIN + 1;
	int new_index[SHORT_MAX];
	struct Centroid centroids[SHORT_MAX];
	for (int i = 0; i < SHORT_MAX; ++i)
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
				if (data[width*(y)+(x)] > SHORT_MIN)
				{
					//check previous pixel for pixel mass
					if (prev_val > SHORT_MIN)
					{
						if (data[width*(y-1)+(x)] > SHORT_MIN){
							//fix past values
							for (int i = x; i >= 0; i--)
							{
								if (data[width*(y)+(i)] == SHORT_MIN)
									break;
								data[width*(y)+(i)] = data[width*(y-1)+(x)];
							}
							new_index[prev_val] = data[width*(y-1)+(x)];
						}else{
							//printf("(%d, %d) PREV CHAIN\n", x, y);
							data[width*(y)+(x)] = new_index[prev_val];
						}
					}
					//THIS IS WHAT HAPPENS WHEN WORLDS COLLIDE!!!
					else if (data[width*(y-1)+(x)] > SHORT_MIN)
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
	static struct Centroid fingers[10];
	int finger_i = 0;
	for (int i = 0; i < SHORT_MAX; ++i)
	{
		fingers[i].x = 0;
		fingers[i].y = 0;
		fingers[i].size = 0;
		if (centroids[i].size > 0)
		{
			centroids[i].x /= centroids[i].size;
			centroids[i].y /= centroids[i].size;
			//add to return buffer
			fingers[finger_i++] = centroids[i];
		}
	}
	return fingers;
}

//Uses run-length coding to compress only the 0's.
//Can handle up to 2^16 0's in a row
//returns: new length of buffer
int zero_length_encode(char *data, int data_len){
	//temp buffer to hold zlc pixels
	unsigned char buffer[data_len];
	int zero_length = 0;
	int buf_i = 0;
	for (int i = 0; i < data_len; ++i)
	{
		if (data[i] == 0)
		{
			//if zero
			zero_length++;
		}else{
			//if there was a run of zeros
			if (zero_length > 0)
			{
				//a zero
				buffer[buf_i++] = 0;
				//highest byte
				buffer[buf_i++] = (zero_length&0x00FF00)>>8;
				//lowest byte
				buffer[buf_i++] = (zero_length&0x0000FF);
				zero_length = 0;
			}
			buffer[buf_i++] = data[i];
		}
	}
	//copy to data
	memcpy(data, buffer, buf_i);
	//return length
	return buf_i;
}

//length must be known. Decompresses a zl coded
//diff buffer into another data buffer.
void zero_length_decode(char *buffer, unsigned char *data, int data_len){
	int buf_i = 0;
	int zero_length = 0;
	for (int i = 0; i < data_len; ++i)
	{
		if (buffer[buf_i] == 0)
		{
			//skip zero
			buf_i++;
			//get length from 2 bytes
			zero_length |= (buffer[buf_i++]&0xFF)<<8;
			zero_length |= (buffer[buf_i++]&0xFF);
			//increase index
			i += zero_length - 1;
			zero_length = 0;
		}else{
			data[i] += buffer[buf_i];
			buf_i++;
		}
	}
}