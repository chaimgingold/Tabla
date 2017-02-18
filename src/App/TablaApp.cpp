#include "TablaApp.h"

#include "geom.h"
#include "xml.h"
#include "View.h"
#include "MainImageView.h"
#include "ocv.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"

#include <map>
#include <string>
#include <memory>

#include <stdlib.h> // system()

const int  kRequestFrameRate = 60 ; // was 120, don't think it matters/we got there
const vec2 kDefaultWindowSize( 640, 480 );

const string kDocumentsDirectoryName = "LaTabla";

TablaApp::~TablaApp()
{
	cout << "Shutting down..." << endl;

	mVisionInput.stop();
}

PolyLine2 TablaApp::getWorldBoundsPoly() const
{
	PolyLine2 p = getPointsAsPoly( mLightLink.getCaptureProfile().mCaptureWorldSpaceCoords, 4 );
	p.setClosed();
	return p;
}

fs::path TablaApp::getDocsPath() const
{
	return getDocumentsDirectory() / kDocumentsDirectoryName ;
}

fs::path TablaApp::getUserLightLinkFilePath() const
{
	return getDocsPath() / "LightLink.xml" ;
}

fs::path TablaApp::getUserSettingsFilePath() const
{
	return getDocsPath() / "settings.xml" ;
}

void TablaApp::setup()
{
	cout << getAppPath() << endl;

	mOverloadedAssetPath = getOverloadedAssetPath();
	
	//
	mAppFPS.start();
	mCaptureFPS.start();

	// enumerate hardware
	enumerateDisplaysAndCamerasToConsole();
	
	// make docs folder if needed
	if ( !fs::exists(getDocsPath()) ) fs::create_directory(getDocsPath());
	
	// app config
	mFileWatch.loadXml( hotloadableAssetPath("config") / "app.xml", [this]( XmlTree xml ) {
		loadAppConfigXml(xml);
	});

	// user LightLink (overloads one in app.xml)
	mFileWatch.loadXml( getUserLightLinkFilePath(), [this]( XmlTree xml ) {
		loadLightLinkXml(xml);
	});

	// Pure Data
	setupPureData();

	// ui stuff (do before making windows)
	mTextureFont = gl::TextureFont::create( Font("Avenir",12) );
	
	// setup main window
	setupWindows();

	// setup rfid functions
	setupRFIDValueToFunction();

	// user settings
	mPipelineStageSelection = "undistorted"; // default (don't want it in header)
	
	mFileWatch.loadXml( getUserSettingsFilePath(), [this]( XmlTree xml )
	{
		if ( xml.hasChild("settings") ) {
			loadUserSettingsFromXml(xml.getChild("settings"));
		}
	});
	
	// load default game, if none loaded (loadUserSettingsFromXml might have done it)
	if ( !mGameWorld ) loadDefaultGame();
}

string TablaApp::getCommandLineArgValue( string param ) const
{
	auto args = getCommandLineArgs();
	
	for( size_t a=0; a<args.size(); ++a )
	{
		if ( args[a]==param && args.size()>a )
		{
			return args[a+1];
		}
	}
	
	return string();
}

string TablaApp::getOverloadedAssetPath() const
{
	string r;
	
	auto overloadAssetPath = [this,&r]( string p )
	{
		cout << "OverloadedAssetPath: " << p << endl;
		r = p;
	};
	
	// env vars for hotloading assets
	{
		const char* srcroot = getenv("LATABLASRCROOT");
		if (srcroot) {
			overloadAssetPath( (fs::path(srcroot) / ".." / "assets").string() );
		}
	}
	
	// command line args
	string cmdline = getCommandLineArgValue("-assets");
	
	if (!cmdline.empty()) overloadAssetPath( cmdline );
	
	//
	return r;
}

void TablaApp::enumerateDisplaysAndCamerasToConsole() const
{
	auto displays = Display::getDisplays() ;
	cout << displays.size() << " Displays" << endl ;
	for ( auto d : displays )
	{
		cout << "\t '" << d->getName() << "' " << d->getBounds() << endl ;
	}

	auto cameras = Capture::getDevices() ;
	cout << cameras.size() << " Cameras" << endl ;
	for ( auto c : cameras )
	{
		cout << "\t '" << c->getName() << "' '" << c->getUniqueId() << "'" << endl ;
	}
}

