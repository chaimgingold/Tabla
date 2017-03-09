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
#include "cinder/Rand.h"

using namespace structured_light;
using namespace ci;

static GameCartridgeSimple sCartridge("ProjectorCalibrateWorld", [](){
	return std::make_shared<ProjectorCalibrateWorld>();
});

ProjectorCalibrateWorld::ProjectorCalibrateWorld()
{
}

void ProjectorCalibrateWorld::setParams( XmlTree xml )
{
	getXml(xml, "Verbose", mVerbose );
	getXml(xml, "ShowPatternLength", mShowPatternLength );
	getXml(xml, "ShowPatternFirstDelay", mShowPatternFirstDelay );
}

void ProjectorCalibrateWorld::update()
{
	// Make patterns?
	mProjectorStage = TablaApp::get()->getPipeline().getStage("projector");
		// this stage is not present during updateVision, so must catch in update.
	
	if (mProjectorStage) {
		// if resolution changes then we start over, so keep testing
		maybeMakePatterns( ivec2(mProjectorStage->mImageSize) );
	}
		
	// Advance pattern?
	if ( mShowPattern >=0
	  && mPatterns.size() > 0
	  && app::getElapsedSeconds() - mShowPatternWhen > mShowPatternLength )
	{
		captureCameraImage(mInputStage);
		showNextPattern();
	}
	
	//
	maybeUpdateProjCoords();
}

void ProjectorCalibrateWorld::maybeUpdateProjCoords()
{
	if ( !isPatternCaptureDone() ) return;
	
	auto &lightLink = TablaApp::get()->getLightLink();
	const LightLink::CaptureProfile& captProf = lightLink.getCaptureProfile();
	LightLink::ProjectorProfile&	 projProf = lightLink.getProjectorProfile();
	
	// TODO: See if we are actually on the same profile we thought we were?
	// (Minor timing bug issue; but not sure how to detect it, so skipping for now.)
	
	// changed since last time?
	int same=0;
	for( int i=0; i<4; ++i )
	{
		if ( mLastCaptureCoords[i] == captProf.mCaptureCoords[i] ) same++;
	}
	
	if (same<4)
	{
		vec2 v[4];
		int  okc=0;
		
		for( int i=0; i<4; ++i )
		{
			bool k = cameraToProjector( captProf.mCaptureCoords[i], v[i] );
			
			if (k) okc++;
		}
		
		if (okc==4)
		{
			for( int i=0; i<4; ++i )
			{
				if (mVerbose) cout << "Updating projCoord[" << toString(i) << "] = " << v[i]
					<< "(" << captProf.mCaptureCoords[i] << ")"
					<< endl;
				projProf.mProjectorCoords[i] = v[i];
				mLastCaptureCoords[i] = captProf.mCaptureCoords[i];
			}
			
			// save/update
			TablaApp::get()->lightLinkDidChange();
		}
	} // same
}

void ProjectorCalibrateWorld::maybeMakePatterns( ivec2 size )
{
	if ( size != mProjectorSize )
	{
		mProjectorSize = mProjectorStage->mImageSize;
		mGenerator = GrayCodePattern::create(mProjectorSize.x,mProjectorSize.y);
		
		mCaptures.clear();
		mCaptureTextures.clear();
		mShowPatternWhen = app::getElapsedSeconds() + mShowPatternFirstDelay;
		mShowPattern = 0;
		
		mGenerator->generate(mPatterns);
		if (mVerbose) {
			cout << "Made " << mPatterns.size()
				<< " patterns @ "
				 << mProjectorSize.x << ", " << mProjectorSize.y
				 << endl; 
		}
		
		if (mVerbose) cout << "Starting with pattern " << mShowPattern+1 << " / " << mPatterns.size() << endl;

		// patterns => textures (as optimization)
		mPatternTextures.resize( mPatterns.size() );
		for ( int i=0; i<mPatterns.size(); ++i )
		{
			const bool topDown=true; // ??? (!) I think that Mat from OCV should be true; false only when dealing with UMat (?)

			mPatternTextures[i] = matToTexture(mPatterns[i],topDown);
		}

		// stomp last capture coords
		for( int i=0; i<4; ++i )
		{
			mLastCaptureCoords[i] = vec2(0,0);
		}
	}
}

