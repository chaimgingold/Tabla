//
//  View.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/31/16.
//
//

#ifndef View_hpp
#define View_hpp

#include <vector>
#include <memory>
#include <string>

#include "cinder/Rect.h"
#include "cinder/gl/gl.h"
#include "geom.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class View;
typedef std::shared_ptr<View> ViewRef;

class View
{
public:

	// name
	string getName() const { return mName; }
	void   setName( string s ) { mName=s; }
	
	// geometry
	Rectf getFrame () const { return mFrame ; }
	Rectf getBounds() const { return mBounds ; }

	void setFrame ( Rectf f ) { mFrame  = f ; }
	void setBounds( Rectf b ) { mBounds = b ; }
	
	mat4 getParentToChildMatrix() const { return getRectMappingAsMatrix(mFrame,mBounds); }
	mat4 getChildToParentMatrix() const { return getRectMappingAsMatrix(mBounds,mFrame); }
	
	vec2 parentToChild( vec2 p ) const { return vec2( getParentToChildMatrix() * vec4(p,0,1) ); }
	vec2 childToParent( vec2 p ) const { return vec2( getChildToParentMatrix() * vec4(p,0,1) ); }
	
	// hierarchy
	void setParent( ViewRef v ) { mParent=v; }
	
	mat4 getRootToChildMatrix() const;
	mat4 getChildToRootMatrix() const;

	vec2 rootToChild( vec2 p ) const { return vec2( getRootToChildMatrix() * vec4(p,0,1) ); }
	vec2 childToRoot( vec2 p ) const { return vec2( getChildToRootMatrix() * vec4(p,0,1) ); }
	
	// drawing
	virtual void draw()
	{
		// framed white box
		// (frame should be in drawFrame, but this is easier to overload)
		gl::color(1,1,1);
		gl::drawSolidRect(mBounds);
		gl::color(0,0,0,.5);
		gl::drawStrokedRect(mBounds);
	}

	virtual void drawFrame()
	{
	}
	
	// interaction
	virtual bool pick( vec2 p ) { return mFrame.contains(p); }

	virtual void mouseDown( MouseEvent ){}
	virtual void mouseUp  ( MouseEvent ){}
	virtual void mouseMove( MouseEvent ){}
	virtual void mouseDrag( MouseEvent ){}
	virtual void resize(){}
	
	void setHasRollover( bool v ) { mHasRollover=v; }
	bool getHasRollover() const { return mHasRollover; }
	
	bool getHasMouseDown() const { return mHasMouseDown; }
	void setHasMouseDown( bool v ) { mHasMouseDown=v; }

protected:
	ivec2 getScissorLowerLeft( Rectf ) const;
	ivec2 getScissorSize( Rectf ) const;
	ivec2 getScissorLowerLeft() const { return getScissorLowerLeft(getFrame()); }
	ivec2 getScissorSize() const { return getScissorSize(getFrame()); }

private:
	bool	mHasRollover=false;
	bool	mHasMouseDown=false;
	
	string	mName;
	Rectf	mFrame = Rectf(0,0,1,1); // where it is in parent coordinate space
	Rectf	mBounds= Rectf(0,0,1,1); // what that coordinate space is mapped to (eg 0,0 .. 640,480)
		// initing to unit rect so that by default it does a valid no transform (no divide by zero)
	
	ViewRef mParent;
};


class ViewCollection
{
public:
	
	void	draw(); // draws all views in order they were added
	ViewRef	pickView( vec2 ); // tests in reverse order added
	ViewRef getViewByName( string );
	
	void addView   ( ViewRef v ) { mViews.push_back(v); }
	bool removeView( ViewRef v ); // returns whether found
	
	void mouseDown( MouseEvent );
	void mouseUp  ( MouseEvent );
	void mouseMove( MouseEvent );
	void mouseDrag( MouseEvent );
	void resize();
	
	ViewRef getMouseDownView() const { return mMouseDownView; }
	ViewRef getRolloverView()  const { return mRolloverView; }
	
private:
	void updateRollover( vec2 );
	
	ViewRef mMouseDownView;
	ViewRef mRolloverView;
	vector< ViewRef > mViews;
	
};

#endif /* View_hpp */
