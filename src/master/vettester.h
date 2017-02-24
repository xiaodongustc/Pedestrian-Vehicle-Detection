/**
* [
*  Copyright (c) 2016 by Vehicle-Eye Technology
*  ALL RIGHTS RESERVED.
*
*  The software and information contained herein are proprietary to,  and comprise
*  valuable trade secrets of, Vehicle-Eye Technology.  Disclosure of the software
*  and information will materially harm the business of Vehicle-Eye Technology.
*  This software is furnished pursuant to a written development agreement and may
*  be used, copied, transmitted, and stored only in accordance with the terms of
*  such agreement and with the inclusion of the above copyright notice.  This
*  software and information or any other copies thereof may not be provided or
*  otherwise made available to any other party.
* ]
*/


/*!
* \file vettester.h
* \author [Zeyu Zhang]
* \version [0.1]
* \date 2016-02-22
*/

#ifndef VETTESTER_H
#define VETTESTER_H

#include "vetdetectorcontext.h"
#include "vetdetectorstrategy.h"
#include "vetimageprocessor.h"
#include "vetconcurrentqueue.h"
#include "vetfastvideocapture.h"

#include <opencv2/opencv.hpp>
 
#include <iostream>
#include <algorithm>

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Tester ID Number
 */ 
#define CAR_HAAR_TESTER 			0
#define NMS_TESTER 					1
#define VIDEO_PLAYER_TESTER 		2
#define FAST_VIDEO_PLAYER_TESTER	3
#define FAST_CAR_HAAR_TESTER		4

/**
 * Tester Video File Path
 */
#define TEST_VIDEO_PATH "../../video/HPIM0026.mov"
#define TEST_TRIMED_VIDEO_PATH "../../video/HPIM0026_Trimed.mov"

/**
 * Tester Picture File Path
 */
#define PIC_NMS_PATH "../../image-dataset/nms.jpg"

/**
 * Tester Functions
 */
void carHaarTester(std::string video_path);
void NMSTester(std::string image_path);
void videoPlayerTester(std::string video_path);
void fastVideoPlayerTester(std::string video_path);
void fastCarHaarTester(std::string video_path);


class VetTester
{
public:
	VetTester(int tester_id);
	~VetTester();

public:
	void run(std::string file_path);
	void setTesterID(int tester_id);

private:
	int tester_id_;
};

#endif