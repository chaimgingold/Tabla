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

#include "ContourVision.h"

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
		
		bool mCaptureAllPipelineStages = false; // this is OR'd in
	
		ContourVision::Params mContourVisionParams;
		// TODO: tokens
	};

	// output
	class Output
	{
	public:
		ContourVector mContours;
		// TODO: tokens
	};

	// configure
	void setParams( Params );
	void setLightLink( const LightLink& );

	// push input through
	Output processFrame( const Surface &surface, Pipeline& tracePipeline );
	
private:
	Params		mParams;
	LightLink	mLightLink;

	// undistort params
	cv::Mat mRemap[2]; // can be empty for none

	// submodules
	ContourVision mContourVision;
	
};

#endif /* Vision_hpp */
