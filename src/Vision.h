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
#include <vector>

#include "cinder/Surface.h"
#include "cinder/Xml.h"
#include "CinderOpenCV.h"
#include "LightLink.h"

#include "Vision.h"
#include "Contour.h"
#include "Pipeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;



class Vision
{
public:

	// settings
	void setParams( XmlTree );

	float mContourMinRadius	=	3;
	float mContourMinArea	=	100;
	float mContourDPEpsilon	=	5;
	float mContourMinWidth	=	5;

	void setLightLink( const LightLink &ll ) { mLightLink=ll; }
	
	// push input through
	void processFrame( const Surface &surface, Pipeline& tracePipeline );
	
	// output
	ContourVector mContourOutput;
	
private:
	LightLink mLightLink;

};

#endif /* Vision_hpp */
