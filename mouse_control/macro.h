/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	Captures USB data from a Playstation 2 Controller
*	and moves the mouse accordingly
*
*	The controllers are hooked into a USB PS2 controller
*	Hub such as this one: http://i00.i.aliimg.com/img/pb/531/730/506/506730531_817.jpg
*
*	This will be modified to run with the DE2 instead
*	of a PS2 Controller, since both use USB.
*
*	type make to build, ./macro to run
*	This was developed on Ubuntu 13.10
*/
#ifndef MACRO_H
#define MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <unistd.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

//product and vendor id of device
#define PS2_VENDOR_ID		0x0810
#define PS2_PRODUCT_ID		0x0001

//usb transfer definitions
#define PS2_ENDPOINT		0x81
#define PS2_ENDPOINT_TO		500

#define NEUTRAL				0x0f0000
#define CROSS_BUTTON		0x400000
#define SQUARE_BUTTON		0x800000
#define TRIANGLE_BUTTON		0x100000
#define CIRCLE_BUTTON		0x200000
#define L1_BUTTON			0x400
#define L2_BUTTON			0x100
#define L3_BUTTON			0x4000
#define R1_BUTTON			0x800
#define R2_BUTTON			0x200
#define R3_BUTTON			0x8000
#define START_BUTTON		0x2000
#define	SELECT_BUTTON		0x1000

#define ZERO_OFFSET			0x7f		//corresponding zero of ps2 analog stick
#define FLOAT_OFFSET		128.0 		//number used to offset and scale stick from -1.0 to 1.0

//sets a number into a byte
#define BYTE4_SET(X)			X << 24
#define BYTE3_SET(X)			X << 16
#define BYTE2_SET(X)			X << 8
#define BYTE1_SET(X)			X << 0

#define bool char
#define false				0
#define true				1

#define MOUSE_CLICK			1 			//left mouse button
#define MOUSE_SCROLL_D		4 			//mouse scroll down
#define MOUSE_SCROLL_U		5 			//mouse scroll up

struct ps2_controller
{
	int key;
	float lstick_x;
	float lstick_y;
	float rstick_x;
	float rstick_y;
};

struct libusb_device *find_ps2_hub(libusb_context *context, libusb_device **devices);
int get_ps2_input(libusb_device_handle* handle, struct ps2_controller *p1, struct ps2_controller *p2);

#endif