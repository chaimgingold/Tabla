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

#include "xfeatures2d.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cv::xfeatures2d;

class Token {

public:
	vec2 mLoc;
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
//	void keyDown( KeyEvent ) override;

protected:


private:
	cv::Ptr<cv::Feature2D> mFeatureDetector;
	std::vector<cv::KeyPoint> mKeypoints;

	Pipeline::StageRef mWorld;

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
