#include <cstdlib>

#include <iostream>

#include <vector>


#include <opencv2/highgui.hpp>

#include <opencv2/imgproc.hpp>

#include <opencv2/opencv.hpp>


#include "GPIOlib.h"


#define PI 3.1415926


/* Uncomment this line at run-time to skip GUI rendering */

#define _DEBUG


using namespace cv;

using namespace std;

using namespace GPIO;


const string CAM_PATH = "/dev/video0";

const string MAIN_WINDOW_NAME = "Processed Image";

const string CANNY_WINDOW_NAME = "Canny";


const int CANNY_LOWER_BOUND = 50;

const int CANNY_UPPER_BOUND = 250;

const int HOUGH_THRESHOLD = 150;


const int high_speed = 8;

const int low_speed = 8;


const float deviation = 0;

const float standard = 1;

//获取绝对值
float get_abs( float a )
{
	if ( a < 0 )
		return(-a);
	return(a);
}

bool judge_normal(int lines)
{
	if ( lines>1 )
		return true;
	else{
		return false;
	}
}

//向右转多少度
void turnToRight( int angle )
{
	turnTo( angle );
	delay( 1000 );
	turnTo( 0 );
}

//向左转多少度
void turnToLeft( int angle )
{
	turnTo( -angle );
	delay( 1000 );
	turnTo( 0 );
}

//前进的封装方法
void forward( int speed )
{
	controlLeft( FORWARD, speed );
	controlRight( FORWARD, speed );
}


//void special_handle( float k1, float k2 )
//{
//	float positive_k, negative_k;
//
//	if ( k1 > 0 )
//	{
//		positive_k = k1;
//
//		negative_k = k2;
//	}else{
//		positive_k = k2;
//
//		negative_k = k1;
//	}
//}


///* 简单处理，哪边斜率大就向另一边转 */
void simple_handle( float k1, float k2 )
{
	int positive_k, negative_k;

	int angle = 20;

	if ( k1 > 0 )
	{
		positive_k = k1;

		negative_k = -k2;
	}else{
		positive_k = k2;

		negative_k = -k1;
	}

	if ( positive_k > negative_k )
	{
		turnToRight( angle );
	}else{
		turnToLeft( angle );
	}
}


void run( float k1, float k2 )
{
	if ( judge_normal( 1 ) )
	{


		forward( high_speed );
	}else{


		forward( low_speed );


		simple_handle( k1, k2 );
	}
}


int main()

