//
//  GameWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/19/16.
//
//

#include "cinder/Rand.h"

#include "TablaApp.h"
#include "GameWorld.h"
#include "geom.h"

GameCartridge::Library* GameCartridge::mLibrary=0;

fs::path GameWorld::getAssetPath( fs::path p ) const
{
	return TablaApp::get()->hotloadableAssetPath(p);
}

GamepadManager& GameWorld::getGamepadManager()
{
	return TablaApp::get()->getGamepadManager();
}

void GameWorld::setWorldBoundsPoly( PolyLine2 p )
{
	bool diff=false;

	if ( p.size()==mWorldBoundsPoly.size() )
	{
		for( int i=0; i<p.size(); ++i )
		{
			if ( p.getPoints()[i] != mWorldBoundsPoly.getPoints()[i] )
			{
				diff=true;
				break;
			}
		}
	}
	else diff=true;
	
	if (diff)
	{
		mWorldBoundsPoly=p;
		
		worldBoundsPolyDidChange();
	}
}

vec2 GameWorld::getRandomPointInWorldBoundsPoly() const
{
	PolyLine2 wb = getWorldBoundsPoly();
	
	if ( wb.getPoints().empty() )
	{
		return vec2(0,0);
	}
	else
	{
		Rectf b( wb.getPoints() );
		
		vec2 p = b.getSize() * vec2( randFloat(), randFloat() ) + b.getUpperLeft();
		
		if ( !wb.contains(p) ) p = closestPointOnPoly( p, wb ) ;
	
		return p;
	}
}

GameCartridge::GameCartridge( string systemName )
	: mSystemName(systemName)
{
	assert( getLibrary().find(systemName) == getLibrary().end() );
	
	getLibraryW()[mSystemName] = this;
}

const GameCartridge::Library& GameCartridge::getLibrary()
{
	return getLibraryW();
}

GameCartridge::Library& GameCartridge::getLibraryW()
{
	if (!mLibrary) mLibrary = new Library();
	return *mLibrary;
} 

GameCartridge::~GameCartridge()
{
	auto i = getLibraryW().find(mSystemName);
	
	assert( i != getLibraryW().end() );
	
	getLibraryW().erase(i);
}