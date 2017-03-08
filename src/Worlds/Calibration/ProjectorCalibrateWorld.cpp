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
		capture(mInputStage);
		nextPattern();
	}
}

void ProjectorCalibrateWorld::maybeMakePatterns( ivec2 size )
{
	if ( size != mProjectorSize )
	{
		mProjectorSize = mProjectorStage->mImageSize;
		mGenerator = GrayCodePattern::create(mProjectorSize.x,mProjectorSize.y);
		
		mCaptures.clear();
		mCaptureTextures.clear();
		mShowPatternWhen = app::getElapsedSeconds();
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
	}
}

void ProjectorCalibrateWorld::nextPattern()
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

void ProjectorCalibrateWorld::capture( Pipeline::StageRef input )
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
	
	if ( drawType != GameWorld::DrawType::UIPipelineThumb )
	{
		if ( mShowPattern >= 0 && mShowPattern < mPatternTextures.size() )
		{
			if (mPatternTextures[mShowPattern])
			{
				// undo the world transform, so we draw in projector image space
				gl::ScopedViewMatrix matscope;
				gl::multViewMatrix(mProjectorStage->mImageToWorld);
				
				if ( drawType == GameWorld::DrawType::Projector ) gl::color(1,1,1);
				else gl::color(1,1,1,.25f);
				
				gl::draw(mPatternTextures[mShowPattern]);
			}
		}
	}
	
	if ( mCaptures.size() == mPatterns.size()
	  && mPatterns.size()>0
	  && drawType != GameWorld::DrawType::UIPipelineThumb )
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
			
			cv::Point projPix;
			
			bool ok = mGenerator->getProjPixel( mCaptures, cameraPix.x, cameraPix.y, projPix );

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
					gl::drawSolidCircle( fromOcv(projPix), 1.f, 3); // 1px radius 
				}

				// draw a line between the two
				// (doesn't seem to land where we expect UNLESS projector and camera pixel quads are
				// fully to the extend of the camera/projector bounds)
				vec2 projPixInWorldSpace = transformPoint( mProjectorStage->mImageToWorld, fromOcv(projPix) );
				gl::color(1,1,1,.5);
				gl::drawLine( cameraPixInWorldSpace, projPixInWorldSpace );

				// or manual transform
//				vec2 projPixInWorldSpace = transformPoint( mProjectorStage->mImageToWorld, fromOcv(projPix) );
//				gl::color(0,1,1);
//				gl::drawSolidCircle(projPixInWorldSpace, .5f, 3); // .5cm radius
			}
		}
	}
}