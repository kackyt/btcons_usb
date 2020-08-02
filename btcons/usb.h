


PVOID  usb_open();
void usb_close(PVOID h);

void usb_pipe_reset(PVOID h, UCHAR pipeid);
void usb_pipe_read(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength);
void usb_pipe_write(PVOID h, UCHAR pipeid, PUCHAR buffer, ULONG bufferLength);