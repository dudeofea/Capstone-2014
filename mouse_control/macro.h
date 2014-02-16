#ifndef MACRO_H
#define MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <unistd.h>

#define PS2_VENDOR_ID		0x0810
#define PS2_PRODUCT_ID		0x0001

#define PS2_ENDPOINT		0x81

#define NEUTRAL				0x0f0000
#define CROSS_BUTTON		0x400000
#define SQUARE_BUTTON		0x800000
#define TRIANGLE_BUTTON		0x100000
#define CIRCLE_BUTTON		0x200000

#define bool char
#define false				0
#define true				1

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