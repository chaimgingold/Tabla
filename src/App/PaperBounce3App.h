//
//  PaperBounce3App.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#ifndef PaperBounce3App_h
#define PaperBounce3App_h

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


using namespace ci;
using namespace ci::app;
using namespace std;

class PaperBounce3App : public App {
  public:
	~PaperBounce3App();
	
	static PaperBounce3App* get() { return dynamic_cast<PaperBounce3App*>(AppBase::get()); }
	
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
	void cleanup() override;
	
	void drawWorld( GameWorld::DrawType );
	
	bool ensureLightLinkHasLocalDeviceProfiles(); // returns if mLightLink changed
	void lightLinkDidChange( bool saveToFile=true ); // calls setupCaptureDevice, tells mVision, tells mGameWorld, saves it to disk
	bool setupCaptureDevice(); // specified by mLightLink.mCameraIndex
	void setupNextValidCaptureProfile(); // iterate through them
	bool tryToSetupValidCaptureDevice();
		// called once by lightLinkDidChange if setupCaptureDevice fails
		// it tries a
	
	LightLink			mLightLink; // calibration for camera <> world <> projector
	CaptureRef			mCapture;	// input device		->
	Vision				mVision ;	// edge detection	->
	Vision::Output		mVisionOutput; // contours, tokens ->
	std::shared_ptr<GameWorld> mGameWorld ;// world simulation
	
	Pipeline			mPipeline; // traces processing

	
	// static debug frame
	SurfaceRef mDebugFrame;
	FileWatch  mDebugFrameFileWatch;
	void updateDebugFrameCaptureDevicesWithPxPerWorldUnit( float );
	
	// game library
	void setupGameLibrary();
	void loadDefaultGame( string byName="" );
	void loadGame( int libraryIndex );
	void loadAdjacentGame( int libraryIndexDelta );
	int  findCartridgeByName( string ); // -1 for fail
	
	vector<GameCartridgeRef> mGameLibrary;
	int mGameWorldCartridgeIndex=-1; // what index of mGameLibrary did mGameWorld come from?

	// game xml params
	void				setGameWorldXmlParams( XmlTree );
	fs::path			getXmlConfigPathForGame( string );
	
	// world info
	PolyLine2 getWorldBoundsPoly() const;
		// This is actually the camera world polygon mapping.
		// For all practical purposes, this is identical to the projector world polygon mapping.
	
	
	// keyboard input (for RFID device code parsing)
	string	mKeyboardString; // aggregate keystrokes here
	float	mLastKeyEventTime=-MAXFLOAT; // when was last keystroke?
	float	mKeyboardStringTimeout=.2f; // how long to wait before clearing the keyboard string buffer?
	bool	parseKeyboardString( string ); // returns true if successful
	
	map<int,string> mRFIDKeyToValue; // maps RFID #s to semantic strings
	map<string,function<void()>> mRFIDValueToFunction; // maps RFID semantic strings to code
	
	void setupRFIDValueToFunction(); // scans game library and binds loader code to rfid values
	
	// ui
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
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

	FPS mAppFPS;
	FPS mCaptureFPS;
	
	// to help us visualize
	void addProjectorPipelineStages();
		
	void updateMainImageTransform( WindowRef );
	
	/* Coordinates spaces, there are a few:
		
		UI (window coordinates, in points, not pixels)
			- pixels
			- points
			
		Image
			- Camera
				e.g. pixel location in capture image
			- Projector
				e.g. pixel location on a screen
			- Arbitrary
				e.g. supersampled camera image subset
				e.g. location of a pixel on a shown image
		
		World
			(sim size, unbounded)	eg. location of a bouncing ball
	 
	 
	Transforms
		UI    <> Image -- handled by View objects
		Image <> World -- currently handled by Pipeline::Stage transforms

	*/
	
	// settings
	bool mAutoFullScreenProjector = false ;
	bool mDrawCameraImage = false ;
	bool mDrawContours = false ;
	bool mDrawContoursFilled = false ;
	bool mDrawMouseDebugInfo = false ;
	bool mDrawPolyBoundingRect = false ;
	bool mDrawContourTree = false ;
	bool mDrawPipeline = false;
	bool mDrawContourMousePick = false;

	bool  mHasConfigWindow = true;
	float mConfigWindowMainImageMargin = 32.f;
	float mConfigWindowPipelineGutter = 8.f ;
	float mConfigWindowPipelineWidth  = 64.f ;
	
	float mDefaultPixelsPerWorldUnit = 10.f; // doesn't quite hot-load; you need to 
	
	fs::path hotloadableAssetPath( fs::path ) const ; // prepends the appropriate thing...
	string mOverloadedAssetPath;
	
	FileWatch mFileWatch;

	fs::path getDocsPath() const;
	fs::path getUserLightLinkFilePath() const;
	fs::path getUserSettingsFilePath() const;
	
	
private: /* starting to make some stuff private... start somewhere */
	void updateVision();

	//
	void loadUserSettingsFromXml( XmlTree );
	XmlTree getUserSettingsXml() const;
	void saveUserSettings();
};

#endif /* PaperBounce3App_h */
