#include	<windows.h>
#include	<tchar.h>
#include	<setupapi.h>
#include	<string.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	"type.h"

#define _DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name \
                    = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

_DEFINE_GUID(GUID_CLASS_BTUSB, 
0x28b0d529, 0xc559, 0x4697, 0xbc, 0x5e, 0x7e, 0x18, 0xa0, 0xdc, 0x62, 0x9b);

PSP_INTERFACE_DEVICE_DETAIL_DATA GetDeviceDeatailData(LPGUID pGuid, int index){
	HDEVINFO                 hardwareDeviceInfo;
	SP_INTERFACE_DEVICE_DATA deviceInfoData;
	ULONG requiredLength, predictedLength;
	DWORD err;
    PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
	
	functionClassDeviceData = NULL;
	hardwareDeviceInfo = SetupDiGetClassDevs (
		pGuid,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | // Only Devices present
		DIGCF_INTERFACEDEVICE)); // Function class devices.
	
	if(!hardwareDeviceInfo) goto end;
	
	deviceInfoData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
	
	if (!SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
		0, // We don't care about specific PDOs
		pGuid,
		index,
		&deviceInfoData)) {
		goto end;
	}
	
    SetupDiGetDeviceInterfaceDetail (
		hardwareDeviceInfo,
		&deviceInfoData,
		NULL, // probing so no output buffer yet
		0, // probing so output buffer length of zero
		&requiredLength,
		NULL); // not interested in the specific dev-node
	
	err = GetLastError();
	if( err != ERROR_INSUFFICIENT_BUFFER) goto end;
	
    predictedLength = requiredLength;
    functionClassDeviceData = (struct _SP_DEVICE_INTERFACE_DETAIL_DATA_A *)malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
    if (! SetupDiGetInterfaceDeviceDetail (
		hardwareDeviceInfo,
		&deviceInfoData,
		functionClassDeviceData,
		predictedLength,
		&requiredLength,
		NULL)) {
		free(functionClassDeviceData);
		functionClassDeviceData = NULL;
    }
end:
	SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
	return functionClassDeviceData;
}

TCHAR  *GetDeviceName(LPGUID pGuid, int index){
	PSP_INTERFACE_DEVICE_DETAIL_DATA DevDetailData;
	TCHAR *name;

	DevDetailData = GetDeviceDeatailData(pGuid, index);
	
	if(DevDetailData == NULL) return NULL;
	
	name = (char *)malloc(_tcslen(DevDetailData->DevicePath)+1);
	
	_tcscpy(name, DevDetailData->DevicePath);
	
	free(DevDetailData);
	return name;
}

HANDLE usb_h;
TCHAR *usb_name;

s32 usb_open(){
	LPGUID pGuid = (LPGUID)&GUID_CLASS_BTUSB;

	usb_name = GetDeviceName(pGuid, 0);
	if(usb_name == NULL) return(1);
	usb_h = CreateFile(
		usb_name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL); // No template file
	if(usb_h==INVALID_HANDLE_VALUE){
		return(1);
	}
	return(0);
	
}

void usb_close(){
	CloseHandle(usb_h);
}

HANDLE usb_pipe_open(s32 n){
	TCHAR name[256];
	HANDLE h;

	_stprintf(name,"%s\\if%03dpipe%03d", usb_name, 0, n);
	h = CreateFile(
		name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0,
		NULL); // No template file
	return(h);
}

void usb_pipe_close(HANDLE h){
	CloseHandle(h);
}

void usb_pipe_reset(HANDLE h){
	DWORD size;
	BOOL err;
	err = DeviceIoControl(h,
			0x00220008/*IOCTL_BULKUSB_RESET_PIPE*/,
			NULL,
			0,
			NULL,
			0,
			&size,
			NULL);
}
