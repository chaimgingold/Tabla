//
//  TablaApp.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#ifndef TablaApp_h
#define TablaApp_h

#include <memory>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Xml.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

#include "LightLink.h"
#include "Vision.h"
#include "VisionInput.h"
#include "Contour.h"
#include "GameWorld.h"
#include "FileWatch.h"
#include "Pipeline.h"
#include "FPS.h"
#include "TablaAppParams.h"

#include "PipelineStageView.h"
#include "TablaWindow.h"

#include "PureDataNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TablaApp : public App {
  public:
	~TablaApp();
	
	static TablaApp* get() { return dynamic_cast<TablaApp*>(AppBase::get()); }
	
	// cinder app
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void fileDrop( FileDropEvent event ) override;
	void update() override;
	void draw() override;
	void resize() override;
	void keyDown( KeyEvent event ) override;
	void keyUp( KeyEvent event ) override;
	void cleanup() override;
	
	//
	void drawWorld( GameWorld::DrawType );

	float getFPS() const { return mAppFPS.mFPS; }
	float getCaptureFPS() const { return mCaptureFPS.mFPS; }	
	float getAVClacker() const { return mAVClacker; }

	// Light link management
	const LightLink& getLightLink() const { return mLightLink; }
	void setCaptureProfile( string );

		// TODO: abstract access, and make both of these private: 
	LightLink& getLightLink() { return mLightLink; }	
	void lightLinkDidChange( bool saveToFile=true, bool doSetupCaptureDevice=true ); // calls setupCaptureDevice, tells mVision, tells mGameWorld, saves it to disk


	// Vision pipeline
	const Pipeline& getPipeline() { return mVisionOutput.mPipeline; }

	void   selectPipelineStage( string s );
	string getPipelineStageSelection() const;

	const Vision::Output& getVisionOutput() const { return mVisionOutput; } // for debug visualization


	// game
	GameWorldRef getGameWorld() const { return mGameWorld; }
	void loadGame( string systemName );
	
	// shared resources
	cipd::PureDataNodeRef getPd() const { return mPd; }
	gl::TextureFontRef getFont() const { return mTextureFont; }
	const TablaAppParams& getParams() const { return mParams; }

	fs::path hotloadableAssetPath( fs::path ) const ; // prepends the appropriate thing...
	// TODO: move all game assets into their own directories and then make this private

		
private:
	
	// === Vision System ===

	void updateVision();
	
	// main players
	LightLink			mLightLink; // calibration for camera <> world <> projector
	Vision				mVision ;	// edge detection	->
	Vision::Output		mVisionOutput; // contours, tokens ->
	
	bool setupCaptureDevice(); // specified by mLightLink.mCameraIndex
	void updateDebugFrameCaptureDevicesWithPxPerWorldUnit( float );
	
	// pipeline 
	void addProjectorPipelineStages( Pipeline& );	
	string mPipelineStageSelection;
	
	// capture device management
	bool ensureLightLinkHasLocalDeviceProfiles(); // returns if mLightLink changed
	void setupNextValidCaptureProfile(); // iterate through them
	bool tryToSetupValidCaptureDevice();
		// called once by lightLinkDidChange if setupCaptureDevice fails
		// it tries a

	// misc.
	void maybeUpdateCaptureProfileWithFrame(); // if VisionInput is a file, see if its file size has changed, and update apture profile
	void setProjectorWorldSpaceCoordsFromCaptureProfile();
	void enumerateDisplaysAndCamerasToConsole() const;
	PolyLine2 getWorldBoundsPoly() const;
		// This is actually the camera world polygon mapping.
		// For all practical purposes, this is identical to the projector world polygon mapping.
	
	
	
	// === Game Management ===
	
	// active world simulation
	GameWorldRef mGameWorld;
	
	// game library
	void loadDefaultGame();
	void loadAdjacentGame( int libraryIndexDelta );
	string getCurrentGameSystemName() const;
	
	// game xml params
	void		setGameWorldXmlParams( XmlTree );
	fs::path	getXmlConfigPathForGame( string );
	
	
	
	// === Keyboard (RFID) Input ===
	
	// keyboard input (for RFID device code parsing)
	string	mKeyboardString; // aggregate keystrokes here
	float	mLastKeyEventTime=-MAXFLOAT; // when was last keystroke?
	bool	parseKeyboardString( string ); // returns true if successful
	
	map<int,string> mRFIDKeyToValue; // maps RFID #s to semantic strings
	map<string,function<void()>> mRFIDValueToFunction; // maps RFID semantic strings to code
	
	void setupRFIDValueToFunction(); // scans game library and binds loader code to rfid values
	
	
	
	// === UI Stuff ===
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	TablaWindowRef		mProjectorWin; // projector
	TablaWindowRef		mUIWindow; // for other debug info, on computer screen
	
	TablaWindow*		getTablaWindow( WindowRef w ) { return w ? w->getUserData<TablaWindow>() : 0 ; }
	TablaWindow*		getTablaWindow() { return getTablaWindow(getWindow()); } // for front window

	void setupWindows();
	void drawContours( bool filled, bool mousePickInfo, bool worldBounds ) const;
	vec2 getMousePosInWorld() const; // uses current window (ci::getWindow())
	string getCommandLineArgValue( string param ) const; // e.g. Tabla -param returnValue
	
	float mAVClacker=0.f;

	FPS mAppFPS;
	FPS mCaptureFPS;

	TablaAppParams mParams; 
	
	
	
	// === File Management
		
	FileWatch mFileWatch;

	// paths
	fs::path getDocsPath() const;
	fs::path getUserLightLinkFilePath() const;
	fs::path getUserSettingsFilePath() const;
	
	string getOverloadedAssetPath() const; // calculates it (from arguments, env vars)
	string mOverloadedAssetPath; // => is stored here
	
	// loading 
	void loadAppConfigXml( XmlTree );
	void loadLightLinkXml( XmlTree );
	
	// user settings
	void loadUserSettingsFromXml( XmlTree );
	XmlTree getUserSettingsXml() const;
	void saveUserSettings();

	// misc
	void saveCameraImageToDisk();


	
	// === Audio Synthesis ===
	cipd::PureDataNodeRef mPd;
	cipd::PatchRef mAVClackerPatch;
	
	void setupPureData();
};

#endif /* TablaApp_h */
