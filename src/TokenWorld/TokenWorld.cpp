//
//  TokenWorld.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#include "TokenWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;


// http://docs.opencv.org/2.4/doc/tutorials/features2d/feature_homography/feature_homography.html

void TokenWorld::setParams( XmlTree xml )
{
	mTokenMatcher.setParams(xml);
}

TokenWorld::TokenWorld()
{
	mTokenMatcher = TokenMatcher();
}

void TokenWorld::updateVision( const ContourVector &contours, Pipeline&pipeline )
{
	mWorld = pipeline.getStage("clipped");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	switch (mMode) {
		case TokenVisionMode::Global:
			mTokenMatcher.getFeatureDetector()->detectAndCompute(mWorld->mImageCV,
																 noArray(),
													    		 mGlobalKeypoints,
																 noArray());
			break;
		case TokenVisionMode::Matching:
		{

			mTokens = mTokenMatcher.findTokens(mWorld, contours, pipeline);

			// N-to-N matching:
			// we want to match each found token with every other found token.
			vector <TokenFeatures> features;
			for (Token &token : mTokens) {
				features.push_back(token.features);
			}
			mMatches = mTokenMatcher.matchTokens(features, features);
		}
			break;
		default:
			break;
	}
}

void TokenWorld::update()
{

}

void TokenWorld::draw( DrawType drawType )
{
	switch (mMode) {
		case TokenVisionMode::Global:
			drawGlobalKeypoints();
			break;
		case TokenVisionMode::Matching:
			drawMatchingKeypoints();
			break;
		default:
			break;
	}
}

void TokenWorld::drawGlobalKeypoints() {
	float hue = (float)mTokenMatcher.mCurrentDetector / mTokenMatcher.mFeatureDetectors.size();
	auto color = cinder::hsvToRgb(vec3(hue, 0.7, 0.9));
	gl::color(color);

	for (auto keypoint : mGlobalKeypoints)
	{
		gl::drawSolidCircle(transformPoint(mWorld->mImageToWorld, fromOcv(keypoint.pt)),
							keypoint.size * 0.01);
	}
}

void TokenWorld::drawMatchingKeypoints() {
	// DEBUG: Drawing contours
	{
		for ( auto token: mTokens )
		{
			// Draw bounding box
			{
				Rectf rw = token.boundingRect ; // world space
				Rectf r  = Rectf(rw.getLowerLeft(),
								 rw.getUpperRight());


				gl::color(0.1, 0.2, 0.3);

				gl::drawStrokedRect(r);
			}
			// Draw polyline
			{
				gl::color(ColorAf(1,1,1));
				gl::draw(token.polyLine);
			}


			// Draw keypoints
			float hue = (float)token.features.index / mTokens.size();
			gl::color(cinder::hsvToRgb(vec3(hue, 0.7, 0.9)));
			for (auto keypoint : token.features.keypoints)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToWorld, fromOcv(keypoint.pt)),
									//keypoint.size * 0.01);
									0.8);
			}


			gl::color(cinder::hsvToRgb(vec3(hue + 0.03, 0.7, 0.9)));
			for (auto keypoint : token.matched)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToWorld, fromOcv(keypoint.pt)),
									0.6);
			}

			gl::color(cinder::hsvToRgb(vec3(hue + 0.06, 0.7, 0.9)));
			for (auto keypoint : token.inliers)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToWorld, fromOcv(keypoint.pt)),
									0.4);
			}

		}
	}

	for (auto matchingPair : mMatches ) {

		Token &token1 = mTokens[matchingPair.first];
		Token &token2 = mTokens[matchingPair.second];

		float hue = (float)token1.features.index / mTokens.size();
		gl::color(cinder::hsvToRgb(vec3(hue, 0.7, 0.9)));
		gl::drawLine(token1.boundingRect.getCenter(), token2.boundingRect.getCenter());
	}
}

void TokenWorld::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_TAB:
		{
//			if (mMode == TokenVisionMode::Matching) {
//				break;
//			}
			if (event.isShiftDown()) {
				if (mTokenMatcher.mCurrentDetector == 0) {
					mTokenMatcher.mCurrentDetector = mTokenMatcher.mFeatureDetectors.size() - 1;
				} else {
					mTokenMatcher.mCurrentDetector = (mTokenMatcher.mCurrentDetector - 1);
				}

			} else {
				mTokenMatcher.mCurrentDetector = (mTokenMatcher.mCurrentDetector + 1) % mTokenMatcher.mFeatureDetectors.size();
			}
			cout << mTokenMatcher.mFeatureDetectors[mTokenMatcher.mCurrentDetector].first << endl;
		}
		break;
		case KeyEvent::KEY_BACKQUOTE:
			mMode = TokenVisionMode(((int)mMode + 1) % (int)TokenVisionMode::kNumModes);
//			if (mMode == TokenVisionMode::Matching) {
//				mTokenMatcher.mCurrentDetector = 0; // AKAZE
//			}
		default:
			break;
	}
}
