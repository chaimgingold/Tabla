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

class WindowData {
public:

	WindowData( WindowRef window, bool isUIWindow, PaperBounce3App& ) ;
	
	void draw();
	void mouseDown( MouseEvent event );
	void mouseUp  ( MouseEvent event );
	void mouseMove( MouseEvent event ) ;
	void mouseDrag( MouseEvent event ) ;

	vec2 getMousePosInWindow() const { return mMousePosInWindow; }
	void updateMainImageTransform(); // configure mMainImageView's transform

	std::shared_ptr<MainImageView> getMainImageView() { return mMainImageView; }
	
	ViewCollection& getViews() { return mViews; }
	
private:

	WindowRef mWindow;
	
	ViewCollection mViews;

	bool mIsUIWindow = false ; // as opposed to projector
	
	vec2 mMousePosInWindow;

	std::shared_ptr<MainImageView> mMainImageView; // main view, with image in it.

	PaperBounce3App& mApp; // hacky transitional refactor thing.

};



#endif /* WindowData_hpp */
