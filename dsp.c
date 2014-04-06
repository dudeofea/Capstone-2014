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

//* Function: Get Circle
//* Description: Calculates a circle using three points that will fit on
//that circle. Deprecated since Centroids no longer have floats
//* Input: Three centroids
//* Returns: A Circle struct
//---------DEPRECATED-----------
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

//* Function: Get Line Intersect
//* Description: Calculates the line intersection of two lines formed
// by 4 points. Deprecated since Centroids no longer have floats
//* Input: Four centroids forming 2 lines
//* Returns: An intersection point as a Centroid
//---------DEPRECATED-----------
struct Centroid line_intersect(struct Centroid a1, struct Centroid a2, struct Centroid b1, struct Centroid b2){
	struct Centroid intersect;
	float slopeA = (a2.y - a1.y) / (a2.x - a1.x);		//get Ma
	float slopeB = (b2.y - b1.y) / (b2.x - b1.x);		//get Mb
	float offsetA = a1.y - slopeA * a1.x;				//get Ba
	float offsetB = b1.y - slopeB * b1.x;				//get Bb
	if (slopeA == slopeB)
	{
		//Either parallel or coincident
		//Both of which suck.
		intersect.x = 0;
		intersect.y = 0;
		intersect.size = -1;
		return intersect;
	}
	//find intersect using y = M*x + B
	intersect.x = (offsetB - offsetA) / (slopeA - slopeB);	//since Ma*x + Ba = Mb*x + Bb
	intersect.y = a1.y + slopeA * (intersect.x - a1.x);		//find y from x using M*x + B
	return intersect;
}

//* Function: Threshold
//* Description: Turns an image into a binary image using a given
//threshold. If a value if below the thresh, it is 0, otherwise
//it is 255(SHORT_MAX)
//* Input: pixel buffer, pixel width/height, threshold value
//* Returns: nothing
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

//* Function: Clip Edges
//* Description: Blackens an image, starting from either TOP, BOTTOM, or LEFT.
//The pixels are set to black (zero) by the specified <size>
//* Input: pixel buffer, pixel width/height, specified edge, size of clip
//* Returns: nothing
//---------------DEPRECATED------------------
void clip_edges(unsigned char *data, int width, int height, int edge, int size){
	switch(edge){
		//start from the top going down
		case TOP:
			for (int i = 0; i < width*size; ++i)
			{
				data[i] = 0;
			}
			break;
		//starting from the bottom going up
		case BOTTOM:
			for (int i = width*(height - size); i < width*height; ++i)
			{
				data[i] = 0;
			}
			break;
		//starting at the left moving to the right
		case LEFT:
			for (int y = 0; y < width*height; y += width)
			{
				for (int x = 0; x < size; ++x)
				{
					data[x+y] = 0;
				}
			}
			break;
		//didn't implement RIGHT since it was not needed at the time
		default: break;
	}
}

//* Function: Quantize
//* Description: Downsamples the image based on <qval>. A <qval>
//of 4 would mean the image levels would go from 0,4,8,12,...,252.
//This function reduces the bit resolution of an image
//* Input: pixel buffer, pixel width/height, quantization value
//* Returns: nothing
void quantize(unsigned char *data, int width, int height, int qval){
	int data_len = width*height;
	int tmp;
	for (int i = 0; i < data_len; ++i)
	{
		tmp = round((float)data[i] / qval);
		data[i] = tmp * qval;
	}
}

//* Function: Erode using Cross Pattern Matrix
//* Description: Performs morphological erosion using a cross
//patterned matrix. visually:
//
//	0 1 0 0 0 0 				0 0 0 0 0 0
//	1 1 1 1 1 0 				0 1 0 0 0 0
//	0 1 1 1 1 0 	becomes ->	0 0 1 1 0 0
//	0 0 1 1 1 0 				0 0 0 0 0 0
//	0 0 0 0 0 0 				0 0 0 0 0 0
//	0 0 0 0 1 0 				0 0 0 0 0 0
//
//* Input: pixel buffer, pixel width/height
//* Returns: nothing
void erode_cross(unsigned char *data, int width, int height){
	unsigned char new_data[width*height];
	memset(new_data, SHORT_MIN, width*height);
	for (int y = 1; y < height - 1; ++y)
	{
		for (int x = 1; x < width - 1; ++x)
		{
			//if pixel has other pixels above and below (basically, a cross pattern)
			if( data[width*(y-1)+(x)] > SHORT_MIN &&
				data[width*(y)+(x+1)] > SHORT_MIN &&
				data[width*(y+1)+(x)] > SHORT_MIN &&
				data[width*(y)+(x-1)] > SHORT_MIN &&
				data[width*(y)+(x)] > SHORT_MIN){
				new_data[width*(y)+(x)] = SHORT_MAX;	//set to max
			}else{
				new_data[width*(y)+(x)] = SHORT_MIN;	//set to 0
			}
		}
	}
	//copy new data to data buffer
	memcpy(data, new_data, width*height);
}

