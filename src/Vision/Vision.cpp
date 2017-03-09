 //
//  Vision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "Vision.h"
#include "xml.h"
#include "ocv.h"

template<class T>
vector<T> asVector( const T &d, int len )
{
	vector<T> v;
	
	for( int i=0; i<len; ++i ) v.push_back(d[i]);
	
	return v;
}

Rectf asBoundingRect( vec2 pts[4] )
{
	vector<vec2> v;
	for( int i=0; i<4; ++i ) v.push_back(pts[i]);
	return Rectf(v);
}

void Vision::Params::set( XmlTree xml )
{
	getXml(xml,"CaptureAllPipelineStages",mCaptureAllPipelineStages);
	
//	cout << xml.getChild("Contours") << endl;
//	cout << xml.getChild("TokenMatcher") << endl;
	if ( xml.hasChild("Contours") )
	{
		mContourVisionParams.set( xml.getChild("Contours") );
	}
	
	if ( xml.hasChild("TokenMatcher") )
	{
		mTokenMatcherParams.set( xml.getChild("TokenMatcher") );
	}
}

Vision::Vision()
{
	mThread = thread([this]()
	{
		visionThreadRunLoop();
	});
}

void Vision::visionThreadRunLoop()
{
	bool run=true;
	
	while (run)
	{
		// get surface
		SurfaceRef     input;
		Vision::Output output;
		
		// get input, process it
		{
			unique_lock<std::mutex> lock(mInputLock);
			input = mInput.getFrame();
			run = !mIsDestructing;
			
			// vision it
			if (input) {
				output = processFrame(input);
			}
		}
		
		// output
		if (input) {
			mVisionOutputChannel.put(output,2);
		}
		else mInput.waitForFrame(mDebugFrameSleep);
	};
}

Vision::~Vision()
{
	mInput.stop();
	
	// tell everyone to wrap up
	{
		std::unique_lock<std::mutex> lock(mInputLock);
		mIsDestructing=true;
	}
		
	// stop token matcher first, in case mThread is waiting on it!
	// (avoid a race condition)
	mTokenMatcher.stop();
	
	// drain output in case mThread is blocked on putting data into it
	// (ie caller thread has shut us down with a full output queue!)
	// avoids another potential race condition
	{
		Output dontcare;
		while ( mVisionOutputChannel.get(dontcare,false) ) {}
	}
	
	// wait for them
	mThread.join();
}

void Vision::setParams( Params p )
{
	{
		std::unique_lock<std::mutex> lock(mInputLock);
		mParams=p;
		mContourVision.setParams(mParams.mContourVisionParams);
	}
	
	// has its own lock
	mTokenMatcher.setParams(mParams.mTokenMatcherParams);
}

void Vision::setDebugFrameSkip( int n )
{
	std::unique_lock<std::mutex> lock(mInputLock);
	if (n<2) mDebugFrameSleep=0s;
	else
	{
		std::chrono::milliseconds ms( (long long)( ((double)n/30.) * 1000. ) );
		mDebugFrameSleep = ms;
	}
	
	cout << mDebugFrameSleep.count() << endl;
}

bool Vision::setCaptureProfile( const LightLink::CaptureProfile& profile )
{
	std::unique_lock<std::mutex> lock(mInputLock);
	
	// compute remap?
	bool remapChanged =
		! isMatEqual<float>(profile.mDistCoeffs,mCaptureProfile.mDistCoeffs)
	||  ! isMatEqual<float>(profile.mCameraMatrix,mCaptureProfile.mCameraMatrix);
	
	// update vars
	mCaptureProfile=profile;
	
	// update remap
	if ( remapChanged ) updateRemap();

	// setup input device
	return mInput.setup(profile);
}

void Vision::updateRemap()
{
	if ( mCaptureProfile.mCameraMatrix.empty() || mCaptureProfile.mDistCoeffs.empty() )
	{
		cout << "Vision:: no remap " << endl;

		mRemap[0] = cv::Mat();
		mRemap[1] = cv::Mat();
	}
	else
	{
		cout << "Vision:: computing remap " << endl;
		
		cv::Size imageSize(mCaptureProfile.mCaptureSize.x,mCaptureProfile.mCaptureSize.y);
		
		cout << "cv::initUndistortRectifyMap" << endl;
		cout << "\tcameraMatrix: " << mCaptureProfile.mCameraMatrix << endl;
		cout << "\tmDistCoeffs: " << mCaptureProfile.mDistCoeffs << endl;
		cout << "\timageSize: " << imageSize << endl;
		
		cv::initUndistortRectifyMap(
			mCaptureProfile.mCameraMatrix,
			mCaptureProfile.mDistCoeffs,
			cv::Mat(), // mono, so not needed
			mCaptureProfile.mCameraMatrix, // mono, so same as 1st param
			imageSize,
			CV_16SC2, // CV_32FC1 or CV_16SC2
			mRemap[0],
			mRemap[1]);
	}
}

void Vision::stopCapture()
{
	std::unique_lock<std::mutex> lock(mInputLock);
	mInput.stop();
}

bool
Vision::getOutput( Vision::Output& output )
{
	if ( mVisionOutputChannel.get(output,false) )
	{
		return true;
	}
	else return false;
}

