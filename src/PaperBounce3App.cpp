#include "PaperBounce3App.h"

#include "BallWorld.h"
#include "PongWorld.h"
#include "MusicWorld.h"
#include "CalibrateWorld.h"

#include "geom.h"
#include "xml.h"
#include "View.h"
#include "ocv.h"

#include <map>
#include <string>
#include <memory>

#include <stdlib.h> // system()

const int  kRequestFrameRate = 60 ; // was 120, don't think it matters/we got there
const vec2 kDefaultWindowSize( 640, 480 );

const string kDocumentsDirectoryName = "PaperES";

PaperBounce3App::~PaperBounce3App()
{
	cout << "Shutting down..." << endl;

//	mGameWorld.reset();

	cipd::PureDataNode::ShutdownGlobal();
}

PolyLine2 PaperBounce3App::getWorldBoundsPoly() const
{
	return getPointsAsPoly( mLightLink.mCaptureWorldSpaceCoords, 4 );
}

fs::path PaperBounce3App::getDocsPath() const
{
	return getDocumentsDirectory() / kDocumentsDirectoryName ;
}

fs::path PaperBounce3App::getUserLightLinkFilePath() const
{
	return getDocsPath() / "LightLink.xml" ;
}

void PaperBounce3App::setup()
{

	cout << getAppPath() << endl;
	
	// command line args
	auto args = getCommandLineArgs();
	
	for( size_t a=0; a<args.size(); ++a )
	{
		if ( args[a]=="-assets" && args.size()>a )
		{
			string overloadedAssetPath = args[a+1];
			if ( fs::exists(overloadedAssetPath) ) {
				mOverloadedAssetPath = overloadedAssetPath;
			}
		}
	}
	
	//
	mLastFrameTime = getElapsedSeconds() ;

	// enumerate hardware
	{
		auto displays = Display::getDisplays() ;
		cout << displays.size() << " Displays" << endl ;
		for ( auto d : displays )
		{
			cout << "\t" << d->getName() << " " << d->getBounds() << endl ;
		}

		auto cameras = Capture::getDevices() ;
		cout << cameras.size() << " Cameras" << endl ;
		for ( auto c : cameras )
		{
			cout << "\t" << c->getName() << endl ;
		}
	}
	
	// make docs folder if needed
	if ( !fs::exists(getDocsPath()) ) fs::create_directory(getDocsPath());
	
	// configuration
	mFileWatch.loadXml( hotloadableAssetPath("config.xml"), [this]( XmlTree xml )
	{
		// 1. get params
		if ( xml.hasChild("PaperBounce3/Games") )
		{
			mGameXmlParams = xml.getChild("PaperBounce3/Games");
			setGameWorldXmlParams();
		}
		
		if (xml.hasChild("PaperBounce3/LightLink") && !fs::exists(getUserLightLinkFilePath()) )
		{
			mLightLink.setParams(xml.getChild("PaperBounce3/LightLink"));
			lightLinkDidChange();
		}
		
		if (xml.hasChild("PaperBounce3/App"))
		{
			XmlTree app = xml.getChild("PaperBounce3/App");
			
			getXml(app,"AutoFullScreenProjector",mAutoFullScreenProjector);
			getXml(app,"DrawCameraImage",mDrawCameraImage);
			getXml(app,"DrawContours",mDrawContours);
			getXml(app,"DrawContoursFilled",mDrawContoursFilled);
			getXml(app,"DrawMouseDebugInfo",mDrawMouseDebugInfo);
			getXml(app,"DrawPolyBoundingRect",mDrawPolyBoundingRect);
			getXml(app,"DrawContourTree",mDrawContourTree);
			getXml(app,"DrawPipeline",mDrawPipeline);
			getXml(app,"DrawContourMousePick",mDrawContourMousePick);
			
			getXml(app,"ConfigWindowPipelineWidth",mConfigWindowPipelineWidth);
			getXml(app,"ConfigWindowPipelineGutter",mConfigWindowPipelineGutter);
			getXml(app,"ConfigWindowMainImageMargin",mConfigWindowMainImageMargin);
			
			getXml(app,"KeyboardStringTimeout",mKeyboardStringTimeout);
		}

		if (xml.hasChild("PaperBounce3/RFID"))
		{
			XmlTree rfid = xml.getChild("PaperBounce3/RFID");
			
			for( auto item = rfid.begin("tag"); item != rfid.end(); ++item )
			{
				if ( item->hasChild("id") && item->hasChild("value") )
				{
					int		id		= item->getChild("id").getValue<int>();
					string	value	= item->getChild("value").getValue();
					
					mRFIDKeyToValue[id] = value;
				}
			}
		}
		
		// 2. respond

		
		// TODO:
		// - get a new camera capture object so that resolution can change live
		// - respond to fullscreen projector flag
		// - aux display config
	});

	// settings: LightLink
	mFileWatch.loadXml( getUserLightLinkFilePath(), [this]( XmlTree xml )
	{
		// this overloads the one in config.xml
		if ( xml.hasChild("LightLink") )
		{
			mLightLink.setParams(xml.getChild("LightLink"));
			lightLinkDidChange(false); // and don't save, since we are responding to a load.
		}
	});
	
	// ui stuff (do before making windows)
	mTextureFont = gl::TextureFont::create( Font("Avenir",12) );
	
	// setup main window
	mMainWindow = getWindow();
	mMainWindow->setTitle("Projector");
	mMainWindow->setUserData( new WindowData(mMainWindow,false,*this) );

	// resize window
	setWindowSize( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) ;

	// Fullscreen main window in secondary display
	auto displays = Display::getDisplays() ;

	if ( displays.size()>1 && mAutoFullScreenProjector )
	{
		// move to 2nd display
		mMainWindow->setPos( displays[1]->getBounds().getUL() );
		
		mMainWindow->setFullScreen(true) ;
	}

	// aux UI display
	if (1)
	{
		// for some reason this seems to create three windows once we fullscreen the main window :P
		mUIWindow = createWindow( Window::Format().size( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) );
		
		mUIWindow->setTitle("Configuration");
		mUIWindow->setUserData( new WindowData(mUIWindow,true,*this) );
		
		// move it out of the way on one screen
		if ( mMainWindow->getDisplay() == mUIWindow->getDisplay() )
		{
			mUIWindow->setPos( mMainWindow->getPos() + ivec2( -mMainWindow->getWidth()-16,0) ) ;
		}
	}
	
	// default pipeline image to query
	// (after loading xml)
	// (we query before making the pipeline because we only store the image requested :P!)
	if ( mPipeline.getQuery().empty() ) mPipeline.setQuery("input");
	mPipeline.setCaptureAllStageImages(mDrawPipeline);
	
	// load the games and the game
	setupGameLibrary();
	loadDefaultGame();
	setupRFIDValueToFunction();
}



