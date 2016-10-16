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
#include "xml.h"
#include "Pipeline.h"
#include "ocv.h"

#define MAX_SCORES 3

using namespace std::chrono;

// ShapeTracker was started to do inter-frame coherency,
// but I realized I don't need this to start with.
// It's gravy.
/*
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
*/

bool MusicWorld::Score::setQuadFromPolyLine( PolyLine2 poly, vec2 timeVec )
{
	// could have simplified a bit and just examined two bisecting lines. oh well. it works.
	// also calling this object 'Score' and the internal scores 'score' is a little confusing.
	if ( poly.size()==4 )
	{
		auto in = [&]( int i ) -> vec2
		{
			return poly.getPoints()[i%4];
		};
		
		auto scoreEdge = [&]( int from, int to )
		{
			return dot( timeVec, normalize( in(to) - in(from) ) );
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
		
		// copy to mQuad
		for ( int i=0; i<4; ++i ) mQuad[i] = in(bestSide+i);
		
		return true ;
	}
	else return false;
}

PolyLine2 MusicWorld::Score::getPolyLine() const
{
	PolyLine2 p;
	p.setClosed();
	
	for( int i=0; i<4; ++i ) p.getPoints().push_back( mQuad[i] );
	
	return p;
}

float MusicWorld::Score::getPlayheadFrac() const
{
	// could modulate quadPhase by size/shape of quad
	
	float t = fmod( (ci::app::getElapsedSeconds() - mStartTime)/mPhase, 1.f ) ;
	
	return t;
}

void MusicWorld::Score::getPlayheadLine( vec2 line[2] ) const
{
	float t = getPlayheadFrac();
	
	line[0] = lerp(mQuad[0],mQuad[1],t);
	line[1] = lerp(mQuad[3],mQuad[2],t);
}



MusicWorld::MusicWorld()
{
	mStartTime = ci::app::getElapsedSeconds();

	mTimeVec = vec2(0,-1);
	mPhase   = 8.f;
	
	setupSynthesis();
}

void MusicWorld::setParams( XmlTree xml )
{
	getXml(xml,"Phase",mPhase);
	getXml(xml,"TimeVec",mTimeVec);
}

void MusicWorld::updateContours( const ContourVector &contours )
{
	// erase old scores
	mScore.clear();
	
	// get new ones
	for( const auto &c : contours )
	{
		if ( !c.mIsHole && c.mPolyLine.size()==4 )
		{
			Score score;
			
			score.setQuadFromPolyLine(c.mPolyLine,mTimeVec);
			score.mStartTime = mStartTime;
			score.mPhase = mPhase; // inherit, but it could be custom based on shape or something
			
			mScore.push_back(score);
		}
	}
}

void MusicWorld::updateCustomVision( Pipeline& pipeline )
{
	// this function grabs the bitmaps for each score
	
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("clipped");
	if ( !world || world->mImageCV.empty() ) return;
	
	// for each score...
	vec2 outsize(100,100);

	vec2		srcpt[4];
	cv::Point2f srcpt_cv[4];
	vec2		dstpt[4]    = { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };
	cv::Point2f dstpt_cv[4]	= { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };

	int scoreNum=1;
	
	for( Score& s : mScore )
	{
		// get src points
		for ( int i=0; i<4; ++i )
		{
			srcpt[i] = vec2( world->mWorldToImage * vec4(s.mQuad[i],0,1) );
			srcpt_cv[i] = toOcv( srcpt[i] );
		}
		
		// use default dstpts
		
		// grab it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt_cv, dstpt_cv ) ;
		cv::warpPerspective( world->mImageCV, s.mImage, xform, cv::Size(outsize.x,outsize.y) );

		//
		pipeline.then( string("score")+toString(scoreNum++), s.mImage);
		pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(dstpt,s.mQuad) );

		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;
	}
}

void MusicWorld::update()
{
	updateScoreSynthesis();
}

void MusicWorld::updateScoreSynthesis() {
	// send scores to Pd
	int scoreNum=0;
	
	for( const auto &score : mScore )
	{
		// Create a float version of the image
		cv::Mat imageFloatMat;
		
		// Copy the uchar version scaled 0-1
		score.mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
		
		// Convert to a vector to pass to Pd
		std::vector<float> imageVector;
		imageVector.assign( (float*)imageFloatMat.datastart, (float*)imageFloatMat.dataend );

		mPureDataNode->writeArray(string("image")+toString(scoreNum), imageVector);
		scoreNum++;
	}

	// Clear remaining scores
	std::vector<float> emptyVector(10000, 1.0);
	for( int remaining=scoreNum; remaining < MAX_SCORES; remaining++ ) {
		mPureDataNode->writeArray(string("image")+toString(remaining), emptyVector);
	}
}

void MusicWorld::draw( bool highQuality )
{
	for( const auto &score : mScore )
	{
		gl::color(.5,0,0);
//		gl::drawSolid( score.getPolyLine() );
		gl::draw( score.getPolyLine() );
		
		//
		gl::color(0,1,0);
		vec2 playhead[2];
		score.getPlayheadLine(playhead);
		gl::drawLine( playhead[0], playhead[1] );
	}
	
	// draw time direction (for debugging score generation)
	if (0)
	{
		gl::color(0,1,0);
		gl::drawLine( vec2(0,0), vec2(0,0) + mTimeVec*10.f );
	}
}

// Synthesis
void MusicWorld::setupSynthesis()
{
	// Create the synth engine
	mPureDataNode = cipd::PureDataNode::Global();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/music.pd")) );
}

MusicWorld::~MusicWorld() {
	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
