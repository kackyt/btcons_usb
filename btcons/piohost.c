#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include 	<sys\timeb.h>
#include 	<time.h>
#include	<conio.h>
#include	<fcntl.h>
#include	<io.h>
#include	<sys/stat.h>
#include	"type.h"
#include	"usb.h"

//#define AGB_DEBUG

#define	CMD_ROM_PROBE			0xffff0001	//
#define	CMD_WRAM_LOADEXEC		0xffff0002	//len
#define	CMD_ROM_BERASE			0xffff0003	//add
#define	CMD_ROM_WRITE			0xffff0004	//add,len
#define CMD_ROM_BWRITE			0xffff0005	//addw,lenw
#define CMD_READ				0xffff0006	//add,len
#define CMD_READ2				0xffff0012	//add,len (ULTRA)
#define CMD_WRITE				0xffff0007	//add,len
#define	CMD_FIND				0xffff0008	//add,len,strlen
#define CMD_BOOT_ROM			0xffff0009	//
#define	CMD_SRAM2EWRAM			0xffff000a	//bank    ERAM:0x02000000
#define	CMD_EWRAM2SRAM			0xffff000b	//bank    ERAM:0x02000000
#define	CMD_BU_PROBE			0xffff000c	//ret 0:none 1:SRAM256K 2:SRAM512K 3:FLASH512 4:EEP4K 5:EEP64K 6:FLASH1M 7:FLASH8M
#define CMD_SRAM_READ			0xffff000d	//ofs,len
#define CMD_EEP_READ			0xffff000e	//ofs,len
#define CMD_EEP_WRITE			0xffff000f	//ofs,len
#define CMD_SRAM_WRITE			0xffff0010	//ofs,len
#define CMD_FLASH_WRITE			0xffff0011	//ofs,len
#define CMD_BLANK				0xffff0020	//
#define CMD_IS_PRO				0xffff0021	//

#define	USB_CMD			0x00	//wlen,cmd....
#define	USB_READ		0x01	//wlen_l,wlen_h
#define	USB_WRITE		0x02	//wlen_l,wlen_h
#define	USB_STATUS		0x03	//return(b0:pow b1:booted)
#define USB_ACK			0x04	//wlen(1-16)

#define PC_RDONLY		0x0001	//読み込み専用
#define PC_WRONLY		0x0002	//書き込み専用
#define PC_RDWR			0x0003	//読み書き
#define PC_APPEND		0x0100	//追加
#define PC_CREAT		0x0200	//ファイルを作成する
#define PC_TRUNC		0x0400	//ファイルの長さを０にする

#define PC_SEEK_SET		0		//ファイルの先頭から
#define PC_SEEK_CUR		1		//現在のファイルポイントから
#define PC_SEEK_END		2		//ファイルの最後から

#define	MODE_HOST	0
#define	MODE_RF		1
#define	MODE_WF		2
#define	MODE_RS		3
#define	MODE_WS		4
#define	MODE_EF		5
#define	MODE_AF		6

#include "agb_prog.h"

void crc32_init();
u32 crc32_calc(u8 *buf,u32 len);

u32 buf[1024*1024/4];

u8  cmd[64];

enum {
	cmd_h=0,
	i_h,
	o_h,
};

PVOID device;
u32 is_pro;

void write_32bit(u32 m){
	u32 size;
	cmd[0]=USB_WRITE;
	cmd[1]=1;
	cmd[2]=0;
	usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
	usb_pipe_write(device, o_h,(PUCHAR)&m,4,&size,NULL);
}

u32 read_32bit(){
	u32 size,m;
	cmd[0]=USB_READ;
	cmd[1]=1;
	cmd[2]=0;
	usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
	usb_pipe_read(device, i_h, (PUCHAR)&m,4,&size,NULL);
	return(m);
}

void write_32bit_n(u32 *m,u32 n){
	u32 size;
	cmd[0]=USB_WRITE;
	cmd[1]=(u8)(n);
	cmd[2]= (u8)(n>>8);
	usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
	usb_pipe_write(device, o_h,(PUCHAR)m,4*n,&size,NULL);
}

void read_32bit_n(u32 *m,u32 n){
	u32 size;
	cmd[0]=USB_READ;
	cmd[1]=(u8)(n);
	cmd[2]=(u8)(n>>8);
	usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
	usb_pipe_read(device, i_h,(PUCHAR)m,4*n,&size,NULL);
}

