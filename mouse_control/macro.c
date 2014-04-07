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
#include "macro.h"

//* Function: Find PS2 Hub
//* Description: Finds the first PS2 controller hub connected to
//* Input: libusb context, list of libusb devices
//* Returns: a libusb device corresponding to the PS2 Hub or NULL
//if none were found
struct libusb_device *find_ps2_hub(libusb_context *context, libusb_device **devices){
	
	//Get List of USB devices
	ssize_t cnt = libusb_get_device_list(context, &devices);
	if (cnt < 1)
	{
		printf("No USB Devices Found\n");
	}else{
		printf("Found %d USB devices\n", (int)cnt);
	}
	//Find PS2 Controller Hub
	for
	 (int i = 0; i < cnt; ++i)
	{
		//create descriptor
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(devices[i], &desc);
		//check Product/Vendor id
		if (desc.idVendor == PS2_VENDOR_ID && desc.idProduct == PS2_PRODUCT_ID)
		{
			printf("Found A PS2 Controller Hub (%d)\n", i);
			return devices[i];
		}
	}
	return NULL;
}

//* Function: Print Controller
//* Description: Prints all available info on a controller
//* Input: a ps2 controller struct
//* Returns: nothing
void print_controller(struct ps2_controller *cont){
	//print button bitmap
	printf("Controller 1: 0x%x ", cont->key);
	//decompose bitmap to get each button pressed
	if(cont->key & CROSS_BUTTON)
		printf("CROSS ");
	if(cont->key & SQUARE_BUTTON)
		printf("SQUARE ");
	if(cont->key & TRIANGLE_BUTTON)
		printf("TRIANGLE ");
	if(cont->key & CIRCLE_BUTTON)
		printf("CIRCLE ");
	if(cont->key & L1_BUTTON)
		printf("L1 ");
	if(cont->key & L2_BUTTON)
		printf("L2 ");
	if(cont->key & L3_BUTTON)
		printf("L3 ");
	if(cont->key & R1_BUTTON)
		printf("R1 ");
	if(cont->key & R2_BUTTON)
		printf("R2 ");
	if(cont->key & R3_BUTTON)
		printf("R3 ");
	if(cont->key & START_BUTTON)
		printf("START ");
	if(cont->key & SELECT_BUTTON)
		printf("SELECT ");
	//print analog stick data
	printf("%f, %f\n", cont->lstick_x, cont->lstick_y);
}

//* Function: Is Not Neutral
//* Description: Determines if a controller is in the
//neutral position or not
//* Input: a ps2 controller struct
//* Returns: true if the controller is being pressed in
//some way (button down, moved joystick) false otherwise
bool is_not_neutral(struct ps2_controller *cont){
	//check button bitmap
	if(cont->key != 0)
		return true;
	//check left analog stick
	if(cont->lstick_x != 0.0)
		return true;
	if(cont->lstick_y != 0.0)
		return true;
	//check right analog stick
	if(cont->rstick_x != 0.0)
		return true;
	if(cont->rstick_y != 0.0)
		return true;
	return false;
}

