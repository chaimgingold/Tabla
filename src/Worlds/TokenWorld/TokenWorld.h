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

#include "TokenMatcher.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cv::xfeatures2d;
using namespace cv;




enum class TokenVisionMode
{
	Global=0,
	Matching,
	kNumModes
};

class TokenWorld : public GameWorld
{
public:
	TokenWorld();

	string getSystemName() const override { return "TokenWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output&, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;


	void mouseClick( vec2 p ) override {  }
	void keyDown( KeyEvent ) override;

protected:


private:
	TokenVisionMode mMode=TokenVisionMode::Matching;

	TokenMatcher mTokenMatcher;

	Pipeline::StageRef mWorld;

	vector<cv::KeyPoint> mGlobalKeypoints;
	vector<cv::Mat>      mGlobalDescriptors;

	vector<TokenCandidate> 	       mTokens;
	vector<MatchingTokenIndexPair> mMatches;
	
	void drawMatchingKeypoints();
	void drawGlobalKeypoints();
};

#endif /* TokenWorld_h */
