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
#include "Contour.h"

class GameWorld
{
public:
	virtual void setParams( XmlTree ){}
	virtual void updateContours( const ContourVector &c ){}
	
	virtual void update(){}
	virtual void draw(){}
	
	// because mice, etc... do sometimes touch these worlds...
	// all positions in world space
	virtual void mouseClick( vec2 ){}
};


#endif /* GameWorld_h */
