/*
 * faceDetection.cpp
 *
 *  Created on: Feb 16, 2014
 *      Author: Zhichao Zhong
 */

 #include "opencv2/objdetect/objdetect.hpp"
 #include "opencv2/highgui/highgui.hpp"
 #include "opencv2/imgproc/imgproc.hpp"
 #include "Hungarian.h"
 #include "BipartiteGraph.h"
 #include <iostream>
 #include <stdio.h>
 #include "KlmFilter.h"
 #include <utility>
 using namespace std;
 using namespace cv;

 /** Define a type -- track**/
 class track
 {
 public:
	 Rect bBox; //the bounding box
	 Point2d velocity; // the velocity of the object
	 int prediction; // a counter whether this track is predicted or not
	 int appearance; // a counter for the appearance of a new track
	 bool isActive; // whether this track is active or not
	 bool isActive2; // whether this track should be shown or not.
	 Mat errorCovariance; // error covariance
	 Mat errorCovariance_size;
	// unsigned int index; // the index of the track
 };

 /** Function Headers */
 void faceDetection( Mat frame, vector< vector<Rect> > & facesi );
 void klmPredict(track km1State, track & kState);
 void klmPredict_size(track km1State, track & kState);
 void klmCorrect(track& kState, track kMeasurement);
 void klmCorrect_size(track& kState, track kMeasurement);

 vector<EID> MunkresBox(vector<double> vec, int nrows);
 vector<int> dataAssociation(vector<track> tracksi, vector<Rect> facesi);
 void drawTracks(vector<vector<track> > tracks, vector<vector<Rect> > faces, vector<vector<int> > connections);
 /** Global variables */
 String face_cascade_name = "haarcascade_frontalface_alt.xml";
 String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
 CascadeClassifier face_cascade;
 CascadeClassifier eyes_cascade;
 string window_name = "Capture - Face detection";
 RNG rng(12345);




 /** Define a kalman filter**/


 /** @function main */
 int main( int argc, const char** argv )
 {
	 FILE	* pFile;
	 pFile=fopen("facesDetection.txt", "w");
	 VideoCapture capture;
	 VideoWriter DetectionVideo;
	 VideoWriter TrackingVideo;
	 vector<Mat> frame;
	 vector< vector<Rect>  >  faces;
	 vector<vector<int> > connections;
	 int occlusionSize=12;
	 int occlusionSize2=5;
	 //set a track_new
	 track track_new;
	 track_new.errorCovariance=(Mat_<double>(4,4)); setIdentity(track_new.errorCovariance, Scalar::all(0.1));
	 track_new.errorCovariance_size=(Mat_<double>(2,2)); setIdentity(track_new.errorCovariance_size, Scalar::all(0.1));
	 track_new.isActive=true;
	 track_new.isActive2=false;
	 track_new.prediction=0;
	 track_new.appearance=0;
	 track_new.velocity.x=0;
	 track_new.velocity.y=0;

	 //-- 1. Load the cascades
	 if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n");/* return -1;*/ };
	 if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };

	 //-- 2. Read the video stream
	 capture = VideoCapture( argv [1]);
	 if( /*argc != 2 ||*/!capture.isOpened())
	 {
       printf( "No image data \n" );
       return -1;
	 }


   if( capture.isOpened() )
   {
	   vector<Scalar> colors;
	   colors.push_back(Scalar(0,0,255));colors.push_back(Scalar(0,255,0));
	   colors.push_back(Scalar(255,0,0));colors.push_back(Scalar(255,255,0));
	   colors.push_back(Scalar(0,255,255));colors.push_back(Scalar(255,0,255));
	   vector<track> tracks_empty;
	   vector<vector<track> > tracks; //the retoration of tracks
	   tracks.push_back(tracks_empty); //insert an empty tracks at the first place
	   Mat frame1,frame2;
	   capture>> frame1;
	   frame2=frame1.clone();
	   DetectionVideo.open("detection.avi", capture.get(CV_CAP_PROP_FOURCC), capture.get(CV_CAP_PROP_FPS), Size(capture.get(CV_CAP_PROP_FRAME_WIDTH),capture.get(CV_CAP_PROP_FRAME_HEIGHT)));
	   TrackingVideo.open("tracking.avi", capture.get(CV_CAP_PROP_FOURCC), capture.get(CV_CAP_PROP_FPS), Size(capture.get(CV_CAP_PROP_FRAME_WIDTH),capture.get(CV_CAP_PROP_FRAME_HEIGHT)));


	   for (int i=1; ; i++)
	   {
		   //cout<<i<<endl;
		   Mat frame_temp;
		   capture>> frame_temp;
		   tracks.push_back(tracks_empty);//allocate an empty track vecto to tracks at frame-i



		   // Apply the classifier to the frame
		   if( !(frame_temp.empty() ))
       	   	   {
			   imshow ("video", frame_temp);
			   frame.push_back(frame_temp);
			   faceDetection( frame[i-1], faces );

			   Mat fr;
			   fr=frame_temp.clone();

			   //show the detection result
			   for( size_t k=0; k<faces[i-1].size(); k++)
			   {
				   int c=(k+1)%6;
	   	   		   rectangle(fr,faces[i-1][k], colors[c], 1, 8, 0 );
	   	   		   if(i<64)
	   	   			   rectangle(frame1,faces[i-1][k], colors[c], 1, 8, 0);
			   }

			   imshow( "detection", fr);
			   DetectionVideo.write(fr);
			   /*char name1[50];
			   int n1=sprintf(name1, "%d_Detection.jpg",i);
			   imwrite(name1,fr);*/


			   /** Solving the initializing problem**/
			   //if there isn't detection, we just copy the tracks from frame-i-1 to frame-i
			   //then we continue to the next frame;
			   if(faces[i-1].empty())
			   {
				   tracks[i]=tracks[i-1];
				   //show the result
				   Mat fr2;
				   fr2=frame[i-1].clone();
				   for( size_t k=0; k<tracks[i].size(); k++)
					   if (tracks[i][k].isActive&&tracks[i][k].isActive2)
					   {
						   int c=(k+1)%6;
						   rectangle(fr2,tracks[i][k].bBox, colors[c], 1, 8, 0 );
					   }
				   imshow( "tracking", fr2);
				   cout<<"no detection"<<endl;
			   }
			   else
			   {
			   //if there is a face detection now and if the tracks are still empty, then we assign the 1st face detection as the 1st tracks;
			   //then we break and go to the next frame
			   if(tracks[i-1].empty())
			   {
				   for(size_t k=0; k<faces[i-1].size();k++)
				   {
					   track_new.bBox=faces[i-1][k];
					   tracks.push_back(tracks_empty);
					   tracks[i].push_back(track_new);
				   }
				   continue;
			   }

			   //cout<<tracks[i-1].size()<<" "<<tracks[i].size()<<endl;

			   /**Time Update/Prediction from frame-i-1 to frame-i**/
			   //Predict from i-1 frame to i frame for each track in i-1 frame.

			   for (size_t j=0; j<tracks[i-1].size();j++)
			   {
				   if(tracks[i-1][j].isActive)
				   {
					   track track_temp=tracks[i-1][j];
					   klmPredict(tracks[i-1][j], track_temp);
					   klmPredict_size(tracks[i-1][j], track_temp);
					   tracks[i].push_back(track_temp);
				   }
				   else
					   tracks[i].push_back(tracks[i-1][j]);
			   }

			   //cout<<tracks[i-1].size()<<" "<<tracks[i].size()<<endl;

			   /**Data association between Tracks_predicted at frame-i and Detections**/
			   // write all the active tracks in frame-i into a tempurary vector
			   vector<track> trackS_temp;
			   for(size_t k=0;k<tracks[i].size();k++)
				   if(tracks[i][k].isActive)
					   trackS_temp.push_back(tracks[i][k]);

			   //cout<<trackS_temp.size()<<endl;
			   // give the correspondence of each track in tracks_temp
			   vector<int> connection=dataAssociation(trackS_temp,faces[i-1]);
			   connections.push_back(connection);
			  for(size_t k=0; k<connection.size();k++)
				   cout<<connection[k]<<" ";cout<<endl;

			   /**Measurement update between Predictions and Detections**/
			   vector<int>::iterator it=connection.begin();
			   //a loop go throuhg all the predicted tracks in frame i;
			   for (size_t k=0; k<tracks[i].size();k++)
			   {
				   //if the track isn't detected for longer than 4 frames, then it is disabled/unactivated.
				if(tracks[i][k].prediction>occlusionSize) tracks[i][k].isActive=false;
				/**   cout<< tracks[i][k].prediction<<" "<<(int)tracks[i][k].isActive<<" ";**/
				   //for all the active tracks (predicted) in frame-i
				   if(tracks[i][k].isActive)
				   {
					   //update the k-th track with corresponding detection in faces[i-1]
					   if((*it)>=0)
					   {
						   track track_temp=track_new;
						   track_temp.velocity.x=faces[i-1][*it].x-tracks[i-1][k].bBox.x;
						   track_temp.velocity.y=faces[i-1][*it].y-tracks[i-1][k].bBox.y;
						   track_temp.bBox=faces[i-1][*it];
						   //assign the velocity
						   klmCorrect(tracks[i][k], track_temp);
						   klmCorrect_size(tracks[i][k], track_temp);
						   tracks[i][k].appearance++;
						   if(tracks[i][k].prediction>0) tracks[i][k].prediction=0;
					   }
					   //if it is not detected, the k-th track is not updated, but the prediction will ++
					   else
					   {
						   if(*it==-1) tracks[i][k].prediction ++;
					   }
					   it ++; //iterator go to the next position;
					   if(tracks[i][k].appearance>occlusionSize2)
						   tracks[i][k].isActive2=true;
				   }

			   }

			   // if there are some new detections and iterator will != connection.end(); then we assign new tracks
			   while(it!= connection.end())
			   {
				   for(int j=0; j<(int)faces[i-1].size();j++) // for each detection in faces[i-1]
				   {
					   bool isNew=true;
					   for(size_t n=0; n<connection.size();n++)
						   if (j==connection[n]) isNew=false; // if j cannot be found in connection, then faces[i-1][j] is a new detection;
					   if (isNew)  // add this new detection in tracks[i]
					   {
						   track_new.bBox=faces[i-1][j];
						   tracks[i].push_back(track_new);
					   }
				   }
				   it ++;
			   }


			   //show the result
			   Mat frm2;
			   frm2=frame[i-1].clone();
			   for( size_t k=0; k<tracks[i].size(); k++)
				   if (tracks[i][k].isActive&&tracks[i][k].isActive2)
				   {
					   int c=(k+1)%6;
					   rectangle(frm2,tracks[i][k].bBox, colors[c], 1, 8, 0 );
					   if(i<64)
						   rectangle(frame2,tracks[i][k].bBox, colors[c], 1, 8, 0 );
				   }


			   imshow( "tracking", frm2);
			   /*char name2[50];
			   int n2=sprintf(name2, "%d_Tracking.jpg",i);
			   imwrite(name2,frm2);*/
			   TrackingVideo.write(frm2);
       	   	   }
       	   	   }
    	 else
    	 { printf(" --(!) No captured frame -- Break!"); break; }
    	 int c = waitKey(10);
    	 if( (char)c == 'q' ) { break; }
      }
	   drawTracks(tracks, faces, connections);
	   imwrite("detectionall.jpg",frame1);
	   imwrite("trackingall.jpg", frame2);
   }

   return 0;
 }