//* Function: Get PS2 Input
//* Description: Gets an interrupt transfer using
//libusb and store the corresponding data into two
//ps2 controller structs
//* Input: a libusb handle, a ps2 controller struct,
//another ps2 controller struct
//* Returns: 0 or error code
int get_ps2_input(libusb_device_handle* handle, struct ps2_controller *p1, struct ps2_controller *p2){
	unsigned char data[8];
	int trans = 0;
	//clear controller keypresses
	if(p1 != NULL)
		p1->key = 0; 
	if(p2 != NULL)
		p2->key = 0;
	//loop until data is caught
	while(!p1->key || !p2->key){
		//get interrupt transfer
		int err = libusb_interrupt_transfer(handle,
			PS2_ENDPOINT,		//endpoint
			data,				//data buffer
			8,					//length
			&trans,				//bytes transfered
			PS2_ENDPOINT_TO);	//timeout
		if(err < 0 && err != LIBUSB_ERROR_TIMEOUT){
			return -1;
		}
		if(trans == -1)
			return -1;
		//set the first controller
		if(data[0] == 1 && p1 != NULL){
			//set button bitmap value
			p1->key = BYTE3_SET(data[5]) | BYTE2_SET(data[6]) | BYTE1_SET(data[7]);
			if(data[1] == ZERO_OFFSET &&
			   data[2] == ZERO_OFFSET &&
			   data[3] == ZERO_OFFSET &&
			   data[4] == ZERO_OFFSET){
			   	//if at center, set to 0
				p1->rstick_x = 0.0;
				p1->rstick_y = 0.0;
				p1->lstick_x = 0.0;
				p1->lstick_y = 0.0;
			}else{
				//otherwise calc it's scaled position
				p1->rstick_x = (data[2] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p1->rstick_y = (data[1] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p1->lstick_x = (data[3] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p1->lstick_y = (data[4] - FLOAT_OFFSET) / FLOAT_OFFSET;
				//this goes from -1.0 to 1.0
			}
		//set the second controller
		}else if(data[0] == 2 && p2 != NULL){
			//set button bitmap value
			p2->key = BYTE3_SET(data[5]) | BYTE2_SET(data[6]) | BYTE1_SET(data[7]);
			if(data[1] == ZERO_OFFSET &&
			   data[2] == ZERO_OFFSET &&
			   data[3] == ZERO_OFFSET &&
			   data[4] == ZERO_OFFSET){
			   	//if at center, set to 0
				p2->rstick_x = 0.0;
				p2->rstick_y = 0.0;
				p2->lstick_x = 0.0;
				p2->lstick_y = 0.0;
			}else{
				//otherwise calc it's scaled position
				p2->rstick_x = (data[2] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p2->rstick_y = (data[1] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p2->lstick_x = (data[3] - FLOAT_OFFSET) / FLOAT_OFFSET;
				p2->lstick_y = (data[4] - FLOAT_OFFSET) / FLOAT_OFFSET;
				//this goes from -1.0 to 1.0
			}
		}
	}
	//subtract the NEUTRAL position from the bitmap
	//to get only the keys pressed
	p1->key -= NEUTRAL;
	p2->key -= NEUTRAL;
	return 0;
}

//* Function: Mouse button down
//* Description: Tells the OS to press down the left mouse
//button. This is the same as when you click and hold your
//Left Mouse Button. Works with other buttons on the mouse
//as well
//* Input: an X Display struct, which button to press
//* Returns: nothing
void mouse_button_down(Display* display, unsigned int button){
    XTestFakeButtonEvent(display, button, True, 0);
    XFlush(display);
}
//* Function: Mouse button up
//* Description: Same as previous function only this one
//raises the specified button
//* Input: an X Display struct, which button to unpress
//* Returns: nothing
void mouse_button_up(Display* display, unsigned int button){
    XTestFakeButtonEvent(display, button, False, 0);
    XFlush(display);
}

//* Function: Mouse button up
//* Description: Moves cursor to the position specified
//by the ps2 controller. Uses the Left Analog Stick to
//scale where the mouse should go. (-1.0, 0.0) would bring
//the mouse to center left whilst (1.0, 1.0) would bring
//the mouse to the top right.
//* Input: an X Display struct, A root window struct,
//a window attributes struct, a ps2 controller struct
//* Returns: nothing
void move_cursor(Display *dpy, Window root_window, XWindowAttributes *window_attr, struct ps2_controller *cont){
	int x = window_attr->width / 2 + (int)(cont->lstick_x * window_attr->width / 2);
	int y = window_attr->height / 2 + (int)(cont->lstick_y * window_attr->height / 2);
	XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, x, y);
	XFlush(dpy);
}

//* Function: Main
//* Description: Moves cursor to the position specified
//by the ps2 controller.
//* Input: an X Display struct, A root window struct,
//a window attributes struct, a ps2 controller struct
//* Returns: nothing
int main(int argc, char const *argv[])
{
	Display *dpy;
	Window root_window;
	//setup X11 stuff
	dpy = XOpenDisplay(0);
	root_window = XRootWindow(dpy, 0);
	XWindowAttributes window_attr;
	XGetWindowAttributes(dpy, root_window, &window_attr);
	XSelectInput(dpy, root_window, KeyReleaseMask);

	struct ps2_controller p1_cont;
	struct ps2_controller p2_cont;
	struct libusb_device **devices;
	struct libusb_context *context;
	libusb_init(&context);
	struct libusb_device *ps2 = find_ps2_hub(context, devices);
	//open usb device
	libusb_device_handle *handle;
	int err = 0;
	err = libusb_open(ps2, &handle);
	if(err)
		printf("Unable to open device, %s\n", libusb_error_name(err));
	//detach from kernel
	if (libusb_kernel_driver_active(handle, 0)){
		printf("Device busy...detaching...\n"); 
		libusb_detach_kernel_driver(handle,0);
	}
	else printf("Device free from kernel\n");
	//set interface
	err = libusb_claim_interface(handle, 0);
	if(err)
		printf("Unable to claim interface, %s\n", libusb_error_name(err));
	//get rid of previous interrupts for a bit
	for (int i = 0; i < 10; ++i)
	{
		get_ps2_input(handle, &p1_cont, &p2_cont);
	}
	while(1){
		//get input form controllers
		get_ps2_input(handle, &p1_cont, &p2_cont);
		if(p1_cont.key & L2_BUTTON){
			XCloseDisplay(dpy);
			return 0;
		}
		//mouse click
		if (p1_cont.key & CROSS_BUTTON)
		{
			mouse_button_down(dpy, MOUSE_CLICK);
		}else{
			mouse_button_up(dpy, MOUSE_CLICK);
		}
		//mouse scroll down
		if (p1_cont.rstick_y < -0.5)
		{
			mouse_button_down(dpy, MOUSE_SCROLL_D);
			mouse_button_up(dpy, MOUSE_SCROLL_D);
		}
		//mouse scroll up
		if (p1_cont.rstick_y > 0.5){
			mouse_button_down(dpy, MOUSE_SCROLL_U);
			mouse_button_up(dpy, MOUSE_SCROLL_U);
		}
		//move cursor to position given in ps2 controller
		move_cursor(dpy, root_window, &window_attr, &p1_cont);
	}
	XCloseDisplay(dpy);
	return 0;
}