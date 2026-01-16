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

struct PIPE_ID
{
    UCHAR  PipeInId;
    UCHAR  PipeOutId;
};

BOOL QueryDeviceEndpoints(PDEVICE_DATA device)
{
    if (device->WinusbHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    BOOL bResult = TRUE;

    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

    WINUSB_PIPE_INFORMATION  Pipe;
    ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));


    bResult = WinUsb_QueryInterfaceSettings(device->WinusbHandle, 0, &InterfaceDescriptor);

    if (bResult)
    {
        for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
        {
            bResult = WinUsb_QueryPipe(device->WinusbHandle, 0, index, &Pipe);

            if (bResult)
            {
                if (Pipe.PipeType == UsbdPipeTypeControl)
                {
                    printf("Endpoint index: %d Pipe type: Control Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeIsochronous)
                {
                    printf("Endpoint index: %d Pipe type: Isochronous Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeBulk)
                {
                    printf("Endpoint index: %d Pipe type: Bulk Pipe ID: %x.\n", index, Pipe.PipeType, Pipe.PipeId);
                    device->PipeIds[index] = Pipe.PipeId;
                }
                if (Pipe.PipeType == UsbdPipeTypeInterrupt)
                {
                    printf("Endpoint index: %d Pipe type: Interrupt Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
            }
            else
            {
                continue;
            }
        }
    }

done:
    return bResult;
}


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

        PIPE_ID pipeid;

        QueryDeviceEndpoints(device);
		return device;
	}

	void usb_close(PVOID h) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;
		CloseDevice(device);
		delete device;
	}

	BOOL usb_pipe_reset(PVOID h, UCHAR pipeid) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_ResetPipe(device->WinusbHandle, device->PipeIds[pipeid]);
	}

	BOOL usb_pipe_read(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_ReadPipe(device->WinusbHandle, device->PipeIds[pipeid], buffer, bufferLength, transferred, overlapped);
	}

	BOOL usb_pipe_write(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;

		return WinUsb_WritePipe(device->WinusbHandle, device->PipeIds[pipeid], buffer, bufferLength, transferred, overlapped);
	}
}

