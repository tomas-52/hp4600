//modified from sources on https://github.com/Triften/hp4600 as of 2023.10.14
//sourced by http://www.chmil.org/hp4600linux/

// This file was originally generated with usbsnoop2libusb.pl from a usbsnoop log file.
// Latest version of the script should be in http://iki.fi/lindi/usb/usbsnoop2libusb.pl
// All giant tables have been moved to hp4600consts.h file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <usb.h>
#include "hp4600consts_150.h"
//just for time evaluation
#include <time.h>

#if 0
 #include <linux/usbdevice_fs.h>
 #define LIBUSB_AUGMENT
 #include "libusb_augment.h"
#endif

//the BMP header and commands moved from hp4600consts.h file here for simplicity when testing
//based on snoop for 150 dpi scan changed BunchOfConfig, SetRegisters, CCDSetup and Table0x00001000
//where possible removed duplicities of tables and commands
//the resulting file dimensions are 1274 x 1760 pixels

//BMP file header:
//BM in the first line identifies the bitmap file, next is the zeroed size and two reserved items
//in the second line is the size of the header (54 bytes) and of the header itself (40 bytes)
//the third line gives the width and height of the picture in pixels (1274x1760) after all changes
//the fourth line gives the number of bits used (1 level, 24 bits per pixel, no compression)
//in the fifth line is the size of the picture in bytes (if 0 it will be calculated from dimensions)
//the sixth row gives the resolution per meter on peripherals (here 150 dpi, but zeroes are accepted)
//the last row gives the number of colours for peripherals (0 means maximum)

const char tmp_file_header[] = {
	0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
	0xfc, 0x04, 0x00, 0x00, 0xe1, 0x06, 0x00, 0x00,
	0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x11, 0x17, 0x00, 0x00, 0x11, 0x17, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char bmp_file_header[] = {
	0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
	0xfa, 0x04, 0x00, 0x00, 0xe0, 0x06, 0x00, 0x00,
	0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x11, 0x17, 0x00, 0x00, 0x11, 0x17, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct usb_dev_handle *devh;

void release_usb_device(int dummy) {
	int ret;
	ret = usb_release_interface(devh, 0);
	if (!ret)
		printf("failed to release interface: %d\n", ret);
	usb_close(devh);
	if (!ret)
		printf("failed to close interface: %d\n", ret);
	exit(1);
}

struct usb_device *find_device(int vendor, int product) {
	struct usb_bus *bus;
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		struct usb_device *dev;
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
			return dev;
		}
	}
	return NULL;
}

void print_file(char *bytes, int len, FILE *output){
	int i;
	if (len > 0) {
		for (i=0; i<len; i++) {
			fprintf(output, "%c", bytes[i]);
		}
	}
}

int writeRegisters(char *buf){
	return usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000004, 1000);
}

int read_register(char *buf, char reg){
	int ret;
	char reg_command[] = {0x8b, 0x00, 0x8b, 0x00};
	reg_command[1] = reg_command[3] = reg;
	memcpy(buf, reg_command, 0x0000004);
	ret = writeRegisters(buf);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE + USB_ENDPOINT_IN, 0x000000c, 0x0000007, 0x0000000, buf, 0x0000001, 1000);
	return ret;
}

int select_register_bank(char *buf, int bank){
	int ret;
	char reg_command[] = {0x5f, 0x00, 0x5f, 0x00};
	if(bank < 0 || bank > 2) { return 0; }
	reg_command[1] = reg_command[3] = bank;
	memcpy(buf, reg_command, 0x0000004);
	ret = writeRegisters(buf);
	return ret;
}

