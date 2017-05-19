//
//  TablaWindow.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#ifndef TablaWindow_hpp
#define TablaWindow_hpp

#include "View.h"
#include <memory>

class TablaApp;
class GameLibraryView;
class PolyEditView;
class MainImageView;
class CaptureProfileMenuView;
class VectorEditView;

class TablaWindow {
public:

	TablaWindow( WindowRef window, bool isUIWindow, TablaApp& ) ;
	
	TablaWindow(const TablaWindow&) = delete; // no copy constructor! bad!
	TablaWindow& operator=( const TablaWindow& ) = delete;	
	
	void draw();
	void mouseDown( MouseEvent event );
	void mouseUp  ( MouseEvent event );
	void mouseMove( MouseEvent event ) ;
	void mouseDrag( MouseEvent event ) ;
	void resize();
	
	WindowRef getWindow() const { return mWindow; }
	vec2 getMousePosInWindow() const { return mMousePosInWindow; }
	void updateMainImageTransform(); // configure mMainImageView's transform

	std::shared_ptr<MainImageView> getMainImageView() { return mMainImageView; }
	
	ViewCollection& getViews() { return mViews; }
	
	bool getIsUIWindow() const { return mIsUIWindow; }
	bool isInteractingWithCalibrationPoly() const;
	
	void updatePipelineViews();
	
private:

	WindowRef mWindow;
	
	ViewCollection mViews;

	bool mIsUIWindow = false ; // as opposed to projector
	
	vec2 mMousePosInWindow=vec2(-1000,-1000); // just get it out of bounds!

	std::shared_ptr<MainImageView> mMainImageView; // main view, with image in it.
	std::shared_ptr<GameLibraryView> mGameLibraryView;
	std::shared_ptr<CaptureProfileMenuView> mCaptureMenuView;

	std::shared_ptr<PolyEditView> mCameraPolyEditView;
	std::shared_ptr<PolyEditView> mProjPolyEditView;
	std::shared_ptr<PolyEditView> mWorldBoundsPolyEditView;
	
	std::shared_ptr<VectorEditView> mVecEditView;
	
	void addPolyEditors();
	void addMenus();
	
	void layoutMenus();
	
	vector<ViewRef> mPipelineViews;
	
	TablaApp& mApp; // hacky transitional refactor thing.

};
typedef std::shared_ptr<TablaWindow> TablaWindowRef;


#endif /* TablaWindow_hpp */