void TablaApp::loadAppConfigXml( XmlTree xml )
{
	// 1. get params
	if (xml.hasChild("PaperBounce3/LightLink") && !fs::exists(getUserLightLinkFilePath()) )
	{
		mLightLink.setParams(xml.getChild("PaperBounce3/LightLink"));
		ensureLightLinkHasLocalDeviceProfiles();
		lightLinkDidChange(); // allow saving, since user settings version doesn't exist yet
		// Note: To simplify logic, we could just have this save the data to getUserLightLinkFilePath()
		// and let the code below load it. (If somehow it was necessary to be so robust that we can't save
		// user data it might be useful, but that's a questionable scenario.)
	}
	
	if (xml.hasChild("PaperBounce3/App"))
	{
		mParams.set( xml.getChild("PaperBounce3/App") );
		mVisionInput.setDebugFrameSkip( mParams.mDebugFrameSkip );
	}
	
	if (xml.hasChild("PaperBounce3/RFID"))
	{
		XmlTree rfid = xml.getChild("PaperBounce3/RFID");
		
		for( auto item = rfid.begin("tag"); item != rfid.end(); ++item )
		{
			if ( item->hasAttribute("id") && item->hasAttribute("do") )
			{
				int		id		= item->getAttribute("id").getValue<int>();
				string	value	= item->getAttribute("do").getValue();
				
				mRFIDKeyToValue[id] = value;
			}
		}
	}
	
	// 2. respond
	updateDebugFrameCaptureDevicesWithPxPerWorldUnit(getParams().mDefaultPixelsPerWorldUnit);
	
	// TODO:
	// - get a new camera capture object so that resolution can change live (I think I do that now)
	// - respond to fullscreen projector flag
	// - aux display config
}

void TablaApp::loadLightLinkXml( XmlTree xml )
{
	// this overloads the one in config.xml
	if ( xml.hasChild("LightLink") )
	{
		mLightLink.setParams(xml.getChild("LightLink"));
		bool didChange = ensureLightLinkHasLocalDeviceProfiles();
		lightLinkDidChange(didChange);
		// don't save, since we are responding to a load, unless ensureLightLinkHasLocalDeviceProfiles() changed.
	}
}

void TablaApp::setupWindows()
{
	mProjectorWin = TablaWindowRef( new TablaWindow(getWindow(),false,*this) );

	// resize window
	mProjectorWin->getWindow()->setSize(
		mLightLink.getCaptureProfile().mCaptureSize.x,
		mLightLink.getCaptureProfile().mCaptureSize.y ) ;

	// Fullscreen main window in secondary display
	auto displays = Display::getDisplays() ;

	if ( displays.size()>1 && getParams().mAutoFullScreenProjector )
	{
		// workaround for cinder multi-window fullscreen bug... :P
//		mProjectorWin->setPos( displays[1]->getBounds().getUL() + ivec2(10,10) ); // redundant to .display(displays[1]); could hide by putting outside of window or something
		mProjectorWin->getWindow()->hide(); // must hide before, not after, fullscreening.
		
		// fullscreen on 2nd display
		mProjectorWin->getWindow()->setFullScreen( true, FullScreenOptions()
			.kioskMode(true)
			.secondaryDisplayBlanking(false)
			.exclusive(true)
			.display(displays[1])
			) ;
	}

	mProjectorWin->getWindow()->getSignalMove()  .connect( [&]{ this->saveUserSettings(); });
	mProjectorWin->getWindow()->getSignalResize().connect( [&]{ this->saveUserSettings(); });
		
	// aux UI display
	if (getParams().mHasConfigWindow)
	{
		// note: would be nice to have this value hot-load, but not sure how to sequence
		// light link loading with that, and this is good enough for now.
		
		// for some reason this seems to sometimes create three windows once we fullscreen the main window :P
		// and then one of those is blank. That is the reason (besides kiosk mode) for the mHasConfigWindow switch,
		// as a workaround for that bug.
		WindowRef uiWindowRef = createWindow( Window::Format().size(
			mLightLink.getCaptureProfile().mCaptureSize.x,
			mLightLink.getCaptureProfile().mCaptureSize.y ) );
		
		mUIWindow = TablaWindowRef( new TablaWindow(uiWindowRef,true,*this) );
		
		// move it out of the way on one screen
		if ( mProjectorWin->getWindow()->getDisplay() == mUIWindow->getWindow()->getDisplay() )
		{
			mUIWindow->getWindow()->setPos(
				mProjectorWin->getWindow()->getPos()
				+ ivec2( -mProjectorWin->getWindow()->getWidth()-16,0) ) ;
		}
		
		// save changes (do this AFTER we set it up, so we don't save initial conditions!)
		mUIWindow->getWindow()->getSignalMove()  .connect( [&]{ this->saveUserSettings(); });
		mUIWindow->getWindow()->getSignalResize().connect( [&]{ this->saveUserSettings(); });
	}
}

