#include "stdafx.h" 
#include "cv.h" 
#include "highgui.h" 
#include <time.h> 
#include <stdio.h> 
 
// Various tracking parameters (in seconds) 
const double MHI_DURATION = 1; 
const double MAX_TIME_DELTA = 0.5; 
const double MIN_TIME_DELTA = 0.05; 
 
// Number of cyclic frame buffer used for motion detection 
const int N = 4; 
 
// ring image buffer 
IplImage **buf = 0; 
int last = 0; 
 
// Flags to trigger other cameras 
int cam1left=0; 
int cam1right=0; 
int cam2left=0; 
int cam2right=0; 
 
// Temporary images 
IplImage *mhi = 0; // MHI: motion history image 
IplImage *orient = 0; // orientation 
IplImage *mask = 0; // valid orientation mask 
IplImage *segmask = 0; // motion segmentation map 
CvMemStorage* storage = 0; // temporary storage 
 
// Parameters: 
// img - input video frame 
// dst - resultant motion picture 64 
 
// args - optional parameters 
 
void update_mhi( IplImage* img, IplImage* dst, int diff_threshold ) { 
	// get current time in seconds 
	double timestamp = (double)clock()/CLOCKS_PER_SEC; 
 
	// get current frame size 
	CvSize size = cvSize(img->width,img->height); 
	int i, idx1 = last, idx2; 
	IplImage* silh; 
	CvSeq* seq; 
	CvRect comp_rect; 
	double count; 
	double angle; 
	CvPoint center; 
	double magnitude; 
	CvScalar color; 
	static int iter = 0; 
	 
	// Allocate images at the beginning or 
	// Reallocate them if the frame size is changed 
	if( !mhi || mhi->width != size.width || mhi->height != size.height ) { 
		if( buf == 0 ) { 
			buf = (IplImage**)malloc(N*sizeof(buf[0])); 
			memset( buf, 0, N*sizeof(buf[0])); 
		} 
		for( i = 0; i < N; i++ ) { 
			cvReleaseImage( &buf[i] ); 
			buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 ); 
			cvZero( buf[i] ); 
		} 
		cvReleaseImage( &mhi ); 
		cvReleaseImage( &orient ); 
		cvReleaseImage( &segmask ); 
		cvReleaseImage( &mask ); 
 
		mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
 
		// clear MHI at the beginning 
		cvZero( mhi ); 
		orient = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
		segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
		mask = cvCreateImage( size, IPL_DEPTH_8U, 1 ); 
 
	} // end of if(mhi) 
 
	// convert frame to grayscale 
	cvCvtColor( img, buf[last], CV_BGR2GRAY ); 
 
	idx2 = (last + 1) % N; // index of (last - (N-1))th frame 
	last = idx2; 
	silh = buf[idx2]; 
 
	// get difference between frames 
	cvAbsDiff( buf[idx1], buf[idx2], silh ); 
 
	// threshold it 
	cvThreshold(silh, silh, diff_threshold,1,CV_THRESH_BINARY ); 
 
	// update MHI 
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); 
 
	cvCvtScale( mhi, dst, 255./MHI_DURATION, ( MHI_DURATION - timestamp)*255./MHI_DURATION ); 
	cvCvtScale( mhi, dst, 255./MHI_DURATION, 0 ); 
	cvSmooth( dst, dst, CV_MEDIAN, 3, 0, 0, 0 ); 
 
	// calculate motion gradient orientation and valid 
	// orientation mask 
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 ); 
 
	// Create dynamic structure and sequence. 
	if( !storage ) 
		storage = cvCreateMemStorage(0); 
	else 
		cvClearMemStorage(storage); 
 
	// segment motion: get sequence of motion components 
	// segmask is marked motion components map. It is not used further 
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA ); 
 
	// iterate through the motion components, 
	// One more iteration (i == -1) corresponds to the whole image (global motion) 
	for( i = -1; i < seq->total; i++ ) { 
		if( i < 0 ){ // case of the whole image  
			comp_rect = cvRect(0,0, size.width, size.height); 
			color = CV_RGB(255,255,255); 
			magnitude = 100; 
		} 
		else{ // i-th motion component 
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect; 
			// reject very small components 
			if( comp_rect.width + comp_rect.height < 1200 ) 
				continue; 
			color = CV_RGB(255,0,0); 67 
			magnitude = 30; 
		} 
 
		// select component ROI 
		cvSetImageROI( silh, comp_rect ); 
		cvSetImageROI( mhi, comp_rect ); 
		cvSetImageROI( orient, comp_rect ); 
		cvSetImageROI( mask, comp_rect ); 
 
		// calculate orientation 
		angle = cvCalcGlobalOrientation( orient, mask, mhi, 
		timestamp, MHI_DURATION); 
	 
		// calculate number of points within silhouette ROI 
		count = cvNorm( silh, 0, CV_L1, 0 ); 
	 
		cvResetImageROI( mhi ); 	
		cvResetImageROI( orient ); 
		cvResetImageROI( mask ); 
		cvResetImageROI( silh ); 
	 
		// check for the case of little motion 
		if( count < comp_rect.width*comp_rect.height * 0.3 ) 
			continue; 
		// draw a clock with arrow indicating the direction 
		center = cvPoint( (comp_rect.x + comp_rect.width/2), (comp_rect.y + comp_rect.height/2) ); 
		cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 ); 
		cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)), 
		cvRound(center.y - magnitude*sin(angle*CV_PI/180) )), color, 3, CV_AA, 0 ); 
	 
		if(iter>3){ //delay given to understand the background  
			if(angle<186.000000 && angle>177.000000) {  
				printf("%f\n",angle); 
				printf("MOVING RIGHT ====CAM 1=====>\n\n"); 
				cam1right=1; 
			} 
			else if(angle>355.000000 || angle<6.000000) { 
				printf("%f\n",angle); 
				printf("MOVING LEFT <=====CAM 1====\n\n"); 
				cam1left=1; 
			} 
		} 
		iter++; 
	} 
} 
 
