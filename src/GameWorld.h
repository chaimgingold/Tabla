//
//  GameWorld.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/9/16.
//
//

#ifndef GameWorld_h
#define GameWorld_h

#include <memory>

#include "cinder/Xml.h"
#include "cinder/PolyLine.h"
#include "Contour.h"
#include "Vision.h"

class GameWorld
{
public:
	virtual string getSystemName() const { return "GameWorld"; } // for xml param block name
	
	void		   setVisionParams( Vision::Params p ) { mVisionParams=p; }
	Vision::Params getVisionParams() const { return mVisionParams; }
	// app automatically loads xml params for GameWorld and sets them.
	// it then gets them when they are needed.
	
	virtual void setParams( XmlTree ){}
	virtual void updateContours( const ContourVector &c ){}
	
	void		setWorldBoundsPoly( PolyLine2 p ) { mWorldBoundsPoly=p; worldBoundsPolyDidChange(); }
	PolyLine2	getWorldBoundsPoly() const { return mWorldBoundsPoly; }
	virtual void worldBoundsPolyDidChange(){}
	
	vec2		getRandomPointInWorldBoundsPoly() const; // a little lamely special case;
	// might be more useful to have something like random point on contour,
	// and then pick whether you want a hole or not hole.
	// randomPointOnContour()

	virtual void gameWillLoad(){}
	virtual void update(){}
	virtual void draw( bool highQuality ){}
	
	// because mice, etc... do sometimes touch these worlds...
	// all positions in world space
	virtual void drawMouseDebugInfo( vec2 ){}
	virtual void mouseClick( vec2 ){}
	virtual void keyDown( KeyEvent ){}
	
private:
	Vision::Params	mVisionParams;
	PolyLine2		mWorldBoundsPoly;
	
};


class GameCartridge
{
public:
	virtual shared_ptr<GameWorld> load() const { return 0; }
	
};

class GameCartridgeSimple : public GameCartridge
{
public:
	typedef function< std::shared_ptr<GameWorld>() > tLoaderFunc;

	GameCartridgeSimple( tLoaderFunc f ) {mLoader=f;}
	
	virtual std::shared_ptr<GameWorld> load() const override
	{
		if (mLoader) return mLoader();
		else return 0;
	}
	
private:
	tLoaderFunc mLoader;
};


#endif /* GameWorld_h */
