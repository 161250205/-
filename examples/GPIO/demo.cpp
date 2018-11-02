#include "GPIOlib.h"

using namespace GPIO;

void init_k(int& k1,int& k2){
    k1 = 0;
    k2 = 0;
}
bool judge_normal(double k1,double k2,double deviation,double standard){
    if(get_abs(k1)>standard&&get_abs(k2)>standard)
        return (get_abs(get_abs(k1)-get_abs(k2))<deviation)?true:false;
    else{
        return false;
    }
}

double get_abs(double a){
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

void special_handle(double  k1,double k2){
    int positive_k,negativeZ_k;
    if(k1>0){
        positive_k = k1;
        negative_k = k2;
    }else{
        positive_k = k2;
        negative_k = k1;
    }
}
//简单处理，哪边斜率大就向另一边转
void simple_handle(double k1,double k2){
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
int main()
{
	init();
    int high_speed;//正常情况下快速直行
    int low_speed;//特殊情况下缓慢前行（比如需要转弯的时候或者线的识别出现了短暂bug)

    double line1_k,line2_k,line1_b,line2_b;
    double deviation = 0;//可以容忍的斜率差（根据实际情况定）
    double standard = 1;//斜率标准，只有当斜率绝对值大于标准才可以与误差比较（当斜率较小的时候，误差普遍较小，没有比较性）

    while(true){
        //初始化k1，k2
        init_k(line1_k,line2_k);
        //默认为0，根据伟哥传过来的值进行修改
        line1_k = 0;
        line2_k = 0;
        //根据两条线的参数判断是否是正常状态
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

	//Don't forget to stop all motors before exiting.
	stopLeft();
	stopRight();
	return 0;
}
