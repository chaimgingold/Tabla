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
		cout << b << endl;
		
		vec2 p = b.getSize() * vec2( randFloat(), randFloat() ) + b.getUpperLeft();
		
		if ( !wb.contains(p) ) p = closestPointOnPoly( p, wb ) ;
	
		return p;
	}
}

