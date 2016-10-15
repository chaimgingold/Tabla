//
//  PipelineStageView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#include "PipelineStageView.h"
#include "PaperBounce3App.h"

void PipelineStageView::draw()
{
	// scissor us so we only draw into our little frame
	// this will fail to work right if we are nested in other transforms
	gl::ScopedScissor scissor(
		toPixels(getFrame().x1), toPixels(getWindowHeight()) - toPixels(getFrame().y2),
		toPixels(getFrame().getWidth()), toPixels(getFrame().getHeight())
		);
	
	const Pipeline::StageRef stage = mPipeline.getStage(mStageName);

	// image
	if ( stage && stage->getGLImage() )
	{
		gl::color(1,1,1);
		gl::draw( stage->getGLImage(), getBounds() );
	}
	else
	{
//		gl::color(0,0,0);
		gl::color(1,1,1);
		gl::drawSolidRect( getBounds() );
	}
	
	// world
	if ( mWorldDrawFunc )
	{
		if (stage)
		{
			gl::pushViewMatrix();
			gl::multViewMatrix( stage->mWorldToImage );
			// we need a pipeline stage so we know what transform to use
			// otherwise we'll just use existing matrix
		}
		
		mWorldDrawFunc();

		if (stage) gl::popViewMatrix();
	}
}

void PipelineStageView::mouseDown( MouseEvent )
{
	mPipeline.setQuery(mStageName);
}

void PipelineStageView::drawFrame()
{
	if ( mStageName == mPipeline.getQuery() ) gl::color(.7f,.3f,.5f);
	else if ( getHasRollover() ) gl::color(1,1,1,1.f);
	else gl::color(1,1,1,.5f);
	
	gl::drawStrokedRect( getFrame(), 2.f );
}



void MainImageView::draw()
{
	const Pipeline::StageRef stage = getPipelineStage();
	
	// vision pipeline image
	if ( stage && stage->getGLImage() )
	{
		gl::color( 1, 1, 1 );
		
		gl::draw( stage->getGLImage() );
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
	gl::pushViewMatrix();
	{
		if (stage)
		{
			// convert world coordinates to drawn texture coords
			gl::multViewMatrix( stage->mWorldToImage );
		}
		
		if (mWorldDrawFunc) mWorldDrawFunc(); // overload it.
		else if (getGameWorld()) getGameWorld()->draw(true); // high quality
	}
	gl::popViewMatrix();
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
	return mStageName.empty() ? mApp.mPipeline.getQuery() : mStageName;
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
	return mApp.mGameWorld.get();
}

Pipeline&  MainImageView::getPipeline() const
{
	return mApp.mPipeline;
}



const float kRadius = 8.f;

PolyEditView::PolyEditView( Pipeline& pipeline, function<PolyLine2()> getter, string polyCoordSpace )
	: mPipeline(pipeline)
	, mGetPolyFunc(getter)
	, mPolyCoordSpace(polyCoordSpace)
{
}

void PolyEditView::setMainImageView( std::shared_ptr<MainImageView> miv )
{
	mMainImageView=miv;
	setParent(miv);
}

void PolyEditView::draw()
{
	if ( !isEditable() ) return; // this trasnform seems to just hosed, so hide it to make things clearer.
	
	//
	const PolyLine2 poly = getPolyInImageSpace();

	if ( isEditable() )
	{
		if ( getHasRollover() ) gl::color(1.,.3,.4);
		else gl::color(.7,.3,.4);
	}
	else gl::color(.5,.5,.7,.5);
	gl::draw( poly );

	if ( isEditable() )
	{
		if ( getHasRollover() ) gl::color(1,0,0);
		else gl::color(.8,.2,1);

		for( vec2 p : poly )
		{
			gl::drawStrokedRect( getPointControlRect(p), 2.f );
		}
	}
}

bool PolyEditView::pick( vec2 p )
{
	return isEditable() && pickPoint( rootToChild(p) ) != -1 ;
}

void PolyEditView::mouseDown( MouseEvent event )
{
	vec2 pos = rootToChild(event.getPos());
	
	mDragPointIndex = pickPoint( pos );
	
	mDragStartMousePos = pos;
	
	if ( mDragPointIndex != -1 )
	{
		// getPolyInPolySpace(false) because we want undragged location (we are in an inconsistent state right now)
		mDragStartPoint = getPolyInPolySpace(false).getPoints()[mDragPointIndex];
		mDragAtPoint    = mDragStartPoint ;
	}
}

void PolyEditView::mouseUp  ( MouseEvent event )
{
	if ( mDragPointIndex!=-1 && !getDoLiveUpdate() && mSetPolyFunc )
	{
		mSetPolyFunc( getPolyInPolySpace() );
	}

	mDragPointIndex = -1;
}

void PolyEditView::mouseDrag( MouseEvent event )
{
	vec2 pos = rootToChild(event.getPos());
	
	if ( mDragPointIndex >= 0 )
	{
		mDragAtPoint = vec2(
			getImageToPolyTransform()
			* vec4( mDragStartPoint + (pos - mDragStartMousePos), 0, 1 )
			);
	}
	
	if ( getDoLiveUpdate() && mSetPolyFunc) mSetPolyFunc( getPolyInPolySpace() );
}

int PolyEditView::pickPoint( vec2 p ) const
{
	const PolyLine2 poly = getPolyInImageSpace();
	
	for( size_t i=0; i<poly.getPoints().size(); ++i )
	{
		if ( getPointControlRect(poly.getPoints()[i]).contains(p) ) return i;
	}
	return -1;
}

Rectf PolyEditView::getPointControlRect( vec2 p ) const
{
	return Rectf(p,p).inflated( vec2(1,1)*kRadius );
}

mat4 PolyEditView::getPolyToImageTransform() const
{
	return mPipeline.getCoordSpaceTransform(
		mPolyCoordSpace,
		mMainImageView ? mMainImageView->getPipelineStageName() : "world"
		);
}

mat4 PolyEditView::getImageToPolyTransform() const
{
	return mPipeline.getCoordSpaceTransform(
		mMainImageView ? mMainImageView->getPipelineStageName() : "world",
		mPolyCoordSpace
		);
	// should be inverse of getPolyToImageTransform()
}

PolyLine2 PolyEditView::getPolyInPolySpace( bool withDrag ) const
{
	PolyLine2 p = mGetPolyFunc();
	p.setClosed();
	
	if ( withDrag && mDragPointIndex >=0 && mDragPointIndex < p.size() )
	{
		p.getPoints()[mDragPointIndex] = mDragAtPoint;
	}
	
	return p;
}

PolyLine2 PolyEditView::getPolyInImageSpace( bool withDrag ) const
{
	const mat4 transform = getPolyToImageTransform();
	
	PolyLine2 poly = getPolyInPolySpace(withDrag);
	
	for ( vec2 &pt : poly.getPoints() )
	{
		pt = vec2( transform * vec4(pt,0,1) );
	}
	
	return poly;
}

bool PolyEditView::isEditable() const
{
	return mMainImageView && ( mEditableInStages.empty()
		|| std::find( mEditableInStages.begin(), mEditableInStages.end(), mMainImageView->getPipelineStageName() ) != mEditableInStages.end()
		);
}