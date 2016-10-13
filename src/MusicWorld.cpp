//
//  MusicWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "MusicWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"

using namespace std::chrono;

class ShapeTracker
{
public:
	
	enum class ShapeState
	{
		Found,
		Missing
	};
	
	class Shape
	{
	public:
	private:
		friend class ShapeTracker;
		
		int	mId=0; // unique id for inter-frame tracking
		
		PolyLine2 mCreatedPoly;
		PolyLine2 mLastSeenPoly;
		
		ShapeState	mState;
		float		mEnteredStateWhen;

		Shape()
		{
			setState(ShapeState::Found);
		}

		void setState( ShapeState s )
		{
			mState = ShapeState::Found;
			mEnteredStateWhen = ci::app::getElapsedSeconds();
		}
	};
	
	void update( const ContourVector& );
	const Shape* getShape( int id ) const;
	
private:
	int getNewShapeId() { return mNextId++; }

	void indexShapes();
	
	int mNextId=1;
	vector<Shape>	mShapes;
	map<int,int>	mShapeById; // maps shape ids to mShapes indices
};

void ShapeTracker::update( const ContourVector& contours )
{
	// we're just going to filter for 4 cornered things for now
	const int kNumCorners = 4;
	
	for( const auto &c : contours )
	{
		if ( c.mPolyLine.size()==kNumCorners &&
			 c.mIsHole==false )
		{
			
		}
	}
}

const ShapeTracker::Shape* ShapeTracker::getShape( int id ) const
{
	auto i = mShapeById.find(id);
	
	if ( i == mShapeById.end() ) return 0;
	else return &mShapes[i->second];
}

void ShapeTracker::indexShapes()
{
	mShapeById.clear();
	
	for( int i=0; i<mShapes.size(); ++i )
	{
		mShapeById[mShapes[i].mId] = i;
	}
}

MusicWorld::MusicWorld()
{
	mStartTime = ci::app::getElapsedSeconds();

	mTimeVec = vec2(0,-1);
	
//	setupSynthesis();
//	cg: pd code seems to be causing crashes, but same code as PongWorld. (???)
}

void MusicWorld::update()
{
}

void MusicWorld::getQuadOrderedSides( const PolyLine2& p, vec2 out[4] )
{
	// could have simplified a bit and just examined two bisecting lines. oh well. it works.
	assert( p.size()==4 );

	/*  Output points in this order:
	
		0---2
	    |   |
		1---3
		
		 --> time
	*/
	
	auto in = [&]( int i ) -> vec2
	{
		return p.getPoints()[i%4];
	};
	
	auto scoreEdge = [&]( int from, int to )
	{
		return dot( mTimeVec, normalize( in(to) - in(from) ) );
	};
	
	auto scoreSide = [&]( int side )
	{
		/* input pts:
		   0--1
		   |  |
		   3--2
		   
		   side 0 : score of 0-->1, 3-->2
		*/
		
		return ( scoreEdge(side+0,side+1) + scoreEdge(side+3,side+2) ) / 2.f;
	};
	
	int   bestSide =0;
	float bestScore=0;
	
	for( int i=0; i<4; ++i )
	{
		float score = scoreSide(i);
		
		if ( score > bestScore )
		{
			bestScore = score ;
			bestSide  = i;
		}
	}
	
	// copy to out
	out[0] = in(bestSide+0);
	out[1] = in(bestSide+3);
	
	out[2] = in(bestSide+1);
	out[3] = in(bestSide+2);
}

void MusicWorld::draw( bool highQuality )
{
	const float kPhase = 8.f;
	const float t = fmod( (ci::app::getElapsedSeconds() - mStartTime)/kPhase, 1.f ) ;
	
	for( const auto &c : mContours )
	{
		if ( c.mPolyLine.size()==4 )
		{
			gl::color(.5,0,0);
			gl::drawSolid(c.mPolyLine);
			
			//
			vec2 pts[4];
			getQuadOrderedSides(c.mPolyLine,pts);
			
			gl::color(0,1,0);
			gl::drawLine( lerp(pts[1],pts[3],t), lerp(pts[0],pts[2],t) );
		}
	}
	
	gl::color(0,1,0);
	gl::drawLine( vec2(0,0), vec2(0,0) + mTimeVec*10.f );
}

// Synthesis
void MusicWorld::setupSynthesis()
{
	auto ctx = audio::master();
	
	// Create the synth engine
	mPureDataNode = ctx->makeNode( new cipd::PureDataNode( audio::Node::Format().autoEnable() ) );
	
	// Enable Cinder audio
	ctx->enable();
	
	// Load pong synthesis
	mPureDataNode->disconnectAll();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
	
	// Connect synth to master output
	mPureDataNode >> audio::master()->getOutput();
}

MusicWorld::~MusicWorld() {
	// Clean up synth engine
//	mPureDataNode->disconnectAll();
//	mPureDataNode->closePatch(mPatch);
}