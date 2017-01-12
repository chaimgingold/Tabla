//
//  GameWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/19/16.
//
//

#include "cinder/Rand.h"

#include "GameWorld.h"
#include "geom.h"

GameCartridge::Library* GameCartridge::mLibrary=0;

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