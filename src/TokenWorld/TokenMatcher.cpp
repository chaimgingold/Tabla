//
//  TokenMatcher.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/12/16.
//
//

#include "TokenMatcher.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

void TokenMatcher::setParams( XmlTree xml )
{
	getXml(xml,"InlierThreshold",mInlierThreshold);
	getXml(xml,"NNMatchRatio",mNNMatchRatio);
	getXml(xml,"NNMatchPercentage",mNNMatchPercentage);
}

TokenMatcher::TokenMatcher()
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
}

Feature2DRef TokenMatcher::getFeatureDetector() {
	return mFeatureDetectors[mCurrentDetector].second;
}

vector<Token> TokenMatcher::findTokens( const Pipeline::StageRef world,
										const ContourVector &contours,
										Pipeline&pipeline )
{
	// Detect features for each region and create Token objects from them
	vector<Token> tokens;

	if ( !world || world->mImageCV.empty() ) return tokens;

	for ( auto c: contours )
	{
		if (c.mIsHole||c.mTreeDepth>0) {
			continue;
		}

		int tokenIndex = tokens.size();
		Token token;

		token.polyLine = c.mPolyLine;
		token.boundingRect = c.mBoundingRect;

		Rectf boundingRect = c.mBoundingRect;

		Rectf imageSpaceRect = boundingRect.transformed(mat4to3(world->mWorldToImage));

		cv::Rect cropRect = toOcv(Area(imageSpaceRect));

		if (cropRect.size().width < 2 || cropRect.size().height < 2) {
			continue;
		}

		Mat tokenContourImage = world->mImageCV(cropRect);

		imageSpaceRect.offset(-imageSpaceRect.getUpperLeft());
		token.tokenToWorld = getRectMappingAsMatrix(imageSpaceRect, boundingRect);

		pipeline.then(string("ContourImage ") + tokenIndex, tokenContourImage);
		pipeline.setImageToWorldTransform( token.tokenToWorld );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;

		Mat tokenContourImageEqualized;
		equalizeHist(tokenContourImage, tokenContourImageEqualized);
		pipeline.then(string("ContourImage equalized ") + tokenIndex, tokenContourImageEqualized);
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;

		token.features = featuresFromImage(tokenContourImageEqualized);
		token.features.index = tokens.size();
		tokens.push_back(token);
	}
	return tokens;
}

TokenFeatures TokenMatcher::featuresFromImage(Mat tokenImage) {
	TokenFeatures features;
	getFeatureDetector()->detectAndCompute(tokenImage,
										   noArray(),
										   features.keypoints,
										   features.descriptors);
	return features;
}

vector<MatchingTokenIndexPair> TokenMatcher::matchTokens( vector<TokenFeatures> tokenLibrary,
														  vector<TokenFeatures> candidates )
{
	vector<MatchingTokenIndexPair> matches;

	// Compare each token against every other for matches
	// (first round starts with
	// ab, ac .. af, then
	// bc, bd.. bf,
	// cd, ce.. cf
	// and so on which handles all unique pairs.)
	for ( int i=0; i < tokenLibrary.size(); i++ )
	{
		TokenFeatures focusedToken = tokenLibrary[i];
		for ( int j=0; j < candidates.size(); j++ ) {

			TokenFeatures candidateToken = candidates[j];

			// FIXME just for testing
			if (focusedToken.index == candidateToken.index) {
				continue;
			}

			// nn_matches will be same size as focusedToken.keypoints.
			vector<vector<DMatch>> nn_matches;
			mMatcher.knnMatch(focusedToken.descriptors,
							  candidateToken.descriptors,
							  nn_matches,
							  2);

			// If way fewer keypoints in one token vs the other, skip

//			float smaller = (float)MIN(focusedToken.keypoints.size(), candidateToken.keypoints.size());
//			float larger = (float)MAX(focusedToken.keypoints.size(), candidateToken.keypoints.size());
//			float ratio = smaller / larger;
//			if (ratio < 0.8) {
//				cout << "SKIPPING" << endl;
//				continue;
//			}

//			cout << "*******************" << endl;
//			cout << focusedToken.keypoints.size() << endl;
//			cout << candidateToken.keypoints.size() << endl;
//			cout << nn_matches.size() << endl;

			int numMatches = 0;
			for (size_t i = 0; i < nn_matches.size(); i++) {
				if (nn_matches[i].size() < 2) {
					continue;
				}

				//				DMatch first = nn_matches[i][0];
				float dist1 = nn_matches[i][0].distance;
				float dist2 = nn_matches[i][1].distance;

				if(dist1 < (dist2 * mNNMatchRatio)) {
					numMatches++;
				}
			}
			cout << "# Keypoints: " << nn_matches.size() << endl;
			cout << "# Matches: " << numMatches << endl;


			// Check if the number of matches with minimum distance is a reasonable percentage
			// of the total number of matches
			if (numMatches > nn_matches.size() * mNNMatchPercentage) {
				matches.push_back(make_pair(focusedToken.index, candidateToken.index));
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
	return matches;
}
