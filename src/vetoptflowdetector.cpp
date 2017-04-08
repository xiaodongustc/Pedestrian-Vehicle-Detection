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
* \file vetoptflowdetector.cpp
* \author [Zeyu Zhang]
* \version [0.1]
* \date 2017-03-24
*/

#include "vetoptflowdetector.h"

using namespace std;
using namespace cv;

VetOptFlowDetector::VetOptFlowDetector()
{
	is_ready_ = false;
	pyrLK_max_corners_ = 200;
	_makeColorPalette();
}

VetOptFlowDetector::~VetOptFlowDetector()
{
	// ...
}

void VetOptFlowDetector::detect(const Mat &frame, vector<VetROI> &rois)
{
	//...
}

void VetOptFlowDetector::detect(Mat &frame, vector<VetROI> &rois)
{
	OptFlowPyrLKResult result;

	if( !_optFlowPyrLK(frame, result) )
		return;

	vector<Point2f> &prev_points = result.prev_points_;
	vector<Point2f> &next_points = result.next_points_;
	vector<uchar> &state = result.state_;
	vector<float> &err = result.err_;

	vector<vector<Point> > roi(2, vector<Point>());

	for(int i = 0; i < (int)state.size();i++)
	{
		Point p = Point((int)prev_points[i].x, (int)prev_points[i].y);
		Point q = Point((int)next_points[i].x, (int)next_points[i].y);

		if(state[i] != 0)
		{
			double delta_x = p.x - q.x;
			double delta_y = p.y - q.y;

			double angle = atan2( (double)delta_y, (double)delta_x );
			double hypotenuse = sqrt( delta_y * delta_y + delta_x * delta_x );
			double distance, angle_in_degree;

			q.x = (int) (p.x - 3 * hypotenuse * cos(angle));
			q.y = (int) (p.y - 3 * hypotenuse * sin(angle));

			distance = _calcDistance(p, q);
			angle_in_degree = _calcAngleInDegree(p, q);

			if( !(p.x > frame.cols / 2 && angle_in_degree >= 135 && angle_in_degree <= 225
				|| p.x <= frame.cols / 2 && angle_in_degree >= 0 && angle_in_degree <= 45) )
				continue;

			if( (p.x <= frame.cols / 2 && distance < 10)
				|| (p.x > frame.cols / 2 && distance < 10) )
				continue;

			if(p.x <= frame.cols / 2)
				roi[0].push_back(p);
			else
				roi[1].push_back(p);

			line(frame, p, q, Scalar(0, 0, 255));

			p.x = (int) (q.x + 9 * cos(angle + PI / 4));
			p.y = (int) (q.y + 9 * sin(angle + PI / 4));
			line(frame, p, q, Scalar(0, 0, 255));

			p.x = (int) (q.x + 9 * cos(angle - PI / 4));
			p.y = (int) (q.y + 9 * sin(angle - PI / 4));
			line(frame, p, q, Scalar(0, 0, 255));
		}
	}

	if(roi[0].size() > 5) 
		rois.push_back( VetROI(_findBoundingRect(roi[0]), "Warn") );
	if(roi[1].size() > 5)
		rois.push_back( VetROI(_findBoundingRect(roi[1]), "Warn") );
}

bool VetOptFlowDetector::optFlowFarneback(const Mat &frame, Mat &flow)
{
	Mat gray_image;
	Mat optical_flow;

	cvtColor(frame, gray_image, CV_BGR2GRAY);

	if( !is_ready_ )
	{
		prev_gray_img_ = gray_image;
		is_ready_ = true;
		return false; 
	}

	calcOpticalFlowFarneback(prev_gray_img_, gray_image, optical_flow, 0.5,
		3, 15, 3, 5, 1.2, 0);

	_motion2color(optical_flow, flow);

	prev_gray_img_ = gray_image;

	return true;
}

bool VetOptFlowDetector::_optFlowPyrLK(const cv::Mat &frame, OptFlowPyrLKResult &result)
{
	Mat cur_gray_image;

	cvtColor(frame, cur_gray_image, CV_BGR2GRAY);

	if( !is_ready_ )
	{
		cur_gray_image.copyTo(prev_gray_img_);
		is_ready_ = true;
		return false;
	}

	vector<Point2f> &prev_points = result.prev_points_;
	vector<Point2f> &next_points = result.next_points_;
	vector<uchar> &state = result.state_;
	vector<float> &err = result.err_;

	Mat mask(frame.size(), CV_8UC1, Scalar(0));
	vector<vector<Point> > contours;
	vector<Point> _rois;
	
	for(int i = 0; i <= frame.cols / 4; i++)
	{
		_rois.push_back( Point(i, frame.rows / 2) );
		_rois.push_back( Point(i, frame.rows) );
	}
	for(int i = frame.rows / 2; i <= frame.rows; i++)
	{
		_rois.push_back( Point(0, i) );
		_rois.push_back( Point(frame.cols / 4, i) );
	}

	contours.push_back(_rois);
	_rois.clear();

	for(int i = frame.cols * 3 / 4; i <= frame.cols; i++)
	{
		_rois.push_back( Point(i, frame.rows / 2) );
		_rois.push_back( Point(i, frame.rows) );
	}
	for(int i = frame.rows / 2; i <= frame.rows; i++)
	{
		_rois.push_back( Point(frame.cols * 3 / 4, i) );
		_rois.push_back( Point(frame.cols, i) );
	}
	contours.push_back(_rois);
	drawContours(mask, contours, -1, Scalar(255));

	goodFeaturesToTrack(prev_gray_img_, prev_points, pyrLK_max_corners_, 
		0.001, 10, mask, 3, false, 0.04);

	cornerSubPix(prev_gray_img_, prev_points, Size(10,10), Size(-1,-1), 
		TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));

	calcOpticalFlowPyrLK(prev_gray_img_, cur_gray_image, prev_points, 
		next_points, state, err, Size(31,31), 3);

	cur_gray_image.copyTo(prev_gray_img_);

	return true;	
}

