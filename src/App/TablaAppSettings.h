//
//  TablaAppSettings.hpp
//  Tabla
//
//  Created by Chaim Gingold on 6/8/17.
//
//

#ifndef TablaAppSettings_hpp
#define TablaAppSettings_hpp


#include "cinder/Xml.h"

using namespace ci;
using namespace std;

class TablaAppSettings
{
public:
	void set( XmlTree );
	XmlTree get() const;
	
	float mDimProjection = 0.f; // everything; done globally by app.
	float mDimProjectionOnBlack = 0.f; // only black parts; not fully generalized yet, implemented per app.
	
};


#endif /* TablaAppSettings_hpp */