void ProjectorCalibrateWorld::showNextPattern()
{
	if (mShowPattern<0)
	{
		// already done
	}
	else if (mShowPattern >= mPatterns.size()-1)
	{
		// done
		if (mVerbose) cout << "Done showing patterns." << endl;
		mShowPattern = -1; // draw nothing
	}
	else
	{
		// advance
		mShowPatternWhen = app::getElapsedSeconds();
		mShowPattern = (mShowPattern+1) % mPatterns.size();
		if (mVerbose) cout << "Advancing to pattern " << mShowPattern+1 << " / " << mPatterns.size() << endl; 
	}
}

void ProjectorCalibrateWorld::captureCameraImage( Pipeline::StageRef input )
{
	if (!input) {
		cout << "No input to capture!" << endl;
	} else {
		cv::Mat gray;
		cv::cvtColor(input->mImageCV, gray, CV_BGR2GRAY);
		mCaptures.push_back(gray);
		mCaptureTextures.push_back( matToTexture(gray,true) );
	}
}

void ProjectorCalibrateWorld::updateVision( const Vision::Output& visionOut, Pipeline& pipeline )
{
	mInputStage = visionOut.mPipeline.getStage("undistorted");
	
	mContours = visionOut.mContours;
	
	// log stuff for debug...
	pipeline.beginOrthoGroup();
	for ( int i=0; i<mPatternTextures.size(); ++i )
	{
		pipeline.then( string("pattern[")+toString(i)+"]", mPatternTextures[i] );
		if (mProjectorStage) pipeline.back()->setImageToWorldTransform( mProjectorStage->mImageToWorld );
		pipeline.back()->mStyle.mScale = .5f;
	}
	pipeline.endOrthoGroup();

	pipeline.beginOrthoGroup();
	for ( int i=0; i<mCaptureTextures.size(); ++i )
	{
		pipeline.then( string("capture[")+toString(i)+"]", mCaptureTextures[i] );
		if (mInputStage) pipeline.back()->setImageToWorldTransform( mInputStage->mImageToWorld );
		pipeline.back()->mStyle.mScale = .5f;
	}
	pipeline.endOrthoGroup();
}

void ProjectorCalibrateWorld::draw( DrawType drawType )
{
	if (!mProjectorStage) return;
	
	const bool isUIThumb = (drawType == GameWorld::DrawType::UIPipelineThumb);

	if ( !isUIThumb ) {
		maybeDrawPattern(drawType);
//		maybeDrawVizPoints();
		maybeDrawContours();
	}

	maybeDrawMouse();
}

void ProjectorCalibrateWorld::maybeDrawPattern( DrawType drawType ) const
{
	if ( mShowPattern >= 0 && mShowPattern < mPatternTextures.size() )
	{
		if (mPatternTextures[mShowPattern])
		{
			// undo the world transform, so we draw in projector image space
			gl::ScopedViewMatrix matscope;
			gl::multViewMatrix(mProjectorStage->mImageToWorld);
			
			if ( drawType == GameWorld::DrawType::Projector ) gl::color(1,1,1);
			else gl::color(1,1,1,.25f); // ...<100% alpha if in config UI
			
			gl::draw(mPatternTextures[mShowPattern]);
		}
	}
}

