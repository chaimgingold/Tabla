//
//  AnimWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "AnimWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;
using namespace Anim;

AnimWorld::AnimWorld()
{
}

void AnimWorld::setParams( XmlTree xml )
{
	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"WorldUnitsToSeconds",mWorldUnitsToSeconds);
	
	mLocalToGlobal = mat2( getTimeVec(), getUpVec() );
	mGlobalToLocal = inverse( mLocalToGlobal );
}

AnimSeqMap
AnimWorld::getAnimSeqs( const FrameVec& frames ) const
{
	AnimSeqMap as;
	
	for ( auto i : frames )
	{
		if ( i.isFirstAnimFrame() )
		{
			AnimSeq seq = getFrameIndicesOfSeq(frames,i);
			
			as[i.mIndex] = seq;
		}
	}
	
	return as;
}

int AnimWorld::getCurrentFrameIndexOfSeq( const FrameVec& frames, const AnimSeq& seq ) const
{
	vec2 worldLength = frames[seq.back()].mContour.mCenter - frames[seq.front()].mContour.mCenter;
	float length = dot( worldLength, getTimeVec() ) / mWorldUnitsToSeconds ;
	
	float t = fmod( mAnimTime, length ) / length;
	int index = constrain( (int)(t * (float)seq.size()), 0, (int)seq.size()-1 );
	int frame = seq[index];
	
//	cout << t << " -> " << index << " of " << seq.size() << " -> " << frame << endl;
	
	return frame;
}

void AnimWorld::updateCurrentFrames( AnimSeqMap& seqs, const FrameVec& frames, float currentTime )
{
	for( auto &i : seqs )
	{
		i.second.mCurrentFrameIndex = getCurrentFrameIndexOfSeq( frames, i.second );
	}
}

vector<int>
AnimWorld::getFrameIndicesOfSeq( const FrameVec& frames, const Frame& firstFrame ) const
{
	vector<int> seq;
	
	for( int n = firstFrame.mIndex; n!=-1; n = frames[n].mNextFrameIndex )
	{
		seq.push_back(n);
	}
	
	return seq;
}

FrameVec AnimWorld::getFrames(
	const Pipeline::StageRef world,
	const ContourVector &contours,
	Pipeline& pipeline ) const
{
	FrameVec frames;
	if ( !world || world->mImageCV.empty() ) return frames;

	for ( auto c : contours )
	{
		if ( c.mIsHole || c.mTreeDepth>0 || c.mPolyLine.size()!=4 ) {
			continue;
		}

		Frame frame;

		frame.mIndex = frames.size();
		frame.mContour = c;

		Rectf boundingRect = c.mBoundingRect;

		Rectf imageSpaceRect = boundingRect.transformed(mat4to3(world->mWorldToImage));

		cv::Rect cropRect = toOcv(Area(imageSpaceRect));

		if (cropRect.size().width < 2 || cropRect.size().height < 2) {
			continue;
		}

		frame.mImageCV = world->mImageCV(cropRect);

		imageSpaceRect.offset(-imageSpaceRect.getUpperLeft());
		frame.mFrameImageToWorld = getRectMappingAsMatrix(imageSpaceRect, boundingRect);

		pipeline.then(string("Frame ") + frame.mIndex, frame.mImageCV);
		pipeline.setImageToWorldTransform( frame.mFrameImageToWorld );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;
		
		// equalize?
		if (0)
		{
			Mat frameContourImageEqualized;
			equalizeHist(frame.mImageCV, frameContourImageEqualized);
			pipeline.then(string("Frame equalized ") + frame.mIndex, frameContourImageEqualized);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;
		}

		frames.push_back(frame);
	}
	return frames;
}

FrameVec
AnimWorld::getFrameTopology( const FrameVec& in ) const
{
	// we could also assign frames to be screens if they are blank.
	// but that could also muck people's ability to create and fill in a sequence.
	
	
	FrameVec out=in;
	
	// next/prev
	for( int i=0; i<mFrames.size(); ++i )
	{
		Frame&f = out[i];
		
		f.mNextFrameIndex = getSuccessorFrameIndex(f,out);
		
		// prev index
		if (f.mNextFrameIndex!=-1)
		{
			out[f.mNextFrameIndex].mPrevFrameIndex = i;
		}
	}
	
	// first and sequence index, length
	for ( auto &f : out )
	{
		if ( f.mPrevFrameIndex==-1 ) // is first
		{
			// first (and get length)
			int count=1;
			
			for( int n = f.mIndex; n!=-1; n = out[n].mNextFrameIndex )
			{
				out[n].mFirstFrameIndex = f.mIndex;
				out[n].mSeqFrameNum = count++;
			}
			
			// set length
			int length = count-1;
			
			for( int n = f.mIndex; n!=-1; n = out[n].mNextFrameIndex )
			{
				out[n].mSeqFrameCount = length;
			}
		}
	}
	
	// screen
	for( int i=0; i<mFrames.size(); ++i )
	{
		Frame& f = out[i];
		if ( f.isAnimFrame() && f.mPrevFrameIndex==-1 ) // is first
		{
			int screen = getScreenFrameIndex( f, out );
			
			if (screen!=-1)
			{
				out[screen].mScreenFirstFrameIndex = f.mFirstFrameIndex;
				
//				for( int n = f.mIndex; n!=-1; n = out[n].mNextFrameIndex )
//				{
//					out[n].mScreenIndex = screen;
//				}
			}
		}
	}
	
	
	return out;
}

