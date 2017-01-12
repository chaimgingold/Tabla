//
//  RibbonWorld.h
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#ifndef RibbonWorld_h
#define RibbonWorld_h


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
#include "FileWatch.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cv;

struct Ribbon {
	int ID=0;
	TriMeshRef triMesh;
	vec2 lastPoint;
	vec2 lastP1;
	vec2 lastP2;
	int numPoints=0;
	float birth;
};

class RibbonWorld : public GameWorld
{
public:
	RibbonWorld();

	string getSystemName() const override { return "RibbonWorld"; }

	void setParams( XmlTree ) override;

	void updateVision( const Vision::Output&, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;


	void mouseClick( vec2 p ) override {  }
	void keyDown( KeyEvent ) override;

protected:

private:
	FileWatch mFileWatch;
	gl::GlslProgRef mRibbonShader;

	Pipeline::StageRef mWorld;
	ContourVector mContours;

	vector<Ribbon> mRibbons;


	// Params
	int mMaxRibbons=50;
	int mMaxRibbonLength=50;
	float mSnipDistance=10;
	float mPointDistance=0.1;
	float mWidthDivision=10;
};

#endif /* RibbonWorld_h */
