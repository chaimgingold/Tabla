//
//  PipelineStageView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#include "PipelineStageView.h"

void PipelineStageView::draw()
{
	const Pipeline::Stage* stage = mPipeline.getStage(mStageName);		

	// image
	if ( stage && stage->mImage )
	{
		gl::color(1,1,1);
		gl::draw( stage->mImage, getBounds() );
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
	const Pipeline::Stage* stage = getPipelineStage();
	
	// vision pipeline image
	if ( stage && stage->mImage )
	{
		gl::color( 1, 1, 1 );
		
		gl::draw( stage->mImage );
	}

//	if (1)
//	{
//		vec2 p = mouseToImage(mMousePos);
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
		else mGameWorld.draw();
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
	mGameWorld.mouseClick( mouseToWorld( event.getPos() ) ) ;
}

vec2 MainImageView::mouseToImage( vec2 p )
{
	// convert screen/window coordinates to image coords
	return rootToChild(p);
}

vec2 MainImageView::mouseToWorld( vec2 p )
{
	// convert image coordinates to world coords
	vec2 p2 = mouseToImage(p);
	
	if (getPipelineStage())
	{
		p2 = vec2( getPipelineStage()->mImageToWorld * vec4(p2,0,1) ) ;
	}
	
	return p2;
}

const float kRadius = 8.f;

PolyEditView::PolyEditView( Pipeline& pipeline, PolyLine2 poly, string polyCoordSpace )
	: mPipeline(pipeline)
	, mPoly(poly)
	, mPolyCoordSpace(polyCoordSpace)
{
	mPoly.setClosed();
}

void PolyEditView::setMainImageView( shared_ptr<MainImageView> miv )
{
	mMainImageView=miv;
	setParent(miv);
}

void PolyEditView::draw()
{
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
	
	if ( mDragPointIndex != -1 ) mDragStartPoint=getPolyInImageSpace().getPoints()[mDragPointIndex];
}

void PolyEditView::mouseUp  ( MouseEvent event )
{
	if ( mDragPointIndex!=-1 && !getDoLiveUpdate() && mPolyFunc) mPolyFunc( mPoly );

	mDragPointIndex = -1;
}

void PolyEditView::mouseDrag( MouseEvent event )
{
	vec2 pos = rootToChild(event.getPos());
	
	if ( mDragPointIndex >= 0 && mDragPointIndex < mPoly.size() )
	{
		mPoly.getPoints()[mDragPointIndex] = vec2(
			getImagetoPolyTransform()
			* vec4( mDragStartPoint + (pos - mDragStartMousePos), 0, 1 )
			);
	}
	
	if ( getDoLiveUpdate() && mPolyFunc) mPolyFunc( mPoly );
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

mat4 PolyEditView::getImagetoPolyTransform() const
{
	return mPipeline.getCoordSpaceTransform(
		mMainImageView ? mMainImageView->getPipelineStageName() : "world",
		mPolyCoordSpace
		);
	// should be inverse of getPolyToImageTransform()
}

PolyLine2 PolyEditView::getPolyInImageSpace() const
{
	const mat4 transform = getPolyToImageTransform();
	
	PolyLine2 poly = mPoly;
	
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