void TablaApp::setupPureData()
{
	auto ctx = audio::master();
	
	cout << "Frames per block: " << endl;
	for (auto &device : ctx->deviceManager()->getDevices()) {
		int currentFramesPerBlock = ctx->deviceManager()->getFramesPerBlock(device);
		cout << "\t" << device->getName() << " - " << currentFramesPerBlock << endl;

//		ctx->deviceManager()->setFramesPerBlock(device, 1024);
//		ctx->deviceManager()->setFramesPerBlock(device, 512);
	}
	

	// Create the synth engine
	mPd = ctx->makeNode( new cipd::PureDataNode( audio::Node::Format().autoEnable() ) );

	// Connect synth to master output
	mPd >> audio::master()->getOutput();

	// Enable Cinder audio
	ctx->enable();
	
	mPd->addToSearchPath(hotloadableAssetPath("synths").string());
	
	for (auto p : fs::directory_iterator(hotloadableAssetPath("synths"))) {
		mPd->addToSearchPath(p.path().string());
	}

	// Lets us use lists to set arrays, which seems to cause less thread contention
	mPd->setMaxMessageLength(1024);

	// Build a virtual clacker
	auto rootPatch = hotloadableAssetPath("synths/clacker.pd");
	mAVClackerPatch = mPd->loadPatch( DataSourcePath::create(rootPatch) ).get();
}

void TablaApp::setupRFIDValueToFunction()
{
	mRFIDValueToFunction.clear();
	
	// game loader functions
	for( auto i : GameCartridge::getLibrary() )
	{
		string name = i.first;
		
		mRFIDValueToFunction[ string("cartridge/") + name ] = [this,name]()
		{
			this->loadGame(name);
		};
	}
}

bool TablaApp::ensureLightLinkHasLocalDeviceProfiles()
{
	bool dirty=false;
	
	// 1. Cameras

	// Make sure all cameras on this computer have capture device profiles
	if ( Capture::getDevices().empty() ) {
		cout << "No camera devices!" << endl;
	}
	else
	{
		auto devices = Capture::getDevices();
		
		for( const auto &d : devices )
		{
			if ( mLightLink.getCaptureProfilesForDevice(d->getName()).empty() )
			{
				vec2 size(640,480); // help! how do i find out the default sizes?
				
				LightLink::CaptureProfile profile(
					/*string("Default ") + */d->getName(),
					d->getName(),
					size,
					getParams().mDefaultPixelsPerWorldUnit
					);
				
				mLightLink.mCaptureProfiles[profile.mName] = profile;
				dirty=true;
			} // make profile for device?
		} // for
	}
	
	// 2. Projectors
	if ( mLightLink.mProjectorProfiles.empty() )
	{
		vec2 size(640,480);
		const vec2 *coords=0;
		
		// base it off the capture profile
		if ( !mLightLink.mCaptureProfiles.empty() )
		{
			mLightLink.ensureActiveProfilesAreValid(); // just in case we made it
			const LightLink::CaptureProfile &ref = mLightLink.getCaptureProfile();
			
			size = ref.mCaptureSize;
			coords = ref.mCaptureWorldSpaceCoords;
		}
		
		// make
		LightLink::ProjectorProfile p( "Default", size, coords );
		
		// insert
		mLightLink.mProjectorProfiles[p.mName] = p;
		dirty=true;
	}
	
	// what if the profile list was empty, and we added some profiles that will be used?
	// so: if there is no active profile, then set it to one that we made.
	if (mLightLink.ensureActiveProfilesAreValid()) dirty=true;
	
	return false;
}

