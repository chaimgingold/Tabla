//
//  View.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/31/16.
//
//

#ifndef View_hpp
#define View_hpp

#include "cinder/Rect.h"
#include "cinder/gl/gl.h"

using namespace ci;


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
		gl::color(1,1,1);
		gl::drawSolidRect(mBounds);
		gl::color(0,0,0,.5);
		gl::drawStrokedRect(mBounds);
	}

private:
	Rectf mFrame;  // where it is in parent coordinate space
	Rectf mBounds; // what that coordinate space is mapped to (eg 0,0 .. 640,480)
	
};


#endif /* View_hpp */