void bt_cmd(u32 c,u32 p0,u32 p1,u32 p2,u32 *ret,u32 n){
	u8  cmd2[18];
	u32 size;
	cmd2[0]=USB_CMD;
	cmd2[1]=4;
	*((u32 *)(cmd2+2))=c;
	*((u32 *)(cmd2+6))=p0;
	*((u32 *)(cmd2+10))=p1;
	*((u32 *)(cmd2+14))=p2;
	usb_pipe_write(device, cmd_h, cmd2, 18, (LPDWORD)&size, NULL);
	if(n){
		cmd2[0]=USB_READ;
		cmd2[1]=(u8)n;
		cmd2[2]=(u8)(n>>8);
		usb_pipe_write(device, cmd_h, cmd2, 3, (LPDWORD)&size, NULL);
		usb_pipe_read(device, i_h,(PUCHAR)ret,4*n,(LPDWORD)&size,NULL);
	}
}

//MAGIC NO
//0:status 1-16:magic (1:gba off clear)
//status(bit0):GBA power
//status(bit1):ready to load PGM
//status(bit2):GBA sio32 transfer ready

u32 bt_getmagic(u8 n){
	u32 ret,size;
	cmd[0]=USB_STATUS;
	cmd[1]=n;
	usb_pipe_write(device, cmd_h, cmd, 2, (LPDWORD)&size, NULL);
	usb_pipe_read(device, i_h,(PUCHAR)&ret,4,(LPDWORD)&size,NULL);
	return(ret);
}

void bt_setmagic(u8 n,u32 m){
	u32 ret,size;
	cmd[0]=USB_STATUS;
	cmd[1]=0x80+n;
	cmd[2]=(u8)m;
	cmd[3]=(u8)(m >> 8);
	cmd[4]=(u8)(m >> 16);
	cmd[5]=(u8)(m >> 24);
	usb_pipe_write(device, cmd_h, cmd, 6, (LPDWORD)&size, NULL);
	usb_pipe_read(device, i_h, (PUCHAR)&ret,4,(LPDWORD)&size,NULL);
}

void dev_init(){
	s32 ret,size;
	static s32 usb_opened=0;

	if(usb_opened==0){
		usb_opened=1;
		BOOL ret;
		device = usb_open();
		if(!device) {
			printf("Error can't found BootCable USB !!.\n");
			exit(-1);
		}
		ret = usb_pipe_reset(device, cmd_h);
		if (!ret) {
			printf("Error can't found BootCable USB !!.\n");

		}
		usb_pipe_reset(device, o_h);
		usb_pipe_reset(device, i_h);
	}

	//get status
	if(bt_getmagic(1)!=0x12345678){
		bt_setmagic(2,0xfedcba98);
		for(;;){
			if(bt_getmagic(2)==0xfedcba98){
				break;
			}
			else{
				usb_pipe_read(device, i_h, (PUCHAR)&ret,4,(LPDWORD)&size,NULL);
			}
		}
		bt_setmagic(2,0);
		for(;;){
			ret=bt_getmagic(0);
			Sleep(200);
			if((ret & 1)==1) break;	//GBA ON (Relaxed check from &3==3 to &1==1)
			//if(ret==3) break;	//GBA ON
		}
	}
}