void ProjectorCalibrateWorld::maybeDrawVizPoints() const
{
	if ( isPatternCaptureDone() )
	{
		// done!
		Rand rnd(1); // fixed random seed
		// TODO: optimize by re-rolling loop so that we aren't changing transforms for each point (like it matters)
		
		for ( int i=0; i<1000; ++i )
		{
			assert( mInputStage );
			assert( mGenerator );
			
			ivec2 cameraPix = mInputStage->mImageSize * vec2( rnd.nextFloat(), rnd.nextFloat() );
			cameraPix = min( cameraPix, ((ivec2)mInputStage->mImageSize) - ivec2(1,1) ); // clamp it in bounds! 
			
			vec2 projPix;
			
			bool ok = cameraToProjector( cameraPix, projPix );

			//
			vec2 cameraPixInWorldSpace = transformPoint( mInputStage->mImageToWorld, cameraPix );

			{
				// undo the world transform, so we draw in camera space
				gl::ScopedViewMatrix matscope;
				gl::multViewMatrix(mInputStage->mImageToWorld);
				gl::color(1,.2,0);
				gl::drawSolidCircle( cameraPix, 1.f, 3); // 1px radius
				
				// or manual transform
//				vec2 cameraPixInWorldSpace = transformPoint( mInputStage->mImageToWorld, cameraPix );
//				gl::color(1,.2,0);
//				gl::drawSolidCircle(cameraPixInWorldSpace, .5f, 3); // .5cm radius
			}
			
			if (ok)
			{
				// undo the world transform, so we draw in projector image space
				{
					gl::ScopedViewMatrix matscope;
					gl::multViewMatrix(mProjectorStage->mImageToWorld);
					gl::color(0,1,1);
					gl::drawSolidCircle( projPix, 1.f, 3); // 1px radius
				}

				// draw a line between the two
				// (doesn't seem to land where we expect UNLESS projector and camera pixel quads are
				// fully to the extend of the camera/projector bounds)
				if (0)
				{
					vec2 projPixInWorldSpace = transformPoint( mProjectorStage->mImageToWorld, projPix );
					gl::color(1,1,1,.5);
					gl::drawLine( cameraPixInWorldSpace, projPixInWorldSpace );
				}

				// or manual transform
//				vec2 projPixInWorldSpace = transformPoint( mProjectorStage->mImageToWorld, fromOcv(projPix) );
//				gl::color(0,1,1);
//				gl::drawSolidCircle(projPixInWorldSpace, .5f, 3); // .5cm radius
			}
		}
	}
}

void ProjectorCalibrateWorld::maybeDrawMouse() const
{
	const vec2 loc = getMousePosInWorld();
	
	gl::color(1,0,0);
	gl::drawSolidCircle(loc,2.f);

	if ( isPatternCaptureDone() )
	{
		vec2 camPt = transformPoint( mInputStage->mWorldToImage, loc );
		{
			// undo the world transform, so we draw in camera space
			gl::ScopedViewMatrix matscope;
			gl::multViewMatrix(mInputStage->mImageToWorld);
			gl::color(1,1,0);
			gl::drawStrokedCircle(camPt,8.f);
		}
		
		// undo the world transform, so we draw in projector image space
		vec2 projPt;
		if ( cameraToProjector(camPt,projPt) )
		{
			gl::ScopedViewMatrix matscope;
			gl::multViewMatrix(mProjectorStage->mImageToWorld);
			
			gl::color(0,1,1);
			gl::drawSolidCircle(projPt,8.f);
		}
	}
}

void ProjectorCalibrateWorld::maybeDrawContours() const
{
	if ( isPatternCaptureDone() )
	{
		gl::color(0,1,1);
		
		for( const auto &c : mContours )
		{
			gl::draw(c.mPolyLine);
		}
	}
}

bool ProjectorCalibrateWorld::isPatternCaptureDone() const
{
	return mCaptures.size() == mPatterns.size()
		&& mPatterns.size()>0;
}

bool ProjectorCalibrateWorld::cameraToProjector( vec2 p, vec2& o ) const
{
	if ( !isPatternCaptureDone() ) {
		return false;
	}
	
	assert( mGenerator );
	assert( mInputStage );

	cv::Point projPix;
	
	const ivec2 ip( roundf(p.x), roundf(p.y) );
	const ivec2 camsize = mInputStage->mImageSize;

//	p.x = camsize.x - p.x;
	
	if ( ip.x<0 || ip.y<0 || ip.x >= camsize.x || ip.y >= camsize.y ) return false;
	
	bool ok = mGenerator->getProjPixel( mCaptures, p.x, p.y, projPix );
	
	if (ok) o = fromOcv(projPix);
	
	return ok;
}