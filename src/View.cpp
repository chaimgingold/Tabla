//
//  View.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/31/16.
//
//

#include "View.h"

void ViewCollection::draw()
{
	for( const auto &v : mViews )
	{
		gl::pushViewMatrix();
		gl::multViewMatrix( v->getChildToParentMatrix() );
		
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