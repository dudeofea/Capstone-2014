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
*/
#include "socket.h"
#include "dsp.h"

#define DATA_LEN	352*288
#define DATA_WIDTH	352
#define DATA_HEIGHT	288

#define CHUNK_LEN	352 * 18
#define CHUNK_NUM	16

int fp;	//FILE pointer to camera feed
int sockfd = 0;	//socket to DE2 Server
unsigned char buffer[DATA_LEN];

unsigned int read_int(int filedes){
	unsigned char buf[4] = {0,0,0,0};
	int n = read(filedes, buf, 4*sizeof(char));
	unsigned int val = (0xFF&buf[0])<<24 | (0xFF&buf[1])<<16 | (0xFF&buf[2])<<8 | (0xFF&buf[3]);
	return val;
}

struct Centroid read_cent(int filedes){
	struct Centroid a;
	int n = read(filedes, &a, sizeof(struct Centroid));
	return a;
}

void write_int(int filedes, unsigned int val){
	unsigned char buf[4] = {
		(val&0xFF000000)>>24,
		(val&0x00FF0000)>>16,
		(val&0x0000FF00)>>8,
		(val&0x000000FF)
	};
	write(filedes, buf, 4*sizeof(char));
}

int de2_init(){
	//Open IR Camera
	fp = open("/dev/video0", O_RDONLY);
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
	return 0;
}

int de2_close(){
	close(fp);
	return 0;
}

Centroid* get_fingers(){
	read(fp, buffer, DATA_LEN);
	//write header
	write_int(sockfd, 0xdeadbeef);
	write_int(sockfd, CHUNK_LEN);
	write_int(sockfd, CHUNK_NUM);
	int offset = 0;
	for (int i = 0; i < CHUNK_NUM; ++i)
	{
		//send to device
		write(sockfd, buffer+offset, CHUNK_LEN);
		offset += CHUNK_LEN;
	}
	threshold(buffer, DATA_WIDTH, DATA_HEIGHT, 0xFF / 4);
	struct Centroid cents[10];// = get_centroids(buffer, DATA_WIDTH, DATA_HEIGHT);
	for (int i = 0; i < 10; ++i)
	{
		cents[i] = read_cent(sockfd);
		printf("cent: %d, %d, %d\n", cents[i].x, cents[i].y, cents[i].size);
	}
    //unsigned int a = read_int(sockfd);
    //printf("Int: %x\n", a); 
	return NULL;
}