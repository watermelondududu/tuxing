#include "stdafx.h"

#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include <GL/glut.h>
#include "Vector048.h"
#include "Matrix048.h"
#include <vector> 
#include <string> 
#include <fstream> 
#include <iostream> 

void myDisplay(void);
#define POINTNUM 152//一共152个点，但编号只到151
#define CIRCLENUM 20
#define pi 3.1416
CVector048 allpos[POINTNUM*CIRCLENUM];//每个关键点及其周围环绕的小圆环的位置向量
CVector048 circlepos[CIRCLENUM];//每个小圆环的位置向量
CVector048 pointpos[POINTNUM];//每个关键点的位置向量
CVector048 robotPos,robotDir;//机器人位置、方向向量
CVector048 eyePos;
int lockMode=0,travelMode=0;//lockmode0自由，lockmode1锁定，lockmode2叠加；travelmode0欧拉，travelmode1子坐标系
int run;//每次display时run++；每+20次改变一下机器人形态
int drawmode;//选择圆环或管道绘制方式
int robotIndex;//机器人现在在哪个点后
float modelangle;//模型整体旋转的角度
float robotspeed=0.1;
float rx=-25,ry=0,rz=0;
float mspeed=5,rspeed=1;//视点的移动速度和旋转速度
float g_IEyeMat[16]={1,0,0,0,
					 0,1,0,0,
					 0,0,1,0,
					 0,0,0,1},
	  g_EyeMat[16]={1,0,0,0,
					0,1,0,0,
					0,0,1,0,
					0,0,0,1};

void SetRC()
{
	//定义了一个圆的路径。
	for(int i=0; i<CIRCLENUM; i++)
	{
		float angle = i*2*3.14/(CIRCLENUM-1);
		circlepos[i].x = 0;
		circlepos[i].y = 1*cos(angle);
		circlepos[i].z = 1*sin(angle);
	}
	//初始化位置向量
	float R=2,seta=0;
	int midlast = 57;//第一段的最后一个点
	for(int i=0; i<POINTNUM; i++)
	{
		if(i<9){
			pointpos[i][0]=-12+i*0.5;
			pointpos[i][1]=2;
			pointpos[i][2]=0;
		}
		else if(i<26){
			pointpos[i][0]=-10;
			pointpos[i][1]=4-(i-9)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<31){
			pointpos[i][0]=-12;
			pointpos[i][1]=-(i-26)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<36){
			pointpos[i][0]=-8;
			pointpos[i][1]=-0.5*(i-31);
			pointpos[i][2]=0;
		}
		else if(i<45){
			pointpos[i][0]=-6+(i-36)*0.5;
			pointpos[i][1]=2;
			pointpos[i][2]=0;
		}
		else if(i<58){
			pointpos[i][0]=-7+(i-45)*0.5;
			pointpos[i][1]=-4;
			pointpos[i][2]=0;
		}
		else if(i<75){
			pointpos[i][0]=-4;
			pointpos[i][1]=4-(i-58)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<78){
			pointpos[i][0]=1.5+(i-75)*0.5;
			pointpos[i][1]=4;
			pointpos[i][2]=0;
		}
		else if(i<91){
			pointpos[i][0]=2;
			pointpos[i][1]=2.5-(i-78)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<109){
			pointpos[i][0]=2.5+(i-91)*0.5;
			pointpos[i][1]=-4;
			pointpos[i][2]=0;
		}
		else if(i<118){
			pointpos[i][0]=6+(i-109)*0.5;
			pointpos[i][1]=4;
			pointpos[i][2]=0;
		}
		else if(i<133){
			pointpos[i][0]=4.5+(i-118)*0.5;
			pointpos[i][1]=2;
			pointpos[i][2]=0;
		}
		else if(i<141){
			pointpos[i][0]=6;
			pointpos[i][1]=1.5-(i-133)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<149){
			pointpos[i][0]=10;
			pointpos[i][1]=1.5-(i-141)*0.5;
			pointpos[i][2]=0;
		}
		else if(i<151){
			pointpos[i][0]=10.5+(i-149)*0.5;
			pointpos[i][1]=-2.5;
			pointpos[i][2]=0;
		}
	}		
	CMatrix048 mat;
	for(int i=0; i<POINTNUM; i++)
	{
		CVector048 dir;
		float rotang = 0;
		if(i!=POINTNUM-1&&i!=midlast&&i!=8&&i!=25&&i!=30&&i!=35&&i!=44&&i!=74&&i!=77&&i!=108&&i!=117&&i!=132&&i!=140)
		{
			dir = pointpos[(i+1)%POINTNUM]-pointpos[i];				
		}
		else
		{
			dir = pointpos[i] - pointpos[(i+POINTNUM-1)%POINTNUM];				
		}
		dir.Normalize();//方向
		rotang = acos(dir.x);
		if(dir.y<0) rotang = -rotang;
		mat.SetRotate(rotang,2);//设置为旋转矩阵。
		mat[12] = pointpos[i].x;	//设置平移部分。
		mat[13] = pointpos[i].y;
		mat[14] = pointpos[i].z;
		for(int j=0;j<CIRCLENUM;j++)
		{
			int index = i*CIRCLENUM+j;
			g_allpos[index] = mat.MulPosition(circlepos[j]);
		}
	}
	robotPos = pointpos[0];
	robotDir = pointpos[1]-pointpos[0];
	robotDir.Normalize();
	eyePos.x=0; eyePos.y=500; eyePos.z=1000;
	glEnable(GL_DEPTH_TEST);
}

