// main.cpp
#include <opencv2/opencv.hpp>
#include "muxer.h"
#include <iostream>
using namespace std;
int main()
{
	//cv::VideoCapture cap(0);
	cv::VideoCapture cap("queen.mp4");
	if (!cap.isOpened())
	{
        cout<<"wrong"<<endl;
		return 0;
	}

	const int width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
	const int height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	const int frameRate = (int)cap.get(cv::CAP_PROP_FPS);
	const int totalFrames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    char * p=(char*)"out.mp4";
	muxer encode;
	int ret = encode.init(width, height, 30, 4992000, p);
    cout<<"helloworld"<<endl;
	cv::Mat image;
	while (true)
	{
		cap >> image;
		if (image.empty())
			break;
		if (!encode.write_image(image.data))
			fprintf(stderr, "write image fail.\n");

		imshow("video", image);
		int key = cv::waitKey(2);
		if ((char)key == 'q')
		{
			break;
		}
	}
	encode.flush();
	encode.uninit();
	
	cap.release();
	cv::destroyAllWindows();

	return 0;
}

