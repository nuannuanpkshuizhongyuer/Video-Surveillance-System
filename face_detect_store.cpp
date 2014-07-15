// Face Detection, Face Extraction and Store 
#include "stdafx.h" 
#include <stdio.h> 
#include "cv.h" 
#include "highgui.h" 
 
CvHaarClassifierCascade *cascade; 
CvMemStorage *storage; 
 
void detectFaces( IplImage *img ); 
 
IplImage *CopySubImage(IplImage *imatge,int xorigen, int 
yorigen,int width, int height); 
 
int main( int argc, char** argv ) { 
	CvCapture *capture; 
	IplImage *frame; 
	int key = 1; 
	char *filename = "haarcascade_frontalface_alt.xml"; 
 
	/* load the classifier */ 
	cascade = ( CvHaarClassifierCascade* )cvLoad( filename, 0, 0, 0 ); 
 
	/* setup memory buffer; needed by the face detector */ 
	storage = cvCreateMemStorage( 0 ); 
 
	/* initialize camera */ 
	capture = cvCaptureFromCAM( CV_CAP_ANY ); 
 
	/* always check */ 
	assert( cascade && storage && capture ); 
 
	/* create a window */ 
	cvNamedWindow( "video", 1 ); 
 
 
	while( key != 'q' ) { 
		/* get a frame */ 
		frame = cvQueryFrame( capture ); 
 
		/* always check */ 
		if( !frame ) break; 
 
		/* 'fix' frame */ 
		/* detect faces and display video */ 
		detectFaces( frame ); 
 
		/* quit if user press 'q' */ 
		key = cvWaitKey( 10 ); 
	} 
 
	 /* free memory */ 
	 cvReleaseCapture( &capture ); 
	 cvDestroyWindow( "video" ); 
	 cvReleaseHaarClassifierCascade( &cascade ); 
	 cvReleaseMemStorage( &storage ); 
	 void cvDestroyAllWindows( void ); 
	 return 0; 
} 
 
void detectFaces( IplImage *img ) { 
	int i; 
	int or_pointx, or_pointy; 
	// To display detected face separately 
	IplImage *Extr_face; 
	IplImage* Extr_face1 = cvCreateImage(cvSize(92, 112), IPL_DEPTH_8U, 1); 
 
	static int image_index = 0; 
	char imgfilename[80]; 
 
	/* detect faces */ 
	CvSeq *faces = cvHaarDetectObjects( img, cascade, storage, 1.1, 3, 0 /*CV_HAAR_DO_CANNY_PRUNNING*/, cvSize( 40, 40 ) ); 
 
	/* for each face found, draw a red box */ 
	for( i = 0 ; i < ( faces ? faces->total : 0 ) ; i++ ) { 
		CvRect *r = ( CvRect* )cvGetSeqElem( faces, i ); 
		cvRectangle( img,cvPoint( r->x, r->y ), 
		cvPoint( r->x + r->width,r->y + r->height ), 
		CV_RGB( 255, 0, 0 ), 1, 8, 0 ); 
 
		or_pointx= (int)(r->x); 
		or_pointy= (int)(r->y); 
		printf ("The co ordinates are %d, %d \n", or_pointx, or_pointy ); 
 
		// To display face 
		Extr_face = CopySubImage(img, r->x, r->y, r->width,r->height); 
		cvNamedWindow( "face_shown", 2 ); 
		cvShowImage ( "face_shown", Extr_face); 
		sprintf(imgfilename,"Colour faces/test_%d.jpg", image_index); 
		cvSaveImage(imgfilename , Extr_face); 
		cvSaveImage("test.jpg" , Extr_face); 
 
		image_index++; 
		IplImage* newim = cvLoadImage("test.jpg", 
		CV_LOAD_IMAGE_GRAYSCALE); 
		cvNamedWindow( "face_shown", 2 ); 
		cvShowImage ( "face_shown", newim); 
		cvResize(newim, Extr_face1, CV_INTER_CUBIC); 
 
		cvNamedWindow( "face_shown", 2 ); 
		cvShowImage ( "face_resize", Extr_face1); 
		sprintf(imgfilename,"Detected 
		faces/test_%d.jpg",image_index); 
		cvSaveImage(imgfilename, Extr_face1); 
	}  
	/* display video */ 
	cvShowImage( "video", img ); 
} 
 
//Routine to extract subimage 
IplImage *CopySubImage(IplImage *imatge,int xorigen, int yorigen,int width, int height) { 
 
	CvRect roi; 
	IplImage *resultat; 
	roi.x = xorigen; 
	roi.y = yorigen; 
	roi.width = width; 
	roi.height = height; 
 
	// Fix the ROI as image 
	cvSetImageROI(imatge,roi); 
 
	// Create a new image with ROI 
	resultat = cvCreateImage( cvSize(roi.width, roi.height), imatge->depth, imatge->nChannels ); 
	cvCopy(imatge,resultat); 
	cvResetImageROI(imatge); 
	return resultat; 
} 