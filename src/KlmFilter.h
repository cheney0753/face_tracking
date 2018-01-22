/*
 * KlmFilter.h
 *
 *  Created on: Mar 5, 2014
 *      Author: chao
 */

#ifndef KLMFILTER_H_
#define KLMFILTER_H_

#include "opencv2/opencv.hpp"
using namespace cv;

class KlmFilter{
	Mat controlMatrix; // A
	Mat deliverMatrx;// B;
	Mat measureMatrix; // H =Mat::eye(2,2);
	Mat measErrCov; // R measurement error covariance
	Mat procErrCov; // Q Process error covariance

public:
	KlmFilter(Mat A,Mat B,Mat H,Mat R,Mat Q);
	~KlmFilter();

	void predict(Mat& x, Mat& u, Mat& P) ;// the state_post includes x_km1,P_km1 , u_km1
												//for Table1.1
	void correct(Mat& x, Mat z, Mat& P, Mat I)  ;//the state_prio includes x_k, p_k
													// and z_k for Table 1.2

};

#endif /* KLMFILTER_H_ */
