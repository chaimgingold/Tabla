//
//  AnimWorld.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#ifndef AnimWorld_h
#define AnimWorld_h


#include <vector>
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Color.h"

#include "GameWorld.h"
#include "Contour.h"
#include "geom.h"

using namespace ci;
using namespace ci::app;
using namespace std;


namespace Anim
{


class Frame
{
public:
	Contour mContour;
	mat4	mFrameImageToWorld;
	UMat	mImageCV;
	
	bool isScreen() const { return mScreenFirstFrameIndex!=-1; }
	bool isAnimFrame() const { return mSeqFrameCount>1; }
	
	// topology
	int		mIndex=-1;
	int		mNextFrameIndex=-1;
	int		mPrevFrameIndex=-1;
	int		mFirstFrameIndex=-1;
	
//	int		mScreenIndex=-1; // what frame plays my animation?
	int		mScreenFirstFrameIndex=-1;
	
	// animation
	int		mSeqFrameNum=-1; // x of y frames (1 indexed)
	int		mSeqFrameCount=-1; // y frames
};
typedef vector<Frame> FrameVec;


class AnimWorld : public GameWorld
{
public:
	AnimWorld();

	string getSystemName() const override { return "AnimWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output&, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;

private:
	vec2 getTimeVec() const { return mTimeVec; } // right
	vec2 getUpVec() const { return -perp(mTimeVec); } // up (??? wtf -perp() works right, and perp() doesnt)
	
	mat2 mLocalToGlobal;
	mat2 mGlobalToLocal;
	// global is world space
	// local is temporal space time+,up
	
	vec2 mTimeVec;
	
	FrameVec getFrames( const Pipeline::StageRef, const ContourVector &contours, Pipeline&pipeline ) const;
	FrameVec getFrameTopology( const FrameVec& ) const;
	int getSuccessorFrameIndex( const Frame&, const FrameVec& ) const;
	int getScreenFrameIndex( const Frame& firstFrameOfSeq, const FrameVec& ) const;
	int getAdjacentFrame( const Frame&, const FrameVec&, vec2 direction ) const;
	
	FrameVec mFrames;

};


} // namespace Anim

class AnimWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "AnimWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<Anim::AnimWorld>();
	}
};


#endif /* AnimWorld_h */
