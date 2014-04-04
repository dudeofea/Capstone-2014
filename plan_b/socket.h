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
#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h> 
#include <pthread.h>

#include "dsp.h"

int de2_init();
int de2_close();
struct Centroid* get_fingers();

#endif