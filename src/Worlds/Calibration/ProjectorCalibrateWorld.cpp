//
//  ProjectorCalibrateWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/12/17.
//
//

#include "ProjectorCalibrateWorld.h"
#include "TablaApp.h" // for config
#include "ocv.h"
#include "xml.h"

/*
Q: Do we need >1 registration hole to fully register this thing? My sense is no, as the projector and camera must always be aligned in terms of clockwise vertex ordering. The only way this could happen is if the projector was somehow under the table and the camera was on top...

Q: TODO: We will probably want to use subpixel precision for the found corner vertices.
 
*/

static GameCartridgeSimple sCartridge("ProjectorCalibrateWorld", [](){
	return std::make_shared<ProjectorCalibrateWorld>();
});

ProjectorCalibrateWorld::ProjectorCalibrateWorld()
{
}

void ProjectorCalibrateWorld::setParams( XmlTree xml )
{
	getXml(xml, "Verbose", mVerbose );
	getXml(xml, "RectSize", mRectSize );
	getXml(xml, "HoleSize", mHoleSize );
	getXml(xml, "HoleOffset", mHoleOffset );
	getXml(xml, "WaitFrameCount", mWaitFrameCount );
}

void ProjectorCalibrateWorld::update()
{
}

void ProjectorCalibrateWorld::updateVision( const Vision::Output& visionOut, Pipeline& pipeline )
{
	mProjectorStage = pipeline.getStage("projector");
	mContours = visionOut.mContours;

	setRegistrationRects();
	// need to do this 1x after we get mProjectorStage, but projector could change, in theory, so keep doing this...
	
	Found f = findRegistrationRect(mContours);
	
	if ( f.isComplete() && (!mFound.isComplete() || mWaitFrames-- < 0) )
	{
		mFound = f;

		updateCalibration(
			mProjectRect,
			mContours[mFound.mRect].mPolyLine,
			mFound.mHole);
			
		mWaitFrames = mWaitFrameCount;
	}
}

void ProjectorCalibrateWorld::updateCalibration ( Rectf in, PolyLine2 out, int outStart )
{
	vec2 image[4] = { in.getUpperLeft(), in.getUpperRight(), in.getLowerRight(), in.getLowerLeft() };
	vec2 world[4];
	
	for( int i=0; i<4; ++i ) world[i] = out.getPoints()[ (i+outStart)%4 ];
	
	// we assume input starts at x1,y1
	mat4 mat = getOcvPerspectiveTransform(world,image);

	if (mVerbose) cout << mat << endl;
	
	if (1)
	{
		LightLink::ProjectorProfile &prof = TablaApp::get()->mLightLink.getProjectorProfile();
		
		for( int i=0; i<4; ++i )
		{
			prof.mProjectorCoords[i] = vec2( mat * vec4( prof.mProjectorWorldSpaceCoords[i], 0, 1 ) );
		}
		
		TablaApp::get()->lightLinkDidChange();
	}	
}

void ProjectorCalibrateWorld::setRegistrationRects()
{
	// define where the registration marks will go
	if (mProjectorStage)
	{
		vec2 c = mProjectorStage->mImageSize/2.f ;
		vec2 size(mRectSize) ;
		
		mProjectRect = Rectf( c - size/2.f, c + size/2.f );
		
		vec2 c1 = mProjectRect.getUpperLeft() + vec2(mHoleOffset);
		mProjectRectHole = Rectf( c1, c1 + vec2(mHoleSize) ); 
		// starts at x1,y1 -- upper left
	}
}

ProjectorCalibrateWorld::Found
ProjectorCalibrateWorld::findRegistrationRect( const ContourVec& cs ) const
{
	int n=0;
	Found found;
	
	for ( const auto &c : cs )
	{
		// find the rect
		if ( c.mPolyLine.size() == 4 && c.mTreeDepth==0 )
		{
			n++;
			if (n>1) return Found(); // failed; >1 match
			
			found.mRect = c.mIndex;
			
			// find the hole
			if ( c.mChild.size()==1 && cs[c.mChild[0]].mPolyLine.size()==4 )
			{
				found.mHole = c.mChild[0];
				
				vec2 hole = cs[found.mHole].mPolyLine.calcCentroid();
				
				// assign hole to a vertex 
				float bestd=MAXFLOAT;
				for( int i=0; i<4; ++i )
				{
					float d = distance(hole,c.mPolyLine.getPoints()[i]);
					if (d<bestd)
					{
						found.mCorner=i;
						bestd=d;
					}					
				} // for 0..4
			} // if
		} // if
	}
	
	return found;
}

void ProjectorCalibrateWorld::draw( DrawType drawType )
{
	if (!mProjectorStage) return;

	// draw registration marks in projector space
	if ( drawType==DrawType::Projector || !(TablaApp::get()->mDebugFrame) )
	{
		// undo the world transform, so we draw in projector image space
		gl::ScopedViewMatrix matscope;
		gl::multViewMatrix(mProjectorStage->mImageToWorld);

		if (0)
		{
			// debug help
			const auto projprof = TablaApp::get()->mLightLink.getProjectorProfile();
			const PolyLine2 projcoords( vector<vec2>(
								projprof.mProjectorCoords,
								projprof.mProjectorCoords+4) );
			const vec2 projsize = mProjectorStage->mImageSize;

			gl::color(1,1,1,.25);
			gl::drawSolidRect( Rectf( vec2(0,0), projsize ) );

			gl::color(.5,1,1,.25);
			gl::drawSolid(projcoords);

			gl::color(1,1,1);
			gl::drawLine( vec2(0,0), projsize );
			gl::drawLine( vec2(projsize.x,0), vec2(0,projsize.y) );
		}
		
		// draw registration marks
		gl::color(1,1,1);
		gl::drawSolidRect(mProjectRect);
		gl::color(0,0,0);
		gl::drawSolidRect(mProjectRectHole);
		
		//
		if ( drawType!=DrawType::Projector )
		{
			gl::color(0,1,0);
			gl::drawSolidCircle(mProjectRect.getUpperLeft(), 5.f);
		}
	}
	
	// draw any contours, to show quality of registration
	gl::color(0,1,1);
	
	for ( const auto &c : mContours )
	{
		if ( c.mPolyLine.size() != 4 ) gl::draw(c.mPolyLine);
	}
	
	// feedback on registration capture
	if ( drawType!=DrawType::Projector )
	{
		if ( mFound.mRect!=-1 )
		{
			gl::color(0,1,1);
			gl::draw(mContours[mFound.mRect].mPolyLine);
		}
		if ( mFound.mHole!=-1 )
		{
			gl::color(1,0,0);
			gl::draw(mContours[mFound.mHole].mPolyLine);
		}
		if ( mFound.mCorner!=-1 )
		{
			gl::color(1,0,0);
			gl::drawSolidCircle(
				mContours[mFound.mRect].mPolyLine.getPoints()[mFound.mCorner],
				1.f );
		}
	}
}