


PVOID  usb_open();
void usb_close(PVOID h);

BOOL usb_pipe_reset(PVOID h, UCHAR pipeid);
BOOL usb_pipe_read(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped);
BOOL usb_pipe_write(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength, PULONG transferred, LPOVERLAPPED overlapped);