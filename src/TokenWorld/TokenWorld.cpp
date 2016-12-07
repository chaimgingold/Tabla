//
//  TokenWorld.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#include "TokenWorld.h"

// http://docs.opencv.org/2.4/doc/tutorials/features2d/feature_homography/feature_homography.html

void TokenWorld::setParams( XmlTree xml )
{

}



TokenWorld::TokenWorld()
{
	mFeatureDetectors.push_back(make_pair("ORB", cv::ORB::create()));
	mFeatureDetectors.push_back(make_pair("BRISK", cv::BRISK::create()));



	mFeatureDetectors.push_back(make_pair("KAZE", cv::KAZE::create()));
	mFeatureDetectors.push_back(make_pair("AKAZE", cv::AKAZE::create()));

	// Corner detectors
	mFeatureDetectors.push_back(make_pair("FAST", cv::FastFeatureDetector::create()));
	mFeatureDetectors.push_back(make_pair("AGAST", cv::AgastFeatureDetector::create()));


	mFeatureDetectors.push_back(make_pair("SURF", cv::xfeatures2d::SURF::create()));
	mFeatureDetectors.push_back(make_pair("SIFT", cv::xfeatures2d::SIFT::create()));

	//	mFeatureDetectors.push_back(make_pair("FREAK", cv::xfeatures2d::FREAK::create())); // Only implements compute() - figure this out
	
	mFeatureDetectors.push_back(make_pair("Star", cv::xfeatures2d::StarDetector::create()));
//	mFeatureDetectors.push_back(make_pair("LUCID", cv::xfeatures2d::LUCID::create(  1 // 3x3 lucid descriptor kernel
//															   , 1 // 3x3 blur kernel
//															   )));


	// "Good Features to Track"
	mFeatureDetectors.push_back(make_pair("GFFT", cv::GFTTDetector::create()));
	mFeatureDetectors.push_back(make_pair("SimpleBlob", cv::SimpleBlobDetector::create()));


}


void TokenWorld::updateVision( const ContourVector &c, Pipeline&pipeline ) {
	

	mWorld = pipeline.getStage("undistorted");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	mFeatureDetectors[mCurrentDetector].second->detect(mWorld->mImageCV, mKeypoints);
}


void TokenWorld::update()
{

}

void TokenWorld::draw( DrawType drawType )
{

	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	float hue = (float)mCurrentDetector / mFeatureDetectors.size();
	auto color = cinder::hsvToRgb(vec3(hue, 0.7, 0.9));
	gl::color(color);

	for (auto keypoint : mKeypoints)
	{
		gl::drawSolidCircle(vec2(mWorld->mImageToWorld * vec4(fromOcv(keypoint.pt),0,1)),
							keypoint.size * 0.01);
	}
}

void TokenWorld::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_TAB:
		{
			if (event.isShiftDown()) {
				if (mCurrentDetector == 0) {
					mCurrentDetector = mFeatureDetectors.size() - 1;
				} else {
					mCurrentDetector = (mCurrentDetector - 1);
				}

			} else {
				mCurrentDetector = (mCurrentDetector + 1) % mFeatureDetectors.size();
			}
			cout << mFeatureDetectors[mCurrentDetector].first << endl;
		}
		break;

		default:
			break;
	}
}
