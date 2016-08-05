//
//  Vision.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef Vision_hpp
#define Vision_hpp

#include <string>

#include "cinder/Surface.h"
#include "cinder/Xml.h"
#include "CinderOpenCV.h"

#include "Vision.h"
#include "Contour.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class Pipeline
{
	/*	Pipeline isn't the pipeline so much as helping us trace the pipeline's process.
		However, I'm sticking with this name as I hope that it can one day develop into
		both debugging insight into the pipeline, controls for it, and eventually a program
		that is the image munging pipeline itself.
	*/
	
public:
	void setQuery( string q ) { mQuery=q; } ;

	void then( Surface &img, string name )
	{
		if (name==mQuery)
		{
			mFrame = gl::Texture::create( img );
		}
	}
	
	void then( cv::Mat &img, string name )
	{
		if (name==mQuery)
		{
			mFrame = gl::Texture::create( fromOcv(img), gl::Texture::Format().loadTopDown() );
		}
	}
	
	// add types:
	// - contour
	
	gl::TextureRef getQueryFrame() const { return mFrame ; }
	
private:
	string		   mQuery ;
	gl::TextureRef mFrame ;
	
} ;


class Vision
{
public:

	// settings
	void setParams( XmlTree );

	float mResScale			=   .2;
	float mContourMinRadius	=	3;
	float mContourMinArea	=	100;
	float mContourDPEpsilon	=	5;
	float mContourMinWidth	=	5;

	// push input through
	void processFrame( const Surface &surface );
	
	// output
	ContourVector mContourOutput;
	
	// tracing output
	Pipeline	  mOCVPipelineTrace ;

};

#endif /* Vision_hpp */