void myReshape(int w,int h)
{	
	GLfloat nRange = 100.0f;
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60,GLfloat(w)/h,1,1000);
	//glOrtho(-20,20,-20,20,-1000,1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void update()//场景更新。
{
	if(robotspeed>0)
	{
		if(robotIndex<=POINTNUM-1)
		{
			float leftlen = (pointpos[(robotIndex+1)%POINTNUM] - robotPos).len();//机器人现在位置与下一个关键点之间相隔的距离
			if(leftlen>robotspeed)//如果是距离大，则按速度移动
			{
				if（）

				robotPos = robotPos + robotDir * robotspeed;
			}
			else
			{
				robotIndex++;
				robotIndex=robotIndex%POINTNUM;
				float temp=robotspeed;
				while(temp>leftlen+(pointpos[(robotIndex+1)%POINTNUM]-pointpos[robotIndex]).len())
				{
					temp=temp-(pointpos[(robotIndex+1)%POINTNUM]-pointpos[robotIndex]).len();
					robotIndex++;
					robotIndex=robotIndex%POINTNUM;
					//leftlen = (pointpos[robotIndex] - robotPos).len();
				}
				robotDir = pointpos[(robotIndex+1)%POINTNUM]-pointpos[robotIndex];
				robotDir.Normalize();
				robotPos = pointpos[robotIndex] + robotDir * (temp-leftlen);				//如果一次移动距离过大，跨越了2个关键路径怎么办？同学自己用代码解决。
			}
		}
	}
	else if(robotspeed<0)
	{
		if(robotIndex>=0)
		{
			float leftlen = (robotPos - pointpos[robotIndex]).len();//小球现在位置与上一个关键点之间相隔的距离
			if(leftlen>(-robotspeed))//如果是距离大，则按速度移动
				robotPos = robotPos + robotDir * robotspeed;
			else
			{		
				//robotIndex--;
				float temp=robotspeed;
				int tempindex=0;
				if(robotIndex<=0)
					tempindex=POINTNUM-1;
				else
					tempindex=robotIndex-1;
				while((-temp)>leftlen+(pointpos[robotIndex]-pointpos[tempindex]).len())
				{
					temp=temp+(pointpos[robotIndex]-pointpos[tempindex]).len();
					if(robotIndex<=0)
						robotIndex=POINTNUM-1;
					else
						robotIndex--;
					if(robotIndex<=0)
						tempindex=POINTNUM-1;
					else
						tempindex=robotIndex-1;
				}
				robotDir = pointpos[robotIndex]-pointpos[tempindex];
				robotDir.Normalize();
				robotPos = pointpos[robotIndex] + robotDir * (temp+leftlen);				//如果一次移动距离过大，跨越了2个关键路径怎么办？同学自己用代码解决。
				robotIndex=tempindex;
			}
		}
	}
}

void myTimerFunc(int val)
{
	modelangle+=0.1;
	//g_angle=0;
	update();
	myDisplay();
	glutTimerFunc(1,myTimerFunc,1);
}
