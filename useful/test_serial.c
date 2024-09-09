#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "stdio.h"



 
//#define BI_BITFIELDS 0x3
 
// typedef char BYTE;
// typedef short WORD;
// typedef int DWORD;
// typedef int LONG;
 #pragma pack(2)

typedef struct tagBITMAPFILEHEADER3{
	WORD bfType;//位图文件的类型，在Windows中，此字段的值总为‘BM’(1-2字节）
	DWORD bfSize;//位图文件的大小，以字节为单位（3-6字节，低位在前）
	WORD bfReserved1;//位图文件保留字，必须为0(7-8字节）
	WORD bfReserved2;//位图文件保留字，必须为0(9-10字节）
	DWORD bfOffBits;//位图数据的起始位置，以相对于位图（11-14字节，低位在前）
	//文件头的偏移量表示，以字节为单位
} BitMapFileHeader;	//BITMAPFILEHEADER;
 #pragma pack()

typedef struct tagBITMAPINFOHEADER3{
	DWORD biSize;//本结构所占用字节数（15-18字节）
	LONG biWidth;//位图的宽度，以像素为单位（19-22字节）
	LONG biHeight;//位图的高度，以像素为单位（23-26字节）
	WORD biPlanes;//目标设备的级别，必须为1(27-28字节）
	WORD biBitCount;//每个像素所需的位数，必须是1（双色），（29-30字节）
	//4(16色），8(256色）16(高彩色)或24（真彩色）之一
	DWORD biCompression;//位图压缩类型，必须是0（不压缩），（31-34字节）
	//1(BI_RLE8压缩类型）或2(BI_RLE4压缩类型）之一
	DWORD biSizeImage;//位图的大小(其中包含了为了补齐行数是4的倍数而添加的空字节)，以字节为单位（35-38字节）
	LONG biXPelsPerMeter;//位图水平分辨率，像素数（39-42字节）
	LONG biYPelsPerMeter;//位图垂直分辨率，像素数（43-46字节)
	DWORD biClrUsed;//位图实际使用的颜色表中的颜色数（47-50字节）
	DWORD biClrImportant;//位图显示过程中重要的颜色数（51-54字节）
} BitMapInfoHeader;	//BITMAPINFOHEADER;
 
 
typedef struct tagRGBQUAD2{
	BYTE rgbBlue;//蓝色的亮度（值范围为0-255)
	BYTE rgbGreen;//绿色的亮度（值范围为0-255)
	BYTE rgbRed;//红色的亮度（值范围为0-255)
	BYTE rgbReserved;//保留，必须为0
} RgbQuad2;	//RGBQUAD;
  
int Rgb565ConvertBmp(char *buf,int width,int height, const char *filename)
{
	FILE* fp;
 
	BitMapFileHeader bmfHdr; //定义文件头
	BitMapInfoHeader bmiHdr; //定义信息头
	RgbQuad2 bmiClr[3]; //定义调色板
 
	bmiHdr.biSize = sizeof(BitMapInfoHeader);
	bmiHdr.biWidth = width;//指定图像的宽度，单位是像素
	bmiHdr.biHeight = height;//指定图像的高度，单位是像素
	bmiHdr.biPlanes = 1;//目标设备的级别，必须是1
	bmiHdr.biBitCount = 16;//表示用到颜色时用到的位数 16位表示高彩色图
	bmiHdr.biCompression = BI_BITFIELDS;//BI_RGB仅有RGB555格式
	bmiHdr.biSizeImage = (width * height * 2);//指定实际位图所占字节数
	bmiHdr.biXPelsPerMeter = 0;//水平分辨率，单位长度内的像素数
	bmiHdr.biYPelsPerMeter = 0;//垂直分辨率，单位长度内的像素数
	bmiHdr.biClrUsed = 0;//位图实际使用的彩色表中的颜色索引数（设为0的话，则说明使用所有调色板项）
	bmiHdr.biClrImportant = 0;//说明对图象显示有重要影响的颜色索引的数目，0表示所有颜色都重要
 
	//RGB565格式掩码
	bmiClr[0].rgbBlue = 0;
	bmiClr[0].rgbGreen = 0xF8;
	bmiClr[0].rgbRed = 0;
	bmiClr[0].rgbReserved = 0;
 
	bmiClr[1].rgbBlue = 0xE0;
	bmiClr[1].rgbGreen = 0x07;
	bmiClr[1].rgbRed = 0;
	bmiClr[1].rgbReserved = 0;
 
	bmiClr[2].rgbBlue = 0x1F;
	bmiClr[2].rgbGreen = 0;
	bmiClr[2].rgbRed = 0;
	bmiClr[2].rgbReserved = 0;
 
 
	bmfHdr.bfType = (WORD)0x4D42;//文件类型，0x4D42也就是字符'BM'
	bmfHdr.bfSize = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(RgbQuad2) * 3 + bmiHdr.biSizeImage);//文件大小
	bmfHdr.bfReserved1 = 0;//保留，必须为0
	bmfHdr.bfReserved2 = 0;//保留，必须为0
	bmfHdr.bfOffBits = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader)+ sizeof(RgbQuad2) * 3);//实际图像数据偏移量
 
	if (!(fp = fopen(filename, "wb"))){
		return -1;
	} else {
		printf("file %s open success\n",filename);
	}
 
	fwrite(&bmfHdr, 1, sizeof(BitMapFileHeader), fp); 
	fwrite(&bmiHdr, 1, sizeof(BitMapInfoHeader), fp); 
	fwrite(&bmiClr, 1, 3*sizeof(RgbQuad2), fp);
 