/** @function detectAndDisplay */
void faceDetection( Mat frame, vector< vector<Rect> > & faces)
{
  vector<Rect> facesi;
  Mat frame_gray;

  cvtColor( frame, frame_gray, CV_BGR2GRAY );
  equalizeHist( frame_gray, frame_gray );
  //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, facesi, 1.1, 1, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

  faces.push_back(facesi);
 }

void klmPredict_size(track km1State, track & kState)
{
//set the kalman filter and assigning all the matrix values
	Mat A= (Mat_<double>(2,2)<< 1,0,0,1);
	Mat B=Mat::zeros(2,2,CV_64F);
	Mat Q=Mat::eye(2,2,CV_64F), R=Mat::eye(2,2,CV_64F);
	Mat H=Mat::eye(2,2,CV_64F);
	setIdentity(Q,Scalar::all(1e-3));
	setIdentity(R,Scalar::all(1e-1));
	KlmFilter klm(A,B,H,R,Q);

	//write the previous (k-1) corrected state into a Mat
	Mat x_km1(2,1,CV_64F);
	x_km1.at<double>(0,0)=km1State.bBox.width;
	x_km1.at<double>(1,0)=km1State.bBox.height;


	//set the control vecto as (0, 0, 0, 0)^T
	Mat u=Mat::zeros(2,1,CV_64F);

	Mat P=km1State.errorCovariance_size;

	// do the predict procedure
	klm.predict(x_km1,u,P);

	kState.bBox.width=x_km1.at<double>(0,0);
	kState.bBox.height=x_km1.at<double>(1,0);
	kState.errorCovariance_size=P;
}
void klmPredict(track km1State, track & kState)
{
	//set the kalman filter and assigning all the matrix values
	Mat A= (Mat_<double>(4,4)<< 1,0,1,0,  0,1,0,1,  0,0,1,0,  0,0,0,1);
	Mat B=Mat::zeros(4,4,CV_64F);
	Mat Q=Mat::eye(4,4,CV_64F), R=Mat::eye(4,4,CV_64F);
	Mat H=Mat::eye(4,4,CV_64F);
	setIdentity(Q,Scalar::all(1e-3));
	setIdentity(R,Scalar::all(1e-1));
	KlmFilter klm(A,B,H,R,Q);

	//write the previous (k-1) corrected state into a Mat
	Mat x_km1(4,1,CV_64F);
	x_km1.at<double>(0,0)=km1State.bBox.x;
	x_km1.at<double>(1,0)=km1State.bBox.y;
	x_km1.at<double>(2,0)=km1State.velocity.x;
	x_km1.at<double>(3,0)=km1State.velocity.y;

	//set the control vecto as (0, 0, 0, 0)^T
	Mat u=Mat::zeros(4,1,CV_64F);

	Mat P=km1State.errorCovariance;

	// do the predict procedure
	klm.predict(x_km1,u,P);

	kState.bBox.x=x_km1.at<double>(0,0);
	kState.bBox.y=x_km1.at<double>(1,0);
	kState.velocity.x=x_km1.at<double>(2,0);
	kState.velocity.y=x_km1.at<double>(3,0);
	kState.errorCovariance=P;
}

