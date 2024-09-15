// main.cpp
#include <opencv2/opencv.hpp>
#include "muxer.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include<windows.h>
using namespace std;

#define MAX_PATH2 1024
void find(char*lppath,std::vector< std::string> &fileList){

char szFind[MAX_PATH2];
WIN32_FIND_DATA FindFileData;
strcpy(szFind,lppath);
strcat(szFind,"\\*.*");




HANDLE hFind=::FindFirstFile(szFind,&FindFileData);
if(INVALID_HANDLE_VALUE==hFind) return;
while(true){

if(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){

	if(FindFileData.cFileName[0]!='.'){

		char szFile[MAX_PATH2];
		strcpy(szFile,lppath);
		strcat(szFile,"\\");
		strcat(szFile,(char*)(FindFileData.cFileName));
		find(szFile,fileList);

	}
}else{

	fileList.push_back(FindFileData.cFileName);
}
if(!FindNextFile(hFind,&FindFileData)) break;
}
FindClose(hFind);


}
int main()
{
	//cv::VideoCapture cap(0);
	//cv::VideoCapture cap("queen.mp4");
	// if (!cap.isOpened())
	// {
    //     cout<<"wrong"<<endl;
	// 	return 0;
	// }

	unsigned codecVer = avcodec_version();
int ver_major,ver_minor,ver_micro;
ver_major = (codecVer>>16)&0xff;
ver_minor = (codecVer>>8)&0xff;
ver_micro = (codecVer)&0xff;
printf("Current ffmpeg version is: %s ,avcodec version is: %d=%d.%d.%d\n",FFMPEG_VERSION,codecVer,ver_major,ver_minor,ver_micro);
return 0;
	 char * p=(char*)"./samsung_dcmi";
    char * output=(char*)"output.mp4";

	std::vector< std::string> fileList;

	find(p,fileList);
	cout<<fileList[0]<<endl;
	cout<<"hello world"<<endl;
	 muxer encode;
	 int ret = encode.init(3024, 4032, 1, 4992000, output);
	for(int i=0;i<fileList.size();i++){
		string filepath="./samsung_dcmi/"+fileList[i];
		cout<<filepath<<endl;
		cv::Mat	srcImage = cv::imread(filepath);
	//	cv::Mat image_part = srcImage(cv::Rect(0,0,600,800)); // 裁剪后的图

	//	cv::imshow("video",image_part);
	//	cv::waitKey(0);

		cv::Mat imgRGB;
	//	break;
		//cvtColor(srcImage,imgRGB,cv::COLOR_RGB2BGR);
		if (!encode.write_image(srcImage.data))
	 		fprintf(stderr, "write image fail.\n");


	}
	// const int width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
	// const int height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	// const int frameRate = (int)cap.get(cv::CAP_PROP_FPS);
	// const int totalFrames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    // char * p=(char*)"out.mp4";
	// muxer encode;
	// int ret = encode.init(width, height, 30, 4992000, p);
    // cout<<"helloworld"<<endl;
	// cv::Mat image;
	// while (true)
	// {
	// 	cap >> image;
	// 	if (image.empty())
	// 		break;
	// 	if (!encode.write_image(image.data))
	// 		fprintf(stderr, "write image fail.\n");

	// 	imshow("video", image);
	// 	int key = cv::waitKey(2);
	// 	if ((char)key == 'q')
	// 	{
	// 		break;
	// 	}
	// }
	// encode.flush();
	// encode.uninit();
	
	// cap.release();
	// cv::destroyAllWindows();

	return 0;
}