void VetOptFlowDetector::_makeColorPalette()
{
    int RY = 15;
    int YG = 6;
    int GC = 4;
    int CB = 11;
    int BM = 13;
    int MR = 6;

	for (int i = 0; i < RY; i++)
		color_palette_.push_back( Scalar(255, 255 * i / RY, 0) );

    for (int i = 0; i < YG; i++)
    	color_palette_.push_back( Scalar(255 - 255*i/YG, 255, 0) );

    for (int i = 0; i < GC; i++)
    	color_palette_.push_back( Scalar(0, 255, 255 * i / GC) );

    for (int i = 0; i < CB; i++)
    	color_palette_.push_back( Scalar(0, 255 - 255*i/CB, 255) );

    for (int i = 0; i < BM; i++)
    	color_palette_.push_back( Scalar(255 * i / BM, 0, 255) );

    for (int i = 0; i < MR; i++)
    	color_palette_.push_back( Scalar(255, 0, 255 - 255*i/MR) );
}

void VetOptFlowDetector::_motion2color(Mat &flow, Mat &color)
{
	if (color.empty())
		color.create(flow.rows, flow.cols, CV_8UC3);

	// determine motion range:
    float maxrad = -1;

	// Find max flow to normalize fx and fy
	for (int i= 0; i < flow.rows; ++i) 
	{
		for (int j = 0; j < flow.cols; ++j) 
		{
			Vec2f flow_at_point = flow.at<Vec2f>(i, j);
			float fx = flow_at_point[0];
			float fy = flow_at_point[1];
			if ((fabs(fx) >  UNKNOWN_FLOW_THRESH) || (fabs(fy) >  UNKNOWN_FLOW_THRESH))
				continue;
			float rad = sqrt(fx * fx + fy * fy);
			maxrad = maxrad > rad ? maxrad : rad;
		}
	}

	for (int i= 0; i < flow.rows; ++i) 
	{
		for (int j = 0; j < flow.cols; ++j) 
		{
			uchar *data = color.data + color.step[0] * i + color.step[1] * j;
			Vec2f flow_at_point = flow.at<Vec2f>(i, j);

			float fx = flow_at_point[0] / maxrad;
			float fy = flow_at_point[1] / maxrad;
			if ((fabs(fx) >  UNKNOWN_FLOW_THRESH) || (fabs(fy) >  UNKNOWN_FLOW_THRESH))
			{
				data[0] = data[1] = data[2] = 0;
				continue;
			}
			float rad = sqrt(fx * fx + fy * fy);

			float angle = atan2(-fy, -fx) / CV_PI;
			float fk = (angle + 1.0) / 2.0 * (color_palette_.size()-1);
			int k0 = (int)fk;
			int k1 = (k0 + 1) % color_palette_.size();
			float f = fk - k0;
			//f = 0; // uncomment to see original color wheel

			for (int b = 0; b < 3; b++) 
			{
				float col0 = color_palette_[k0][b] / 255.0;
				float col1 = color_palette_[k1][b] / 255.0;
				float col = (1 - f) * col0 + f * col1;
				if (rad <= 1)
					col = 1 - rad * (1 - col); // increase saturation with radius
				else
					col *= .75; // out of range
				data[2 - b] = (int)(255.0 * col);
			}
		}
	}
}

double VetOptFlowDetector::_calcDistance(const Point &a, const Point &b)    
{    
    double distance;

    distance = powf((a.x - b.x),2) + powf((a.y - b.y),2);    
    distance = sqrtf(distance);    
    
    return distance;    
}

double VetOptFlowDetector::_calcAngleInDegree(const Point &a, const Point &b)
{
	int delta_x = b.x - a.x;
	int delta_y = b.y - a.y;
	double delta_hypotenuse = sqrt(delta_x * delta_x + delta_y * delta_y);

	double cos_value = delta_x / delta_hypotenuse;

	double angle = acos(cos_value) * 180 / 3.1415;

	// In opencv, y axis is up-side-down
	if(delta_y > 0)
		angle = 360 - angle;

	return angle;
}

Rect VetOptFlowDetector::_findBoundingRect(const vector<Point> &src)
{
	Point tl(9999, 9999), br(0, 0);

	vector<Point>::const_iterator iter = src.begin();
	vector<Point>::const_iterator end = src.end();

	while(iter != end)
	{
		if(iter->x < tl.x)
			tl.x = iter->x;
		
		if(iter->y < tl.y)
			tl.y = iter->y;
		
		if(iter->x > br.x)
			br.x = iter->x;

		if(iter->y > br.y)
			br.y = iter->y;

		iter++;
	}

	return Rect(tl, br);
}