void klmCorrect_size(track& kState, track kMeasurement)
{
	//set the kalman filter and assigning all the matrix values
		Mat A= (Mat_<double>(2,2)<< 1,0,0,1);
		Mat B=Mat::zeros(2,2,CV_64F);
		Mat Q=Mat::eye(2,2,CV_64F), R=Mat::eye(2,2,CV_64F);
		Mat H=Mat::eye(2,2,CV_64F);
		setIdentity(Q,Scalar::all(1e-3));
		setIdentity(R,Scalar::all(1e-1));
		KlmFilter klm(A,B,H,R,Q);


	//write the current (k) measured state into a Mat
	Mat z(2,1,CV_64F);
	z.at<double>(0,0)=kMeasurement.bBox.width;
	z.at<double>(1,0)=kMeasurement.bBox.height;

	Mat x_k(2,1,CV_64F);
	//write the predicted state into a Mat
	x_k.at<double>(0,0)=kState.bBox.width;
	x_k.at<double>(1,0)=kState.bBox.height;

	Mat P=kState.errorCovariance_size;

	Mat I=Mat::eye(2,2,CV_64F);
	//compute the corrected state and errorCovariance
	klm.correct(x_k, z, P, I);

	kState.bBox.width=x_k.at<double>(0,0);
	kState.bBox.height=x_k.at<double>(1,0);

	kState.errorCovariance_size=P;

}
void klmCorrect(track& kState, track kMeasurement)
{
	//set the kalman filter and assigning all the matrix values
	Mat A= (Mat_<double>(4,4)<< 1,0,1,0,  0,1,0,1,  0,0,1,0,  0,0,0,1);
	Mat B=Mat::zeros(4,4,CV_64F);
	Mat Q=Mat::eye(4,4,CV_64F), R=Mat::eye(4,4,CV_64F);
	Mat H=Mat::eye(4,4,CV_64F);
	setIdentity(Q,Scalar::all(1e-3));
	setIdentity(R,Scalar::all(1e-1));
	KlmFilter klm(A,B,H,R,Q);

	//write the current (k) measured state into a Mat
	Mat z(4,1,CV_64F);
	z.at<double>(0,0)=kMeasurement.bBox.x;
	z.at<double>(1,0)=kMeasurement.bBox.y;
	z.at<double>(2,0)=kMeasurement.velocity.x;
	z.at<double>(3,0)=kMeasurement.velocity.y;

	Mat x_k(4,1,CV_64F);
	//write the predicted state into a Mat
	x_k.at<double>(0,0)=kState.bBox.x;
	x_k.at<double>(1,0)=kState.bBox.y;
	x_k.at<double>(2,0)=kState.velocity.x;
	x_k.at<double>(3,0)=kState.velocity.y;
	Mat P=kState.errorCovariance;

	Mat I=Mat::eye(4,4,CV_64F);
	//compute the corrected state and errorCovariance
	klm.correct(x_k, z, P, I);

	kState.bBox.x=x_k.at<double>(0,0);
	kState.bBox.y=x_k.at<double>(1,0);
	kState.velocity.x=x_k.at<double>(2,0);
	kState.velocity.y=x_k.at<double>(3,0);
	kState.errorCovariance=P;

}

