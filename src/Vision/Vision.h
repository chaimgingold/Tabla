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
#include <thread>

#include "cinder/Surface.h"
#include "cinder/Xml.h"

#include "channel.h"
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

	Vision();
	~Vision();
	
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
		LightLink::CaptureProfile mCaptureProfile; // profile used
		Pipeline mPipeline;
		ContourVector mContours;
		TokenMatchVec mTokens;
	};

	// configure
	void setParams( Params );
	void setDebugFrameSkip( int );
	
	// setup input
	bool setCaptureProfile( const LightLink::CaptureProfile& );
	void stopCapture();
	
	// get output
	bool getOutput( Output& ); // returns true if output available
	
private:

	void visionThreadRunLoop();

	bool mIsDestructing=false;
	
	//
	Params mParams;

	// main threading
	mutex  mInputLock;
	thread mThread;
	channel<Output> mVisionOutputChannel;
	
	Output processFrame( SurfaceRef );
	
	// input
	VisionInput mInput;
	LightLink::CaptureProfile mCaptureProfile;
	chrono::nanoseconds mDebugFrameSleep=0s;
	
	// undistort params
	void updateRemap();
	cv::Mat mRemap[2]; // can be empty for none

	// submodules
	ContourVision mContourVision;
	TokenMatcherThreaded mTokenMatcher;
	
	//
	TokenMatchVec mOldTokenMatcherOutput;
};

#endif /* Vision_hpp */
