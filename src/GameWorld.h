//
//  GameWorld.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/9/16.
//
//

#ifndef GameWorld_h
#define GameWorld_h

#include "cinder/Xml.h"
#include "cinder/PolyLine.h"
#include "Contour.h"

class GameWorld
{
public:
	virtual void setParams( XmlTree ){}
	virtual void updateContours( const ContourVector &c ){}
	
	void		setWorldBoundsPoly( PolyLine2 p ) { mWorldBoundsPoly=p; }
	PolyLine2	getWorldBoundsPoly() const { return mWorldBoundsPoly; }
	
	vec2		getRandomPointInWorldBoundsPoly() const; // a little lamely special case;
	// might be more useful to have something like random point on contour,
	// and then pick whether you want a hole or not hole.
	// randomPointOnContour()

	
	virtual void update(){}
	virtual void draw( bool highQuality ){}
	
	// because mice, etc... do sometimes touch these worlds...
	// all positions in world space
	virtual void mouseClick( vec2 ){}
	
private:
	PolyLine2 mWorldBoundsPoly;
	
};


#endif /* GameWorld_h */
