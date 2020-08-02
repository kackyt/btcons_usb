#include	<windows.h>
#include    <winusb.h>
#include	<tchar.h>
#include	<setupapi.h>
#include	<string.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	"type.h"
#include    "device.h"


HANDLE usb_h;
TCHAR *usb_name;

extern "C" {
	PVOID usb_open() {
		PDEVICE_DATA device = new DEVICE_DATA;
		HRESULT               hr;
		BOOL                  noDevice;

		//
		// Find a device connected to the system that has WinUSB installed using our
		// INF
		//
		hr = OpenDevice(device, &noDevice);

		if (FAILED(hr)) {
			delete device;
			return nullptr;
		}
		return device;
	}

	void usb_close(PVOID h) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;
		CloseDevice(device);
		delete device;
	}

	BOOL usb_pipe_reset(PVOID h, UCHAR pipeid) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_ResetPipe(device->WinusbHandle, pipeid);
	}

	BOOL usb_pipe_read(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_ReadPipe(device->WinusbHandle, pipeid, buffer, bufferLength, transferred, overlapped);
	}

	BOOL usb_pipe_write(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_WritePipe(device->WinusbHandle, pipeid, buffer, bufferLength, transferred, overlapped);
	}
}

