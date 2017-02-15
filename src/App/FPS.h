//
//  FPS.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/14/17.
//
//

#ifndef FPS_hpp
#define FPS_hpp

class FPS
{
public:
	void start() {
		mLastFrameTime = ci::app::getElapsedSeconds();
	}
	
	void mark()
	{
		double now = ci::app::getElapsedSeconds();
		
		mLastFrameLength = now - mLastFrameTime ;
		
		mLastFrameTime = now;
		
		mFPS = 1.f / mLastFrameLength;
	}
	
	double mLastFrameTime = 0. ;
	double mLastFrameLength = 0.f ;
	float  mFPS=0.f;
};

#endif /* FPS_hpp */
