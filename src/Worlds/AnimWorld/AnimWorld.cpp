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
	
	mLocalToGlobal = mat2( getTimeVec(), getUpVec() );
	mGlobalToLocal = inverse( mLocalToGlobal );
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
//	return getAdjacentFrame(firstFrameOfSeq, frames, getUpVec());
	
	vector<int> score(frames.size(),0);
	
	for( int n = firstFrameOfSeq.mIndex; n!=-1; n = frames[n].mNextFrameIndex )
	{
		auto &f = frames[n];
		
		int si = getAdjacentFrame(f, frames, getUpVec());
		
		if ( si!=-1 && frames[si].mSeqFrameCount<=1 )
		{
			score[si]++;
		}
	}
	
	int bestscore=0;
	int best=-1;
	for( int i=0; i<score.size(); ++i )
	{
		if ( score[i] > bestscore )
		{
			bestscore=score[i];
			best=i;
		}
	}
	return best;
}

int
AnimWorld::getSuccessorFrameIndex( const Frame& f, const FrameVec& fs ) const
{
	return getAdjacentFrame(f, fs, getTimeVec());
}

int AnimWorld::getAdjacentFrame( const Frame& f, const FrameVec& fs, vec2 direction ) const
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
	
	return best;
}

void AnimWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	const Pipeline::StageRef source = pipeline.getStage("clipped");
	if ( !source || source->mImageCV.empty() ) return;

	mFrames = getFrames( source, visionOut.mContours, pipeline );
	
	mFrames = getFrameTopology( mFrames );
}

void AnimWorld::update()
{

}

void AnimWorld::draw( DrawType drawType )
{
	for( const auto &f : mFrames )
	{
		if ( f.isAnimFrame() )
		{
			gl::color( lerp( Color(0,1,1),Color(1,.5,0), (float)(f.mSeqFrameNum-1) / (float)(f.mSeqFrameCount-1) ) );
			gl::drawSolid(f.mContour.mPolyLine);
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