void host_proc(s32 mode){//mode 0:normal 1:disconサポート
	s32 fh;
	u32 i;
	u32 command,len,flag,where;
	s8 s[256];
	for(;;){
		for(;;){
			i=bt_getmagic(0);
			if(mode==1) Sleep(50);
			if((mode==0)&&((i & 1)==0)) return;		//GBA OFF or ケーブルが抜けた
			if((i & 0x04)==0) break;
			Sleep(100);
		}
		command =read_32bit();
		switch(command){
		case 0xf0f0f001://open
			len=read_32bit();
			read_32bit_n((void *)s,len);
			len=read_32bit();
			for(i=0;i<strlen(s);i++){
				if(s[i]=='/') s[i]='\\';
			}
			if((len & 3)==PC_RDONLY){
				flag=O_RDONLY;
			}
			else if((len & 3)==PC_WRONLY){
				flag=O_WRONLY;
			}
			else{
				flag=O_RDWR;
			}
			if(len & PC_APPEND){
				flag |= O_APPEND;
			}
			if(len & PC_CREAT){
				flag |= O_CREAT;
			}
			if(len & PC_TRUNC){
				flag |= O_TRUNC;
			}
			fh=(u32)open(s,O_BINARY | flag,_S_IREAD | _S_IWRITE);
			write_32bit((u32)fh);
			break;
		case 0xf0f0f002://close
			fh=read_32bit();
			close(fh);
			break;
	    case 0xf0f0f003://read(pre)
	    	fh=read_32bit();
	    	len=read_32bit();
			len=read(fh,buf,len);
			write_32bit(len);
			write_32bit_n(buf,(len+3)/4);
			break;
		case 0xf0f0f004://write
			fh=read_32bit();
			len=read_32bit();
			read_32bit_n(buf,(len+3)/4);
			write(fh,buf,len);
			break;
		case 0xf0f0f005://lseek
			fh=read_32bit();
			len=read_32bit();
			where=read_32bit();
			len=lseek(fh,len,where);
			write_32bit(len);
			break;
		case 0xf0f0f000://PcPrintf()
			len=read_32bit();
			read_32bit_n((void *)s,len);
			printf("%s",s);
			break;
		case 0x5555aaaa:
			command=0;
			break;
		default:
			command=0;
			break;
		}
	}
}

