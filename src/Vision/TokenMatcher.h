//
//  TokenMatcher.h
//  PaperBounce3
//
//  Created by Luke Iannini on 12/12/16.
//
//

#ifndef TokenMatcher_h
#define TokenMatcher_h

#include <thread>
#include <chrono>

#include "cinder/Xml.h"

#include "channel.h"
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

class TokenMatch
{
public:
	TokenMatch( AnalyzedToken library, AnalyzedToken candidate, float score )
	: mLibrary(library)
	, mCandidate(candidate)
	, mScore(score){}

	string getName() const { return mLibrary.name; }
	const PolyLine2& getPoly() const { return mCandidate.fromContour.polyLine; }
	
	const AnalyzedToken& getCandidate() const { return mCandidate; }

	float getScore() const { return mScore; }
	
private:
	AnalyzedToken mLibrary;
	AnalyzedToken mCandidate;
	float mScore;
};

class TokenMatchVec : public vector<TokenMatch>
{
public:
	const TokenMatch* doesOverlapToken( const PolyLine2& ) const;
	// useful for filtering out contours that describe tokens
	// What we do is N poly<>poly intersection tests.
	
};

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
		
		bool mVerbose   = false;
		bool mIsEnabled = true;

		// Filtering out degenerate contours

		// Checks that the smallest edge / largest edge
		// is roughly square-ish
		float mTokenContourMinAspect=0.7;
		float mTokenContourMinWidth=5;
		float mTokenContourMaxWidth=10;
		
		// Tuning
		// Distance threshold to identify inliers
		float mInlierThreshold=2.5;
		// Nearest neighbor matching ratio
		float mNNMatchRatio=0.8;
		float mNNMatchPercentage=0.8;

		// Minimum score to be considered a matching token
		float mMinMatchScore=10;

		// token library
		struct TokenDef
		{
			fs::path mPath;
			string   mName;
		};
		vector<TokenDef> mTokenDefs;
	};
	
	void setParams( Params );
	TokenMatcher();

	bool isEnabled() const { return mParams.mIsEnabled; }
	
	AnalyzedToken analyzeToken(Mat tokenImage);

	vector<AnalyzedToken> tokensFromContours(
							   const Pipeline::StageRef world,
							   const ContourVector &contours,
							   Pipeline&pipeline );

	TokenMatchVec matchTokens( vector<AnalyzedToken> candidates );
	
	const vector<AnalyzedToken>& getTokenLibrary() const { return mTokenLibrary; }

	void reanalyze();
	
private:
	Params mParams;
	vector<AnalyzedToken> mTokenLibrary;
	vec2 mAverageLibraryTokenSize = vec2(0,0);
	int doKnnMatch(Mat descriptorsA, Mat descriptorsB);
	
};

class TokenMatcherThreaded {
public:
	TokenMatcherThreaded();
	~TokenMatcherThreaded();

	struct Input
	{
		Pipeline mPipeline;
		ContourVec mContours;
		float mTimeStamp; // you set it
	};
	
	struct Output
	{
		Pipeline mPipeline;
		TokenMatchVec mTokens;
		float mTimeStamp; // your input time stamp
	};
	
	void setParams( TokenMatcher::Params );

	// assuming you are calling all of these three from a single other thread!
	// (or we will need to put another mutex guard around these :P)
	void put( Input );
	bool get( Output& );
	bool isBusy() { return mBusy>0; }
	void stop(); // idempotent; also called by destructor. call during shutdown
	
private:
	int mBusy=0;
	
	channel<Input>  mIn;
	channel<Output> mOut;
	
	mutex  mInputLock;
	thread mThread;		
	
	TokenMatcher mMatcher;
	
};

#endif /* TokenMatcher_h */
