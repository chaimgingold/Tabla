//
//  TokenMatcher.h
//  PaperBounce3
//
//  Created by Luke Iannini on 12/12/16.
//
//

#ifndef TokenMatcher_h
#define TokenMatcher_h

#include <stdio.h>

#include "GameWorld.h"
#include "Contour.h"

#include "opencv2/features2d.hpp"
#include "opencv2/opencv.hpp"
#include "xfeatures2d.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cv::xfeatures2d;
using namespace cv;

typedef cv::Ptr<cv::Feature2D> Feature2DRef;

typedef pair<int, int> MatchingTokenIndexPair;

struct TokenFeatures {
	vector<KeyPoint> keypoints;
	Mat              descriptors;
	int              index=0;
};

struct Token {
	// Set during feature detection
	TokenFeatures    features;
	PolyLine2        polyLine;
	Rectf            boundingRect;

	mat4             tokenToWorld;
	// Set on comparison
	vector<KeyPoint> matched;
	vector<KeyPoint> inliers;
	vector<DMatch>   good_matches;
};


class TokenMatcher {
public:
	int mCurrentDetector=0;
	vector<pair<string, Feature2DRef>> mFeatureDetectors;

	BFMatcher mMatcher;

	// Distance threshold to identify inliers
	float mInlierThreshold=2.5;
	// Nearest neighbor matching ratio
	float mNNMatchRatio=0.8;
	float mNNMatchPercentage=0.8;

	Feature2DRef getFeatureDetector();

	void setParams( XmlTree xml );
	TokenMatcher();

	TokenFeatures featuresFromImage(Mat tokenImage);

	vector<Token> findTokens(  const Pipeline::StageRef world,
							   const ContourVector &contours,
							   Pipeline&pipeline );

	vector<MatchingTokenIndexPair> matchTokens( vector<TokenFeatures> tokenLibrary,
												vector<TokenFeatures> candidates );
};

#endif /* TokenMatcher_h */