void TablaApp::setCaptureProfile( string name )
{
	// Remember old one and switch back if name doesn't work out?
	mLightLink.setCaptureProfile(name);
	lightLinkDidChange();
}

void TablaApp::lightLinkDidChange( bool saveToFile, bool doSetupCaptureDevice )
{
	// start-up (and maybe choose) a valid capture device
	if ( doSetupCaptureDevice && !setupCaptureDevice() )
	{
		tryToSetupValidCaptureDevice();
	}

	// notify people
	// might need to privatize mLightLink and make this a proper setter
	// or rename it to be "notify" or "onChange" or "didChange" something
	mVision.setCaptureProfile(mLightLink.getCaptureProfile());
	if (mGameWorld) mGameWorld->setWorldBoundsPoly( getWorldBoundsPoly() );
	
	if (saveToFile)
	{
		XmlTree lightLinkXml = mLightLink.getParams();
		lightLinkXml.write( writeFile(getUserLightLinkFilePath()) );
	}
}

bool TablaApp::tryToSetupValidCaptureDevice()
{
	// try other profiles...
	for ( auto i : mLightLink.mCaptureProfiles )
	{
		mLightLink.setCaptureProfile(i.second.mName);
		if ( setupCaptureDevice() ) return true;
	}
	
	// fail
	return false;
}

bool TablaApp::setupCaptureDevice()
{
	if ( mLightLink.mCaptureProfiles.empty() )
	{
		return false;
	}
	else
	{
		setProjectorWorldSpaceCoordsFromCaptureProfile();

		return mVisionInput.setup( mLightLink.getCaptureProfile() );
	}
}

void TablaApp::maybeUpdateCaptureProfileWithFrame( SurfaceRef frame )
{
	// check for debug file image size change
	if ( frame && mVisionInput.isFile() )
	{
		auto profile = mLightLink.getCaptureProfile();
		
		if ( frame->getSize() != ivec2(profile.mCaptureSize) )
		{
			// update it
			mLightLink.getCaptureProfile() = LightLink::CaptureProfile(
				fs::path(profile.mFilePath),
				vec2(frame->getSize()),
				getParams().mDefaultPixelsPerWorldUnit
			);
			
			setProjectorWorldSpaceCoordsFromCaptureProfile();
			
			lightLinkDidChange(true,false);
			// BUT WE ARE NOW!
			// note: we aren't successfully saving new size to LightLink.xml
			// if we call lightLinkDidChange, we crash; it could be a number of factors.
			// but clearly it's messy to do so... we could be recursively in mFileWatch::load,
			// or enter a loop, or some craziness... 
			// might be cleanest to change to a dirty/event mechanism.
		}
	}
}

void TablaApp::setProjectorWorldSpaceCoordsFromCaptureProfile()
{
	if ( mLightLink.mCaptureProfiles.empty() ) return;
	if ( mLightLink.mProjectorProfiles.empty() ) return;

	const LightLink::CaptureProfile &profile = mLightLink.getCaptureProfile();

	auto &p = mLightLink.getProjectorProfile();
	
	for( int i=0; i<4; ++i ) {
		p.mProjectorWorldSpaceCoords[i] = profile.mCaptureWorldSpaceCoords[i];
	}
}

void TablaApp::setupNextValidCaptureProfile()
{
	int n=0;
	
	while ( n++ < mLightLink.mCaptureProfiles.size() )
	{
		auto i = mLightLink.mCaptureProfiles.find( mLightLink.getCaptureProfile().mName );
		
		i = ++i;
		
		if ( i==mLightLink.mCaptureProfiles.end() )
		{
			// wrap
			i = mLightLink.mCaptureProfiles.begin();
		}
		
		// set
		mLightLink.setCaptureProfile(i->second.mName);
		
		if ( setupCaptureDevice() ) {
			lightLinkDidChange();
			return; // success
		}
		// else: keep searching...
	}
	
	// mega-fail
}

