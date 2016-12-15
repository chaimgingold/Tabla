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

void Vision::Params::set( XmlTree xml )
{
	getXml(xml,"CaptureAllPipelineStages",mCaptureAllPipelineStages);

	if ( xml.hasChild("Contours") )
	{
		mContourVisionParams.set( xml.getChild("Contours") );
	}
	
	if ( xml.hasChild("Tokens") )
	{
		mTokenMatcherParams.set( xml.getChild("Tokens") );
	}
}

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

void Vision::setParams( Params p )
{
	mParams=p;
	mContourVision.setParams(mParams.mContourVisionParams);
	mTokenMatcher.setParams(mParams.mTokenMatcherParams);
}

void Vision::setLightLink( const LightLink &ll )
{
	// compute remap?
	bool updateRemap =
		! isMatEqual<float>(ll.mDistCoeffs,mLightLink.mDistCoeffs)
	||  ! isMatEqual<float>(ll.mCameraMatrix,mLightLink.mCameraMatrix);
	
	// update vars
	mLightLink=ll;
	
	// update remap
	if ( updateRemap )
	{
		cout << "Vision:: computing remap " << endl;
		
		cv::Size imageSize(mLightLink.mCaptureSize.x,mLightLink.mCaptureSize.y);
		
		cout << "cv::initUndistortRectifyMap" << endl;
		cout << "\tcameraMatrix: " << mLightLink.mCameraMatrix << endl;
		cout << "\tmDistCoeffs: " << mLightLink.mDistCoeffs << endl;
		cout << "\timageSize: " << imageSize << endl;
		
		cv::initUndistortRectifyMap(
			mLightLink.mCameraMatrix,
			mLightLink.mDistCoeffs,
			cv::Mat(), // mono, so not needed
			mLightLink.mCameraMatrix, // mono, so same as 1st param
			imageSize,
			CV_16SC2, // CV_32FC1 or CV_16SC2
			mRemap[0],
			mRemap[1]);
	}
}

Vision::Output
Vision::processFrame( const Surface &surface, Pipeline& pipeline )
{
	// ---- Input ----
	
	// make cv frame
	cv::UMat input_color = toOcvRef((Surface &)surface).getUMat(cv::ACCESS_READ); // we type-cast to non-const so this works. :P
	cv::UMat input;
	cv::UMat gray, clipped;
		// toOcvRef is much, much faster than toOcv, which does a lot of dumb bit twiddling.
		// this requires a cast to non-const, but hopefully cv::ACCESS_READ semantics makes this kosher enough.
		// Just noting this inconsistency.
 
	cv::cvtColor(input_color, input, CV_BGR2GRAY);
	// net performance of doing color -> grey conversion on CPU vs GPU is negligible.
	// the main impact is downloading the data, which we may be able to do faster if we rewrite toOcv.
	// but at least we now have color frame in the pipeline at the same total performance cost, so that's a net gain.
	// might want to make it a setting, whether to do it on CPU or GPU.
	
	pipeline.then( "input_color", input_color );
	
	pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(
		mLightLink.mCaptureCoords,
		mLightLink.mCaptureWorldSpaceCoords ) );

	pipeline.then( "input", input );
	
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
	pipeline.then( "world-boundaries", vec2(4,4)*100.f ); // 4m^2 configurable area
	pipeline.setImageToWorldTransform( mat4() ); // identity; do it in world space
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
			srcpt[i] = toOcv( mLightLink.mCaptureCoords[i] );
			dstpt[i] = toOcv( mLightLink.mCaptureWorldSpaceCoords[i] );
		}

		// compute output size pixel scaling factor
		const Rectf inputBounds  = asBoundingRect( mLightLink.mCaptureCoords );
		const Rectf outputBounds = asBoundingRect( mLightLink.mCaptureWorldSpaceCoords );

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
		
		pipeline.setImageToWorldTransform( imageToWorld );
	}
	
	// find contours
	Vision::Output output;
	
	auto clippedStage = pipeline.getStages().back();
	
	output.mContours = mContourVision.findContours( clippedStage, pipeline, contourPixelToWorld );
	
	if ( mTokenMatcher.getTokenLibrary().empty() ) output.mTokens = TokenMatches();
	else output.mTokens   = mTokenMatcher.getMatches(
		mTokenMatcher.findTokenCandidates( clippedStage, output.mContours, pipeline ));
	
	// output
	return output;
}