void PaperBounce3App::setupGameLibrary()
{
	mGameLibrary.push_back( make_shared<BallWorldCartridge>() );
	mGameLibrary.push_back( make_shared<PongWorldCartridge>() );
	mGameLibrary.push_back( make_shared<CalibrateWorldCartridge>() );
	mGameLibrary.push_back( make_shared<MusicWorldCartridge>() );
}

void PaperBounce3App::setupRFIDValueToFunction()
{
	mRFIDValueToFunction.clear();
	
	// game loader functions
	for( auto i : mGameLibrary )
	{
		string name = i->getSystemName();
		
		mRFIDValueToFunction[ string("cartridge/") + name ] = [this,name]()
		{
			int cartNum = findCartridgeByName(name);
			
			if (cartNum==-1) cout << "Failed to find cartridge '" << name << "'" << endl;
			else if (cartNum!=mGameWorldCartridgeIndex) this->loadGame(cartNum);
		};
	}
}

void PaperBounce3App::lightLinkDidChange( bool saveToFile )
{
	// notify people
	// might need to privatize mLightLink and make this a proper setter
	// or rename it to be "notify" or "onChange" or "didChange" something
	setupCaptureDevice();
	mVision.setLightLink(mLightLink);
	if (mGameWorld) mGameWorld->setWorldBoundsPoly( getWorldBoundsPoly() );
	
	if (saveToFile)
	{
		XmlTree lightLinkXml = mLightLink.getParams();
		lightLinkXml.write( writeFile(getUserLightLinkFilePath()) );
	}
}

