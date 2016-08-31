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

	m *= glm::translate( vec3( to.getUpperLeft() - from.getUpperLeft(), 0.f ) );
	m *= glm::scale( vec3(to.getWidth() / from.getWidth(), to.getHeight() / from.getHeight(), 1.f ) );
	
	return m;
}


class View
{
public:

	Rectf mFrame;  // where it is in parent coordinate space
	Rectf mBounds; // what that coordinate space is mapped to (eg 0,0 .. 640,480)
	
	mat4 getParentToChildMatrix() { return getRectMappingAsMatrix(mFrame,mBounds); }
	mat4 getChildToParentMatrix() { return getRectMappingAsMatrix(mBounds,mFrame); }
	
	virtual void draw()
	{
		// framed white box
		gl::color(1,1,1);
		gl::drawSolidRect(mBounds);
		gl::color(0,0,0,.5);
		gl::drawStrokedRect(mBounds);
	}
	
};


#endif /* View_hpp */
