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

using namespace ci;
using namespace ci::app;
using namespace std;

inline mat4 getRectMappingAsMatrix( Rectf from, Rectf to )
{
	mat4 m;

	m *= glm::translate( vec3(  to.getUpperLeft(), 0.f ) );
	
	m *= glm::scale( vec3(to.getWidth() / from.getWidth(), to.getHeight() / from.getHeight(), 1.f ) );
	
	m *= glm::translate( vec3( -from.getUpperLeft(), 0.f ) );
	
	return m;
}

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
	
private:
	string	mName;
	Rectf	mFrame;  // where it is in parent coordinate space
	Rectf	mBounds; // what that coordinate space is mapped to (eg 0,0 .. 640,480)
	
};

typedef std::shared_ptr<View> ViewRef;


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
	
private:
	ViewRef mMouseDownView;
	vector< ViewRef > mViews;
	
};

#endif /* View_hpp */
