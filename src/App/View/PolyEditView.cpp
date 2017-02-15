//
//  PolyEditView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#include "PolyEditView.h"
#include "MainImageView.h"
#include "TablaApp.h"
#include "Pipeline.h"

const float kRadius = 8.f;

PolyEditView::PolyEditView( TablaApp& app, function<PolyLine2()> getter, string polyCoordSpace )
	: mApp(app)
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
	
	if (!mDrawPipelineStage.empty())
	{
		auto stage = getPipeline().getStage(mDrawPipelineStage);
		
		if (stage && stage->getGLImage())
		{
			gl::ScopedModelMatrix m;
			gl::multModelMatrix( getPipeline().getCoordSpaceTransform(mDrawPipelineStage,mPolyCoordSpace) );
			
			gl::color(1,1,1,.5);
			gl::draw(stage->getGLImage());
		}
	}
	
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
				
				float alpha; 
				
				if (!canEditVertex(i)) alpha = .1f;
				else if (getHasRollover()) alpha = 1.f;
				else alpha = .5f;
				
				gl::color( ColorA( c[min(i,3)], alpha ) );
				
				if (i==0) gl::drawStrokedCircle( p, kRadius, 2.f );
				else	  gl::drawStrokedRect  ( getPointControlRect(p), 2.f );
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
	
	if (mDrawSize)
	{
		gl::color(1,1,1);
		
		auto font = TablaApp::get()->getFont();
		
		const vec2 strsize = font->measureString("Test",gl::TextureFont::DrawOptions().pixelSnap(false));
		const float fontheight = strsize.y;
		const float scale = 15.f / fontheight ;
		const vec2 center = poly.calcCentroid(); 
		const Rectf rect = Rectf( poly.getPoints() ); 
		const auto options = gl::TextureFont::DrawOptions().scale(scale).pixelSnap(false);
		
		const float kGutter = 10.f;
		
		auto getStr = []( float f ) {
			return toString(f) + " cm";
		};
		
		font->drawString( getStr(rect.getWidth()), vec2(center.x,rect.y2+kGutter+fontheight/2), options );
		font->drawString( getStr(rect.getHeight()), vec2(rect.x2+kGutter,center.y), options );
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
			
		mDragAtPoint = quantize(mDragAtPoint);
	}
	
	if ( getDoLiveUpdate() && mSetPolyFunc) mSetPolyFunc( getPolyInPolySpace() );
}

int PolyEditView::pickPoint( vec2 p ) const
{
	const PolyLine2 poly = getPolyInImageSpace();
	
	for( size_t i=0; i<poly.getPoints().size(); ++i )
	{
		if ( canEditVertex(i)
		  && getPointControlRect(poly.getPoints()[i]).contains(p) )
		{
			return i;
		}
	}
	return -1;
}

Rectf PolyEditView::getPointControlRect( vec2 p ) const
{
	return Rectf(p,p).inflated( vec2(1,1)*kRadius );
}

mat4 PolyEditView::getPolyToImageTransform() const
{
	return getPipeline().getCoordSpaceTransform(
		mPolyCoordSpace,
		mMainImageView ? mMainImageView->getPipelineStageName() : "world"
		);
}

mat4 PolyEditView::getImageToPolyTransform() const
{
	return getPipeline().getCoordSpaceTransform(
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
		
		p = constrainAspectRatio(p, mDragPointIndex);
		p = constrainToRect(p, mDragPointIndex);
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

float PolyEditView::quantize( float f ) const
{
	if (mQuantizeToUnit<=0.f) return f;
	else return roundf( f / mQuantizeToUnit ) * mQuantizeToUnit; 
}

void PolyEditView::setConstrainToRect( bool constrain )
{
	mConstrainToRect=constrain;
	mConstrainToRectWithAspectRatioOfPipelineStage.clear();
}

void PolyEditView::setConstrainToRectWithAspectRatioOfPipelineStage( string s )
{
	mConstrainToRect=true;
	mConstrainToRectWithAspectRatioOfPipelineStage=s;
}

PolyLine2 PolyEditView::constrainAspectRatio( PolyLine2 p, int i ) const
{
	if ( mConstrainToRect && !mConstrainToRectWithAspectRatioOfPipelineStage.empty() )
	{
		auto stage = getPipeline().getStage(mConstrainToRectWithAspectRatioOfPipelineStage);
		
		if ( stage && p.size()==4 && i >=0 && i<4 )
		{
			float fixAspectRatio = stage->mImageSize.x / stage->mImageSize.y;
			cout << fixAspectRatio << endl; 

			vec2* v = &p.getPoints()[0];

			vec2 oldSize = glm::abs( v[(i+2)%4] - v[i] );
			
			bool lockW = true; // this should be adaptive to feel more consistent, but it works fine.
			
			// get new size
			vec2 newSize = oldSize;
			
			if (lockW) newSize.y = oldSize.x / fixAspectRatio;
			else	   newSize.x = fixAspectRatio * oldSize.y;
			
			vec2 dSize = newSize - oldSize;
			
			float sx[4] = { -1.f,  1.f, 1.f, -1.f };
			float sy[4] = { -1.f, -1.f, 1.f,  1.f };
			
			v[i].x += dSize.x * sx[i];
			v[i].y += dSize.y * sy[i];		
		}
	}
	
	return p;
}

PolyLine2 PolyEditView::constrainToRect( PolyLine2 p, int i ) const
{
	if ( mConstrainToRect && p.size()==4 && i >=0 && i<4 )
	{
		vec2* v = &p.getPoints()[0];
		
		// rect
		int x[4] = { 3, 2, 1, 0 };
		int y[4] = { 1, 0, 3, 2 };
		
		v[x[i]].x = v[i].x;
		v[y[i]].y = v[i].y;
	}
	
	return p;
}

bool PolyEditView::canEditVertex( int i ) const
{
	if ( mCanEditVertexMask.empty() ) return true;
	else if ( i>=0 && i<mCanEditVertexMask.size() ) return mCanEditVertexMask[i];
	else return false;
}

const Pipeline& PolyEditView::getPipeline() const
{
	return mApp.getPipeline();
}