void PaperBounce3App::setupCaptureDevice()
{
	auto cameras = Capture::getDevices();
	
	Capture::DeviceRef camera;
	
	if (mLightLink.mCameraIndex<0) camera = cameras.back();
	else camera = cameras[ mLightLink.mCameraIndex % cameras.size() ];

	if ( !mCapture || mCapture->getDevice() != camera )
	{
		mCapture = Capture::create( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y, camera ) ; // get last camera
		mCapture->start();
	}
}

void PaperBounce3App::chooseNextCaptureDevice()
{
	mLightLink.mCameraIndex = (mLightLink.mCameraIndex+1) % Capture::getDevices().size();
}

void PaperBounce3App::loadDefaultGame()
{
	if ( !mGameLibrary.empty() )
	{
		loadGame(0);
	}
}

void PaperBounce3App::loadGame( int libraryIndex )
{
	assert( libraryIndex >=0 && libraryIndex < mGameLibrary.size() );
	
	mGameWorldCartridgeIndex = libraryIndex ;
	
	mGameWorld = mGameLibrary[libraryIndex]->load();
	
	if ( mGameWorld )
	{
		cout << "loadGame: " << mGameWorld->getSystemName() << endl;
		
		setGameWorldXmlParams();
		mVision.setParams( mGameWorld->getVisionParams() );
		mPipeline.setCaptureAllStageImages( mDrawPipeline || mGameWorld->getVisionParams().mCaptureAllPipelineStages );
			// this won't quite hotload right with mDrawPipeline,
			// but it never did.
			// we might just want to switch pipeline to always capturing everything.
		
		mGameWorld->setWorldBoundsPoly( getWorldBoundsPoly() );
		mGameWorld->gameWillLoad();
	}
}

void PaperBounce3App::loadAdjacentGame( int libraryIndexDelta )
{
	if ( !mGameLibrary.empty() )
	{
		int i = mGameWorldCartridgeIndex + libraryIndexDelta;
		
		if ( i >= (int)mGameLibrary.size() ) i = i % (int)mGameLibrary.size();
		if ( i < 0 ) i = (int)mGameLibrary.size() - ((-i) % (int)mGameLibrary.size()) ;
		
		loadGame(i);
	}
}

int PaperBounce3App::findCartridgeByName( string name )
{
	for( size_t i=0; i<mGameLibrary.size(); ++i )
	{
		if ( mGameLibrary[i]->getSystemName() == name ) return i;
	}
	return -1;
}

void PaperBounce3App::setGameWorldXmlParams()
{
	if ( mGameWorld )
	{
		string xmlNodeName = mGameWorld->getSystemName();
		
		if ( mGameXmlParams.hasChild(xmlNodeName) )
		{
			XmlTree gameParams = mGameXmlParams.getChild(xmlNodeName);
			
			// load game specific params
			mGameWorld->setParams( gameParams );
			
			// load Vision params for it
			if ( gameParams.hasChild("Vision") )
			{
				Vision::Params p;
				p.set( gameParams.getChild("Vision") );
				mGameWorld->setVisionParams(p);
				mVision.setParams( mGameWorld->getVisionParams() );
			}
		}
	}
}

fs::path
PaperBounce3App::hotloadableAssetPath( fs::path p ) const
{
	if ( mOverloadedAssetPath.empty() ) return getAssetPath(p);
	else return fs::path(mOverloadedAssetPath) / p ;
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	if (getWindowData()) getWindowData()->mouseDown(event);
}

void PaperBounce3App::mouseUp( MouseEvent event )
{
	if (getWindowData()) getWindowData()->mouseUp(event);
}

void PaperBounce3App::mouseMove( MouseEvent event )
{
	if (getWindowData()) getWindowData()->mouseMove(event);
}

void PaperBounce3App::mouseDrag( MouseEvent event )
{
	if (getWindowData()) getWindowData()->mouseDrag(event);
}

void PaperBounce3App::addProjectorPipelineStages()
{
	// set that image
	mPipeline.then( "projector", mLightLink.mProjectorSize ) ;
	
	mPipeline.setImageToWorldTransform(
		getOcvPerspectiveTransform(
			mLightLink.mProjectorCoords,
			mLightLink.mProjectorWorldSpaceCoords ));
}

