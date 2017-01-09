//
//  QuadTestWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "TablaApp.h" // for text
#include "QuadTestWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;

/*
PolyLine2 QuadTestWorld::Frame::getQuadAsPoly() const
{
	PolyLine2 p;
	
	for( int i=0; i<4; ++i ) p.push_back(mQuad[i]);
	
	p.setClosed();
	return p;
}*/

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
		
		frame.mIsOK = mRectFinder.getRectFromPoly(
			frame.mContourPoly,
			frame.mContourPolyResult,
			frame.mCandidates
		);

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
		// candidates
		for( const auto &c : f.mCandidates )
		{
			float a = c.mPerimScore;
			
			if (!c.mAllowed) a = max( .15f, a * .25f );
			
			gl::color( 1,1,1, a );
			gl::draw(c.getAsPoly());
		}
		
		// diff
		if (1)
		{
			for( const auto &c : f.mCandidates )
			{
				if ( c.mAllowed )
				{
					gl::color(1, 0, 0, .5f);
					for( const auto &p : c.mPolyDiff )
					{
						if (p.size()>0)
						{
							gl::drawSolid(p);
						}
					}
				}
			}
		}	

		// original poly
		gl::color( 1., 1., .1 );
		gl::draw(f.mContourPoly);				

		// fill solution
		if (f.mIsOK)
		{
			gl::color(0,1,1,.5);
			gl::drawSolid( f.mContourPolyResult );
			gl::color(0,1,0);
			gl::draw( f.mContourPolyResult );
		}

		// frame
		/*
		if (1)
		{
			gl::color(1,0,0);
			gl::draw( f.mContourPolyReduced.size()==0 ? f.mConvexHull : f.mContourPolyReduced );
			
//			if ( !f.mIsValid && f.mConvexHull.size()>0 )
			if (1)
			{
				TablaApp::get()->mTextureFont->drawString(
				//	toString(f.mConvexHull.size()) + " " + toString(f.mContourPoly.calcArea()),	
					toString(floorf(f.mOverlapScore*100.f)) + "%",
					//f.mConvexHull.getPoints()[0] + vec2(0,-1),
					f.mContourPoly.calcCentroid(),
					gl::TextureFont::DrawOptions().scale(1.f/8.f).pixelSnap(false)
					);
			}
		}*/
		
		// frame corners 
		if (f.mIsOK)
		{
			Color c[4] = { Color(1,0,0), Color(0,1,0), Color(0,0,1), Color(1,1,1) };
			
			for( int i=0; i<4; ++i )
			{
				gl::color(c[i]);
				gl::drawSolidCircle(f.mContourPolyResult.getPoints()[i],.5f,6);
			}
		}
	}
}

void QuadTestWorld::drawMouseDebugInfo( vec2 v )
{
//	cout << v << " => " << mGlobalToLocal * v << endl;
}