//	fwrite(buf, 1, bmiHdr.biSizeImage, fp);	//mirror
	for(int i=0; i<height; i++){
		fwrite(buf+(width*(height-i-1)*2), 2, width, fp);
	}
 
	printf("Image size=%d, file size=%d, width=%d, height=%d\n", bmiHdr.biSizeImage, bmfHdr.bfSize, width, height);
	printf("%s over\n", __FUNCTION__);
	fclose(fp);
 
	 return 0;
}












// 拆除 max30102 (max30102)vcc -vin(esp32),sda----d21,scl-----d22,
#if 1 // 开启DEBUG打印
#define LOGD(...) printf(__VA_ARGS__)
#else // 关闭DEBUG打印
#define LOGD(...)
#endif

#if 1 // 开启ERROR打印
#define LOGE(...) printf(__VA_ARGS__)
#else // 关闭ERROR打印
#define LOGE(...)
#endif

// 缓冲区大小
#define BUF_SIZE 61440
#define EXIT_STR "exit"
#define I_EXIT "I exit.\r\n"
#define I_RECEIVE "I receive.\r\n"
#include <time.h>

// 打开串口
HANDLE OpenSerial(const char *com, // 串口名称，如COM1，COM2
                  int baud,        // 波特率：常用取值：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800
                  int byteSize,    // 数位大小：可取值7、8；
                  int parity,      // 校验方式：可取值NOPARITY、ODDPARITY、EVENPARITY、MARKPARITY、SPACEPARITY
                  int stopBits)    // 停止位：ONESTOPBIT、ONE5STOPBITS、TWOSTOPBITS；
{
    DCB dcb;
    BOOL b = FALSE;
    COMMTIMEOUTS CommTimeouts;
    HANDLE comHandle = INVALID_HANDLE_VALUE;

    // 打开串口
    comHandle = CreateFile(com,                          // 串口名称
                           GENERIC_READ | GENERIC_WRITE, // 可读、可写
                           0,                            // No Sharing
                           NULL,                         // No Security
                           OPEN_EXISTING,                // Open existing port only
                           FILE_ATTRIBUTE_NORMAL,        // Non Overlapped I/O
                           NULL);                        // Null for Comm Devices

    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("CreateFile fail\r\n");
        return comHandle;
    }

    // 设置读写缓存大小
    b = SetupComm(comHandle, BUF_SIZE, BUF_SIZE);
    if (!b)
    {
        LOGE("SetupComm fail\r\n");
    }

    // 设定读写超时
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;   // 读间隔超时
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;   // 读时间系数
    CommTimeouts.ReadTotalTimeoutConstant = 0;     // 读时间常量
    CommTimeouts.WriteTotalTimeoutMultiplier = 1;  // 写时间系数
    CommTimeouts.WriteTotalTimeoutConstant = 1;    // 写时间常量
    b = SetCommTimeouts(comHandle, &CommTimeouts); // 设置超时
    if (!b)
    {
        LOGE("SetCommTimeouts fail\r\n");
    }

    // 设置串口状态属性
    GetCommState(comHandle, &dcb);     // 获取当前
    dcb.BaudRate = baud;               // 波特率
    dcb.ByteSize = byteSize;           // 每个字节有位数
    dcb.Parity = parity;               // 无奇偶校验位
    dcb.StopBits = stopBits;           // 一个停止位
    b = SetCommState(comHandle, &dcb); // 设置
    if (!b)
    {
        LOGE("SetCommState fail\r\n");
    }

    return comHandle;
}
#pragma pack(2)

typedef struct tagBITMAPFILEHEADER2
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;

} BITMAPFILEHEADER2;
#pragma pack()

typedef struct tagBITMAPINFOHEADER2
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER2;

