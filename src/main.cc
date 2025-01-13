#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <thread>

#include "MvCameraControl.h"

using namespace cv;
using namespace std;
#define MAX_BUF_SIZE (1920 * 1200 * 3)

bool PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo) {
    if (NULL == pstMVDevInfo) {
        printf("The Pointer of pstMVDevInfo is NULL!\n");
        return false;
    }
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE) {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
        printf("CurrentIp: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    } else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE) {
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
        printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
    } else {
        printf("Not support.\n");
    }

    return true;
}

void __stdcall ImageCallBackEx(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser) {
    if (pFrameInfo) {
        printf(
            "GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n",
            pFrameInfo->nWidth,
            pFrameInfo->nHeight,
            pFrameInfo->nFrameNum);
    }
}

int main() {
    int nRet = MV_OK;
    void* handle = NULL;
    unsigned int nIndex = 0;

    do {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        // enum device
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet) {
            printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
            break;
        }

        while (1) {
            if (stDeviceList.nDeviceNum > 0) {
                for (int i = 0; i < stDeviceList.nDeviceNum; i++) {
                    printf("[device %d]:\n", i);
                    MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                    if (NULL == pDeviceInfo) {
                        break;
                    }
                    PrintDeviceInfo(pDeviceInfo);
                }

                printf("Please Intput camera index: ");
                scanf("%d", &nIndex);

                if (nIndex >= stDeviceList.nDeviceNum) {
                    printf("Intput error!\n");
                    exit(1);
                }
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                printf("Find No Devices!\n");
                nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
            }
        }

        // select device and create handle
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet) {
            printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
            break;
        }

        // open device
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet) {
            printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
            break;
        }

        int WidthValue = 1024;
        int HeightValue = 768;
        int ExposureTimeValue = 40000;
        int GainValue = 60;

        // set trigger mode as off
        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);

        // start grab image
        //
        MV_CC_SetExposureTime(handle, ExposureTimeValue);
        MV_CC_SetGain(handle, GainValue);
        // MV_CC_RegisterImageCallBackEx(handle,ImageCallBackEx,NULL);
        nRet = MV_CC_StartGrabbing(handle);

        MVCC_INTVALUE stIntvalue = { 0 };
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stIntvalue);
        int nBufSize = stIntvalue.nCurValue;
        nBufSize = MAX_BUF_SIZE;

        unsigned char* pFrameBuf = NULL;
        pFrameBuf = (unsigned char*)malloc(nBufSize);

        MV_FRAME_OUT_INFO_EX stInfo;
        memset(&stInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));

        while (true) {
            nRet = MV_CC_GetImageForBGR(handle, pFrameBuf, nBufSize, &stInfo, 1000);
            int width = stInfo.nWidth;
            int height = stInfo.nHeight;

            if (stInfo.enPixelType == PixelType_Gvsp_BGR8_Packed) {
                Mat pImg(height, width, CV_8UC3, pFrameBuf);
                cv::resize(pImg, pImg, cv::Size(), 0.75, 0.75);

                imshow("Image1", pImg);
                waitKey(1);
            }
        }

    } while (0);
    return 0;
}