int openscanchip(char *buf){
	int ret;
	memcpy(buf, "\x64\x64\x64\x64", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x65\x65\x65\x65", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x44\x44\x44\x44", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x45\x45\x45\x45", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	return ret;
}

int closescanchip(char *buf){
	int ret;
	memcpy(buf, "\x64\x64\x64\x64", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x65\x65\x65\x65", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x16\x16\x16\x16", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	if(ret != 0){ return ret; }
	memcpy(buf, "\x17\x17\x17\x17", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000090, 0x0000000, buf, 0x0000004, 1000);
	return ret;
}

int do_command_one(char *buf){
	int ret;
	memcpy(buf, "\x04\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	return ret;
}

int do_command_two(char *buf){
	int ret;
	memcpy(buf, "\x01\xa6\xe0\x08\x01\x02\x04\x08\x10\x20\x40\x80\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	return ret;
}

int do_command_three(char *buf){
	int ret;
	memcpy(buf, "\x02\xa6\xe0\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	return ret;
}

int do_command_four(char *buf){
	int ret;
	memcpy(buf, "\x02\xa0\x05\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	ret = read_register(buf, 0x03);
	return ret;
}

int do_command_five(char *buf){
	int ret;
	memcpy(buf, "\x04\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	return ret;
}

int do_command_six(char *buf){
	int ret;
	memcpy(buf, "\x02\xa0\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	return ret;
}

int adjustPowerSaveRegisters(char *buf){
	int ret;
	memcpy(buf, "\x94\x31\x92\x57", 0x0000004);
	ret = writeRegisters(buf);
	return ret;
}

int do_command_eight(char *buf){
	int ret;
	memcpy(buf, "\x96\x21\x96\x21", 0x0000004);
	ret = writeRegisters(buf);
	return ret;
}

int clearFIFO(char *buf){
	int ret;
	memcpy(buf, "\x00\x00\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000005, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, "\x00\x00\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, SCANCONTROL, 0x0000000, buf, 0x0000004, 1000);
	return ret;
}

int readChipString(char *buf){
	int ret;
	ret = read_register(buf, 0x1f);
	ret = read_register(buf, 0x1e);
	ret = read_register(buf, 0x1d);
	ret = read_register(buf, 0x1c);
	return ret;
}

int readScanState(char *buf){
	int ret;
	ret = read_register(buf, 0x01);
	return ret;
}

int doSDRAMSetup(char *buf){
	int ret;
	memcpy(buf, "\x87\xf1\x87\xa5\x87\x91\x87\x81\x87\x11\x87\xf0", 0x000000c);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000000c, 1000);
	return ret;
}

int doCmdSequenceAlpha(char *buf){
	int ret;
	ret = do_command_one(buf);
	ret = do_command_two(buf);
	ret = do_command_three(buf);
	ret = do_command_four(buf);
	ret = readChipString(buf);
	return ret;
}

int setScannerIdle(char *buf){
	memcpy(buf, "\xf4\x00\x86\x00", 0x0000004);
	return writeRegisters(buf);
}

int setRegisters2ax(char *buf){
	int ret;
	ret = select_register_bank(buf, 2);
	memcpy(buf, "\xa0\x01\xa0\x01", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, 
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
		"\xa1\x00\xa2\x11\xa1\x00\xa2\x18\xa1\x00\xa2\x17\xa1\x00\xa2\x18"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, "\xa0\x00\xa0\x00", 0x0000004);
	ret = writeRegisters(buf);
	ret = select_register_bank(buf, 0);
	return ret;
}

int setRegisters2xx(char *buf){
	int ret;
	ret = select_register_bank(buf, 2);
	memcpy(buf, 
		"\x60\x32\x61\x14\x62\x32\x63\x05\x64\x32\x65\x05\x66\x32\x67\x05"
		"\x68\x32\x69\x05\x6a\x32\x6b\x05\x6c\x32\x6d\x05\x6e\x32\x6f\x05"
		"\x70\x40\x71\x05\x72\x00\x73\x40\x74\x05\x75\x00\x76\x40\x77\x05"
		"\x78\x00\x79\x40\x7a\x05\x7b\x00\x7c\x80\x7d\x0a\x7e\x00\x7f\x80"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\x80\x0a\x81\x00\x82\x80\x83\x0a\x84\x00\x85\x80\x86\x0a\x87\x00"
		"\x88\xc0\x89\x0f\x8a\x00\x8b\xc0\x8c\x0f\x8d\x00\x8e\xc0\x8f\x0f"
		"\x90\x00\x91\xc0\x92\x0f\x93\x00\xb0\x00\xb1\x00\xb2\x00\xb3\x00"
		"\xb4\x00\xb5\x00\xb6\x00\xb7\x00\xb8\x00\xb9\x00\xba\x00\xbb\x00"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xbc\x00\xbd\x00\xbe\x00\xbf\x00\xc0\x00\xc1\x00\xc2\x00\xc3\x00"
		"\xc4\x00\xc5\x00\xc6\x00\xc7\x00\xc8\x00\xc9\x00\xca\x00\xcb\x00"
		"\xcc\x00\xcd\x00\xce\x00\xcf\x00"
	, 0x0000028);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000028, 1000);
	ret = select_register_bank(buf, 0);
	return ret;
}

int CCDSetup(char *buf){
	int ret;
	ret = select_register_bank(buf, 1);
	memcpy(buf, 
		"\x60\x08\x61\x0b\x62\x0e\x63\x11\x64\x14\x65\x17\x66\x02\x67\x05"
		"\x68\x08\x69\x50\x6a\x01\x6b\x0c\x6d\x48\x6e\x6a\x6f\x0e\x70\x50"
		"\x71\x7f\x72\x0e\x73\xe8\x74\x6c\x75\x0e\x76\xf0\x77\x81\x78\x0e"
		"\x79\x48\x7a\x2b\x7b\x0e\x7c\x50\x7d\x40\x7e\x0e\x7f\xe8\x80\x2d"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\x81\x0e\x82\xf0\x83\x42\x84\x0e\x85\x48\x86\xa9\x87\x0e\x88\x50"
		"\x89\xbe\x8a\x0e\x8b\xe8\x8c\xab\x8d\x0e\x8e\xf0\x8f\xc0\x90\x0e"
		"\x9a\x00\x9b\x15\x9d\x02\x9e\x00\x9f\x00\xa0\x0e\xa1\x00\xa2\x3f"
		"\xa3\x0e\xa4\x00\xa5\x7e\xa6\x0e\xa7\xff\xa8\x3e\xa9\x0e\xaa\xff"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xab\x7d\xac\x0e\xad\xff\xae\xbc\xaf\x0e\xb0\x4b\xb1\x01\xb9\x2b"
		"\xba\x05\xcd\x00\xce\x0f\xd4\x00\xd5\x8e\xd6\xe3\xd7\x38\xec\x00"
		"\xed\x00\xee\xc0\xef\x00\xf0\x00\xf1\x00\xf2\x00\xf3\x30\xf4\x1a"
		"\xf5\x00\xf6\x05\xf7\x05\xf8\xaa\xf9\x00\xfa\x55\xfb\x00\xfc\x3f"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, "\xfd\x00\xfd\x00", 0x0000004);
	ret = writeRegisters(buf);
	ret = select_register_bank(buf, 0);
	return ret;
}

int bunchOfConfig(char *buf){
	int ret;
	memcpy(buf, 
		"\x58\x00\x5e\xff\x82\x00\x83\x38\x84\x8e\x85\xe3\x8a\x00\x8e\x00"
		"\x90\x64\x91\x00\x94\x30\x95\xa7\x96\x21\x9b\x3d\x9d\x3f\x9e\x00"
		"\x9f\x20\xa0\x00\xa1\x00\xa2\x00\xa3\xff\xa4\xfe\xa5\x0d\xa6\x41"
		"\xab\x00\xae\x30\xaf\x00\xb0\x08\xb1\x07\xb2\x14\xb3\x01\xb4\x7b"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xb5\x00\xb6\x70\xb7\x05\xc9\x00\xca\x00\xcb\x00\xcc\x00\xcd\x3c"
		"\xce\x3c\xcf\x3c\xd0\x07\xd8\x05\xd9\x3c\xda\x54\xdb\x00\xdc\x01"
		"\xde\x01\xdf\x17\xe0\x2c\xe1\x01\xe2\x00\xe3\x00\xe4\x00\xe5\xc8"
		"\xe6\x80\xe7\x00\xe8\x80\xe9\x00\xea\x2c\xeb\x01\xec\x80\xed\x00"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xee\x5d\xef\x80\xf0\x69\xf1\x1c\xf2\x00\xf3\x0c\xf5\x10\xf6\x00"
		"\xf7\x05\xf8\x02\xf9\x82\xfa\x37\xfb\x3b\xfc\x00\xfd\xb5\xfe\x05"
		"\xff\x50"
	, 0x0000022);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000022, 1000);
	return ret;
}

int resetHostStartAddr(char *buf){
	memcpy(buf, "\xa0\x00\xa1\x00\xa2\x00", 0x0000006);
	return usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000006, 1000);
}

void showtime() {
	time_t t;
	struct tm *info;
	char cas[10];
	time(&t);
	info = localtime(&t);
	strftime(cas, 10, "%H:%M:%S", info);
	printf("time : %s\n", cas);
}

int check_if_ready(char *buf) {
	int ret;
	do {
		ret = read_register(buf, 0x05);
		ret = read_register(buf, 0x04);
		usleep(15*1000);
	} while(ret == 0x0);
	return ret;
}

int scancontrol(char *buf){
	int ret;
	memcpy(buf, "\xf3\x21\xf4\x00", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, "\xf3\xf3\xf3\xf3", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000005, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, "\xf3\xf3\xf3\xf3", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, SCANCONTROL, 0x0000000, buf, 0x0000004, 1000);
	usleep(12*1000);
	return ret;
}

int setup_bank1(char *buf){
	int ret;
	memcpy(buf, "\x82\x00\x83\x38\x84\x8e\x85\xe3", 0x0000008);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000008, 1000);
	ret = select_register_bank(buf, 1);
	memcpy(buf, 
		"\x60\x08\x61\x0b\x62\x0e\x63\x11\x64\x14\x65\x17\x66\x02\x67\x05"
		"\x68\x08\xd4\x00\xd5\x8e\xd6\xe3\xd7\x38\xec\x00\xed\x00\xee\xc0"
		"\xef\x00\xf0\x00\xf1\x00\xf2\x00\xf3\x30"
	, 0x000002a);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000002a, 1000);
	ret = select_register_bank(buf, 0);
	return ret;
}

int writeTable1000(char *buf){
	int ret;
	memcpy(buf, "\xa0\x00\xa1\xf4\xa2\x0f\xa3\xff\xa4\xff\xa5\x0f\x7c\x00\x7d\x10" "\x7e\x00\x7f\x00", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(27*1000);
	memcpy(buf, "\x00\x10\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAWRITE, 0x0000000, buf, 0x0000004, 1000);
	usleep(16*1000);
	memcpy(buf, table0x00001000, 0x0001000);
	ret = usb_bulk_write(devh, 0x00000001, buf, 0x0001000, 1245);
	usleep(16*1000);
	ret = clearFIFO(buf);
	usleep(31*1000);
	memcpy(buf, "\xa0\x00\xa1\x00\xa2\x00\xa3\xff\xa4\xff\xa5\x0f\x7c\x00\x7d\x01" "\x7e\x00\x79\x40", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(31*1000);
	memcpy(buf, "\x00\x02\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAREAD, 0x0000000, buf, 0x0000004, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	return ret;
}

int main(int argc, char **argv) {
    int ret, vendor, product;
    struct usb_device *dev;
    char buf[65535], *endptr;
    FILE * myfile;
	FILE * tmp;
	FILE * infile;
	unsigned char inbuf[3828];
	int scanloop, x, i, linenum;
	char yesorno;

	#if 0
		usb_urb *isourb;
		struct timeval isotv;
		char isobuf[32768];
	#endif

	if (argc!=2) {
		printf("usage: %s outputfile\n", argv[0]);
		exit(1);
	}
	badname:
	if (myfile = fopen(argv[1], "r")) {
		fclose(myfile);
		printf("File '%s' already exists!\n", argv[1]);
		printf("Enter a name for the new scan: ");
		scanf("%s", argv[1]);
		goto badname;
	}

//	showtime();
	vendor = 0x03f0;
	product = 0x3005;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	dev = find_device(vendor, product);
	if (dev == NULL) {
		printf("Could not find scanner\n");
		return -1;
	}
	devh = usb_open(dev);
	assert(devh);
	signal(SIGTERM, release_usb_device);
	ret = usb_get_driver_np(devh, 0, buf, sizeof(buf));
	if (ret == 0) {
		printf("interface 0 already claimed by driver \"%s\", attempting to detach it\n", buf);
		ret = usb_detach_kernel_driver_np(devh, 0);
		printf("usb_detach_kernel_driver_np returned %d\n", ret);
	}
	ret = usb_claim_interface(devh, 0);
	if (ret != 0) {
		printf("claim failed with error %d\n", ret);
		exit(1);
	}
	ret = usb_set_altinterface(devh, 0);
	assert(ret >= 0);
	ret = usb_get_descriptor(devh, 0x0000001, 0x0000000, buf, 0x0000012);
	ret = usb_get_descriptor(devh, 0x0000002, 0x0000000, buf, 0x0000009);
	ret = usb_get_descriptor(devh, 0x0000002, 0x0000000, buf, 0x0000027);
	ret = usb_release_interface(devh, 0);
	if (ret != 0) printf("failed to release interface before set_configuration: %d\n", ret);
	ret = usb_set_configuration(devh, 0x0000001);
	ret = usb_claim_interface(devh, 0);
	if (ret != 0) printf("claim after set_configuration failed with error %d\n", ret);
	ret = usb_set_altinterface(devh, 0);
	usleep(100*1000);

	ret = doCmdSequenceAlpha(buf);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x1c);
	ret = readScanState(buf);
	ret = read_register(buf, 0x03);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	ret = do_command_five(buf);

	ret = doCmdSequenceAlpha(buf);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x1c);
	ret = readScanState(buf);
	ret = read_register(buf, 0x03);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	ret = do_command_five(buf);
	usleep(40*1000);
	ret = usb_interrupt_read(devh, 0x00000083, buf, 0x0000001, 1000);
	usleep(200*1000);

	ret = doCmdSequenceAlpha(buf);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_eight(buf);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_five(buf);
	memcpy(buf, "\x02\xa0\x10\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	ret = do_command_six(buf);
	ret = do_command_one(buf);
	ret = setScannerIdle(buf);
	ret = readScanState(buf);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = doSDRAMSetup(buf);

	ret = scancontrol(buf);
	ret = read_register(buf, 0x00);
	ret = readChipString(buf);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);

	memcpy(buf, "\x00\x00\x86\x01", 0x0000004);
	ret = writeRegisters(buf);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	printf("Please wait, warming up the lamp ...\n");
	usleep(45000*1000);
	ret = do_command_five(buf);

	next:
	memcpy(buf, "\x02\xa0\x2b\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);

	ret = do_command_one(buf);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	ret = do_command_five(buf);
	usleep(300*1000);

	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = read_register(buf, 0x03);
	ret = do_command_eight(buf);
	ret = do_command_five(buf);
	ret = do_command_one(buf);
	ret = do_command_four(buf);
	ret = setScannerIdle(buf);
	ret = readScanState(buf);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = doSDRAMSetup(buf);

	ret = scancontrol(buf);
	ret = setup_bank1(buf);

	memcpy(buf,
		"\x74\x00\x78\x00\x82\x00\x83\x38\x84\x8e\x85\xe3\x86\x01\x88\x80"
		"\x89\x80\x90\x64\x8e\x00\x92\x57\x93\x0a\x95\x27\x96\x21\x97\xf0"
		"\x98\xbe\x99\x30\xb2\x14\xb3\x01\xb8\x00\xb9\x00\xba\x00\xbb\x00"
		"\xbc\x00\xbd\x00\xbe\x00\xbf\x00\xc0\x00\xc1\x00\xc2\x00\xc3\x00"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xc4\x00\xc5\x00\xc6\x00\xc7\x00\xc8\x00\xcc\x00\xd0\x07\xd1\x00"
		"\xd2\x00\xd3\x00\xd4\x00\xd5\x00\xd6\x00\xd7\x00\xd8\x05\xdb\x00"
		"\xdc\x01\xde\x01\xea\x2c\xeb\x01\xf8\x01"
	, 0x000002a);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000002a, 1000);

	ret = select_register_bank(buf, 1);
	memcpy(buf, 
		"\x60\x08\x61\x0b\x62\x0e\x63\x11\x64\x14\x65\x17\x66\x02\x67\x05"
		"\x68\x08\x6c\x1c\xd0\x00\xd1\x00\xd2\x00\xd3\x00\xd4\x00\xd5\x8e"
		"\xd6\xe3\xd7\x38\xd8\xff\xd9\xff\xda\xff\xdb\xff\xdc\xff\xdd\xff"
		"\xde\xff\xdf\xff\xe0\xff\xe1\xff\xe2\xff\xe3\xff\xe4\xff\xe5\xff"
	, 0x0000040);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000040, 1000);
	memcpy(buf, 
		"\xe6\xff\xe7\xff\xe8\xff\xe9\xff\xea\xff\xeb\xff\xec\x00\xed\x00" "\xee\xc0\xef\x00\xf0\x00\xf1\x00\xf2\x00\xf3\x30", 0x000001c);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000001c, 1000);
	ret = select_register_bank(buf, 0);

	ret = select_register_bank(buf, 2);
	memcpy(buf, "\xd0\x01\xd1\x09\xd2\x11\xd3\x19\xd4\x21\xd5\x29\xd6\x31\xd7\x39", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000010, 1000);
	ret = select_register_bank(buf, 0);

	memcpy(buf, "\xa0\x00\xa1\x08\xa2\x80\xa3\xff\xa4\xff\xa5\x0f\x7c\x00\x7d\x06" "\x7e\x00\x7f\x00", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(29*1000);
	memcpy(buf, "\x00\x06\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAWRITE, 0x0000000, buf, 0x0000004, 1000);
	usleep(16*1000);
	memcpy(buf, table0x00000600, 0x0000600);
	ret = usb_bulk_write(devh, 0x00000001, buf, 0x0000600, 1092);
	usleep(16*1000);
	ret = clearFIFO(buf);
	usleep(15*1000);
	memcpy(buf, "\xa0\x00\xa1\x08\xa2\x80\xa3\xff\xa4\xff\xa5\x0f\x7c\x00\x7d\x03" "\x7e\x00\x79\x40", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(32*1000);
	memcpy(buf, "\x00\x06\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAREAD, 0x0000000, buf, 0x0000004, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000600, 1092);
	usleep(15*1000);

	memcpy(buf, "\x9a\x00\x9a\x00", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, "\x00\x00\x02\x0f", 0x0000004);
	ret = writeRegisters(buf);
	usleep(16*1000);
	ret = read_register(buf, 0x00);
	ret = do_command_eight(buf);
	memcpy(buf, "\xf3\x32\xfd\x28\xfe\x23\xa6\x4d\xf6\x00\x96\x21\xe4\x01\xe2\x32" "\xe3\x00\xe4\x00\x95\x27\xf4\x01", 0x0000018);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000018, 1000);
	ret = readScanState(buf);
	usleep(60*1000);
	ret = readScanState(buf);
	ret = read_register(buf, 0x0b);
	ret = read_register(buf, 0x0a);
	ret = read_register(buf, 0x09);
	ret = do_command_eight(buf);
	usleep(14*1000);

	ret = setScannerIdle(buf);
	usleep(49*1000);
	ret = readScanState(buf);
	memcpy(buf, "\x96\x23\x96\x23", 0x0000004);
	ret = writeRegisters(buf);
	usleep(222*1000);

	ret = writeTable1000(buf);
	ret = setRegisters2ax(buf);
	ret = CCDSetup(buf);
	ret = setRegisters2xx(buf);
	ret = bunchOfConfig(buf);

	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	usleep(7*1000);
	ret = do_command_five(buf);

	ret = do_command_one(buf);
	ret = setup_bank1(buf);
	ret = readScanState(buf);
	ret = setScannerIdle(buf);
	usleep(16*1000);
	ret = readScanState(buf);

	memcpy(buf, "\x00\x01\x02\x0f\x04\x00\x06\x00\x08\x00\x0a\x00\x0c\xfa\x0e\xfa" "\x10\xfa\x12\xfa", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	ret = setScannerIdle(buf);
	usleep(30*1000);
	ret = readScanState(buf);
	ret = resetHostStartAddr(buf);
	usleep(31*1000);

	memcpy(buf, "\x9a\x02\x9a\x02", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, "\x00\x01\x02\x0f", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, "\x90\xa0\x91\xa0", 0x0000004);
	ret = writeRegisters(buf);
	memcpy(buf, "\x96\x23\x96\x23", 0x0000004);
	ret = writeRegisters(buf);
	usleep(222*1000);

	ret = read_register(buf, 0x00);
	ret = readScanState(buf);
	usleep(62*1000);
	ret = read_register(buf, 0x0b);
	ret = read_register(buf, 0x0a);
	ret = read_register(buf, 0x09);

	ret = do_command_eight(buf);
	usleep(64*1000);
	ret = read_register(buf, 0x00);
	ret = do_command_eight(buf);

	memcpy(buf, "\xf3\x32\xfd\x94\xfe\x11\xa6\x4d\xf6\x00\x96\x21\xe4\x01\xe2\x32" "\xe3\x00\xe4\x00\x95\xa7\xf4\x01", 0x0000018);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000018, 1000);

	ret = readScanState(buf);
	usleep(58*1000);
	ret = readScanState(buf);
	ret = read_register(buf, 0x0b);
	ret = read_register(buf, 0x0a);
	ret = read_register(buf, 0x09);
	ret = do_command_eight(buf);
	usleep(76*1000);

	memcpy(buf, "\xa0\x00\xa1\x00\xa2\x80\xa3\xff\xa4\xff\xa5\x0f\x7c\x00\x7d\x06" "\x7e\x00\x7f\x00", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(31*1000);
	memcpy(buf, "\x00\x06\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAWRITE, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, table0x00000600, 0x0000600);
	ret = usb_bulk_write(devh, 0x00000001, buf, 0x0000600, 1092);
	usleep(16*1000);
	ret = clearFIFO(buf);
	usleep(34*1000);

	memcpy(buf, "\xa0\x00\xa1\x40\xa2\x0f\xa3\xff\xa4\xff\xa5\x0f\x7c\x6c\x7d\x0c" "\x7e\x01\x7f\x00", 0x0000014);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000014, 1000);
	usleep(28*1000);
	memcpy(buf, "\x00\xf0\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAWRITE, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, table0x000f000, 0x000f000);
	ret = usb_bulk_write(devh, 0x00000001, buf, 0x000f000, 4686);
	usleep(57*1000);
	memcpy(buf, "\x6c\x1c\x00\x00", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAWRITE, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, table0x00001c6c, 0x0001c6c);
	ret = usb_bulk_write(devh, 0x00000001, buf, 0x0001c6c, 1436);
	usleep(16*1000);

	memcpy(buf, "\x6c\x6c\x6c\x6c", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x0000005, 0x0000000, buf, 0x0000004, 1000);
	memcpy(buf, "\x6c\x6c\x6c\x6c", 0x0000004);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, SCANCONTROL, 0x0000000, buf, 0x0000004, 1000);
	usleep(16*1000);
	ret = setScannerIdle(buf);
	usleep(31*1000);
	ret = readScanState(buf);

	ret = writeTable1000(buf);
	ret = setRegisters2ax(buf);
	ret = CCDSetup(buf);
	ret = setRegisters2xx(buf);

	ret = select_register_bank(buf, 1);
	memcpy(buf, "\xb3\x00\xb4\x00\xb5\x00\xb6\x00\xb7\x01\xb8\x00", 0x000000c);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000000c, 1000);
	ret = select_register_bank(buf, 0);

	ret = bunchOfConfig(buf);

	memcpy(buf, "\xf8\x02\xf4\x01", 0x0000004);
	ret = writeRegisters(buf);
	ret = check_if_ready(buf);

	tmp = fopen("tempscan.bmp", "w");
	if (tmp == NULL){ 
		printf("Error opening temp file!\n");
		exit(1);
	}
	memcpy(buf, tmp_file_header, 0x0000036);
	print_file(buf, 0x36, tmp);

	memcpy(buf, "\x7c\xc0\x7d\xcd\x7e\x02\x7f\x00", 0x0000008);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000008, 1000);
	for(scanloop = 0; scanloop < 19; scanloop++) {
		memcpy(buf, "\x80\x9b\x05\x00", 0x0000004);
		ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, DMAREAD, 0x0000000, buf, 0x0000004, 1000);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000fc00, 4870);
		print_file(buf, ret, tmp);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000fc00, 4870);
		print_file(buf, ret, tmp);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000fc00, 4870);
		print_file(buf, ret, tmp);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000fc00, 4870);
		print_file(buf, ret, tmp);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000fc00, 4870);
		print_file(buf, ret, tmp);
		ret = usb_bulk_read(devh, 0x00000082, buf, 0x000af80, 4870);
		print_file(buf, ret, tmp);
	}
	ret = setScannerIdle(buf);
	usleep(31*1000);
	ret = readScanState(buf);
	ret = do_command_six(buf);
	memcpy(buf, "\x01\xa0\x00\x04\x00\x00\x00\x1e\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	usleep(20*1000);
	ret = do_command_four(buf);
	memcpy(buf, "\x01\xa0\x05\x01\x81\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	usleep(17*1000);
	memcpy(buf, "\x02\xa0\x36\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	ret = usb_bulk_read(devh, 0x00000082, buf, 0x0000200, 1030);
	memcpy(buf, "\x01\xa0\x46\x02\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 0x0000010);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, 0x000000a, 0x0000000, buf, 0x0000010, 1000);
	usleep(19*1000);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	usleep(6*1000);

	ret = read_register(buf, 0x0b);
	ret = read_register(buf, 0x0a);
	ret = read_register(buf, 0x09);
	usleep(499*1000);
	ret = read_register(buf, 0x00);
	ret = do_command_eight(buf);
	memcpy(buf, "\xf3\x22\xfd\xc4\xfe\x09\xa6\x41\xf6\x00\x96\x21\xe4\x01\xe2\x40" "\xe3\x1f\xe4\x00\x95\xa7\xf4\x01", 0x0000018);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000018, 1000);
	memcpy(buf, "\x00\x00\x86\x01", 0x0000004);
	ret = writeRegisters(buf);

	fclose(tmp);
	//rotating the image fixes colours (no geometry fix needed)
	infile = fopen("tempscan.bmp", "r");
	myfile = fopen(argv[1], "w");
	if (myfile == NULL){ 
		printf("Error opening (%s) file! \n", argv[1]);
		exit(1);
	}
	memcpy(inbuf, bmp_file_header, 54);
	for (i=0; i < 54; i++){
		fprintf(myfile, "%c", inbuf[i]);
	}
	for(linenum = 1760; linenum > 0; linenum--){
		fseek(infile, (linenum*3828+54), SEEK_SET);
		fread(inbuf, 3828, 1, infile);
		for (i=3824; i > 0; i--) {
			fprintf(myfile, "%c", inbuf[i]);
		}
	}
	pclose(infile);
	fclose(myfile);
	remove("tempscan.bmp");

	usleep(7000*1000); //for the scan head to move to its home position !!! DO NOT REDUCE !!!
	printf("\nScan finished.\n");
	printf("Do you want to continue? (y/n) ");
	newinput:
	scanf("%c", &yesorno);
	if (yesorno == 121) { goto newname; }
	if (yesorno == 110) { goto quit; }
	goto newinput;
	newname:
	printf("Enter a name for the new scan: ");
	scanf("%s", argv[1]);
	if (myfile = fopen(argv[1], "r")) {
		fclose(myfile);
		printf("File '%s' already exists!\n", argv[1]);
		goto newname;
	}
	goto next;

	quit:
	ret = do_command_five(buf);
	usleep(300*1000);

	ret = do_command_one(buf);
	ret = read_register(buf, 0x1c);
	usleep(18*1000);
	ret = readScanState(buf);
	ret = read_register(buf, 0x03);
	memcpy(buf, "\x82\x00\x83\x00\x84\x00\x85\x00", 0x0000008);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x0000008, 1000);
	ret = select_register_bank(buf, 1);
	usleep(27*1000);
	memcpy(buf, 
		"\x60\xff\x61\xff\x62\xff\x63\xff\x64\xff\x65\xff\x66\xff\x67\xff"
		"\x68\xff\xd4\xff\xd5\xff\xd6\xff\xd7\xff\xec\x00\xed\x00\xee\x00"
		"\xef\x00\xf0\x00\xf1\x00\xf2\x00\xf3\x00"
	, 0x000002a);
	ret = usb_control_msg(devh, USB_TYPE_VENDOR + USB_RECIP_DEVICE, 0x0000004, REGISTERS, 0x0000000, buf, 0x000002a, 1000);
	ret = select_register_bank(buf, 0);
	ret = closescanchip(buf);
	ret = openscanchip(buf);
	ret = adjustPowerSaveRegisters(buf);
	ret = do_command_five(buf);

	ret = usb_release_interface(devh, 0);
	assert(ret == 0);
	ret = usb_close(devh);
	assert(ret == 0);
//	showtime();
	return 0;
}