void PaperBounce3App::update()
{
	mFileWatch.update();
	
	if ( mCapture && mCapture->checkNewFrame() )
	{
		// start pipeline
		mPipeline.start();

		// get image
		Surface frame( *mCapture->getSurface() ) ;
		
		// vision it
		mVision.processFrame(frame,mPipeline) ;
		
		// finish off the pipeline with draw stage
		addProjectorPipelineStages();
		
		// pass contours to ballworld (probably don't need to store here)
		mContours = mVision.mContourOutput ;
		
		if (mGameWorld)
		{
			mGameWorld->updateVision( mContours, mPipeline );
		}
		
		// update pipeline visualization
		if (mUIWindow && mUIWindow->getUserData<WindowData>() ) mUIWindow->getUserData<WindowData>()->updatePipelineViews();
		
		// since the pipeline stage we are drawing might have changed... (or come into existence)
		// update the window mapping
		// this is a little weird to put here, but ah well
		updateMainImageTransform(mUIWindow);
		updateMainImageTransform(mMainWindow);
	}
	
	if (mGameWorld) mGameWorld->update();
}

void PaperBounce3App::updateMainImageTransform( WindowRef w )
{
	if (w)
	{
		WindowData *win = w->getUserData<WindowData>();
		
		// might not have been created yet (e.g., in setup)
		if (win) win->updateMainImageTransform();
	}
}

void PaperBounce3App::resize()
{
	updateMainImageTransform(getWindow());
}

void PaperBounce3App::drawWorld( GameWorld::DrawType drawType )
{
	WindowData *win = getWindow()->getUserData<WindowData>();
	
	const vec2 mouseInWindow   = win->getMousePosInWindow();
	const vec2 mouseInWorld    = win->getMainImageView()->windowToWorld(mouseInWindow);
	const bool isMouseInWindow = win->getWindow()->getBounds().contains(win->getMousePosInWindow());
	
	const bool isUIWindow = win->getIsUIWindow();
	
	// draw frame
	if (0)
	{
		gl::color( 1, 1, 1 );
		gl::draw( getWorldBoundsPoly() ) ;
	}

	// draw contour bounding boxes, etc...
	if (mDrawPolyBoundingRect)
	{
		for( auto c : mContours )
		{
			if ( !c.mIsHole ) gl::color( 1.f, 1.f, 1.f, .2f ) ;
			else gl::color( 1.f, 0.f, .3f, .3f ) ;

			gl::drawStrokedRect(c.mBoundingRect);
		}
	}

	// draw contours
	if ( mDrawContours || isUIWindow )
	{
		// filled
		if ( mDrawContoursFilled )
		{
			// TODO make fill colors tunable
			ColorA holecolor(.0f,.0f,.0f,.8f);
			ColorA fillcolor(.2f,.2f,.4f,.5f);
			
			// recursive tree
			function<void(const Contour&)> drawOne = [&]( const Contour& c )
			{
				if ( c.mIsHole ) gl::color( holecolor );
				else gl::color( fillcolor );
				
				gl::drawSolid(c.mPolyLine);
				
				for( auto childIndex : c.mChild )
				{
					drawOne( mContours[childIndex] ) ;
				}
			};
			
			for( auto const &c : mContours )
			{
				if ( c.mTreeDepth==0 )
				{
					drawOne(c) ;
				}
			}
		}
		// outlines
		else
		{
			for( auto c : mContours )
			{
				ColorAf color ;
				
				if ( !c.mIsHole ) color = ColorAf(1,1,1);
				else color = ColorAf::hex( 0xF19878 ) ;
				
				gl::color(color) ;
				gl::draw(c.mPolyLine) ;
			}
		}

		if ( mDrawContourMousePick || isUIWindow )
		{
			// picked highlight
			const Contour* picked = mContours.findLeafContourContainingPoint( mouseInWorld ) ;
			
			if (picked)
			{
				if (picked->mIsHole) gl::color(6.f,.4f,.2f);
				else gl::color(.2f,.6f,.4f,.8f);
				
				gl::draw(picked->mPolyLine);
				
				if (0)
				{
					vec2 x = closestPointOnPoly(mouseInWorld,picked->mPolyLine);
					gl::color(.5f,.1f,.3f,.9f);
					gl::drawSolidCircle(x, 5.f);
				}
			}
		}
	}
	
	// draw balls
	if (mGameWorld) mGameWorld->draw(drawType);
	
	// mouse debug info
	if ( (mDrawMouseDebugInfo||isUIWindow) && isMouseInWindow && mGameWorld )
	{
		mGameWorld->drawMouseDebugInfo(mouseInWorld);
	}
}