int	piohost_main( int argc, char *argv[] ){
	FILE *fp = NULL;
	s32 i,j;
	s32 mode;
	s32 len,dev_ofs,dev_len;
	s8	*endptr;
	s32 rom_type,rom_size,rom_blksize;
	s32 bu_type,bu_size,bu_blksize;
	s32 flen;
	u32 ulen;
	u8  cmd[64];
	u32 size,discon_mode;

	printf("*********** BOOT CABLE USB CONSOLE TOOL Ver 3.541 ************\n");
	if(argc<2) goto USAGE;
	for(i=0;;i++){
		if(argv[1][i]==0) break;
		argv[1][i]=toupper(argv[1][i]);
	}
	if(strcmp(argv[1],"FR")==0){//フラッシュ読み込み
		mode=MODE_RF;
	}
	else if(strcmp(argv[1],"FW")==0){//フラッシュ書き込み
		mode=MODE_WF;
	}
	else if(strcmp(argv[1],"FE")==0){//フラッシュ消去
		mode=MODE_EF;
	}
	else if(strcmp(argv[1],"FA")==0){//フラッシュ追記書き込み
		mode=MODE_AF;
	}
	else if(strcmp(argv[1],"SR")==0){//SRAM読み込み
		mode=MODE_RS;
	}
	else if(strcmp(argv[1],"SW")==0){//SRAM書き込み
		mode=MODE_WS;
	}
	else if((argc==2)||(argc==3)){
		mode=0;
		if((fp=fopen(argv[1],"rb"))==NULL){
			printf("Can't open %s file.\n",argv[1]);
			goto USAGE;
		}
		len=fread(buf,1,1024*256,fp);
		fclose(fp);
		if((argc==3)&&((strcmp(argv[2],"d")==0)||(strcmp(argv[2],"D")==0))){
			discon_mode=1;
		}
		else{
			discon_mode=0;
		}
	}
	else{
		goto USAGE;
	}
	if(mode!=MODE_HOST){
		if(argc<2) goto USAGE;

#ifdef AGB_DEBUG
	if((fp=fopen("c:\\agb\\devkitadv\\wramloader\\fwlib.bin","rb"))==NULL){
		printf("Can't open fwlib.bin file.\n");
		exit(-1);
	}
	len=fread(buf,1,1024*32,fp);
	fclose(fp);
	if((fp=fopen("c:\\agb\\devkitadv\\fwbulib_hs\\fwlib.bin","rb"))==NULL){
		printf("Can't open fwlib.bin file.\n");
		exit(-1);
	}
	len=1024*32+fread((u8 *)buf+1024*32,1,1024*32,fp);
	fclose(fp);
#else
	len=sizeof(agb_prog);
	memcpy(buf,agb_prog,len);
#endif
		if((mode==MODE_RF)||(mode==MODE_RS)){
			if((fp=fopen(argv[2],"wb"))==NULL){
				printf("Can't open %s file.\n",argv[2]);
				exit(-1);
			}
		}
		else if(mode!=MODE_EF){
			if((fp=fopen(argv[2],"rb"))==NULL){
				printf("Can't open %s file.\n",argv[2]);
				exit(-1);
			}
			fseek(fp,0,SEEK_END);
			flen=ftell(fp);
			fseek(fp,0,SEEK_SET);
		}
		dev_len=dev_ofs=-1;
		if(argc>4){
			if(memcmp(argv[3],"0x",2)==0){
				dev_ofs=strtoul(argv[3],&endptr,16);
			}
			else{
				dev_ofs=strtoul(argv[3],&endptr,10);
			}
			if(memcmp(argv[4],"0x",2)==0){
				dev_len=strtoul(argv[4],&endptr,16);
			}
			else{
				dev_len=strtoul(argv[4],&endptr,10);
			}
			if(toupper(*endptr)=='K'){
				dev_len*=1024;
			}
			else if(toupper(*endptr)=='M'){
				dev_len*=1024*1024;
			}
		}
	}
	dev_init();
	if(bt_getmagic(1)!=0x12345678){
		//PGMの長さを渡す
		cmd[0]=USB_WRITE;
		cmd[1]=1;
		cmd[2]=0;
		usb_pipe_write(device, cmd_h, cmd, 3, (LPDWORD)&size, NULL);
		Sleep(100);
		printf("Sending PGM size (len=%d)...\n", len);
		if(!usb_pipe_write(device, o_h, (LPCVOID)&len, 4, (LPDWORD)&size, NULL)) {
			printf("Failed to send PGM size.\n");
		}
		Sleep(100);
		//PGM本体を渡す
		u32 word_count = (len+3)/4;
		if (word_count > 0xFFFF) {
			printf("Warning: PGM size %d exceeds protocol limit. Caps to 65535 words (262140 bytes).\n", len);
			word_count = 0xFFFF;
			len = word_count * 4; // Adjust len to match the capped word count
		}
		cmd[0]=USB_WRITE;
		cmd[1]=(u8)word_count;
		cmd[2]=(u8)(word_count>>8);
		usb_pipe_write(device, cmd_h, cmd, 3, (LPDWORD)&size, NULL);
		Sleep(200);
		printf("Sending PGM body...\n");
		u32 total_sent = 0;
		u32 pgm_size = (len+3)/4*4;
		u8 *p_buf = (u8*)buf;
		while (total_sent < pgm_size) {
			u32 chunk = 4096;
			if (total_sent + chunk > pgm_size) chunk = pgm_size - total_sent;
			
			if(!usb_pipe_write(device, o_h, p_buf + total_sent, chunk, (LPDWORD)&size, NULL)) {
				printf("\nFailed to send PGM body at offset %d. size=%d\n", total_sent, chunk);
				break;
			}
			total_sent += size;
			if (total_sent % (4096 * 4) == 0) printf("."); // Print dot every 16KB
		}
		printf("\nPGM body sent: %d / %d bytes.\n", total_sent, pgm_size);
		Sleep(200);					//wait GBA is up

		if(mode!=MODE_HOST){
			bt_cmd(CMD_BU_PROBE,0,0,0,(u32 *)&bu_type,1);
			bt_cmd(CMD_ROM_PROBE,0,0,0,buf,3);
			rom_type=buf[0];
			rom_size=buf[1];
			rom_blksize=buf[2];
			bt_setmagic(3,bu_type);
			bt_setmagic(4,rom_type);
			bt_setmagic(5,rom_size);
			bt_setmagic(6,rom_blksize);
		}
	}
	else{
		bu_type=bt_getmagic(3);
		rom_type=bt_getmagic(4);
		rom_size=bt_getmagic(5);
		rom_blksize=bt_getmagic(6);
	}

	if(mode!=MODE_HOST){
		bt_cmd(CMD_IS_PRO,0,0,0,&is_pro,1);
		bt_setmagic(1,0x12345678);

		if(rom_type==1){
			printf("MASK ROM SIZE=0x%X\n",rom_size);
		}
		else if(rom_type!=0){
			printf("FLASH ROM SIZE=0x%X\n",rom_size);
		}

		bu_size=0;
		bu_blksize=0x1000;
		switch(bu_type){
		case 1:
			bu_size=0x8000;
			bu_blksize=0x1000;
			printf("SRAM256K");
			break;
		case 2:
			if(is_pro==2){//ultra
				bu_size=0x40000*4;
				bu_blksize=0x1000;
				printf("SRAM8M");
			}
			else{
				bu_size=0x40000;
				bu_blksize=0x1000;
				printf("SRAM2M");
			}
			break;
		case 3:
			bu_size=0x10000;
			bu_blksize=0x1000;
			printf("FLASH512K");
			break;
		case 4:
			bu_size=0x200;
			bu_blksize=0x200;
			printf("EEP4K");
			break;
		case 5:
			bu_size=0x2000;
			bu_blksize=0x2000;
			printf("EEP64K");
			break;
		case 6:
			bu_size=0x20000;
			bu_blksize=0x1000;
			printf("FLASH1M");
			break;
		case 7:
			bu_size=0x100000-0x2000;
			bu_blksize=0x2000;
			printf("FLASH8M");
			break;
		}
		if(bu_type){
			printf(" SIZE=0x%X\n",bu_size);
		}
	}
	switch(mode){
	case MODE_HOST:
		host_proc(discon_mode);
		break;
	case MODE_RF:
		if(dev_len==-1) dev_len=rom_size;
		if(dev_ofs==-1) dev_ofs=0;
		printf("0x%X bytes reading.\n",dev_len);
		bt_cmd(CMD_READ2,0x08000000+dev_ofs,dev_len,0,NULL,0);
		for(;;){
			if(dev_len>=0x1000){
				j=0x1000;
			}
			else{
				j=dev_len;
			}
			cmd[0]=USB_READ;
			cmd[1]=(j+3)/4;
			cmd[2]=((j+3)/4)>>8;
			*((u16 *)(cmd+3))=7;
			usb_pipe_write(device, cmd_h, cmd, 64, &size, NULL);
			usb_pipe_read(device, i_h,buf,((j+3)/4)*4,&size,NULL);
			fwrite(buf,1,j,fp);
			printf("R");
			dev_len-=j;
			if(dev_len==0) break;
		}
		break;
	case MODE_WF:
		if(dev_len==-1) dev_len=flen;
		if(dev_ofs==-1) dev_ofs=0;
		if(rom_size<dev_len){
			printf("Write size(0x%X) is too big.\n",dev_len);
			dev_len=rom_size;
		}
		if(dev_len>flen){
			printf("Write size is larger than file size.\n");
			dev_len=flen;
		}
		printf("0x%X bytes writeing.\n",dev_len);
		bt_cmd(CMD_ROM_BWRITE,dev_ofs,dev_len,0,NULL,0);
		u32 total_len = dev_len;
		u32 current_written = 0;
		for(;;){
			if(dev_len>=0x4000){
				j=0x4000;
			}
			else{
				j=dev_len;
			}
			fread(buf,1,j,fp);
			cmd[0]=USB_WRITE;
			cmd[1]=(j+3)/4;
			cmd[2]=((j+3)/4)>>8;
			usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
			usb_pipe_write(device, o_h,buf,((j+3)/4)*4,&size,NULL);
			
			current_written += j;
			printf("\rWriting... %d / %d bytes (%.1f%%)", current_written, total_len, (float)current_written / total_len * 100.0f);
			
			dev_len-=j;
			if(dev_len==0) break;
		}
		printf("\n");
		break;
	case MODE_AF:
		bt_cmd(CMD_BLANK,0,0,0,&ulen,1);
		if(dev_len==-1) dev_len=flen;
		dev_ofs=rom_size-(ulen & 0xffff8000);
		if(dev_ofs!=0){
			if(is_pro==0){//PROでない
				if((dev_ofs & 0xffc08000)!=(u32)dev_ofs){
					dev_ofs=(dev_ofs+0x3fffff) & 0xffc00000;
				}
				dev_ofs |= 0x8000;
			}
		}
		if((rom_size-dev_ofs)<dev_len){
			printf("Write size(0x%X) is too big.\n",dev_len);
			dev_len=rom_size-dev_ofs;
		}
		printf("Start=0x%X  0x%X bytes writeing.\n",dev_ofs,dev_len);
		if(dev_len<=0) break;
		bt_cmd(CMD_ROM_BWRITE,dev_ofs,dev_len,0,NULL,0);
		for(;;){
			if(dev_len>=0x4000){
				j=0x4000;
			}
			else{
				j=dev_len;
			}
			fread(buf,1,j,fp);
			cmd[0]=USB_WRITE;
			cmd[1]=(j+3)/4;
			cmd[2]=((j+3)/4)>>8;
			usb_pipe_write(device, cmd_h, cmd, 3, &size, NULL);
			usb_pipe_write(device, o_h,buf,((j+3)/4)*4,&size,NULL);
			printf("W");
			dev_len-=j;
			if(dev_len==0) break;
		}
		break;
	case MODE_EF:
		printf("Erase START.\n");
		for(i=0;i<rom_size;i+=rom_blksize){
			bt_cmd(CMD_ROM_BERASE,i,0,0,buf,1);
			printf("E");
		}
		printf("\nErase END.\n");
		exit(0);
		break;
	case MODE_RS:
		if(dev_len==-1) dev_len=bu_size;
		if(dev_ofs==-1) dev_ofs=0;
/*
		if((dev_len & (bu_blksize-1))||(dev_ofs & (bu_blksize-1))){
			printf("SAVE read size and offset must be 0x%X boundary.\n",bu_blksize);
			exit(-1);
		}
*/
		printf("0x%X bytes reading.\n",dev_len);
		for(i=0;i<dev_len;){
			if((bu_type==4)||(bu_type==5)){//EEP
				bt_cmd(CMD_EEP_READ,dev_ofs+i,bu_blksize,0,NULL,0);
			}
			else if(bu_type==7){
				bt_cmd(CMD_READ,0x09f00000+dev_ofs+i,bu_blksize,0,NULL,0);
			}
			else{
				bt_cmd(CMD_SRAM_READ,dev_ofs+i,bu_blksize,0,NULL,0);
			}
			printf("R");
			cmd[0]=USB_READ;
			cmd[1]=(u8)(bu_blksize/4);
			cmd[2]=(bu_blksize/4)>>8;
			usb_pipe_write(device, cmd_h, cmd, 3, (LPDWORD)&size, NULL);
			usb_pipe_read(device, i_h,buf,bu_blksize,(LPDWORD)&size,NULL);
			fwrite(buf,1,bu_blksize,fp);
			i+=bu_blksize;
		}
		break;
	case MODE_WS:
		if(dev_ofs==-1) dev_ofs=0;
		if(dev_len==-1) dev_len=flen;
		if(dev_len>(bu_size-dev_ofs)){
			printf("Write size is longer than device.\n");
			dev_len=bu_size-dev_ofs;
		}
		if((dev_len & (bu_blksize-1))||(dev_ofs & (bu_blksize-1))){
			printf("SAVE read size and offset must be 0x%X boundary.\n",bu_blksize);
			exit(-1);
		}
		printf("0x%X bytes writeing.\n",dev_len);
		for(i=0;i<dev_len;){
			fread(buf,1,bu_blksize,fp);
			if((bu_type==4)||(bu_type==5)){//EEP
				bt_cmd(CMD_EEP_WRITE,dev_ofs+i,bu_blksize,0,NULL,0);
			}
			else if((bu_type==1)||(bu_type==2)){//SRAM
				bt_cmd(CMD_SRAM_WRITE,dev_ofs+i,bu_blksize,0,NULL,0);
			}
			else if((bu_type==3)||(bu_type==6)||(bu_type==7)){//FLASH
				bt_cmd(CMD_FLASH_WRITE,dev_ofs+i,bu_blksize,0,NULL,0);
			}
			printf("W");
			cmd[0]=USB_WRITE;
			cmd[1]=(u8)(bu_blksize/4);
			cmd[2]=(bu_blksize/4)>>8;
			usb_pipe_write(device, cmd_h, cmd, 3, (LPDWORD)&size, NULL);
			usb_pipe_write(device, o_h,buf,bu_blksize,(LPDWORD)&size,NULL);
			read_32bit();
			i+=bu_blksize;
		}
		break;
	}
	if(mode!=MODE_HOST){
		fclose(fp);
	}
	exit(0);

USAGE:
	printf("FLASH READ ------- btcons fr file_name [start_offset len(?? ??k ??m)]\n");
	printf("FLASH WRITE ------ btcons fw file_name [start_offset len(?? ??k ??m)]\n");
	printf("FLASH APEND ------ btcons fa file_name\n");
	printf("FLASH ALL ERASE -- btcons fe\n");
	printf("SAVE READ -------- btcons sr file_name [start_offset len(?? ??k ??m)]\n");
	printf("SAVE WRITE ------- btcons sw file_name [start_offset len(?? ??k ??m)]\n");
	printf("PGM LOAD & GO ---- btcons pgm_file [d]\n");
	exit(-1);
}
