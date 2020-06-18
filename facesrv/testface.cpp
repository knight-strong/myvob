#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "cxcore.h"
#include "highgui.h"
#include "cv.h"
#include "opencv2/opencv.hpp"
using namespace cv;

#include "FiStdDefEx.h"
#include "THFeature_i.h"


//get current system time
double msecond()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec * 1.0e3 + tv.tv_usec * 1.0e-3);
}

int main(int argc, char **argv)
{
    if (argc<3)
    {
        printf("You must input 2 image files.\n");
        return 0;
    }
    char* sFile1=argv[1];
    char* sFile2=argv[2];
    // char* sTimes=argv[3];

    //
    int nTimes=1;
    /*if(sTimes)
    {
        nTimes=atoi(sTimes);
    }*/

    double compare_start;
    double compare_end;

    int ret;


    THFI_Param detParam;
    detParam.nMinFaceSize = 50;
    detParam.nRollAngle = 60;

    ret = THFI_Create(1, &detParam);
    if (ret < 0)
    {
        printf("THFI_Create failed!(ret=%d)\n",ret);
        return -1;
    }

    ret=EF_Init(1);
    if (ret < 0)
    {
        printf("EF_Init failed!(ret=%d)\n",ret);
        THFI_Release();	
        return -1;	
    }

    int size = EF_Size();
    printf("feature size=%d\n\n",size);

    double ms_init = msecond();

    printf("**********************************************************\n");
    int index=1;
    while(nTimes-->0)
    {	
        //first image

        Mat img1=imread(sFile1);
        if(img1.empty())
        {
            THFI_Release();	
            EF_Release();
            printf("read first image file failed.\n");
            return -1;
        }

        int nWidth1=img1.cols;
        int nHeight1=img1.rows;

        THFI_FacePos fps1[1];

        compare_start = msecond();
        ret = THFI_DetectFace(0, img1.data, 24, nWidth1, nHeight1, fps1, 1,0);
        compare_end = msecond();

        printf("THFI_DetectFace time in first image = %04f ms\n", (float)(compare_end - compare_start));

        printf("left=(%d,%d),right=(%d,%d),nose=(%d,%d),mouth=(%d,%d),roll=%d,yaw=%d,pitch=%d,c=%f,q=%d\n",
                fps1[0].ptLeftEye.x,
                fps1[0].ptLeftEye.y,
                fps1[0].ptRightEye.x,
                fps1[0].ptRightEye.y,
                fps1[0].ptNose.x,
                fps1[0].ptNose.y,
                fps1[0].ptMouth.x,
                fps1[0].ptMouth.y,
                fps1[0].fAngle.roll,
                fps1[0].fAngle.yaw,
                fps1[0].fAngle.pitch,
                fps1[0].fAngle.confidence,
                fps1[0].nQuality);

        if(ret<=0)
        {
            printf("THFI_DetectFace failed in first image.(ret=%d)\n",ret);
            THFI_Release();	
            EF_Release();
            return -1;
        }

        BYTE* feature1 = new BYTE[size];
        memset(feature1,0,size);

        compare_start = msecond();
        ret=EF_Extract(0, img1.data,nWidth1, nHeight1, 3, &fps1[0], feature1);
        compare_end = msecond();

        printf("EF_Extract time in first image = %04f ms\n", (float)(compare_end - compare_start));

        if(ret!=1)
        {
            delete []feature1;
            THFI_Release();	
            EF_Release();
            printf("EF_Extract failed in first image.(ret=%d)\n",ret);	
            return -1;
        }

        printf("\n");

        //second image	

        Mat img2=imread(sFile2);  
        if(img2.empty())
        {
            THFI_Release();	
            EF_Release();
            printf("read second image file failed.\n");
            return -1;

        }

        int nWidth2=img2.cols;
        int nHeight2=img2.rows;

        THFI_FacePos fps2[10];

        compare_start = msecond();
        ret = THFI_DetectFace(0, img2.data, 24, nWidth2, nHeight2, fps2, 10,360);
        compare_end = msecond();

        printf("THFI_DetectFace time in second image = %04f ms\n", (float)(compare_end - compare_start));


        printf("left=(%d,%d),right=(%d,%d),nose=(%d,%d),mouth=(%d,%d),roll=%d,yaw=%d,pitch=%d,c=%f,q=%d\n",
                fps2[0].ptLeftEye.x,
                fps2[0].ptLeftEye.y,
                fps2[0].ptRightEye.x,
                fps2[0].ptRightEye.y,
                fps2[0].ptNose.x,
                fps2[0].ptNose.y,
                fps2[0].ptMouth.x,
                fps2[0].ptMouth.y,
                fps2[0].fAngle.roll,
                fps2[0].fAngle.yaw,
                fps2[0].fAngle.pitch,
                fps2[0].fAngle.confidence,
                fps2[0].nQuality);


        if(ret<=0)
        {
            printf("THFI_DetectFace failed in second image.(ret=%d)\n",ret);
            THFI_Release();	
            EF_Release();
            return -1;
        }


        BYTE* feature2 = new BYTE[size*ret];
        memset(feature2,0,size);

        compare_start = msecond();
        //ret=EF_Extract(0, img2.data,nWidth2, nHeight2, 3, &fps2[0], feature2);
        ret=EF_Extract_M(0, img2.data,nWidth2, nHeight2, 3, fps2,feature2, ret);
        compare_end = msecond();

        printf("EF_Extract time in second image = %04f ms\n", (float)(compare_end - compare_start));	

        if(ret!=1)
        {
            delete []feature1;
            delete []feature2;
            THFI_Release();	
            EF_Release();
            printf("EF_Extract failed in second image.(ret=%d)\n",ret);	
            return -1;
        }

        printf("\n");
        //feature
        compare_start = msecond();
        float score12;
        //for (int i=0; i<10*10000; i++) {
            score12 =  EF_Compare(feature1, feature2);
        //}
        compare_end = msecond();

        printf("compare time = %04f ms\r\n", (float)(compare_end - compare_start));

        printf("face1 vs face2 compare score = %f\r\n", score12);

        delete []feature1;
        delete []feature2;

        printf("********************************************************** %d end\n",index++);
    }

    double ms_end = msecond();
    printf("### total ms: %f\n", ms_end - ms_init);
    //release SDK
    THFI_Release();	
    EF_Release();

    //	printf("Press any key to exit\n");
    //	getchar();

    return 0;
}


