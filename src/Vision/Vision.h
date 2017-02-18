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
#include "VisionInput.h"
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
		Pipeline mPipeline;
		ContourVector mContours;
		TokenMatchVec mTokens;
	};

	// configure
	void setParams( Params );
	void setDebugFrameSkip( int n ) { mInput.setDebugFrameSkip(n); }
	
	// setup input
	bool setCaptureProfile( const LightLink::CaptureProfile& );
	void stopCapture();
	
	// get output
	bool getOutput( Output& ); // returns true if output available
	
private:

	Params		mParams;

	// input
	VisionInput mInput;
	LightLink::CaptureProfile mCaptureProfile;

	// undistort params
	void updateRemap();
	cv::Mat mRemap[2]; // can be empty for none

	// submodules
	ContourVision mContourVision;
	TokenMatcher  mTokenMatcher;
	
	//
	int mFrameCount=0;
	int mTokenMatchSkip=30;
	
	TokenMatchVec mOldTokenMatcherOutput;
};

#endif /* Vision_hpp */
