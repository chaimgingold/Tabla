//
//  TokenMatcher.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/12/16.
//
//

#include "TablaApp.h"
#include "TokenMatcher.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

void TokenMatcher::Params::set( XmlTree xml )
{
	getXml(xml,"InlierThreshold",mInlierThreshold);
	getXml(xml,"NNMatchRatio",mNNMatchRatio);
	getXml(xml,"NNMatchPercentage",mNNMatchPercentage);
	
	mTokenLibraryPaths.clear();
	const bool kVerbose = true;
	if (kVerbose) cout << "Tokens:" << endl;

	for( auto i = xml.begin( "Tokens/Token" ); i != xml.end(); ++i )
	{
		if ( i->hasChild("Path") )
		{
			fs::path path = i->getChild("Path").getValue();
			
			path = TablaApp::get()->hotloadableAssetPath(path);
			
			mTokenLibraryPaths.push_back(path);

			if (kVerbose) cout << "\t" << path << endl;
		}
	}
}

TokenMatcher::TokenMatcher()
{
	// TODO: see http://docs.opencv.org/3.1.0/d3/da1/classcv_1_1BFMatcher.html
	// for what NORM option to use with which algorithm here
	mMatcher = cv::BFMatcher(NORM_HAMMING);

	mFeatureDetectors.push_back(make_pair("AKAZE", cv::AKAZE::create()));
	mFeatureDetectors.push_back(make_pair("ORB",   cv::ORB::create()));
	mFeatureDetectors.push_back(make_pair("BRISK", cv::BRISK::create()));

	// TODO: these want some different Mat type... figure out the conversion
//	mFeatureDetectors.push_back(make_pair("KAZE", cv::KAZE::create()));
//	mFeatureDetectors.push_back(make_pair("SURF", cv::xfeatures2d::SURF::create()));
//	mFeatureDetectors.push_back(make_pair("SIFT", cv::xfeatures2d::SIFT::create()));

	// Only implement detect:

	// Corner detectors
//	mFeatureDetectors.push_back(make_pair("FAST", cv::FastFeatureDetector::create()));
//	mFeatureDetectors.push_back(make_pair("AGAST", cv::AgastFeatureDetector::create()));
//	mFeatureDetectors.push_back(make_pair("Star", cv::xfeatures2d::StarDetector::create()));


	// Only implements compute:

	//	mFeatureDetectors.push_back(make_pair("FREAK", cv::xfeatures2d::FREAK::create()));
	//	mFeatureDetectors.push_back(make_pair("LUCID", cv::xfeatures2d::LUCID::create(  1 // 3x3 lucid descriptor kernel
	//															   , 1 // 3x3 blur kernel
	//															   )));

	// (so, TODO: handle these and allow swapping detectors and computers separately
}

Feature2DRef TokenMatcher::getFeatureDetector() {
	return mFeatureDetectors[mCurrentDetector].second;
}

// Detects features for each given countour and create AnalyzedToken objects from them
vector<AnalyzedToken> TokenMatcher::tokensFromContours( const Pipeline::StageRef world,
										const ContourVector &contours,
										Pipeline&pipeline )
{
	vector<AnalyzedToken> tokens;

	if ( !world || world->mImageCV.empty() ) return tokens;
	
	for ( auto c: contours )
	{
		if (c.mIsHole||c.mTreeDepth>0) {
			continue;
		}


		// Calculate a rect to crop out the interior
		Rectf boundingRect = c.mBoundingRect;

		Rectf imageSpaceRect = boundingRect.transformed(mat4to3(world->mWorldToImage));

		cv::Rect cropRect = toOcv(Area(imageSpaceRect));

		// Skip tiny objects
		if (cropRect.size().width < 2 || cropRect.size().height < 2) {
			continue;
		}

		// Create an image by cropping the contour's interior
		UMat tokenContourImage = world->mImageCV(cropRect);

		// Run keypoint analysis on the cropped image
		// (creating a copy to avoid issues with UMat->Mat leaving dangling references)
		Mat imageCopy;
		tokenContourImage.getMat(ACCESS_READ).copyTo(imageCopy);
		AnalyzedToken analyzedToken = analyzeToken(imageCopy);
		analyzedToken.index = tokens.size();

		// Record the original contour the token arose from
		TokenContour tokenContour;
		tokenContour.polyLine = c.mPolyLine;
		tokenContour.boundingRect = c.mBoundingRect;

		// Remove the offset of the imageSpaceRect, we don't need it
		imageSpaceRect.offset(-imageSpaceRect.getUpperLeft());
		// Get a matrix for mapping points drawn relative to the token back into world-space
		tokenContour.tokenToWorld = getRectMappingAsMatrix(imageSpaceRect, boundingRect);

		analyzedToken.fromContour = tokenContour;

		tokens.push_back(analyzedToken);
	}
	return tokens;
}

AnalyzedToken TokenMatcher::analyzeToken(Mat tokenImage) {
	AnalyzedToken analyzedToken;
	getFeatureDetector()->detectAndCompute(tokenImage,
										   noArray(),
										   analyzedToken.keypoints,
										   analyzedToken.descriptors);
	analyzedToken.image = tokenImage;

	return analyzedToken;
}

vector<TokenMatch> TokenMatcher::matchTokens( vector<AnalyzedToken> candidates )
{

	vector <TokenMatch> matches;

	cout << "***Scoring images..." << endl;
	for ( AnalyzedToken candidateToken : candidates )
	{
		float bestMatchScore = 0;
		AnalyzedToken bestMatch;

		cout << "Scoring image..." << endl;
		for ( AnalyzedToken libraryToken : mTokenLibrary ) {


			int numMatched = doKnnMatch(libraryToken.descriptors, candidateToken.descriptors);
			
			float finalMatchScore = numMatched;

			cout << "\t" << libraryToken.name << " : " << finalMatchScore << endl;

			if (bestMatchScore < finalMatchScore) {
				bestMatchScore = finalMatchScore;
				bestMatch = libraryToken;
			}
		}
		matches.push_back(make_pair(bestMatch, candidateToken));
	}
	return matches;
}

// Via http://docs.opencv.org/3.0-beta/doc/tutorials/features2d/akaze_matching/akaze_matching.html#akazematching
int TokenMatcher::doKnnMatch(Mat descriptorsA, Mat descriptorsB) {
	// nn_matches will be same size as descriptorsA.keypoints.
	vector<vector<DMatch>> nn_matches;
	mMatcher.knnMatch(descriptorsA,
				      descriptorsB,
					  nn_matches,
					  2);
	int numMatched = 0;
	for(size_t i = 0; i < nn_matches.size(); i++) {
		float dist1 = nn_matches[i][0].distance;
		float dist2 = nn_matches[i][1].distance;

		if(dist1 < mParams.mNNMatchRatio * dist2) {
			numMatched++;
		}
	}
	return numMatched;
}

void TokenMatcher::setParams( Params p )
{
	mParams=p;
	
	mTokenLibrary.clear();
	for ( fs::path path : mParams.mTokenLibraryPaths )
	{
		cv::Mat input;
		
		try
		{
			input = Mat( toOcv( Channel( loadImage(path) ) ) );
			
			AnalyzedToken features = analyzeToken(input);
			features.index = mTokenLibrary.size();
			features.name = path.string();
			mTokenLibrary.push_back( features );
		}
		catch (...) {
			cout << "TokenMatcher failed to load library image " << path << endl;
		} // try
	} // for
}
