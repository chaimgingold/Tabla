//
//  View.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/31/16.
//
//

#include "View.h"

mat4 View::getRootToChildMatrix() const
{
	if (mParent) return mParent->getRootToChildMatrix() * getParentToChildMatrix();
	else return getParentToChildMatrix();
}

mat4 View::getChildToRootMatrix() const
{
	if (mParent) return getChildToParentMatrix() * mParent->getChildToRootMatrix();
	else return getChildToParentMatrix();
}

void ViewCollection::draw()
{
	for( const auto &v : mViews )
	{
		gl::pushViewMatrix();
		gl::multViewMatrix( v->getChildToRootMatrix() );
		
		v->draw();
		
		gl::popViewMatrix();
		
		v->drawFrame();
	}
}

ViewRef	ViewCollection::pickView( vec2 p )
{
	for( int i=(int)mViews.size()-1; i>=0; --i )
	{
		if ( mViews[i]->pick(p) ) return mViews[i];
	}
	
	return nullptr;
}

ViewRef ViewCollection::getViewByName( string name )
{
	for( const auto &v : mViews )
	{
		if ( v->getName()==name ) return v;
	}
	
	return nullptr;
}

bool ViewCollection::removeView( ViewRef v )
{
	for ( auto i = mViews.begin(); i != mViews.end(); ++i )
	{
		if ( *i==v )
		{
			mViews.erase(i);
			return true ;
		}
	}
	
	return false;
}

void ViewCollection::mouseDown( MouseEvent event )
{
	// mouse down event
	mMouseDownView = pickView( event.getPos() );
	
	if (mMouseDownView) {
		mMouseDownView->setHasMouseDown(true);
		mMouseDownView->mouseDown(event);
	}
	
	// nuke rollover
	if (mRolloverView) mRolloverView->setHasRollover(false);
	mRolloverView=0;
}

void ViewCollection::mouseUp( MouseEvent event )
{
	if (mMouseDownView) {
		mMouseDownView->mouseUp(event);
		mMouseDownView->setHasMouseDown(false);
		mMouseDownView=0;
	}
}

void ViewCollection::mouseMove( MouseEvent event )
{
	if (mMouseDownView)
	{
		mMouseDownView->mouseMove(event);
	}
	else
	{
		if (mRolloverView) mRolloverView->setHasRollover(false);
		
		mRolloverView = pickView( event.getPos() );
		
		if (mRolloverView) mRolloverView->setHasRollover(true);
	}
}

void ViewCollection::mouseDrag( MouseEvent event )
{
	if (mMouseDownView) mMouseDownView->mouseDrag(event);
}

void ViewCollection::resize()
{
	for( auto &v : mViews ) {
		v->resize();
	}
}