vector<EID>
MunkresBox(vector<double> vec, size_t nrows, size_t ncols)
{
	Matrix m;
	vector<double>::iterator it;
	it=vec.begin();
	m.resize(nrows);
	for(size_t i=0; i<nrows; i++)
		m[i].resize(ncols);
	for(size_t i=0; i<nrows; i++)
		for(size_t j=0; j<ncols; j++)
			m[i][j].SetWeight(*it++);
	//define a bipartite graph
	BipartiteGraph bg(m);

	//run Hungarian method
	vector<VID> S;
	vector<VID> T;
	vector<VID> N;
	vector<EID> EG;
	vector<EID> M;
	Hungarian h(bg);
	h.HungarianAlgo(bg,S,T,N,EG,M);

	//h.DisplayData(M);
	//h.DisplayMatrix(m);
	return M;
}


/**return a vector in which the correspondence of prio-tracks in [k] are given w.r.t. to faces**/
vector<int> dataAssociation(vector<track> tracksi, vector<Rect> facesi)
{
	vector<double> overlap_vector;
	vector<EID> matchResult;
	vector<int> result;

	if (facesi.size()==tracksi.size() ||facesi.size()<tracksi.size())
	{
		// calculate the overlapped proportion and write into a vector e.g. (0.9, 0, 0, 0.8)
		for (int j=0; j<(int)facesi.size(); j++)
			for (int k=0; k<(int)tracksi.size(); k++)
			{
				Rect olArea;
				olArea=tracksi[k].bBox&facesi[j];
				double olProp=(double)olArea.area()/tracksi[k].bBox.area();///(double)tracksi[k].bBox.area();
				overlap_vector.push_back(olProp);
			}
		matchResult=MunkresBox(overlap_vector, facesi.size(), tracksi.size()); //return (index-face, index-track)
		for(size_t k=0; k<matchResult.size(); k++)
		{
			bool exist=false; // to prevent the case of (0, 0) (1, 0)
			for(size_t j=0; j<matchResult.size();j++)
			{
				if(matchResult[j].second==k) exist=true;
			}
			if(!exist)
			{
				bool zeroOverlap=true;
				for(size_t n=0; n<tracksi.size(); n++)
					if(overlap_vector[k*tracksi.size()+n]>0.2)
						zeroOverlap=false;
				if(zeroOverlap)
					{
					matchResult[k].second=k;
					}
			}
		}

		//go through all the tracks, and give the corresponding index of detection order
		for(size_t k=0; k<tracksi.size(); k++)
		{
			bool isDetected=false;
			for(size_t j=0; j<matchResult.size(); j++)
				if(matchResult[j].second==k)
				{
					result.push_back((int) matchResult[j].first);
					isDetected=true;
				}
			if(!isDetected)
			{
				result.push_back(-1); //means that this track is not detected in this k frame, the track.prediction ++
			}
		}

	}

	if(facesi.size()>tracksi.size())
	{
		/**for(size_t k=0; k< facesi.size();k++)
			cout<<facesi[k].x<<" "<<facesi[k].y<<" "<<facesi[k].height<<" "<<facesi[k].width<<endl;**/
		// calculate the overlapped proportion and write into a vector e.g. (0.9, 0, 0, 0.8)
		for (int j=0; j<(int)tracksi.size(); j++)
			for (int k=0; k<(int)facesi.size(); k++)
			{
				Rect olArea;
				olArea=tracksi[j].bBox&facesi[k];
				double olProp=(double)olArea.area();///(double)facesi[k].area();
				overlap_vector.push_back(olProp);
			}
		/**for(size_t k=0; k<overlap_vector.size();k++)
			cout<<overlap_vector[k]<<" ";
		cout<<endl;**/
		matchResult=MunkresBox(overlap_vector, tracksi.size(), facesi.size());
		for (size_t k=0; k<tracksi.size();k++)
		{
			result.push_back((int)matchResult[k].second);
		}

		for(size_t k=tracksi.size();k<facesi.size();k++)
		{
			result.push_back(-2); //means that a new track appears
		}
	}
	return result;
}