int main(int argc, char *argv[])
{
    BOOL b = FALSE;
    DWORD wRLen = 0;
    DWORD wWLen = 0;
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char buf2[BUF_SIZE*10] = {0};

    HANDLE comHandle = INVALID_HANDLE_VALUE; // 串口句柄

    char buffer[12];
    COMMTIMEOUTS ct;
    ct.ReadIntervalTimeout = 50;
    ct.ReadTotalTimeoutConstant = 50000;
    ct.ReadTotalTimeoutMultiplier = 500;

    // 打开串口
    // if port >9 \\\\.\\COM%d
    // else COM5
    const char *com = "\\\\.\\COM15";
    comHandle = OpenSerial(com, CBR_115200, 8, NOPARITY, ONESTOPBIT);
    SetCommTimeouts(comHandle, &ct);
   // SetupComm(comHandle, 10000000, 10000000);
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("OpenSerial COM15 fail!\r\n");
        return -1;
    }
    LOGD("Open COM15 Successfully!\r\n");
    char sab[15] = {0x31, 0x32, 0x3, 'y', 'i', 'x', 'i', 'n', 'x', 'i', 'n'};

    // 循环接收消息，收到消息后将消息内容打印并回复I_RECEIVE, 如果收到EXIT_STR就回复EXIT_STR并退出循环
    time_t rawtime;
    struct tm *info_time;
    BITMAPFILEHEADER2 header;
    BITMAPFILEHEADER2 *pheader;

    BITMAPINFOHEADER2 info;
    BITMAPINFOHEADER2 *pinfo;
    pinfo = &info;
    pheader = &header;
    pheader->bfType = 0x4d42;
    pheader->bfSize = 22710;
    pheader->bfReserved1 = 0;
    pheader->bfReserved2 = 0;
    pheader->bfOffBits = 0x36;
    pinfo->biSize = 0x28;
    pinfo->biWidth = 0xec;
    pinfo->biHeight = 0x30;
    pinfo->biPlanes = 0x01;
    pinfo->biBitCount = 0x10;
    pinfo->biSizeImage = 22656;
    pinfo->biXPelsPerMeter = 0x1d87;
    pinfo->biYPelsPerMeter = 0x1d87;
    pinfo->biClrUsed = 0;
    pinfo->biClrImportant = 0;

    FILE *fptr;
    fptr = fopen("test.bin", "w");
    // fwrite(pheader, sizeof(BITMAPFILEHEADER2), 1, fptr);
    // fwrite(pinfo, sizeof(BITMAPINFOHEADER2), 1, fptr);

    printf("%d", sizeof(BITMAPFILEHEADER2));

    //  while (1)
    //  {
    wRLen = 0;

    time(&rawtime);
    info_time = localtime(&rawtime);
    strftime(buffer, 12, "%H:%M:%S", info_time);
    memcpy(sab + 3, buffer, 12);
    printf("----%s\n", sab + 3);

    // 写串口消息
    // b = WriteFile(comHandle, sab, strlen(sab), &wWLen, NULL);
    // if (!b)
    // {
    //     LOGE("WriteFile fail\r\n");
    // }

    // b = ReadFile(comHandle, buf, 200, &wRLen, NULL);
     //   b = ReadFile(comHandle, buf, sizeof(buf), &wRLen, NULL);
    //     fwrite(&buf, sizeof(buf), 1, fptr);
    int j=0;
    char file[20];

    while (1)
    {
        if(j==10)break;
        b = ReadFile(comHandle, buf, sizeof(buf) , &wRLen, NULL);
        sprintf(file,"test%d.bmp",j);
        printf("abc,%d",j);
        memcpy(buf2+j*61440,buf,sizeof(buf));
       // fwrite(&buf, sizeof(buf), 1, fptr);
        //  Rgb565ConvertBmp(buf,640,48,file);

            j+=1;


      //  Sleep(500);
    }

   Rgb565ConvertBmp(buf2,640,480,"test.bmp");

    fclose(fptr);

    if (b && 0 < wRLen)
    { // 读成功并且数据大小大于0
       // buf[wRLen] = '\0';
        // LOGD("[RECV]%s\r\n", buf); // 打印收到的数据
        // if (0 == strncmp(buf, EXIT_STR, strlen(EXIT_STR))) {
        //     //回复
        //     b = WriteFile(comHandle, TEXT(I_EXIT), strlen(I_EXIT), &wWLen, NULL);
        //     if (!b) {
        //         LOGE("WriteFile fail\r\n");
        //     }
        //     break;
        // }
    }
    //     //回复
    // 	b = WriteFile(comHandle, sab, strlen(sab), &wWLen, NULL);
    //     if (!b) {
    //         LOGE("WriteFile fail\r\n");
    //     }
    // }
    //  }

    // 关闭串口
    b = CloseHandle(comHandle);
    if (!b)
    {
        LOGE("CloseHandle fail\r\n");
    }

    LOGD("Program Exit.\r\n");
    return 0;
}