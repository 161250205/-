#include <cstdlib>
#include <iostream>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>


#include "GPIOlib.h"

#define PI 3.1415926

//Uncomment this line at run-time to skip GUI rendering
#define _DEBUG

using namespace cv;
using namespace std;
using namespace GPIO;

const string CAM_PATH="/dev/video0";
const string MAIN_WINDOW_NAME="Processed Image";
const string CANNY_WINDOW_NAME="Canny";

const int CANNY_LOWER_BOUND=50;
const int CANNY_UPPER_BOUND=250;
const int HOUGH_THRESHOLD=150;

const int high_speed = 30;//正常情况下快速直行
const int low_speed = 15;//特殊情况下缓慢前行（比如需要转弯的时候或者线的识别出现了短暂bug)

const float deviation = 0;//可以容忍的斜率差（根据实际情况定）
const float standard = 1;//斜率标准，只有当斜率绝对值大于标准才可以与误差比较（当斜率较小的时候，误差普遍较小，没有比较性）
void init_k(int& k1,int& k2){
    k1 = 0;
    k2 = 0;
}
bool judge_normal(float k1,float k2,float deviation,float standard){
    if(get_abs(k1)>standard&&get_abs(k2)>standard)
        return (get_abs(get_abs(k1)-get_abs(k2))<deviation)?true:false;
    else{
        return false;
    }
}

float get_abs(float a){
    if(a<0) return -a;
    return a;
}
void turnToRight(int angle){
    turnTo(angle);
    delay(500);
    turnTo(-angle);
    delay(300);
}
void turnToLeft(int angle){

    turnTo(-angle);
    delay(500);
    turnTo(angle);
    delay(300);
}
void forward(int speed){
    controlLeft(FORWARD,speed);
    controlRight(FORWARD,speed);
}

void special_handle(float  k1,float k2){
    float positive_k,negativeZ_k;
    if(k1>0){
        positive_k = k1;
        negative_k = k2;
    }else{
        positive_k = k2;
        negative_k = k1;
    }
}
//简单处理，哪边斜率大就向另一边转
void simple_handle(float k1,float k2){
    int positive_k,negative_k;
    int angle = 20;//旋转角度，默认20
    if(k1>0){
        positive_k = k1;
        negative_k = -k2;
    }else{
        positive_k = k2;
        negative_k = -k1;
    }
    if(positive_k>negativeZ_k){
        turnToRight(angle);//右转
    }else{
        turnToLeft(angle);//左转
    }
}
void run(float k1,float k2){
    if(judge_normal(line1_k,line2_k,deviation,standard)){
                //是正常状态则快速前行
                forward(high_speed);
       }else{
                //特殊状态则减速前行
                forward(low_speed);
                //特殊情况对舵机调整进行转弯
                simple_handle(line1_k,line2_k);
    }
}
int main()
{
    init();
	VideoCapture capture(CAM_PATH);
	//If this fails, try to open as a video camera, through the use of an integer param
	if (!capture.isOpened())
	{
		capture.open(atoi(CAM_PATH.c_str()));
	}

	double dWidth=capture.get(CV_CAP_PROP_FRAME_WIDTH);			//the width of frames of the video
	double dHeight=capture.get(CV_CAP_PROP_FRAME_HEIGHT);		//the height of frames of the video
	clog<<"Frame Size: "<<dWidth<<"x"<<dHeight<<endl;

	Mat image;
	while(true)
	{
		capture>>image;
		if(image.empty())
			break;

		//Set the ROI for the image
		Rect roi(0,image.rows/3,image.cols,image.rows/3);
		Mat imgROI=image(roi);

		cvtColor(imgROI,grey,CV_RGB2GRAY);
		GaussianBlur(grey,blur,Size(3,3),0);
		medianBlur(blur,smooth,3);
		morphologyEx(smooth, close, CV_MOP_CLOSE, element);
		morphologyEx(close, open, CV_MOP_OPEN, element);
		threshold(open, binary, THRESHOLD , 255, THRESH_BINARY_INV);

		//Canny algorithm
		Mat contours;
		Canny(imgROI,contours,CANNY_LOWER_BOUND,CANNY_UPPER_BOUND);
		#ifdef _DEBUG
		imshow(CANNY_WINDOW_NAME,contours);
		#endif

		vector<Vec2f> lines;
		HoughLines(contours,lines,1,PI/180,HOUGH_THRESHOLD);
		Mat result(imgROI.size(),CV_8U,Scalar(255));
		imgROI.copyTo(result);
		clog<<lines.size()<<endl;
		
		float maxRad=-2*PI;
		float minRad=2*PI;
		//Draw the lines and judge the slope
		for(vector<Vec2f>::const_iterator it=lines.begin();it!=lines.end();++it)
		{
			float rho=(*it)[0];			//First element is distance rho
			float theta=(*it)[1];		//Second element is angle theta

			//Filter to remove vertical and horizontal lines,
			//and atan(0.09) equals about 5 degrees.

			//5-85 （右）   ||  93-175（左）
			if((theta>0.09&&theta<1.48)||(theta>1.62&&theta<3.05))
			{
				if(theta>maxRad)
					maxRad=theta;
				if(theta<minRad)
					minRad=theta;
				
				#ifdef _DEBUG
				//point of intersection of the line with first row
				Point pt1(rho/cos(theta),0);
				//point of intersection of the line with last row
				Point pt2((rho-result.rows*sin(theta))/cos(theta),result.rows);
				//Draw a line
				line(result,pt1,pt2,Scalar(0,255,255),3,CV_AA);
				#endif
			}

			float k = -cot(theta);

			#ifdef _DEBUG
			clog<<"Line: ("<<rho<<","<<theta<<")\n";
			#endif
		}

		float linek[2];
        run(linek[0],linek[1]);
		#ifdef _DEBUG
		stringstream overlayedText;
		overlayedText<<"Lines: "<<lines.size();
		putText(result,overlayedText.str(),Point(10,result.rows-10),2,0.8,Scalar(0,0,255),0);
		imshow(MAIN_WINDOW_NAME,result);
		#endif

		lines.clear();
		waitKey(1);
	}
	return 0;
}
