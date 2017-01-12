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

class Pipeline;

class GameWorld
{
public:
	virtual string getSystemName() const { return "GameWorld"; } // for xml param block name
	virtual string getUserName() const { return getSystemName(); }
	
	void		   setVisionParams( Vision::Params p ) { mVisionParams=p; }
	Vision::Params getVisionParams() const { return mVisionParams; }
	// app automatically loads xml params for GameWorld and sets them.
	// it then gets them when they are needed.
	
	virtual void setParams( XmlTree ){}
	virtual void updateVision( const Vision::Output&, Pipeline& ){} // probably lower freq than update()
	
	void		setWorldBoundsPoly( PolyLine2 p ) { mWorldBoundsPoly=p; worldBoundsPolyDidChange(); }
	PolyLine2	getWorldBoundsPoly() const { return mWorldBoundsPoly; }
	virtual void worldBoundsPolyDidChange(){}
	
	vec2		getRandomPointInWorldBoundsPoly() const; // a little lamely special case;
	// might be more useful to have something like random point on contour,
	// and then pick whether you want a hole or not hole.
	// randomPointOnContour()

	enum class DrawType
	{
		Projector,
		UIPipelineThumb,
		UIMain,
		CubeMap
	};
	
	virtual void gameWillLoad(){}
	virtual void update(){} // called each frame
	virtual void prepareToDraw() {} // called once per frame, before all draw calls.
	virtual void draw( DrawType ){} // might be called many times per frame

	virtual void cleanup(){}
	
	// because mice, etc... do sometimes touch these worlds...
	// all positions in world space
	virtual void drawMouseDebugInfo( vec2 ){}
	virtual void mouseClick( vec2 ){}
	virtual void keyDown( KeyEvent ){}
	virtual void keyUp( KeyEvent ){}
	
private:
	Vision::Params	mVisionParams;
	PolyLine2		mWorldBoundsPoly;
	
};


class GameCartridge
{
public:
	GameCartridge( string systemName ); // should match GameWorld name
	virtual ~GameCartridge();
	
	string getSystemName() const { return mSystemName; }
	string getUserName() const { return getSystemName(); }
	virtual std::shared_ptr<GameWorld> load() const { return 0; }

	typedef map<string,const GameCartridge*> Library;
	static const Library& getLibrary();
	
private:
	static Library& getLibraryW();

	string mSystemName = "GameWorld";
	static Library* mLibrary;
	
};
typedef std::shared_ptr<GameCartridge> GameCartridgeRef;

class GameCartridgeSimple : public GameCartridge
{
public:
	typedef function< std::shared_ptr<GameWorld>() > tLoaderFunc;

	GameCartridgeSimple( string systemName, tLoaderFunc f ) : GameCartridge(systemName) {mLoader=f;}
	
	virtual std::shared_ptr<GameWorld> load() const override
	{
		if (mLoader) return mLoader();
		else return 0;
	}
	
private:
	tLoaderFunc mLoader;
};


#endif /* GameWorld_h */