void TablaApp::loadGame( string systemName )
{
	auto cart = GameCartridge::getLibrary().find(systemName);
	if (cart==GameCartridge::getLibrary().end()) {
		cout << "loadGame('" << systemName << "'), failed to find game."  << endl;
		return;
	}
	
	mGameWorld = cart->second->load();
	
	if ( mGameWorld )
	{
		cout << "loadGame: " << mGameWorld->getSystemName() << endl;
		
		// get config xml
		fs::path xmlConfigPath = getXmlConfigPathForGame(mGameWorld->getSystemName()) ;
		
		mFileWatch.loadXml( xmlConfigPath, [xmlConfigPath,this]( XmlTree xml )
		{
			// if we had already loaded this once, we stomp that old lambda and force a reload now.
			
			// why conditional?
			// make sure we aren't hotloading xml from game we aren't running anymore
			if ( xmlConfigPath == getXmlConfigPathForGame(mGameWorld->getSystemName()) )
			{
				// set params from xml
				setGameWorldXmlParams(xml);
			}
		});
		
		// set world bounds
		mGameWorld->setWorldBoundsPoly( getWorldBoundsPoly() );
		
		// notify
		mGameWorld->gameWillLoad();
	}
	
	// save settings
	saveUserSettings();
}

string TablaApp::getCurrentGameSystemName() const
{
	if ( mGameWorld ) return mGameWorld->getSystemName();
	else return string();
}

void TablaApp::loadDefaultGame()
{
	string systemName;
	
	// command line?
	systemName = getCommandLineArgValue("-gameworld");
	
	// default (BallWorld)
	if ( systemName.empty() ) systemName = getParams().mDefaultGameWorld;
	
	// first?
	if ( systemName.empty() && !GameCartridge::getLibrary().empty() )
	{
		systemName = GameCartridge::getLibrary().begin()->first;
	}
	
	// try
	if ( !systemName.empty() ) loadGame(systemName);
}

void TablaApp::loadAdjacentGame( int libraryIndexDelta )
{
	auto lib = GameCartridge::getLibrary();
	auto begin = lib.begin();
	auto end = lib.end();
	auto i = lib.find( getCurrentGameSystemName() );
	
	if ( i!=end )
	{
		while (libraryIndexDelta>0) {
			i = ++i;
			if (i==end) i = begin;
			libraryIndexDelta--;
		}
		while (libraryIndexDelta<0) {
			// this negative wrap is busted (*shrug*)
			if (i==begin) i = --end;
			else i = --i;
			libraryIndexDelta++;
		}
		
		if (i!=end) loadGame(i->first);
	}
}

fs::path
TablaApp::getXmlConfigPathForGame( string name )
{
	return hotloadableAssetPath("config") / (name + ".xml") ;
}

