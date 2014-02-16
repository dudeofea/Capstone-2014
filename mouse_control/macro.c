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
		int r = libusb_get_device_descriptor(devices[i], &desc);
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
				p1->lstick_x = 0.0;
				p1->lstick_y = 0.0;
				p1->rstick_x = 0.0;
				p1->rstick_y = 0.0;
			}else{
				p1->lstick_x = (data[2] - 128) / 128.0;
				p1->lstick_y = (data[1] - 128) / 128.0;
				p1->rstick_x = (data[3] - 128) / 128.0;
				p1->rstick_y = (data[4] - 128) / 128.0;
			}
		}else if(data[0] == 2){
			p2->key = data[5] << 16 | data[6] << 8 | data[7];
			if(data[1] == 0x7f && data[2] == 0x7f && data[3] == 0x7f && data[4] == 0x7f){
				p2->lstick_x = 0.0;
				p2->lstick_y = 0.0;
				p2->rstick_x = 0.0;
				p2->rstick_y = 0.0;
			}else{
				p2->lstick_x = (data[2] - 128) / 128.0;
				p2->lstick_y = (data[1] - 128) / 128.0;
				p2->rstick_x = (data[3] - 128) / 128.0;
				p2->rstick_y = (data[4] - 128) / 128.0;
			}
		}
	}
	p1->key -= NEUTRAL;
	p2->key -= NEUTRAL;
	//print_controller(p1);
	return 0;
}