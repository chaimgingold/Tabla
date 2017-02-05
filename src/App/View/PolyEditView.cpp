//
//  PolyEditView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#include "PolyEditView.h"
#include "MainImageView.h"

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
		if (1)
		{
			// new style, indicate orientation and indexing
			Colorf c[4] = {
				Colorf(1,1,1),
				Colorf(1,0,0),
				Colorf(0,1,0),
				Colorf(0,0,1)
			};
			
			for( int i=0; i<poly.size(); ++i )
			{
				vec2 p = poly.getPoints()[i];

				gl::color( ColorA( c[min(i,3)], getHasRollover() ? 1.f : .5f) );
				
				if (i==0)
				{
					gl::drawStrokedCircle( p, kRadius, 2.f );
				}
				else
				{
					gl::drawStrokedRect( getPointControlRect(p), 2.f );
				}
			}
		}
		else
		{
			// old style
			if ( getHasRollover() ) gl::color(1,0,0);
			else gl::color(.8,.2,1);

			for( vec2 p : poly ) gl::drawStrokedRect( getPointControlRect(p), 2.f );
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