void TablaApp::setGameWorldXmlParams( XmlTree xml )
{
	if ( mGameWorld )
	{
		string name = mGameWorld->getSystemName();
		
		if ( xml.hasChild(name) )
		{
			XmlTree gameParams = xml.getChild(name);
			
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
TablaApp::hotloadableAssetPath( fs::path p ) const
{
	if ( mOverloadedAssetPath.empty() ) return getAssetPath(p);
	else return fs::path(mOverloadedAssetPath) / p ;
}

void TablaApp::mouseDown( MouseEvent event )
{
	if (getTablaWindow()) getTablaWindow()->mouseDown(event);
}

void TablaApp::mouseUp( MouseEvent event )
{
	if (getTablaWindow()) getTablaWindow()->mouseUp(event);
}

void TablaApp::mouseMove( MouseEvent event )
{
	if (getTablaWindow()) getTablaWindow()->mouseMove(event);
}

void TablaApp::mouseDrag( MouseEvent event )
{
	if (getTablaWindow()) getTablaWindow()->mouseDrag(event);
}

void TablaApp::fileDrop( FileDropEvent event )
{
	auto files = event.getFiles();
	if (files.size() > 0) {
		auto file = files.front();
		
		LightLink::CaptureProfile* profile = mLightLink.getCaptureProfileForFile(file.string());
		
		if ( profile )
		{
			mLightLink.setCaptureProfile(profile->mName);
			lightLinkDidChange();
		}
		else
		{
			auto surf = make_shared<Surface>( loadImage(file) );
			// let's verify it's cool, and get the size, but for consistency
			// we'll let lightLinkDidChange load the image.
			
			if (surf)
			{
				LightLink::CaptureProfile cp(file,surf->getSize(),getParams().mDefaultPixelsPerWorldUnit); 
				mLightLink.mCaptureProfiles[cp.mName] = cp;
				mLightLink.setCaptureProfile(cp.mName);
				lightLinkDidChange();
			}
		}
	}
}

void TablaApp::updateDebugFrameCaptureDevicesWithPxPerWorldUnit( float x )
{
	bool dirty=false;
	
	for( auto &c : mLightLink.mCaptureProfiles )
	{
		if ( !c.second.mFilePath.empty() )
		{
			dirty=true;
			c.second.setWorldCoordsFromCaptureCoords(x);
		}
	}
	
	if (dirty) lightLinkDidChange();
}

void TablaApp::addProjectorPipelineStages( Pipeline& p )
{
	// set that image
	p.then( "projector", mLightLink.getProjectorProfile().mProjectorSize ) ;
	
	p.back()->setImageToWorldTransform(
		getOcvPerspectiveTransform(
			mLightLink.getProjectorProfile().mProjectorCoords,
			mLightLink.getProjectorProfile().mProjectorWorldSpaceCoords ));
}

void TablaApp::selectPipelineStage( string s )
{
	string old = mPipelineStageSelection;
	
	mPipelineStageSelection=s;
	
	if (old!=s) saveUserSettings();
}

string TablaApp::getPipelineStageSelection() const
{
	return mPipelineStageSelection;
}

void TablaApp::update()
{
	mAppFPS.mark();
	
	mFileWatch.update();
	
	updateVision();
	
	if (mGameWorld) {
		mGameWorld->update();
		mGameWorld->prepareToDraw();
	}
	
	if (mAVClacker>0.f) mAVClacker = max(0.f,mAVClacker-.1f);
}

void TablaApp::updateVision()
{
	SurfaceRef frame = mVisionInput.getFrame();

	if ( frame )
	{
		// misc
		maybeUpdateCaptureProfileWithFrame(frame);
		mCaptureFPS.mark();

		// vision it
		mVisionOutput = mVision.processFrame(frame);
		
		// finish off the pipeline with draw stage
		addProjectorPipelineStages(mVisionOutput.mPipeline);
		
		// pass contours to ballworld
		if (mGameWorld)
		{
			mGameWorld->updateVision( mVisionOutput, mVisionOutput.mPipeline );
		}
		
		// update pipeline visualization
		if (mUIWindow) mUIWindow->updatePipelineViews();
		
		// since the pipeline stage we are drawing might have changed... (or come into existence)
		// update the window mapping
		// this is a little weird to put here, but ah well
		if (mUIWindow) mUIWindow->updateMainImageTransform();
		if (mProjectorWin) mProjectorWin->updateMainImageTransform();
	}
}

void TablaApp::cleanup() {
	if (mGameWorld) mGameWorld->cleanup();
}

void TablaApp::resize()
{
	if ( getTablaWindow() ) getTablaWindow()->resize();
}

vec2 TablaApp::getMousePosInWorld() const
{
	TablaWindow *win = getWindow()->getUserData<TablaWindow>();
	
	const vec2 mouseInWindow   = win->getMousePosInWindow();
	const vec2 mouseInWorld    = win->getMainImageView()->windowToWorld(mouseInWindow);
	
	return mouseInWorld;
}

void TablaApp::drawWorld( GameWorld::DrawType drawType )
{
	TablaWindow *win = getWindow()->getUserData<TablaWindow>();

	const bool isMouseInWindow = win->getWindow()->getBounds().contains(win->getMousePosInWindow());
	
	const bool isUIWindow = win->getIsUIWindow();
	
	// draw frame
	if (0)
	{
		gl::color( 1, 1, 1 );
		gl::draw( getWorldBoundsPoly() ) ;
	}

	// draw contour bounding boxes, etc...
	if (getParams().mDrawPolyBoundingRect)
	{
		for( auto c : mVisionOutput.mContours )
		{
			if ( !c.mIsHole ) gl::color( 1.f, 1.f, 1.f, .2f ) ;
			else gl::color( 1.f, 0.f, .3f, .3f ) ;

			gl::drawStrokedRect(c.mBoundingRect);
		}
	}

	// draw contours
	if ( getParams().mDrawContours || isUIWindow )
	{
		drawContours( getParams().mDrawContoursFilled, getParams().mDrawContourMousePick || isUIWindow, false );
	}
	else if (drawType==GameWorld::DrawType::Projector
			&& mUIWindow
			&& mUIWindow->isInteractingWithCalibrationPoly() )
	{
		drawContours( false, false, true );
	}

	
	// draw balls
	if (mGameWorld) mGameWorld->draw(drawType);
	
	// mouse debug info
	if ( (getParams().mDrawMouseDebugInfo&&isUIWindow) && isMouseInWindow && mGameWorld )
	{
		mGameWorld->drawMouseDebugInfo( getMousePosInWorld() );
	}
}

void TablaApp::drawContours( bool filled, bool mousePickInfo, bool worldBounds ) const
{
	// filled
	if ( filled )
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
				drawOne( mVisionOutput.mContours[childIndex] ) ;
			}
		};
		
		for( auto const &c : mVisionOutput.mContours )
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
		for( auto c : mVisionOutput.mContours )
		{
			ColorAf color ;
			
			if ( !c.mIsHole ) color = ColorAf(1,1,1);
			else color = ColorAf::hex( 0xF19878 ) ;
			
			gl::color(color) ;
			gl::draw(c.mPolyLine) ;
		}
	}

	// outline capture area
	if (worldBounds)
	{
		gl::color(0,1,1);
		gl::draw(getWorldBoundsPoly());
		
		vec2 k(1.f);
		
		auto v = getWorldBoundsPoly().getPoints(); // for some reason i have to put this here..., not in for ()
		
		for( auto p : v )
		{
			Rectf r(p-k,p+k);
			gl::drawSolidRect(r);
		}
	}

	if ( mousePickInfo )
	{
		vec2 mouseInWorld = getMousePosInWorld();
		
		// picked highlight
		const Contour* picked = mVisionOutput.mContours.findLeafContourContainingPoint( mouseInWorld ) ;
		
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

void TablaApp::draw()
{
	// draw window
	TablaWindow* win = getWindow()->getUserData<TablaWindow>() ;
	
	if (win) win->draw();
}

void TablaApp::keyDown( KeyEvent event )
{
	// string aggregation -- still allow it to cascade to later handlers though
	if ( !event.isMetaDown() && !event.isControlDown() && !event.isAltDown() )
	{
		float now = getElapsedSeconds();
		
		if ( now - mLastKeyEventTime > getParams().mKeyboardStringTimeout )
		{
			mKeyboardString.clear();
		}
		
		mKeyboardString += event.getChar();
		mLastKeyEventTime=now;
		
		bool handled = parseKeyboardString(mKeyboardString);
		if (handled) mKeyboardString="";
	}
	
	bool handled = false;
	
	auto openFile = []( fs::path path ) {
		::system( (string("open \"") + path.string() + string("\"")).c_str() );		
	};
	
	// meta chars
	if ( event.isMetaDown() )
	{
		bool caught=true;
		
		switch ( event.getCode() )
		{
			case KeyEvent::KEY_x:
				if (mGameWorld) {
					openFile( hotloadableAssetPath( fs::path("config") / "app.xml" ) );
				}
				break;
			
			case KeyEvent::KEY_l:
				openFile( getUserLightLinkFilePath() );
				break;

			case KeyEvent::KEY_c:
				mParams.mDrawContours = !mParams.mDrawContours;
				break;

			case KeyEvent::KEY_t:
				mParams.mDrawPipeline = !mParams.mDrawPipeline;
				break;
				
			case KeyEvent::KEY_k:
				mPd->sendBang("clack-clacker");
				mAVClacker=1.f;
				break;

			case KeyEvent::KEY_s:
				if (!mVisionInput.isFile()) saveCameraImageToDisk();
				break;
			
			case KeyEvent::KEY_r:
				// reset game world
				loadGame( getCurrentGameSystemName() );
				break;
			
			default:
				caught=false;
				break;
		}
		
		if (caught) handled=true;
	}
	
	// handle char
	if ( !handled )
	{
		switch ( event.getCode() )
		{
			case KeyEvent::KEY_x:
				openFile( getXmlConfigPathForGame( mGameWorld->getSystemName() ) );
				break ;
			
			case KeyEvent::KEY_UP:
			{
				string stage;

				if (event.isMetaDown()) stage = getPipeline().getFirstStageName() ;
				else stage = getPipeline().getPrevStageName( getPipelineStageSelection() );
				
				selectPipelineStage(stage);
			}
			break;
				
			case KeyEvent::KEY_DOWN:
			{
				string stage;

				if (event.isMetaDown()) stage = getPipeline().getLastStageName() ;
				else stage = getPipeline().getNextStageName( getPipelineStageSelection() );

				selectPipelineStage(stage);				
			}
			break;

			case KeyEvent::KEY_ESCAPE:
				{
					if ( mLightLink.getCaptureProfile().isFile() )
					{
						mVisionInput.stop();
						mLightLink.eraseCaptureProfile(mLightLink.getCaptureProfile().mName);
						setupNextValidCaptureProfile();
					}
				}
				break;
			
			default:
				if (mGameWorld) mGameWorld->keyDown(event);
				break;
		}
	}
}

void TablaApp::keyUp( KeyEvent event )
{
	if (mGameWorld) mGameWorld->keyUp(event);
}

bool TablaApp::parseKeyboardString( string str )
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

void TablaApp::loadUserSettingsFromXml( XmlTree xml )
{
	cout << "loadUserSettingsFromXml." << endl;
	
	if ( xml.hasChild("GameWorld") )
	{
		string name = xml.getChild("GameWorld").getValue();
		
		if ( !mGameWorld || mGameWorld->getSystemName() != name )
		{
			cout << "loadUserSettingsFromXml: going to game world '" << name << "'" << endl;
			
			loadGame(name);
		}
	}
	
	if ( xml.hasChild("PipelineQuery") )
	{
		string name = xml.getChild("PipelineQuery").getValue();
		selectPipelineStage(name);
	}
	
	auto getWindowBounds = [&xml]( string xmlName, TablaWindowRef win )
	{
		if ( xml.hasChild(xmlName) && win )
		{
			try {
				string val = xml.getChild(xmlName).getValue();
				
				Area a;
				if ( sscanf(val.c_str(), "(%d, %d)-(%d, %d)",&a.x1,&a.y1,&a.x2,&a.y2)==4 )
				{
					win->getWindow()->setSize(a.getSize());
					win->getWindow()->setPos (a.getUL());
				}
			} catch (...) {}
		}
	};
	
	getWindowBounds( "UIWindowBounds", mUIWindow );
	if (!mProjectorWin->getWindow()->isFullScreen()) getWindowBounds( "MainWindowBounds", mProjectorWin );
}

XmlTree TablaApp::getUserSettingsXml() const
{
	XmlTree xml("settings","");
	
	if (mGameWorld)
	{
		xml.push_back( XmlTree("GameWorld",mGameWorld->getSystemName()) );
	}
	
	xml.push_back( XmlTree("PipelineQuery", getPipelineStageSelection() ) );
	
	auto setWindowBounds = [&xml]( string xmlName, TablaWindowRef win ) {
		if (win) {
			xml.push_back( XmlTree(xmlName, toString(
				win->getWindow()->getBounds() + win->getWindow()->getPos() ))
			);
		}
	};
	
	setWindowBounds("UIWindowBounds",mUIWindow);
	setWindowBounds("MainWindowBounds",mProjectorWin);
	
	return xml;
}

void TablaApp::saveUserSettings()
{
	XmlTree t = getUserSettingsXml();
	t.write( writeFile(getUserSettingsFilePath()) );
}

void TablaApp::saveCameraImageToDisk()
{
	auto savestage = getPipeline().getStage("clipped");
	if (!savestage) return;
	auto saveglimage = savestage->getGLImage();
	if (!saveglimage) return;
	
	vector<string> allowedExtensions = {"png"};
	
	fs::path savepath = getSaveFilePath(fs::path(),allowedExtensions);
	
	if (!savepath.empty())
	{
		if (savepath.extension() != ".png") savepath.append(".png");
		
		writeImage( savepath, Surface8u(saveglimage->createSource()) );
	}
}

CINDER_APP( TablaApp, RendererGl(RendererGl::Options().msaa(8)), [&]( App::Settings *settings ) {
	settings->setFrameRate(kRequestFrameRate);
	settings->setWindowSize(kDefaultWindowSize);
	settings->setPowerManagementEnabled(true); // contrary to intuition, 'true' means "disable sleep/idle"
	settings->setTitle("Tabla") ;
})
