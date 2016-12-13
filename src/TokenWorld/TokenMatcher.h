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



struct Token {
	// Set during feature detection
	vector<KeyPoint> keypoints;
	Mat              descriptors;
	PolyLine2        polyLine;
	Rectf            boundingRect;
	int              index=0;
	mat4             tokenToImage;
	// Set on comparison
	vector<KeyPoint> matched;
	vector<KeyPoint> inliers;
	vector<DMatch>   good_matches;
};


class TokenMatcher {
public:
	int mCurrentDetector=0;
	vector<pair<string, Feature2DRef>> mFeatureDetectors;

	void updateMatches( const Pipeline::StageRef world,
					    const ContourVector &contours,
					    Pipeline&pipeline
					   );

	BFMatcher mMatcher;

	// Distance threshold to identify inliers
	float mInlierThreshold=2.5;
	// Nearest neighbor matching ratio
	float mNNMatchRatio=0.8;
	float mNNMatchPercentage=0.8;

	vector<Token> mTokens;

	vector<pair<int, int>> mMatches;

	Feature2DRef getFeatureDetector();

	void setParams( XmlTree xml );
	TokenMatcher();
};

#endif /* TokenMatcher_h */
