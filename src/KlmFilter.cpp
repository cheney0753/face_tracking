/*
 * KlmFilter.cpp
 *
 *  Created on: Mar 5, 2014
 *      Author: chao
 */

#include "KlmFilter.h"

KlmFilter::KlmFilter(Mat A,Mat B,Mat H,Mat R,Mat Q) {
	// TODO Auto-generated constructor stub
	controlMatrix=A;
	deliverMatrx=B;
	measureMatrix=H;
	measErrCov=R;
	procErrCov=Q;

}

KlmFilter::~KlmFilter() {
	// TODO Auto-generated destructor stub
}

void
KlmFilter::predict(Mat& x, Mat& u, Mat& P)
{
	vector<Mat> output;
	Mat x_p=controlMatrix*x+deliverMatrx*u; // equation 1.9
	Mat P_p=controlMatrix*P*controlMatrix.t()+procErrCov;
	x=x_p; P=P_p;
}

void
KlmFilter::correct(Mat& x, Mat z, Mat& P, Mat I)
{
	vector<Mat> output;
	Mat k=P*measureMatrix.t()*(measureMatrix*P*measureMatrix.t()+measErrCov).inv();
	Mat x_upd=x+k*(z-measureMatrix*x);
	Mat P_upd=(I-k*measureMatrix)*P;
	P=P_upd;
	x=x_upd;

}


