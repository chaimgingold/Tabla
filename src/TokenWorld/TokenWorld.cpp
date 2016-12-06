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
	mFeatureDetector = cv::xfeatures2d::SURF::create();

}

void TokenWorld::updateVision( const ContourVector &c, Pipeline&pipeline ) {
	

	mWorld = pipeline.getStage("undistorted");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;


	mFeatureDetector->detect(mWorld->mImageCV, mKeypoints);
}




void TokenWorld::update()
{

}

void TokenWorld::draw( DrawType drawType )
{

	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	for (auto keypoint : mKeypoints) {
		gl::color(1.0, 0.0, 0.0);
		gl::drawSolidCircle(vec2(mWorld->mImageToWorld * vec4(fromOcv(keypoint.pt),0,1)), keypoint.size * 0.01);
	}
}