void PaperBounce3App::draw()
{
	WindowData* win = getWindow()->getUserData<WindowData>() ;
	
	if (win) win->draw();
}

void PaperBounce3App::keyDown( KeyEvent event )
{
	// string aggregation -- still allow it to cascade to later handlers though
	if ( !event.isMetaDown() && !event.isControlDown() && !event.isAltDown() )
	{
		float now = getElapsedSeconds();
		
		if ( now - mLastKeyEventTime > mKeyboardStringTimeout )
		{
			mKeyboardString.clear();
		}
		
		mKeyboardString += event.getChar();
		mLastKeyEventTime=now;
		
		bool handled = parseKeyboardString(mKeyboardString);
		if (handled) mKeyboardString="";
	}
	
	bool handled = false;
	
	// meta chars
	if ( event.isAltDown() )
	{
		switch ( event.getCode() )
		{
			case KeyEvent::KEY_TAB:
				handled=true;
				chooseNextCaptureDevice();
				lightLinkDidChange();
				break;
		}
	}
	
	// handle char
	if ( !handled )
	{
		switch ( event.getCode() )
		{
			case KeyEvent::KEY_f:
				cout << "Frame rate: " << getFrameRate() << endl ;
				break ;

			case KeyEvent::KEY_x:
				::system( (string("open \"") + hotloadableAssetPath("config.xml").string() + "\"").c_str() );
				break ;
				
			case KeyEvent::KEY_UP:
				if (event.isMetaDown()) mPipeline.setQuery( mPipeline.getFirstStageName() );
				else mPipeline.setQuery( mPipeline.getPrevStageName(mPipeline.getQuery() ) );
				break;
				
			case KeyEvent::KEY_DOWN:
				if (event.isMetaDown()) mPipeline.setQuery( mPipeline.getLastStageName() );
				else mPipeline.setQuery( mPipeline.getNextStageName(mPipeline.getQuery() ) );
				break;
			
			case KeyEvent::KEY_LEFT:
				loadAdjacentGame(-1);
				break;
			
			case KeyEvent::KEY_RIGHT:
				loadAdjacentGame(1);
				break;
			
			default:
				if (mGameWorld) mGameWorld->keyDown(event);
				break;
		}
	}
}

bool PaperBounce3App::parseKeyboardString( string str )
{
	bool handled = false;
	
	// RFID reader
	// - digits + newline
	
//	cout << "parseKeyboardString( '" << str << "' )" << endl;
	
	auto getAsRFID = []( string str ) -> int
	{
		if ( str.back()!='\n' && str.back()!='\r' ) return 0;
		
		for( int i=0; i<str.length()-2; ++i ) if ( !isdigit(str[i]) ) return 0;
		
		return stoi( str.substr(0,str.length()-1) );
	};
	
	int rfid = getAsRFID(str);
	
	if (rfid)
	{
		// rfid -> value
		cout << "RFID: #" << rfid << endl;
		
		auto value = mRFIDKeyToValue.find(rfid);
		
		if (value == mRFIDKeyToValue.end()) cout << "\tUnmapped RFID key " << rfid << endl;
		else
		{
			cout << "\t" << rfid << " => " << value->second << endl;
			
			// value -> function
			auto func = mRFIDValueToFunction.find(value->second);
			if ( func != mRFIDValueToFunction.end() && func->second )
			{
				// do it
				func->second();
				handled=true;
			}
			else cout << "\tNo function defined" << endl;
		}
	}

	return handled;
}

CINDER_APP( PaperBounce3App, RendererGl(RendererGl::Options().msaa(8)), [&]( App::Settings *settings ) {
	settings->setFrameRate(kRequestFrameRate);
	settings->setWindowSize(kDefaultWindowSize);
	settings->setPowerManagementEnabled(true); // contrary to intuition, 'true' means "disable sleep/idle"
	settings->setTitle("See Paper") ;
})