{
	init();

	VideoCapture capture( CAM_PATH );



	if ( !capture.isOpened() )

	{
		capture.open( atoi( CAM_PATH.c_str() ) );
	}


	double dWidth = capture.get( CV_CAP_PROP_FRAME_WIDTH );

	double dHeight = capture.get( CV_CAP_PROP_FRAME_HEIGHT );

	clog << "Frame Size: " << dWidth << "x" << dHeight << endl;


	Mat image;

	while ( true )

	{
		capture >> image;

		if ( image.empty() )

			break;



		Rect roi( 0, image.rows / 3, image.cols, image.rows / 3 );


		Mat imgROI = image( roi );

		cvtColor(imgROI,grey,CV_RGB2GRAY);
		GaussianBlur(grey,blur,Size(3,3),0);
		medianBlur(blur,smooth,3);
		morphologyEx(smooth, close, CV_MOP_CLOSE, element);
		morphologyEx(close, open, CV_MOP_OPEN, element);
		threshold(open, binary, THRESHOLD , 255, THRESH_BINARY_INV);



		Mat contours;

		Canny( binary, contours, CANNY_LOWER_BOUND, CANNY_UPPER_BOUND );

		#ifdef _DEBUG

		imshow( CANNY_WINDOW_NAME, contours );

		#endif


		vector<Vec2f> lines;

		HoughLines( contours, lines, 1, PI / 180, HOUGH_THRESHOLD );

		Mat result( imgROI.size(), CV_8U, Scalar( 255 ) );


		imgROI.copyTo( result );

		clog << lines.size() << endl;


		float maxRad = -2 * PI;

		float minRad = 2 * PI;



		vector<float> linesLeft;
		vector<float> bLeft;
		vector<float> xlLeft;

		vector<float> linesRight;
		vector<float> bRight;
		vector<float> xlRight;
//		float lineLeft = 0;
//		float lineRight = 0;


		for ( vector<Vec2f>::const_iterator it = lines.begin(); it != lines.end(); ++it )

		{
			float rho = (*it)[0];

			float theta = (*it)[1];







//			/* 5-85 （右）   ||  93-175（左） */

			if ( (theta > 0.09 && theta < 1.48) || (theta > 1.62 && theta < 3.05) )

			{
				if ( theta > maxRad )

					maxRad = theta;

				if ( theta < minRad )

					minRad = theta;


				#ifdef _DEBUG

				Point pt1( rho / cos( theta ), 0 );
				Point pt2( (rho - result.rows * sin( theta ) ) / cos( theta ), result.rows );

				line( result, pt1, pt2, Scalar( 0, 255, 255 ), 3, CV_AA );
                #endif

				float k = -1 * (cos(theta)/sin(theta));
				float b = rho / sin(theta);
				float xl = rho / cos(theta);
//
				if ( k > 0 ) {

                    linesLeft.push_back(k);
                    bLeft.push_back(b);
                    xlLeft.push_back(xl);
				} else if ( k < 0 ) {
					linesRight.push_back(k);
					bRight.push_back(b);
					xlRight.push_back(xl);
				}
			}



				#ifdef _DEBUG

				clog<<"Line: ("<<rho<<","<<theta<<")\n";

				#endif
		}

		float kLeft = 0;
		float kRight = 0;

		if(xlLeft.size()!=0){

            int countNumLeft = 0;
            float minXl = get_abs(xlLeft[0]);
            for(int i = 0;i<xlLeft.size();i++){

                cout<<"ll:"<<xlLeft[i]<<endl;
                if(get_abs(xlLeft[i])<minXl){
                    minXl = get_abs(xlLeft[i]);
                    countNumLeft = i;
                }
            }
            kLeft = linesLeft[countNumLeft];

		}
//        if(xlLeft.size()!=0){
//
//            int countNumLeft = 0;
//            float maxXl = get_abs(xlLeft[0]);
//            for(int i = 0;i<xlLeft.size();i++){
//                if(get_abs(xlLeft[i])>maxXl){
//                    maxXl = get_abs(xlLeft[i]);
//                    countNumLeft = i;
//                }
//            }
//            kLeft = linesLeft[countNumLeft];
//
//        }


		if(xlRight.size()!=0){
		    int countNumRight = 0;
		    float minXl2 = get_abs(xlRight[0]);
		    for(int i = 0;i<xlRight.size();i++){
		        if(get_abs(xlRight[i])<minXl2){
		            minXl2 = get_abs(xlRight[i]);
		            countNumRight = i;
		        }
		    }

		    kRight = linesRight[countNumRight];
		}


//          kLeft和kRight  分别是左右两条线的斜率  如果没识别到 就是0    取x轴交点距离原点最近的那条线


        int lines = 1;

        if ( judge_normal( lines ) )
        {


            forward( high_speed );
        }else{


            forward( low_speed );


            simple_handle( k1, k2 );
        }

		#ifdef _DEBUG

		stringstream overlayedText;

		overlayedText << "Lines: " << lines.size()<< " kl"<<kLeft<<" kR"<<kRight;

		putText( result, overlayedText.str(), Point( 10, result.rows - 10 ), 2, 0.8, Scalar( 0, 0, 255 ), 0 );

		imshow( MAIN_WINDOW_NAME, result );

		#endif


		lines.clear();

		linesLeft.clear();
		bLeft.clear();
		xlLeft.clear();
		linesRight.clear();
        bRight.clear();
        xlRight.clear();


		waitKey( 1 );
	}

	return(0);
}