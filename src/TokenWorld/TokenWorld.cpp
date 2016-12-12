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
	getXml(xml,"InlierThreshold",mInlierThreshold);
	getXml(xml,"NNMatchRatio",mNNMatchRatio);
	getXml(xml,"NNMatchPercentage",mNNMatchPercentage);
}



TokenWorld::TokenWorld()
{
	// TODO: see http://docs.opencv.org/3.1.0/d3/da1/classcv_1_1BFMatcher.html
	// for what NORM option to use with which algorithm here
	mMatcher = cv::BFMatcher(NORM_HAMMING);

	mFeatureDetectors.push_back(make_pair("AKAZE", cv::AKAZE::create()));



	mFeatureDetectors.push_back(make_pair("ORB", cv::ORB::create()));
	mFeatureDetectors.push_back(make_pair("BRISK", cv::BRISK::create()));

	mFeatureDetectors.push_back(make_pair("KAZE", cv::KAZE::create()));

	// Corner detectors
	mFeatureDetectors.push_back(make_pair("FAST", cv::FastFeatureDetector::create()));
	mFeatureDetectors.push_back(make_pair("AGAST", cv::AgastFeatureDetector::create()));


	mFeatureDetectors.push_back(make_pair("SURF", cv::xfeatures2d::SURF::create()));
	mFeatureDetectors.push_back(make_pair("SIFT", cv::xfeatures2d::SIFT::create()));

	// Only implements compute() - figure this out
//	mFeatureDetectors.push_back(make_pair("FREAK", cv::xfeatures2d::FREAK::create()));

	mFeatureDetectors.push_back(make_pair("Star", cv::xfeatures2d::StarDetector::create()));
//	mFeatureDetectors.push_back(make_pair("LUCID", cv::xfeatures2d::LUCID::create(  1 // 3x3 lucid descriptor kernel
//															   , 1 // 3x3 blur kernel
//															   )));


	// These seem pretty primitive, so leaving them out.
	// But might be good for something/worth checking param tweaks...
//	mFeatureDetectors.push_back(make_pair("GFFT", cv::GFTTDetector::create())); // "Good Features to Track"
//	mFeatureDetectors.push_back(make_pair("SimpleBlob", cv::SimpleBlobDetector::create()));

}

void TokenWorld::globalVision( const ContourVector &contours, Pipeline&pipeline ) {
	getFeatureDetector()->detect(mWorld->mImageCV,
							     mGlobalKeypoints);
}

