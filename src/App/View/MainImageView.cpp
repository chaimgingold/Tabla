//
//  MainImageView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#include "MainImageView.h"
#include "TablaApp.h"

void MainImageView::draw()
{
	const Pipeline::StageRef stage = getPipelineStage();
	
	bool drawWorld = true;
	
	// vision pipeline image
	if ( !mIsProjectorView && mApp.getParams().mConfigWindowMainImageDrawBkgndImage )
	{
		if ( stage && stage->getGLImage() )
		{
			gl::color( 1, 1, 1 );
			
			gl::draw( stage->getGLImage() );
		}
		else if ( stage && stage->mImageCubeMapGL )
		{
			drawWorld = false;
			gl::color(1,1,1);
			gl::drawHorizontalCross( stage->mImageCubeMapGL, getBounds() );
		}
	}

//	if (1)
//	{
//		vec2 p = windowToImage(mMousePos);
//		gl::color( 0., .5, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(100,100)) );
//		gl::color( 1., .8, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(10,10)) );
//		gl::color( .9, .8, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(1,1)) );
//	}

	// ===== World space =====
	if (drawWorld)
	{
		gl::ScopedViewMatrix viewm;
		
		if (stage)
		{
			// convert world coordinates to drawn texture coords
			gl::multViewMatrix( stage->mWorldToImage );
		}
		
		if (mWorldDrawFunc) mWorldDrawFunc(); // overload it.
		else if (getGameWorld())
		{
			getGameWorld()->draw(
				mIsProjectorView
				? GameWorld::DrawType::Projector
				: GameWorld::DrawType::UIMain ); // high quality
		}
	}
}

void MainImageView::drawFrame()
{
	if ( mFrameColor.a > 0 )
	{
		gl::color( mFrameColor );
		gl::drawStrokedRect( getFrame() );
		
		if ( mTextureFont )
		{
			gl::color(1,1,1,1);
			mTextureFont->drawString( getPipelineStageName(), getFrame().getUpperLeft() + vec2(0,-4.f) );
		}
	}
}

void MainImageView::mouseDown( MouseEvent event )
{
	if (getGameWorld()) getGameWorld()->mouseClick( windowToWorld( event.getPos() ) ) ;
}

void MainImageView::setPipelineStageName( string n )
{
	mStageName=n;
}

string MainImageView::getPipelineStageName() const
{
	return mStageName.empty() ? mApp.getPipelineStageSelection() : mStageName;
}

const Pipeline::StageRef MainImageView::getPipelineStage() const
{
	return getPipeline().getStage(getPipelineStageName());
}

vec2 MainImageView::windowToImage( vec2 p )
{
	// convert screen/window coordinates to image coords
	return rootToChild(p);
}

vec2 MainImageView::windowToWorld( vec2 p )
{
	// convert image coordinates to world coords
	vec2 p2 = windowToImage(p);
	
	if (getPipelineStage())
	{
		p2 = vec2( getPipelineStage()->mImageToWorld * vec4(p2,0,1) ) ;
	}
	
	return p2;
}

vec2 MainImageView::worldToWindow( vec2 p )
{
	if (getPipelineStage())
	{
		p = vec2( getPipelineStage()->mWorldToImage * vec4(p,0,1) ) ;
	}
	
	return childToRoot(p);
}

GameWorld* MainImageView::getGameWorld() const
{
	return mApp.getGameWorld().get();
}

const Pipeline&  MainImageView::getPipeline() const
{
	return mApp.getPipeline();
}