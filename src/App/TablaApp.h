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
#include "Contour.h"
#include "GameWorld.h"
#include "FileWatch.h"
#include "Pipeline.h"

#include "PipelineStageView.h"
#include "WindowData.h"

#include "PureDataNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TablaApp : public App {
  public:
	~TablaApp();
	
	static TablaApp* get() { return dynamic_cast<TablaApp*>(AppBase::get()); }
	
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
	
	void drawWorld( GameWorld::DrawType );


	// Light link management
	void lightLinkDidChange( bool saveToFile=true, bool doSetupCaptureDevice=true ); // calls setupCaptureDevice, tells mVision, tells mGameWorld, saves it to disk

private:
	bool ensureLightLinkHasLocalDeviceProfiles(); // returns if mLightLink changed
	bool setupCaptureDevice(); // specified by mLightLink.mCameraIndex
	void setupNextValidCaptureProfile(); // iterate through them
	bool tryToSetupValidCaptureDevice();
		// called once by lightLinkDidChange if setupCaptureDevice fails
		// it tries a

public:	
	LightLink			mLightLink; // calibration for camera <> world <> projector
	CaptureRef			mCapture;	// input device		->
	Vision				mVision ;	// edge detection	->
	Vision::Output		mVisionOutput; // contours, tokens ->
	GameWorldRef		mGameWorld ;// world simulation
	
	Pipeline			mPipeline; // traces processing

	
	// static debug frame
	SurfaceRef mDebugFrame;
	FileWatch  mDebugFrameFileWatch;
	void updateDebugFrameCaptureDevicesWithPxPerWorldUnit( float );
	
	// game library
	void loadGame( string systemName );
	void loadDefaultGame( string systemName="" );
	void loadAdjacentGame( int libraryIndexDelta );
	
	string mGameWorldCartridgeName; // what name of mGameLibrary did mGameWorld come from?

	// game xml params
	void				setGameWorldXmlParams( XmlTree );
	fs::path			getXmlConfigPathForGame( string );
	
	// world info
	PolyLine2 getWorldBoundsPoly() const;
		// This is actually the camera world polygon mapping.
		// For all practical purposes, this is identical to the projector world polygon mapping.
	
	
private:
	// keyboard input (for RFID device code parsing)
	string	mKeyboardString; // aggregate keystrokes here
	float	mLastKeyEventTime=-MAXFLOAT; // when was last keystroke?
	float	mKeyboardStringTimeout=.2f; // how long to wait before clearing the keyboard string buffer?
	bool	parseKeyboardString( string ); // returns true if successful
	
	map<int,string> mRFIDKeyToValue; // maps RFID #s to semantic strings
	map<string,function<void()>> mRFIDValueToFunction; // maps RFID semantic strings to code
	
	void setupRFIDValueToFunction(); // scans game library and binds loader code to rfid values
	
	// ui
public:
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
private:
	WindowRef			mMainWindow;// projector
	WindowRef			mUIWindow; // for other debug info, on computer screen
	
	WindowData*			getWindowData() { return getWindow() ? getWindow()->getUserData<WindowData>() : 0 ; }
	// for front window
	
	class FPS
	{
	public:
		void start(); // sets mLastFrameTime to now
		void mark();
		
		double				mLastFrameTime = 0. ;
		double				mLastFrameLength = 0.f ;
		float				mFPS=0.f;
	};

public:
	FPS mAppFPS;
	FPS mCaptureFPS;

	float mAVClacker=0.f;
	
private:
	
	// to help us visualize
	void addProjectorPipelineStages();
		
	void updateMainImageTransform( WindowRef );

	
	// settings
public:
	bool mAutoFullScreenProjector = false ;
	bool mDrawCameraImage = false ;
	bool mDrawContours = false ;
	bool mDrawContoursFilled = false ;
	bool mDrawMouseDebugInfo = false ;
	bool mDrawPolyBoundingRect = false ;
	bool mDrawContourTree = false ;
	bool mDrawPipeline = false;
	bool mDrawContourMousePick = false;
	bool mConfigWindowMainImageDrawBkgndImage = true;
	
	bool  mHasConfigWindow = true;
	float mConfigWindowMainImageMargin = 32.f;
	float mConfigWindowPipelineGutter = 8.f ;
	float mConfigWindowPipelineWidth  = 64.f ;
	
	float mDefaultPixelsPerWorldUnit = 10.f; // doesn't quite hot-load; you need to 
	
	int mDebugFrameSkip = 30;
	
public:
	fs::path hotloadableAssetPath( fs::path ) const ; // prepends the appropriate thing...

private:
	string mOverloadedAssetPath;
	
	FileWatch mFileWatch;

	fs::path getDocsPath() const;
	fs::path getUserLightLinkFilePath() const;
	fs::path getUserSettingsFilePath() const;
	
	// Synthesis
public:
	cipd::PureDataNodeRef mPd;
private:
	cipd::PatchRef mAVClackerPatch;
	
private: /* starting to make some stuff private... start somewhere */
	void updateVision();

	string getOverloadedAssetPath() const;

	void setupPureData();
	void setupWindows();
	
	void loadAppConfigXml( XmlTree );
	void loadLightLinkXml( XmlTree );
	void enumerateDisplaysAndCamerasToConsole() const;
	
	string getCommandLineArgValue( string param ) const;
	// e.g. Tabla -param returnValue
	
	//
	void loadUserSettingsFromXml( XmlTree );
	XmlTree getUserSettingsXml() const;
	void saveUserSettings();
};

#endif /* TablaApp_h */
