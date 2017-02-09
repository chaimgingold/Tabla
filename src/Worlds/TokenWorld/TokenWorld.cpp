//
//  TokenWorld.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#include "TokenWorld.h"
#include "TablaApp.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;


// http://docs.opencv.org/2.4/doc/tutorials/features2d/feature_homography/feature_homography.html

static GameCartridgeSimple sCartridge("TokenWorld", [](){
	return std::make_shared<TokenWorld>();
});

TokenWorld::TokenWorld()
{
	mTokenMatcher = TokenMatcher();
}

void TokenWorld::setParams( XmlTree xml )
{
	// we're parsing the xml data as if the <TokenWorld> xml node contains the
	// data that would normally be within Vision/Tokens
//	TokenMatcher::Params params;
//	params.set(xml);
//	mTokenMatcher.setParams(params);
	
	getXml( xml, "DrawKeypoints", mDrawKeypoints );
}

void TokenWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	mWorld = pipeline.getStage("clipped_gray");
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
			cout << "*****" << endl;
			for (auto &match : visionOut.mTokens) {
				cout << match.first.name << endl;
			}
			mTokens = visionOut.mTokens;

//			mTokens = mTokenMatcher.tokensFromContours(mWorld, visionOut.mContours, pipeline);

			// N-to-N matching:
			// we want to match each found token with every other found token.
//			vector <AnalyzedToken> features;
//			for (TokenContour &token : mTokens) {
//				features.push_back(token.features);
//			}
//			mMatches = mTokenMatcher.matchTokens(features, features);
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
			drawMatchingKeypoints( mDrawKeypoints && drawType==GameWorld::DrawType::UIMain );
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

void TokenWorld::drawMatchingKeypoints( bool drawKeypoints ) {
	// DEBUG: Drawing contours
	{
//		cout << "************drawMatchingKeypoints()************" << endl;
		for ( auto match: mTokens )
		{

			auto token = match.second;
			// Draw bounding box
//			{
//				Rectf rw = token.fromContour.boundingRect ; // world space
//				Rectf r  = Rectf(rw.getLowerLeft(),
//								 rw.getUpperRight());
//
//
//				gl::color(0.1, 0.2, 0.3);
//
//				gl::drawStrokedRect(r);
//			}
//			// Draw polyline
//			{
//				gl::color(ColorAf(1,1,1));
//				gl::draw(token.fromContour.polyLine);
//			}
//
//
			float hue = (float)token.index / mTokens.size();
			auto color = cinder::hsvToRgb(vec3(hue, 0.7, 0.9));

			// Draw keypoints
			if (drawKeypoints)
			{
				gl::color(color);
				for (auto keypoint : token.keypoints)
				{
					gl::drawSolidCircle(transformPoint(token.fromContour.tokenToWorld, fromOcv(keypoint.pt)),
										//keypoint.size * 0.01);
										0.5);
				}
			}

			
			auto font = TablaApp::get()->mTextureFont;
			
			if (font)
			{
				gl::color(color);
				
				vec2 size = font->measureString(match.first.name);
				
				float worldHeight = 5.f; // cm
				float scale = worldHeight / size.y ;
				
				Rectf r(match.second.fromContour.polyLine.getPoints());
				vec2 center = r.getCenter();
				
				font->drawString( match.first.name,
								  vec2(center.x,r.y2) + vec2(-size.x/2.f,size.y)*scale,
								  gl::TextureFont::DrawOptions().scale(scale).pixelSnap(false)
									   );
			}

//
//			gl::color(cinder::hsvToRgb(vec3(hue + 0.03, 0.7, 0.9)));
//			for (auto keypoint : token.fromContour.matched)
//			{
//				gl::drawSolidCircle(transformPoint(token.fromContour.tokenToWorld, fromOcv(keypoint.pt)),
//									0.6);
//			}
//
//			gl::color(cinder::hsvToRgb(vec3(hue + 0.06, 0.7, 0.9)));
//			for (auto keypoint : token.fromContour.inliers)
//			{
//				gl::drawSolidCircle(transformPoint(token.fromContour.tokenToWorld, fromOcv(keypoint.pt)),
//									0.4);
//			}

		}
	}

//	for (auto matchingPair : mMatches ) {
//
//		TokenContour &token1 = mTokens[matchingPair.first];
//		TokenContour &token2 = mTokens[matchingPair.second];
//
//		float hue = (float)token1.index / mTokens.size();
//		gl::color(cinder::hsvToRgb(vec3(hue, 0.7, 0.9)));
//		gl::drawLine(token1.boundingRect.getCenter(), token2.boundingRect.getCenter());
//	}
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
			mTokenMatcher.reanalyze();
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