void drawTracks(vector<vector<track> > tracks, vector<vector<Rect> > faces, vector<vector<int> > connections)
{
	int w=20*(int)tracks.size()+100;
	//set a background image
	Mat image = Mat::zeros( 300, w, CV_8UC3 );

	//set iterators for each vector
	vector<vector<track> >::iterator it_frame=tracks.begin();
	vector<vector<Rect> >::iterator it_fr2=faces.begin();
	vector<vector<int> >::iterator it_fr3=connections.begin();

	int thickness=-1; int linetype =8;
	int i_fr=1;

	for(it_frame=it_frame+1;it_frame<tracks.end();it_frame++)
	{
		int i_tr=1;
		for(vector<track>::iterator it_track=(*it_frame).begin(); it_track <(*it_frame).end(); it_track ++)
		{
		circle(image, Point(i_fr*20+50, i_tr*30+50), 2, Scalar(0,0,255), thickness, linetype);
		i_tr++;
		}

		int i_fa=1;
		for(vector<Rect>::iterator it_face=(*it_fr2).begin(); it_face <(*it_fr2).end(); it_face ++)
		{
		circle(image, Point(i_fr*20+40, i_fa*30+50), 2, Scalar(0,200,0), thickness, linetype);
		i_fa++;
		}

		for(vector<int>::iterator it_line=(*it_fr3).begin(); it_line <(*it_fr3).end(); it_line ++)
		{
			if((*it_line)>0)
			{
				line(image, Point(i_fr*20+40, (*it_line+1)*30+50), Point(i_fr*20+50, (*it_line+1)*30+50), Scalar(255,255,255), 1, 8);
			}


		}
		i_fr ++;
	}
	imwrite("trackingresult.jpg", image);
}