////////// The function for update mhi is repeated for 
////////// another camera to perform motion detection 
 
IplImage **buf1 = 0; 
int last1 = 0; 
 
// temporary images 
IplImage *mhi1 = 0; // MHI: motion history image 
IplImage *orient1 = 0; // orientation 
IplImage *mask1 = 0; // valid orientation mask1 
IplImage *segmask1 = 0; // motion segmentation map 
CvMemStorage* storage1 = 0; // temporary storage1 
 
// parameters: 
// img - input video frame 
// dst - resultant motion picture 
// args - optional parameters 
 
void update_mhi1( IplImage* img, IplImage* dst, int diff_threshold ) { 
	// get current time in seconds 
	double timestamp = (double)clock()/CLOCKS_PER_SEC; 
 
	// get current frame size 
	CvSize size = cvSize(img->width,img->height); 
	int i, idx1 = last1, idx2; 
	IplImage* silh; 
	CvSeq* seq; 
	CvRect comp_rect; 
	double count; 
	double angle; 
	CvPoint center; 
	double magnitude; 
	CvScalar color; 
	static int iter = 0; 
 
	// allocate images at the beginning or 
	// reallocate them if the frame size is changed 
	if( !mhi1 || mhi1->width != size.width || mhi1->height != size.height ) { 
		if( buf1 == 0 ) { 
			buf1 = (IplImage**)malloc(N*sizeof(buf1[0])); 
			memset( buf1, 0, N*sizeof(buf1[0])); 
		} 
		for( i = 0; i < N; i++ ) { 
			cvReleaseImage( &buf1[i] ); 
			buf1[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 ); 
			cvZero( buf1[i] ); 
		} 
		cvReleaseImage( &mhi1 ); 
		cvReleaseImage( &orient1 ); 
		cvReleaseImage( &segmask1 ); 
		cvReleaseImage( &mask1 ); 
 
		mhi1 = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
		// clear MHI at the beginning 
		cvZero( mhi1 ); 
 
		 orient1 = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
		 segmask1 = cvCreateImage( size, IPL_DEPTH_32F, 1 ); 
		 mask1 = cvCreateImage( size, IPL_DEPTH_8U, 1 ); 
	} // end of if(mhi1) 
 
	// convert frame to grayscale 
	cvCvtColor( img, buf1[last1], CV_BGR2GRAY ); 
 
	idx2 = (last1 + 1) % N; // index of (last1 - (N-1))th frame 
	last1 = idx2; 
	silh = buf1[idx2]; 
 
	// get difference between frames 
	cvAbsDiff( buf1[idx1], buf1[idx2], silh ); 
 
	// threshold it 
	cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); 
 
	// update MHI 
	cvUpdateMotionHistory( silh, mhi1, timestamp, MHI_DURATION ); 
 
	cvCvtScale( mhi1, dst, 255./MHI_DURATION, (MHI_DURATION - timestamp)*255./MHI_DURATION ); 
	cvCvtScale( mhi1, dst, 255./MHI_DURATION, 0 ); 
	cvSmooth( dst, dst, CV_MEDIAN, 3, 0, 0, 0 ); 
 
	// calculate motion gradient orientation and valid orientation mask1 
	cvCalcMotionGradient( mhi1, mask1, orient1, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 ); 
 
	// Create dynamic structure and sequence. 
	if( !storage1 ) 
		storage1 = cvCreateMemStorage(0); 
	else 
		cvClearMemStorage(storage1); 
  
	// segment motion: get sequence of motion components 
	// segmask1 is marked motion components map. It is not used further 
	seq = cvSegmentMotion( mhi1, segmask1, storage1, timestamp,  MAX_TIME_DELTA ); 
 
	for( i = -1; i < seq->total; i++ ) { 
		if( i < 0 ) { 
			comp_rect = cvRect(0,0, size.width, size.height); 
			color = CV_RGB(255,255,255); 
			magnitude = 100; 
		} 
		else { 
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect; 
			// reject very small components 

			if( comp_rect.width + comp_rect.height < 1200 ) 
				continue; 
			color = CV_RGB(255,0,0); 
			magnitude = 30; 
		} 
 
		// select component ROI 
		cvSetImageROI( silh, comp_rect ); 
		cvSetImageROI( mhi1, comp_rect ); 
		cvSetImageROI( orient1, comp_rect ); 
		cvSetImageROI( mask1, comp_rect ); 
 
		// calculate orientation 
		angle = cvCalcGlobalOrientation( orient1, mask1, mhi1,  timestamp, MHI_DURATION); 
 
		// calculate number of points within silhouette ROI 72 
 
		count = cvNorm( silh, 0, CV_L1, 0 ); 
 
		cvResetImageROI( mhi1 ); 
		cvResetImageROI( orient1 ); 
		cvResetImageROI( mask1 ); 
		cvResetImageROI( silh ); 
 
		 // check for the case of little motion 
		 if( count < comp_rect.width*comp_rect.height * 0.3 ) 
			continue; 
 
		 // draw a clock with arrow indicating the direction 
		 center = cvPoint( (comp_rect.x + comp_rect.width/2),  (comp_rect.y + comp_rect.height/2) ); 
		 cvCircle( dst, center, cvRound(magnitude*1.2), color, 3,  CV_AA, 0 ); 
		 cvLine( dst, center, cvPoint( cvRound( center.x +  magnitude*cos(angle*CV_PI/180)), 
		 cvRound(center.y - magnitude*sin(angle*CV_PI/180)  )), color, 3, CV_AA, 0 ); 
 
		if(iter>3){ //delay given to understand the background  
			if(angle<186.000000 && angle>177.000000)  { 
				printf("%f\n",angle); 
				printf("MOVING RIGHT ====CAM 2=====>\n\n"); 
				cam2right=1; 
			} 
			else if(angle>355.000000 || angle<6.000000) { 
				printf("%f\n",angle); 
				printf("MOVING LEFT <=====CAM 2====\n\n"); 
				cam2left=1; 
			} 
		} 
		iter++; 
	} 
} 
 
 
int main(int argc, char** argv) { 
	//create an image file to read images from camera 1 
	IplImage* motion = 0; 
	CvCapture* capture = 0; 
	capture = cvCaptureFromCAM(0); 
 
	//create an image file to read images from camera 2 
	IplImage* motion1 = 0; 
	CvCapture* capture1 = 0; 
	capture1 = cvCaptureFromCAM(1); 
 
	//create an image file to read images from camera 3 
	CvCapture* capture2 = 0; 
	capture2 = cvCaptureFromCAM(2); 
 
	if( capture ) { 
		cvNamedWindow( "Motion", 1 ); 
		cvNamedWindow( "Motion1", 2 ); 
		cvNamedWindow( "Motion2", 3 ); 
 
		while (1) { 
			IplImage* image = cvQueryFrame( capture ); 
			IplImage* image1 = cvQueryFrame( capture1 ); 
			IplImage* image2 = cvQueryFrame( capture2 ); 
 
			if( image )  { 
				if( !motion ) { 
					motion = cvCreateImage( cvSize(image->width,image->height), 8, 1 ); 
					cvZero( motion ); 
					motion->origin = image->origin; 
				} 
			}
 
			if( image1 ) { 
				if( !motion1 ) { 
					motion1 = cvCreateImage( cvSize(image1->width,image1->height), 8, 1 ); 
					cvZero( motion1 ); 
					motion1->origin = image1->origin; 
				} 
			} 
			
			update_mhi( image, motion, 30 ); 
			cvShowImage( "Motion", image); 
 
			// turn camera 2 on 
			if(cam1left==1) { 
				update_mhi1( image1, motion1, 30 ); 
				cvShowImage( "Motion1", image1 ); 
			} 
 
			//to turn on the camera in the right if required 
			//if(cam1right==1) 
			// { 
			// cvShowImage( "Motion2", image2 ); 
			// } 
 
			// turn camera 3 on 
			if(cam2left==1) { 
				cvShowImage( "Motion2", image2 ); 
			} 
 
			if( cvWaitKey(20) >= 0 ) 
				break; 
		} 
 
		 // free memory to relaese the images 
		 cvReleaseCapture( &capture ); 75 
		 
		 cvDestroyWindow( "Motion" ); 
		 cvReleaseCapture( &capture1 ); 
		 cvDestroyWindow( "Motion1" ); 
		 cvReleaseCapture( &capture2 ); 
		 cvDestroyWindow( "Motion2" ); 
	} 
	return 0; 
} 