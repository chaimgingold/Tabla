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
#include "TokenMatcher.h"

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
		TokenMatcher::Params  mTokenMatcherParams;
	};

	// output
	class Output
	{
	public:
		ContourVector mContours;
		TokenMatches  mTokens;
	};

	// configure
	void setParams( Params );
	void setCaptureProfile( const LightLink::CaptureProfile& );

	// push input through
	Output processFrame( const Surface &surface, Pipeline& tracePipeline );
	
private:
	Params		mParams;
	LightLink::CaptureProfile mCaptureProfile;

	// undistort params
	cv::Mat mRemap[2]; // can be empty for none

	// submodules
	ContourVision mContourVision;
	TokenMatcher  mTokenMatcher;
	
};

#endif /* Vision_hpp */
