//
//  QuadTestWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "PaperBounce3App.h" // for text
#include "QuadTestWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;

PolyLine2 QuadTestWorld::Frame::getQuadAsPoly() const
{
	PolyLine2 p;
	
	for( int i=0; i<4; ++i ) p.push_back(mQuad[i]);
	
	p.setClosed();
	return p;
}

QuadTestWorld::QuadTestWorld()
{
}

void QuadTestWorld::setParams( XmlTree xml )
{
	getXml(xml,"TimeVec",mTimeVec);

	if ( xml.hasChild("RectFinder") ) {
		mRectFinder.mParams.set( xml.getChild("RectFinder") );
	}
}

void QuadTestWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	const Pipeline::StageRef source = pipeline.getStage("clipped");
	if ( !source || source->mImageCV.empty() ) return;

	mFrames = getFrames( source, visionOut.mContours, pipeline );
}

QuadTestWorld::FrameVec QuadTestWorld::getFrames(
	const Pipeline::StageRef world,
	const ContourVector &contours,
	Pipeline& pipeline ) const
{
	FrameVec frames;
	if ( !world || world->mImageCV.empty() ) return frames;

	for ( auto c : contours )
	{
		if ( c.mIsHole || c.mTreeDepth>0 ) continue;

		// do it
		Frame frame;
		frame.mContourPoly = c.mPolyLine;
		frame.mIsValid = mRectFinder.getRectFromPoly(frame.mContourPoly,frame.mContourPolyReduced);
		frame.mConvexHull = getConvexHull(frame.mContourPoly);
		
		PolyLine2* diffPoly=0;
		
		if ( frame.mContourPolyReduced.size() > 0 ) {
			diffPoly = &frame.mContourPolyReduced; 
		} else {
			diffPoly = &frame.mConvexHull;
		}

		frame.mOverlapScore = calcPolyEdgeOverlapFrac(
			*diffPoly,
			frame.mContourPoly,
			mRectFinder.mParams.mEdgeOverlapDistAttenuate);
				
		RectFinder::getPolyDiffArea(*diffPoly,frame.mContourPoly,&frame.mReducedDiff);
		
		if ( frame.mIsValid ) {
			getOrientedQuadFromPolyLine(frame.mContourPolyReduced, mTimeVec, frame.mQuad);
		}
		
		frames.push_back(frame);
	}
	return frames;	
}

void QuadTestWorld::update()
{
}

void QuadTestWorld::draw( DrawType drawType )
{
	for( const auto &f : mFrames )
	{
		// fill
		if (1)
		{
			gl::color(0,1,1,.5);
			gl::drawSolid( f.getQuadAsPoly() );
		}
		
		// diff
		if (1)
		{
			gl::color(1, 0, 0, .6f);
			for( const auto &p : f.mReducedDiff )
			{
				gl::drawSolid(p);
			}
		}
		
		// frame
		if (1)
		{
			gl::color(1,0,0);
			gl::draw( f.mContourPolyReduced.size()==0 ? f.mConvexHull : f.mContourPolyReduced );
			
//			if ( !f.mIsValid && f.mConvexHull.size()>0 )
			if (1)
			{
				PaperBounce3App::get()->mTextureFont->drawString(
				//	toString(f.mConvexHull.size()) + " " + toString(f.mContourPoly.calcArea()),	
					toString(floorf(f.mOverlapScore*100.f)) + "%",
					//f.mConvexHull.getPoints()[0] + vec2(0,-1),
					f.mContourPoly.calcCentroid(),
					gl::TextureFont::DrawOptions().scale(1.f/8.f).pixelSnap(false)
					);
			}
		}
		
		// frame corners 
		if (f.mIsValid)
		{
			Color c[4] = { Color(1,0,0), Color(0,1,0), Color(0,0,1), Color(1,1,1) };
			
			for( int i=0; i<4; ++i )
			{
				gl::color(c[i]);
				gl::drawSolidCircle(f.mQuad[i],1.f);
			}
		}
	}
}

void QuadTestWorld::drawMouseDebugInfo( vec2 v )
{
//	cout << v << " => " << mGlobalToLocal * v << endl;
}