//* Function: Get Centroids
//* Description: Calculates the centers of mass (Centroids) of a
//binary image. This returns MAX_CENTROIDS number of Centroids.
//* Input: pixel buffer, pixel width/height
//* Returns: Centroid array of size MAX_CENTROIDS
struct Centroid* get_centroids(unsigned char* data, int width, int height){
	int prev_val = 0;					//previous pixel value
	int cent_index = SHORT_MIN + 1;		//set grouping index to one above lowest value
	int new_index[SHORT_MAX];			//array for group reassignment (in case groups merge)
	struct Centroid centroids[SHORT_MAX];	//centroid array. can handle up to SHORT_MAX pixel groups
	for (int i = 0; i < SHORT_MAX; ++i)
	{
		new_index[i] = i;		//new_index set to regular index
		centroids[i].x = 0;		//blank out centroids
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
			val = width*y+x;		//get 1D equivalent
			data[val] = new_index[data[val]];
			if (data[val] > 0)
			{
				centroids[data[val] - 1].x += x;	//perform average minus division
				centroids[data[val] - 1].y += y;
				centroids[data[val] - 1].size++;	//increase size by one
			}
		}
	}
	//normalize centroids
	static struct Centroid fingers[MAX_CENTROIDS];
	int finger_i = 0;
	for (int i = 0; i < MAX_CENTROIDS; ++i)
	{
		fingers[i].x = 0;		//blank out fingers
		fingers[i].y = 0;
		fingers[i].size = 0;
	}
	for (int i = 0; i < SHORT_MAX; ++i)
	{
		if (centroids[i].size > 0)
		{
			centroids[i].x /= centroids[i].size;	//complete averaging calc
			centroids[i].y /= centroids[i].size;
			//add to return buffer
			fingers[finger_i++] = centroids[i];
			if (finger_i >= MAX_CENTROIDS)
			{
				break;
			}
		}
	}
	//return array of size MAX_CENTROIDS
	return fingers;
}

//* Function: Get Centroids
//* Description: Uses run-length coding to compress only the 0's.
//Can handle up to 2^16 0's in a row. Example:
//
//	0,0,0,0,0,0,2,3,4,5,0,0 becomes-> 0,6,2,3,4,5,0,2
//
//* Input: pixel buffer, pixel length
//* Returns: New length of buffer
int zero_length_encode(char *data, int data_len){
	//temp buffer to hold zlc pixels
	unsigned char buffer[data_len];
	int zero_length = 0;		//running count of zeros
	int buf_i = 0;				//output buffer position
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

//* Function: Get Centroids
//* Description: length must be known. Decompresses a zl coded
//diff buffer into another data buffer. See unit tests for examples
//* Input: pixel difference buffer, pixel output buffer, data length
//* Returns: Centroid array of size MAX_CENTROIDS
void zero_length_decode(char *buffer, unsigned char *data, int data_len){
	int buf_i = 0;			//input buffer position
	int zero_length = 0;	//number of positions to skip (because the diff is 0)
	for (int i = 0; i < data_len; ++i)
	{
		if (buffer[buf_i] == 0)		//if zero, skip some values
		{
			//skip zero
			buf_i++;
			//get length from 2 bytes
			zero_length |= (buffer[buf_i++]&0xFF)<<8;
			zero_length |= (buffer[buf_i++]&0xFF);
			//increase index
			i += zero_length - 1;
			zero_length = 0;
		}else{						//otherwise copy
			data[i] += buffer[buf_i];
			buf_i++;
		}
	}
}