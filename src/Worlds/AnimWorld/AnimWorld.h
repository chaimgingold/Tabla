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
	bool isFirstAnimFrame() const { return isAnimFrame() && mFirstFrameIndex==mIndex; }
	
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

class AnimSeq : public vector<int> // indices of frames 1..N
{
public:
	AnimSeq(){}
	AnimSeq( vector<int> v ) {
		insert(begin(), v.begin(), v.end());
	}
	
	int mCurrentFrameIndex=-1; // index into FrameVec
};

class AnimSeqMap : public map<int,AnimSeq>
{
public:
};

class AnimWorld : public GameWorld
{
public:
	AnimWorld();

	string getSystemName() const override { return "AnimWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output&, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;

	void drawMouseDebugInfo( vec2 ) override;
	
private:
	vec2 getTimeVec() const { return mTimeVec; } // right
	vec2 getUpVec() const { return -perp(mTimeVec); }
		// up (??? wtf -perp() works right, and perp() doesnt)
		// i think because this single vector isn't enough data to construct a full reference frame.
		// but it works with music. but maybe that's a fluke.
		// it may be as simple as the inverted y coordinate frame that is confusing me: everything is actually upside down!
		// we want y+ = up, but y- is up in the general scheme of things, so best to be consistent with that.
	
	mat2 mLocalToGlobal;
	mat2 mGlobalToLocal;
	// global is world space
	// local is temporal space time+,up
	
	PolyLine2 globalToLocal( PolyLine2 ) const;
	Rectf getContourBBoxInLocalSpace( const Contour& ) const;
	
	float mAnimTime; // our local animation time, in case we want to pause, etc...
	vec2 mTimeVec;
	float mWorldUnitsToSeconds;
	float mMaxFrameDist; // for same anim
	
	// scope is mFrames
	FrameVec mFrames;
	AnimSeqMap mAnims; // key is index of first frame
	
	AnimSeqMap getAnimSeqs( const FrameVec& ) const;
	int getCurrentFrameIndexOfSeq( const FrameVec&, const AnimSeq& ) const;
	vector<int> getFrameIndicesOfSeq( const FrameVec&, const Frame& firstFrame ) const;
	void updateCurrentFrames( AnimSeqMap&, const FrameVec&, float currentTime );

	// frame parsing
	FrameVec getFrames( const Pipeline::StageRef, const ContourVector &contours, Pipeline&pipeline ) const;
	FrameVec getFrameTopology( const FrameVec& ) const;
	int getSuccessorFrameIndex( const Frame&, const FrameVec& ) const;
	int getFirstFrameIndexOfAnimForScreen( const Frame&, const FrameVec& ) const;
	int getAdjacentFrameIndex( const Frame&, const FrameVec&, vec2 direction, float* distance=0 ) const;
	
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
