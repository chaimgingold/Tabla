//
//  Vision.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef Vision_hpp
#define Vision_hpp

#include <string>
#include <vector>

#include "cinder/Surface.h"
#include "cinder/Xml.h"
#include "CinderOpenCV.h"
#include "LightLink.h"

#include "Vision.h"
#include "Contour.h"
#include "Pipeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;



class Vision
{
public:

	// settings
	class Params
	{
	public:
		void set( XmlTree );
	
		float mContourMinRadius	=	3;
		float mContourMinArea	=	100;
		float mContourDPEpsilon	=	5;
		float mContourMinWidth	=	5;
		bool  mContourGetExactMinCircle = false; // otherwise, we'll approximate with min bounding box
		
		bool mCaptureAllPipelineStages = false; // this is OR'd in
	};

	void setParams( Params p ) { mParams=p; }

	void setLightLink( const LightLink& );
	
	// push input through
	void processFrame( const Surface &surface, Pipeline& tracePipeline );
	
	// output
	ContourVector mContourOutput;
	
private:
	Params		mParams;
	LightLink	mLightLink;

	// undistort params
	cv::Mat mRemap[2]; // can be empty for none

};

#endif /* Vision_hpp */
