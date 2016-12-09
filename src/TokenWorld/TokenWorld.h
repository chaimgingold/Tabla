//
//  TokenWorld.h
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#ifndef TokenWorld_h
#define TokenWorld_h


#include <vector>
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Color.h"

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

class TokenWorld : public GameWorld
{
public:
	TokenWorld();

	string getSystemName() const override { return "TokenWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const ContourVector &c, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;


	void mouseClick( vec2 p ) override {  }
	void keyDown( KeyEvent ) override;

protected:


private:
	int mCurrentDetector=0;
	vector<pair<string, Feature2DRef>> mFeatureDetectors;
	
	BFMatcher mMatcher;

	vector<cv::KeyPoint> mGlobalKeypoints;
	vector<cv::Mat>      mGlobalDescriptors;

	Pipeline::StageRef   mWorld;
	ContourVector		 mContours;

	vector<Token> mTokens;

	vector<pair<int, int>> mMatches;

	void detectTokens();

	Feature2DRef getFeatureDetector();

	// Distance threshold to identify inliers
	float mInlierThreshold=2.5;
	// Nearest neighbor matching ratio
	float mNNMatchRatio=0.8;
};

class TokenWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "TokenWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<TokenWorld>();
	}
};


#endif /* TokenWorld_h */
