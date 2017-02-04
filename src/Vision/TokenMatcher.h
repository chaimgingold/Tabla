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

#include "cinder/Xml.h"

#include "Pipeline.h"
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



struct TokenContour {

	PolyLine2        polyLine;
	Rectf            boundingRect;
	mat4             tokenToWorld;


	// Set on comparison
	vector<KeyPoint> matched;
	vector<KeyPoint> inliers;
	vector<DMatch>   good_matches;
};

struct AnalyzedToken {
	string			 name;
	int              index=0;
	Mat              image;
	Mat              descriptors;
	vector<KeyPoint> keypoints;
	TokenContour     fromContour;
};

typedef pair<AnalyzedToken, AnalyzedToken> TokenMatch;

class TokenMatcher {
public:
	int mCurrentDetector=0;
	vector<pair<string, Feature2DRef>> mFeatureDetectors;

	BFMatcher mMatcher;

	Feature2DRef getFeatureDetector();

	class Params
	{
	public:
	
		void set( XmlTree );
		
		// Tuning
		// Distance threshold to identify inliers
		float mInlierThreshold=2.5;
		// Nearest neighbor matching ratio
		float mNNMatchRatio=0.8;
		float mNNMatchPercentage=0.8;

		// token library
		vector<fs::path> mTokenLibraryPaths;
	};
	
	void setParams( Params );
	TokenMatcher();

	AnalyzedToken analyzeToken(Mat tokenImage);

	vector<AnalyzedToken> tokensFromContours(
							   const Pipeline::StageRef world,
							   const ContourVector &contours,
							   Pipeline&pipeline );

	vector<TokenMatch> matchTokens( vector<AnalyzedToken> candidates );
	
	const vector<AnalyzedToken>& getTokenLibrary() const { return mTokenLibrary; }
	
private:
	Params mParams;
	vector<AnalyzedToken> mTokenLibrary;
	int doKnnMatch(Mat descriptorsA, Mat descriptorsB);
	
};

#endif /* TokenMatcher_h */
