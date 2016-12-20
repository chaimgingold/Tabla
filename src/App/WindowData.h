//
//  WindowData.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#ifndef WindowData_hpp
#define WindowData_hpp

#include "View.h"
#include "PipelineStageView.h"
#include <memory>

class PaperBounce3App;
class GameLibraryView;

class WindowData {
public:

	WindowData( WindowRef window, bool isUIWindow, PaperBounce3App& ) ;
	
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

	void updatePipelineViews();
	
private:

	WindowRef mWindow;
	
	ViewCollection mViews;

	bool mIsUIWindow = false ; // as opposed to projector
	
	vec2 mMousePosInWindow=vec2(-1000,-1000); // just get it out of bounds!

	std::shared_ptr<MainImageView> mMainImageView; // main view, with image in it.
	std::shared_ptr<GameLibraryView> mGameLibraryView;
	
	vector<ViewRef> mPipelineViews;
	
	PaperBounce3App& mApp; // hacky transitional refactor thing.

};



#endif /* WindowData_hpp */