Vision::Output Vision::processFrame( SurfaceRef surface )
{
	Vision::Output output;
	
	output.mCaptureProfile = mCaptureProfile;
	
	Pipeline& pipeline = output.mPipeline; // patch us in (refactor in progress)

	// start pipeline
	pipeline.setCaptureAllStageImages(true);
		// reality is, we now need to capture all because even contour extraction
		// relies on images captured in the pipeline.
		// if we did need such an optimization, we could have everyone report the stages they want to capture,
		// and have a list of stages to capture.
//			    mDrawPipeline
//			|| (!mGameWorld || mGameWorld->getVisionParams().mCaptureAllPipelineStages)
//		);
	pipeline.start();
	
	// ---- Input ----
	
	// make cv frame
	cv::UMat input = toOcvRef(*surface.get()).getUMat(cv::ACCESS_READ);
	cv::UMat clipped;
		// toOcvRef is much, much faster than toOcv, which does a lot of dumb bit twiddling.
		// this requires a cast to non-const, but hopefully cv::ACCESS_READ semantics makes this kosher enough.
		// Just noting this inconsistency.
 
	pipeline.then( "input", input );
	
	pipeline.back()->setImageToWorldTransform( getOcvPerspectiveTransform(
		mCaptureProfile.mCaptureCoords,
		mCaptureProfile.mCaptureWorldSpaceCoords ) );

	// undistort
	cv::UMat undistorted;
	
	if ( !mRemap[0].empty() && !mRemap[1].empty() ) {
		cv::remap(input,undistorted,mRemap[0],mRemap[1],cv::INTER_LINEAR);
	}
	else
	{
		// 2x it, so we always have an 'undistorted' image frame to keep things sane
		undistorted = input;
	}
	pipeline.then( "undistorted", undistorted );
	
	// ---- World Boundaries ----
	pipeline.then( "world-boundaries", vec2(2.5,2.5)*100.f ); // Xm^2 configurable area
	pipeline.back()->setImageToWorldTransform( mat4() ); // identity; do it in world space
		// this is here just so it can be configured by the user.
	
	// ---- Clipped ----

	// clip
	float contourPixelToWorld = 1.f;
	
	{
		// gather transform parameters
		cv::Point2f srcpt[4], dstpt[4], dstpt_pixelspace[4];
		
		cv::Size outputSize;
		
		for( int i=0; i<4; ++i )
		{
			srcpt[i] = toOcv( mCaptureProfile.mCaptureCoords[i] );
			dstpt[i] = toOcv( mCaptureProfile.mCaptureWorldSpaceCoords[i] );
		}

		// compute output size pixel scaling factor
		const Rectf inputBounds  = asBoundingRect( mCaptureProfile.mCaptureCoords );
		const Rectf outputBounds = asBoundingRect( mCaptureProfile.mCaptureWorldSpaceCoords );

		const float pixelScale
			= max( inputBounds .getWidth(), inputBounds .getHeight() )
			/ max( outputBounds.getWidth(), outputBounds.getHeight() ) ;
		// more robust would be to process input edges: outputSize = [ max(l-edge,r-edge), max(topedge,botedge) ] 
		
		contourPixelToWorld = 1.f / pixelScale ;
		
		outputSize.width  = outputBounds.getWidth()  * pixelScale ;
		outputSize.height = outputBounds.getHeight() * pixelScale ;

		// compute dstpts in desired destination pixel space
		for( int i=0; i<4; ++i )
		{
			dstpt_pixelspace[i] = dstpt[i] * pixelScale ;
		}
		
		// do it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt, dstpt_pixelspace ) ;
		cv::warpPerspective( undistorted, clipped, xform, outputSize );
		// To speed it up on CPU:
		// http://romsteady.blogspot.com/2015/07/calculate-opencv-warpperspective-map.html
		
		// log to pipeline
		pipeline.then( "clipped", clipped );
		
		glm::mat4 imageToWorld = glm::scale( vec3( 1.f / pixelScale, 1.f / pixelScale, 1.f ) );
		
		pipeline.back()->setImageToWorldTransform( imageToWorld );
		
		// grey
		cv::UMat clipped_gray;
		cv::cvtColor(clipped, clipped_gray, CV_BGR2GRAY);
		pipeline.then( "clipped_gray", clipped_gray );
	}
	
	// find contours
	auto clippedStage = pipeline.back(); // gets "clipped_gray"
	
	output.mContours = mContourVision.findContours( clippedStage, pipeline, contourPixelToWorld );

	// tokens
	if (1)
	{
		TokenMatcherThreaded::Output tokens; 
		
		if ( mTokenMatcher.get(tokens) )
		{
			output.mTokens = tokens.mTokens;
			mOldTokenMatcherOutput = tokens.mTokens;
			// TODO: add/capture pipeline stages added by tokenizer
			
//			float now = ci::app::getElapsedSeconds();
//			cout << "TOKENS @ " << now << " took " << now - tokens.mTimeStamp << "s" << endl;
		}
		else output.mTokens = mOldTokenMatcherOutput;
		
		if ( !mTokenMatcher.isBusy() )
		{
			TokenMatcherThreaded::Input input;
			input.mPipeline = pipeline; // not a deep copy; this could lead to trouble with multiple pointers into same data. but i don't think there is a lot of rewriting going on. :P
			input.mContours = output.mContours;
			input.mTimeStamp = ci::app::getElapsedSeconds();
			mTokenMatcher.put(input);
		}
	}
	
	return output;
}