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
#include "XmlFileWatch.h"
#include "Pipeline.h"

#include "PipelineStageView.h"
#include "WindowData.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class PaperBounce3App : public App {
  public:
	~PaperBounce3App();
	
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void resize() override;
	void keyDown( KeyEvent event ) override;
	
	void drawWorld( bool highQuality );
	
	void lightLinkDidChange( const LightLink& ll )
	{
		// notify people
		// might need to privatize mLightLink and make this a proper setter
		// or rename it to be "notify" or "onChange" or "didChange" something
		mVision.setLightLink(ll);
		if (mGameWorld) mGameWorld->setWorldBoundsPoly( getWorldBoundsPoly() );
	}
	
	LightLink			mLightLink; // calibration for camera <> world <> projector
	CaptureRef			mCapture;	// input device		->
	Vision				mVision ;	// edge detection	->
	ContourVector		mContours;	// edges output		->
	std::shared_ptr<GameWorld> mGameWorld ;// world simulation
	
	Pipeline			mPipeline; // traces processing
	
	// game library
	void setupGameLibrary();
	void loadDefaultGame();
	void loadGame( int libraryIndex );
	void loadAdjacentGame( int libraryIndexDelta );
	
	vector< std::shared_ptr<GameCartridge> > mGameLibrary;
	int mGameWorldCartridgeIndex=-1; // what index of mGameLibrary did mGameWorld come from?

	// game xml params
	XmlTree				mGameXmlParams; // right now for all games
	void				setGameWorldXmlParams(); // sets mGameWorld params from mGameXmlParams

	// world info
	PolyLine2 getWorldBoundsPoly() const;
	vec2	  getWorldSize() const; // almost completely deprecated; hardly used.
		// This is actually the camera world polygon mapping.
		// For all practical purposes, this is identical to the projector world polygon mapping.
	
	
	// ui
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	WindowRef			mMainWindow;// projector
	WindowRef			mUIWindow; // for other debug info, on computer screen
	
	WindowData*			getWindowData() { return getWindow() ? getWindow()->getUserData<WindowData>() : 0 ; }
	// for front window
	
	double				mLastFrameTime = 0. ;
	
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

	float mConfigWindowMainImageMargin = 32.f;
	float mConfigWindowPipelineGutter = 8.f ;
	float mConfigWindowPipelineWidth  = 64.f ;
	
	fs::path myGetAssetPath( fs::path ) const ; // prepends the appropriate thing...
	string mOverloadedAssetPath;
	
	XmlFileWatch mXmlFileWatch;

	fs::path getDocsPath() const;
	fs::path getUserLightLinkFilePath() const;
};

#endif /* PaperBounce3App_h */