void TokenWorld::matchingVision( const ContourVector &contours, Pipeline&pipeline ) {
	// Detect features for each region and create Token objects from them
	mTokens.clear();
	for ( auto c: contours )
	{
		if (c.mIsHole||c.mTreeDepth>0) {
			continue;
		}

		Token token;
		token.index = mTokens.size();
		token.polyLine = c.mPolyLine;
		token.boundingRect = c.mBoundingRect;

		Rectf boundingRect = c.mBoundingRect;

		Rectf imageSpaceRect = boundingRect.transformed(mat4to3(mWorld->mWorldToImage));

		cv::Rect cropRect = toOcv(Area(imageSpaceRect));

		if (cropRect.size().width < 2 || cropRect.size().height < 2) {
			continue;
		}

		Mat tokenContourImage = mWorld->mImageCV(cropRect);

		imageSpaceRect.offset(-imageSpaceRect.getUpperLeft());
		token.tokenToImage = getRectMappingAsMatrix(imageSpaceRect, boundingRect);

		pipeline.then(string("ContourImage")+token.index, tokenContourImage);
		pipeline.setImageToWorldTransform( token.tokenToImage );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;

		getFeatureDetector()->detectAndCompute(tokenContourImage,
											   noArray(),
											   token.keypoints,
											   token.descriptors);
		mTokens.push_back(token);
	}

	// Compare each token against every other for matches
	// (first round starts with
	// ab, ac .. af, then
	// bc, bd.. bf,
	// cd, ce.. cf
	// and so on which handles all unique pairs.)
	mMatches.clear();
	for ( int i=0; i < mTokens.size(); i++ )
	{
		Token focusedToken = mTokens[i];
		for ( int j=i+1; j < mTokens.size(); j++ ) {
			Token candidateToken = mTokens[j];

			vector<vector<DMatch>> nn_matches;
			mMatcher.knnMatch(focusedToken.descriptors,
							  candidateToken.descriptors,
							  nn_matches,
							  2);

			// If way fewer keypoints in one token vs the other, skip
			float smaller = (float)MIN(focusedToken.keypoints.size(), candidateToken.keypoints.size());
			float larger = (float)MAX(focusedToken.keypoints.size(), candidateToken.keypoints.size());
			float ratio = smaller / larger;
			if (ratio < 0.7) {
				continue;
			}


//			cout << focusedToken.descriptors.size() << endl;
//			cout << candidateToken.descriptors.size() << endl;
//			cout << nn_matches.size() << endl;

			int numMatches = 0;
			for (size_t i = 0; i < nn_matches.size(); i++) {
				if (nn_matches[i].size() < 2) {
					continue;
				}

//				DMatch first = nn_matches[i][0];
				float dist1 = nn_matches[i][0].distance;
				float dist2 = nn_matches[i][1].distance;

				if(dist1 < mNNMatchRatio * dist2) {
					numMatches++;
//					focusedToken.matched.push_back(focusedToken.keypoints[first.queryIdx]);
//					candidateToken.matched.push_back(candidateToken.keypoints[first.trainIdx]);
				}


			}
//			cout << nn_matches.size() << endl;


			// Check if the number of matches with minimum distance is a reasonable percentage
			// of the total number of matches
			if (numMatches > nn_matches.size() * mNNMatchPercentage) {
				mMatches.push_back(make_pair(focusedToken.index, candidateToken.index));
			}


			/*
			// Find matching descriptors...
			for (size_t i = 0; i < nn_matches.size(); i++) {
				if (nn_matches[i].size() < 2) {
					continue;
				}

				DMatch first = nn_matches[i][0];
				float dist1 = nn_matches[i][0].distance;
				float dist2 = nn_matches[i][1].distance;

				if(dist1 < mNNMatchRatio * dist2) {
					focusedToken.matched.push_back(focusedToken.keypoints[first.queryIdx]);
					candidateToken.matched.push_back(candidateToken.keypoints[first.trainIdx]);
				}
			}

			for (unsigned i = 0; i < focusedToken.matched.size(); i++) {

				if (i >= candidateToken.matched.size()) {
					continue;
				}
				Mat col = Mat::ones(3, 1, CV_64F);
				col.at<double>(0) = focusedToken.matched[i].pt.x;
				col.at<double>(1) = focusedToken.matched[i].pt.y;

				col /= col.at<double>(2);
				double dist = sqrt( pow(col.at<double>(0) - candidateToken.matched[i].pt.x, 2) +
								   pow(col.at<double>(1) - candidateToken.matched[i].pt.y, 2)
								   );

				if(dist < mInlierThreshold) {
					int new_i = static_cast<int>(focusedToken.inliers.size());
					focusedToken.inliers.push_back(focusedToken.matched[i]);
					candidateToken.inliers.push_back(candidateToken.matched[i]);
					focusedToken.good_matches.push_back(DMatch(new_i, new_i, 0));

					mMatches.push_back(make_pair(focusedToken.index, candidateToken.index));
				}
			}
			 */
		}
	}
}

void TokenWorld::updateVision( const ContourVector &contours, Pipeline&pipeline )
{
	mContours = contours;

	mWorld = pipeline.getStage("clipped");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	switch (mMode) {
		case TokenVisionMode::Global:
			globalVision(contours, pipeline);
			break;
		case TokenVisionMode::Matching:
			matchingVision(contours, pipeline);
			break;
		default:
			break;
	}



}


Feature2DRef TokenWorld::getFeatureDetector() {
	return mFeatureDetectors[mCurrentDetector].second;
}



void TokenWorld::update()
{

}

void TokenWorld::draw( DrawType drawType )
{

	if ( !mWorld || mWorld->mImageCV.empty() ) return;


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
	float hue = (float)mCurrentDetector / mFeatureDetectors.size();
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
			float hue = (float)token.index / mTokens.size();
			gl::color(cinder::hsvToRgb(vec3(hue, 0.7, 0.9)));
			for (auto keypoint : token.keypoints)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToImage, fromOcv(keypoint.pt)),
									//keypoint.size * 0.01);
									0.8);
			}


			gl::color(cinder::hsvToRgb(vec3(hue + 0.03, 0.7, 0.9)));
			for (auto keypoint : token.matched)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToImage, fromOcv(keypoint.pt)),
									0.6);
			}

			gl::color(cinder::hsvToRgb(vec3(hue + 0.06, 0.7, 0.9)));
			for (auto keypoint : token.inliers)
			{
				gl::drawSolidCircle(transformPoint(token.tokenToImage, fromOcv(keypoint.pt)),
									0.4);
			}

		}
	}

	for (auto matchingPair : mMatches ) {

		Token &token1 = mTokens[matchingPair.first];
		Token &token2 = mTokens[matchingPair.second];

		float hue = (float)token1.index / mTokens.size();
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
			if (mMode == TokenVisionMode::Matching) {
				break;
			}
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
		case KeyEvent::KEY_BACKQUOTE:
			mMode = TokenVisionMode(((int)mMode + 1) % (int)TokenVisionMode::kNumModes);
			if (mMode == TokenVisionMode::Matching) {
				mCurrentDetector = 0; // AKAZE
			}
		default:
			break;
	}
}
