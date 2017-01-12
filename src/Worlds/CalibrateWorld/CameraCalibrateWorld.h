//
//  CameraCalibrateWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/1/16.
//
//

#ifndef CameraCalibrateWorld_hpp
#define CameraCalibrateWorld_hpp

#include "BallWorld.h"

class CameraCalibrateWorld : public GameWorld
{
public:

//	CameraCalibrateWorld();
//	~CameraCalibrateWorld();
	
	string getSystemName() const override { return "CameraCalibrateWorld"; }
	void setParams( XmlTree ) override;

//	void gameWillLoad() override;
	void update() override;
	void updateVision( const Vision::Output&, Pipeline& ) override;
	
	void draw( DrawType ) override;

private:

	// params
	ivec2	mBoardNumCorners = ivec2(7,7);
	bool	mDrawBoard=false;
	int		mNumBoardsToSolve=5;
	float	mUniqueBoardDiffThresh=1.f; // world units per corner
	float	mMinSecsBetweenCameraSamples=1.f;
	bool	mVerbose=true;
	
	//
	void drawChessboard( vec2 c, vec2 size  ) const;
	
	bool areBoardCornersUnique( vector<cv::Point2f> ) const; // make sure different enough from those we know already
	void foundFullSetOfCorners( vector<cv::Point2f> );
	
	//
	bool mIsDone=false;
	
	//
	vector< vector<cv::Point2f> > mKnownBoards;
	void tryToSolveWithKnownBoards( cv::Size );
	
	// for visual feedback; in world space
	vector<vec2> mLiveCorners;
	float mLastCornersSeenWhen=-MAXFLOAT;
	
	bool mLiveAllCornersFound=false;
	
	mat4 mImageToWorld;
	
};

#endif /* CameraCalibrateWorld_hpp */
