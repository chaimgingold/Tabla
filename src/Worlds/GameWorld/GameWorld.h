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
#include "GamepadManager.h"

class Pipeline;

class GameWorld
{
public:
	virtual ~GameWorld() {} // make sure children can overload destructor
	
	
	// names used for loading associated files and showing to user
	virtual string getSystemName() const { return "GameWorld"; } // for xml param block name
	virtual string getUserName() const { return getSystemName(); }
	
	
	// load hard coded configuration/tuning xml data
	virtual void setParams( XmlTree ){}

	void		   setVisionParams( Vision::Params p ) { mVisionParams=p; }
	Vision::Params getVisionParams() const { return mVisionParams; }
	// app automatically loads xml params for GameWorld and sets them.
	// it then gets them when they are needed.
	
	
	// user settings
	virtual void    initSettings() {} // called if there is no settings file on disk
	virtual XmlTree getUserSettings() const { return XmlTree(); }
	virtual void    setUserSettings( XmlTree ) {}
	// tell system that settings changed so it can save them; (and it can tell us it saved them)
	void setAreUserSettingsDirty( bool dirty=true ) { mUserSettingsDirty=dirty; }
	bool getAreUserSettingsDirty() { return mUserSettingsDirty; }
	
	// world bounds + orientation
	void		setWorldBoundsPoly( PolyLine2 ); // only calls didChange if it is different
	PolyLine2	getWorldBoundsPoly() const { return mWorldBoundsPoly; }
	virtual void worldBoundsPolyDidChange(){}
	
	vec2		getRandomPointInWorldBoundsPoly() const; // a little lamely special case;
	// might be more useful to have something like random point on contour,
	// and then pick whether you want a hole or not hole.
	// randomPointOnContour()

	virtual map<string,vec2> getOrientationVecs() const { return map<string,vec2>(); }
	virtual void			 setOrientationVec ( string, vec2 ) {}


	// event handling
	enum class DrawType
	{
		Projector,
		UIPipelineThumb,
		UIMain,
		CubeMap
	};
	
	virtual void gameWillLoad(){}
	virtual void update(){} // called each frame
	virtual void updateVision( const Vision::Output&, Pipeline& ){} // probably lower freq than update()
	virtual void prepareToDraw() {} // called once per frame, before all draw calls.
	virtual void draw( DrawType ){} // might be called many times per frame

	virtual void cleanup(){}
	
	// because mice, etc... do sometimes touch these worlds...
	// all positions in world space
	virtual void drawMouseDebugInfo( vec2 ){}
	virtual void mouseClick( vec2 ){}
	virtual void keyDown( KeyEvent ){}
	virtual void keyUp( KeyEvent ){}
	void setMousePosInWorld( vec2 p ) { mMousePosInWorld=p; } // called by app

	// gamepad
	virtual void gamepadEvent( const GamepadManager::Event& ) {}
	GamepadManager& getGamepadManager();
	
protected:
	vec2 getMousePosInWorld() const { return mMousePosInWorld; }
	fs::path getAssetPath( fs::path ) const; // TODO: make relative to game world asset directory
	
private:
	Vision::Params	mVisionParams;
	PolyLine2		mWorldBoundsPoly;
	vec2			mMousePosInWorld;
	bool			mUserSettingsDirty=false;
};
typedef std::shared_ptr<GameWorld> GameWorldRef;


class GameCartridge
{
public:
	GameCartridge( string systemName ); // should match GameWorld name
	virtual ~GameCartridge();
	
	string getSystemName() const { return mSystemName; }
	string getUserName() const { return getSystemName(); }
	virtual GameWorldRef load() const { return 0; }

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
	typedef function< GameWorldRef() > tLoaderFunc;

	GameCartridgeSimple( string systemName, tLoaderFunc f ) : GameCartridge(systemName) {mLoader=f;}
	
	virtual GameWorldRef load() const override
	{
		if (mLoader) return mLoader();
		else return 0;
	}
	
private:
	tLoaderFunc mLoader;
};


#endif /* GameWorld_h */