int
AnimWorld::getScreenFrameIndex( const Frame& firstFrameOfSeq, const FrameVec& frames ) const
{
	if (0)
	{
		// above first
		return getAdjacentFrameIndex(firstFrameOfSeq, frames, getUpVec());
	}
	else
	{
		// voting
		vector<float> score(frames.size(),MAXFLOAT);
		
		for( int n = firstFrameOfSeq.mIndex; n!=-1; n = frames[n].mNextFrameIndex )
		{
			auto &f = frames[n];
			float dist;
			
			int si = getAdjacentFrameIndex(f, frames, getUpVec(), &dist );
			
			if ( si!=-1 && frames[si].mSeqFrameCount<=1 )
			{
				score[si] = min(dist,score[si]) ;
				// subtract?
			}
		}
		
		int bestscore=MAXFLOAT;
		int best=-1;
		for( int i=0; i<score.size(); ++i )
		{
			if ( score[i] < bestscore )
			{
				bestscore=score[i];
				best=i;
			}
		}
		return best;
	}
}

int
AnimWorld::getSuccessorFrameIndex( const Frame& f, const FrameVec& fs ) const
{
	return getAdjacentFrameIndex(f, fs, getTimeVec());
}

int AnimWorld::getAdjacentFrameIndex( const Frame& f, const FrameVec& fs, vec2 direction, float* distance ) const
{
	int best=-1;
	float bestdist=MAXFLOAT;
	
	vec2 origin = f.mContour.mCenter;
	
	for( int i=0; i<fs.size(); ++i )
	{
		//if ( &fs[i] != &f )
		if ( fs[i].mIndex != f.mIndex )
		{
			float t;
			
			if ( fs[i].mContour.rayIntersection(origin, direction, &t) && t < bestdist )
			{
				bestdist = t;
				best=i;
			}
		}
	}
	
	if (distance) *distance = bestdist;
	return best;
}

void AnimWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	const Pipeline::StageRef source = pipeline.getStage("clipped");
	if ( !source || source->mImageCV.empty() ) return;

	mFrames = getFrames( source, visionOut.mContours, pipeline );
	
	mFrames = getFrameTopology( mFrames );
	
	mAnims = getAnimSeqs(mFrames);
	updateCurrentFrames(mAnims,mFrames,mAnimTime);
}

void AnimWorld::update()
{
	mAnimTime = ci::app::getElapsedSeconds();
	
	updateCurrentFrames(mAnims,mFrames,mAnimTime);
}

void AnimWorld::draw( DrawType drawType )
{
	for( const auto &f : mFrames )
	{
		if ( f.isAnimFrame() )
		{
			bool isCurrentFrame = ( mAnims[f.mFirstFrameIndex].mCurrentFrameIndex == f.mIndex );
			
			gl::color( lerp( Color(0,1,1),Color(1,.5,0), (float)(f.mSeqFrameNum-1) / (float)(f.mSeqFrameCount-1) ) );

			if ( isCurrentFrame ) {
				gl::drawSolid(f.mContour.mPolyLine);
			} else {
				gl::draw(f.mContour.mPolyLine);
			}
		}
		else
		{
			gl::color( Color(1,.8,0) );
			gl::draw(f.mContour.mPolyLine);
			
			gl::color(1,0,0);
			gl::drawLine(f.mContour.mCenter, f.mContour.mCenter + getTimeVec() * 5.f );
			gl::color(0,1,0);
			gl::drawLine(f.mContour.mCenter, f.mContour.mCenter + getUpVec() * 5.f );
		}
	}
	
	for( const auto &f : mFrames )
	{
		if ( f.mNextFrameIndex!=-1 )
		{
			gl::color(0,0,1);
			gl::drawLine(f.mContour.mCenter, mFrames[f.mNextFrameIndex].mContour.mCenter );
		}

		if ( f.isScreen() ) //&& f.mScreenFirstFrameIndex!=-1 )
		{
			gl::color(1,0,0);
			gl::drawLine(f.mContour.mCenter, mFrames[f.mScreenFirstFrameIndex].mContour.mCenter );
		}
	}
}
