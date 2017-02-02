//
//  ProjectorCalibrateWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/12/17.
//
//

#ifndef ProjectorCalibrateWorld_hpp
#define ProjectorCalibrateWorld_hpp

#include "GameWorld.h"

class ProjectorCalibrateWorld : public GameWorld
{
public:

	ProjectorCalibrateWorld();
	
	string getSystemName() const override { return "ProjectorCalibrateWorld"; }
	void setParams( XmlTree ) override;

	void update() override;
	void updateVision( const Vision::Output&, Pipeline& ) override;
	
	void draw( DrawType ) override;

private:

	// params
	
	bool	mVerbose=true;
	float	mRectSize = 100.f;
	float	mHoleSize = 20.f;
	float	mHoleOffset = 20.f;
	int     mWaitFrameCount = 60;
	// all these are in pixels!
	
	//
	void setRegistrationRects();
	Rectf mProjectRect;
	Rectf mProjectRectHole;
	
	struct Found
	{
		int mRect=-1;
		int mHole=-1;
		int mCorner=-1;
		
		bool isComplete() const {
			return mRect!=-1 && mHole !=-1 && mCorner != -1;
		}
	};
	Found findRegistrationRect( const ContourVec& ) const;
	void updateCalibration ( Rectf in, PolyLine2 out, int outStart );
	// we assume input starts at x1,y1

	Pipeline::StageRef mProjectorStage;
	ContourVec mContours;
	ContourVec mFoundContours;
	
	Found mFound;
	int mWaitFrames=0;
};

#endif /* ProjectorCalibrateWorld_hpp */
