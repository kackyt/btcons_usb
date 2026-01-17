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


BOOL QueryDeviceEndpoints(PDEVICE_DATA device)
{
    int outPipeCount = 0;
    // Initialize PipeIds mappings to invalid
    for(int i=0; i<32; i++) device->PipeIds[i] = 0xFF;

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
                    ULONG timeout = 5000;
                    WinUsb_SetPipePolicy(device->WinusbHandle, Pipe.PipeId, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout);

                    BOOL bEnable = TRUE;
                    WinUsb_SetPipePolicy(device->WinusbHandle, Pipe.PipeId, AUTO_CLEAR_STALL, sizeof(bEnable), &bEnable);

                    printf("Endpoint index: %d Pipe type: Bulk Pipe ID: 0x%X (Timeout set, AutoClearStall).\n", index, Pipe.PipeId);

                    if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId)) {
                        // IN Endpoint -> Map to i_h (1)
                        printf(" -> Mapped to i_h (1)\n");
                        device->PipeIds[1] = Pipe.PipeId;
                    } else {
                        // OUT Endpoint
                        if (outPipeCount == 0) {
                             // First OUT -> Map to cmd_h (0)
                             printf(" -> Mapped to cmd_h (0)\n");
                             device->PipeIds[0] = Pipe.PipeId;
                             // Fallback: also map to o_h (2) in case there is only one OUT pipe
                             device->PipeIds[2] = Pipe.PipeId;
                        } else {
                             // Second OUT -> Map to o_h (2)
                             printf(" -> Mapped to o_h (2)\n");
                             device->PipeIds[2] = Pipe.PipeId;
                        }
                        outPipeCount++;
                    }
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
        BOOL ret = WinUsb_ReadPipe(device->WinusbHandle, device->PipeIds[pipeid], buffer, bufferLength, transferred, overlapped);
        // printf("[USB READ] Pipe: %d, ReqLen: %lu, Ret: %d, Trans: %lu\n", pipeid, bufferLength, ret, transferred ? *transferred : 0);
        if (!ret) {
            printf("[USB READ] Pipe: %d Failed. Error: %lu\n", pipeid, GetLastError());
        }
        if (ret && transferred && *transferred > 0) {
            // printf("  Data: ");
            // for (ULONG i = 0; i < *transferred && i < 16; i++) {
            //     printf("%02X ", buffer[i]);
            // }
            // printf("\n");
        }
        return ret;
	}

	BOOL usb_pipe_write(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped) {
		PDEVICE_DATA device = (PDEVICE_DATA)h;
        BOOL ret = WinUsb_WritePipe(device->WinusbHandle, device->PipeIds[pipeid], buffer, bufferLength, transferred, overlapped);
        // printf("[USB WRITE] Pipe: %d, ReqLen: %lu, Ret: %d, Trans: %lu\n", pipeid, bufferLength, ret, transferred ? *transferred : 0);
        if (!ret) {
            printf("[USB WRITE] Pipe: %d Failed. Error: %lu\n", pipeid, GetLastError());
        }
        return ret;
	}
}

