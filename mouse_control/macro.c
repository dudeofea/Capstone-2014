/*
*	© Copyright 2014, Group 4, ECE 492
*
*	This code was created through the collaboration
*	of Alix Krahn, Denis Lachance and Adam Thomson. 
*
*	Captures USB data from a Playstation 2 Controller
*	and moves the mouse accordingly
*
*	This will be modified to run with the DE2 instead
*	of a PS2 Controller, since both use USB.
*
*	type make to build, ./macro to run
*	This was developed on Ubuntu 13.10
*/
#include "macro.h"

//find the first quckcam express camera attached to usb
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
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(devices[i], &desc);
		if (desc.idVendor == PS2_VENDOR_ID && desc.idProduct == PS2_PRODUCT_ID)
		{
			printf("Found A PS2 Controller Hub (%d)\n", i);
			return devices[i];
		}
	}
	return NULL;
}

void print_controller(struct ps2_controller *cont){
	printf("Controller 1: 0x%x ", cont->key);
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
	printf("%f, %f\n", cont->lstick_x, cont->lstick_y);
}

bool is_not_neutral(struct ps2_controller *cont){
	if(cont->key != 0)
		return true;
	if(cont->lstick_x != 0.0)
		return true;
	if(cont->lstick_y != 0.0)
		return true;
	if(cont->rstick_x != 0.0)
		return true;
	if(cont->rstick_y != 0.0)
		return true;
	return false;
}

int get_ps2_input(libusb_device_handle* handle, struct ps2_controller *p1, struct ps2_controller *p2){
	unsigned char data[8];
	int trans = 0;
	p1->key = 0; p2->key = 0;
	//loop until data is caught
	while(!p1->key || !p2->key){
		int err = libusb_interrupt_transfer(handle,
			PS2_ENDPOINT,		//endpoint
			data,				//data buffer
			8,					//length
			&trans,				//bytes transfered
			500);				//timeout
		if(err < 0 && err != LIBUSB_ERROR_TIMEOUT){
			return -1;
		}
		if(trans == -1)
			return -1;
		if(data[0] == 1){
			p1->key = data[5] << 16 | data[6] << 8 | data[7];
			if(data[1] == 0x7f && data[2] == 0x7f && data[3] == 0x7f && data[4] == 0x7f){
				p1->rstick_x = 0.0;
				p1->rstick_y = 0.0;
				p1->lstick_x = 0.0;
				p1->lstick_y = 0.0;
			}else{
				p1->rstick_x = (data[2] - 128) / 128.0;
				p1->rstick_y = (data[1] - 128) / 128.0;
				p1->lstick_x = (data[3] - 128) / 128.0;
				p1->lstick_y = (data[4] - 128) / 128.0;
			}
		}else if(data[0] == 2){
			p2->key = data[5] << 16 | data[6] << 8 | data[7];
			if(data[1] == 0x7f && data[2] == 0x7f && data[3] == 0x7f && data[4] == 0x7f){
				p2->rstick_x = 0.0;
				p2->rstick_y = 0.0;
				p2->lstick_x = 0.0;
				p2->lstick_y = 0.0;
			}else{
				p2->rstick_x = (data[2] - 128) / 128.0;
				p2->rstick_y = (data[1] - 128) / 128.0;
				p2->lstick_x = (data[3] - 128) / 128.0;
				p2->lstick_y = (data[4] - 128) / 128.0;
			}
		}
	}
	p1->key -= NEUTRAL;
	p2->key -= NEUTRAL;
	//print_controller(p1);
	return 0;
}

void mouse_button_down(Display* display, unsigned int button){
    XTestFakeButtonEvent(display, button, True, 0);
    XFlush(display);
}
void mouse_button_up(Display* display, unsigned int button){
    XTestFakeButtonEvent(display, button, False, 0);
    XFlush(display);
}

void move_cursor(Display *dpy, Window root_window, XWindowAttributes *window_attr, struct ps2_controller *cont){
	int x = window_attr->width / 2 + (int)(cont->lstick_x * window_attr->width / 2);
	int y = window_attr->height / 2 + (int)(cont->lstick_y * window_attr->height / 2);
	XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, x, y);
	XFlush(dpy);
}

int main(int argc, char const *argv[])
{
	Display *dpy;
	Window root_window;
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
		get_ps2_input(handle, &p1_cont, &p2_cont);
		//print_controller(&p1_cont);
		if(p1_cont.key & L2_BUTTON){
			XCloseDisplay(dpy);
			return 0;
		}
		if (p1_cont.key & CROSS_BUTTON)
		{
			mouse_button_down(dpy, 1);
		}else{
			mouse_button_up(dpy, 1);
		}
		if (p1_cont.rstick_y < -0.5)
		{
			mouse_button_down(dpy, 4);
			mouse_button_up(dpy, 4);
		}
		if (p1_cont.rstick_y > 0.5){
			mouse_button_down(dpy, 5);
			mouse_button_up(dpy, 5);
		}
		move_cursor(dpy, root_window, &window_attr, &p1_cont);
	}
	XCloseDisplay(dpy);
	return 0;
}