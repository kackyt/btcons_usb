s32  usb_open();
void usb_close();
HANDLE usb_pipe_open(s32 n);
void usb_pipe_close(HANDLE h);
void usb_pipe_reset(HANDLE h);
