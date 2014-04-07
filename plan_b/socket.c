/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	API to send pixels and get positions via
*	sockets
*
*	type make to build, ./dsp to run
*	This was developed on Ubuntu 13.10
*
	de2_init();
	while(1)
		get_fingers();
	de2_close();
*/
//to fix popen
#define _GNU_SOURCE

#include "socket.h"
#include "dsp.h"

#define DATA_LEN	352*288
#define DATA_WIDTH	352
#define DATA_HEIGHT	288

#define CHUNK_LEN	352 * 18
#define CHUNK_NUM	16

//Byte setters and getters
#define BYTE4_SET(X)	(0xFF&X)<<24
#define BYTE3_SET(X)	(0xFF&X)<<16
#define BYTE2_SET(X)	(0xFF&X)<<8
#define BYTE1_SET(X)	(0xFF&X)<<0
#define BYTE4_GET(X)	(0xFF000000&X)>>24
#define BYTE3_GET(X)	(0x00FF0000&X)>>16
#define BYTE2_GET(X)	(0x0000FF00&X)>>8
#define BYTE1_GET(X)	(0x000000FF&X)>>0
//socket client header
#define BYTE_HEADER		0xdeadbeef

int fp;	//FILE pointer to camera feed
int frame_ready = 0;	//mutex for camera
int running = 0;
int sockfd = 0;	//socket to DE2 Server
unsigned char buffer[DATA_LEN];			//pixels buffer
unsigned char static_pixels[DATA_LEN];	//initial pixels buffer to subtract
unsigned char send_pixels[DATA_LEN];	//pixels to send
pthread_t cam_thread;

unsigned int read_int(int filedes){
	unsigned char buf[4] = {0,0,0,0};
	read(filedes, buf, 4*sizeof(char));
	unsigned int val = BYTE4_SET(buf[0]) | 
					   BYTE3_SET(buf[1]) | 
					   BYTE2_SET(buf[2]) | 
					   BYTE1_SET(buf[3]);
	return val;
}

struct Centroid read_cent(int filedes){
	struct Centroid a;
	read(filedes, &a, sizeof(struct Centroid));
	return a;
}

void write_int(int filedes, unsigned int val){
	unsigned char buf[4] = {
		BYTE4_GET(val),
		BYTE3_GET(val),
		BYTE2_GET(val),
		BYTE1_GET(val)
	};
	write(filedes, buf, 4*sizeof(char));
}

void *read_from_camera(void *args){
	while(running){
		read(fp, buffer, DATA_LEN);
		frame_ready = 0;
		for (int i = 0; i < DATA_LEN; ++i)
		{
			if (buffer[i] > static_pixels[i])
			{
				send_pixels[i] = buffer[i] - static_pixels[i];
			}else{
				send_pixels[i] = 0;
			}
		}
		frame_ready = 1;
	}
	return NULL;
}

//Finds out how many camers we have (1 or 2)
int get_camera_num(){
	size_t len;
	char* buf = NULL;
	FILE* cam_pointer = popen("ls /dev/ | grep [Vv]ideo1", "r");
	getline(&buf, &len, cam_pointer);
	if (buf != NULL)
	{
		return 2;
	}
	return 1;
	pclose(cam_pointer);
}

int de2_init(){
	running = 1;

	int cam_num = get_camera_num();
	//Open IR Camera
	if (cam_num == 1)
	{
		fp = open("/dev/video0", O_RDONLY);
	}else{
		fp = open("/dev/video1", O_RDONLY);
	}
	
	int n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;

    //init recv buffer
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    //init server address
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(30); 
    if(inet_pton(AF_INET, "192.168.1.1", &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 
    //connect to server
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    n = read(sockfd, recvBuff, sizeof(recvBuff));
    recvBuff[n] = 0;
    printf("Welcome Message: %s", recvBuff);
    //Send buffer size
    write_int(sockfd, DATA_WIDTH);
	write_int(sockfd, DATA_HEIGHT);

	//read static image
	for (int i = 0; i < 5; ++i)
	{
		read(fp, static_pixels, DATA_LEN);
	}

	//make pthread
	pthread_create(&cam_thread, NULL, read_from_camera, NULL);
	return 0;
}

int de2_close(){
	running = 0;
	pthread_join(cam_thread, NULL);
	close(fp);
	return 0;
}

struct Centroid* get_fingers(){
	while(!frame_ready){ ; }
	//write header
	for (int i = 0; i < 10; ++i)
	{
		//padding
		write_int(sockfd, 0x0);
	}
	write_int(sockfd, BYTE_HEADER);
	write_int(sockfd, CHUNK_LEN);
	write_int(sockfd, CHUNK_NUM);
	int offset = 0;
	for (int i = 0; i < CHUNK_NUM; ++i)
	{
		//send to device
		write(sockfd, send_pixels+offset, CHUNK_LEN);
		offset += CHUNK_LEN;
	}
	int check = checksum(send_pixels, DATA_LEN);
	printf("%x\n", check);
	//write checksum
	write_int(sockfd, check);
	static struct Centroid cents[10];
	for (int i = 0; i < 10; ++i)
	{
		cents[i] = read_cent(sockfd);
		//printf("cent: %d, %d, %d\n", cents[i].x, cents[i].y, cents[i].size);
	}
	read_cent(sockfd);
	return cents;
}