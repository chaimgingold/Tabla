//
//  TablaAppSettings.cpp
//  Tabla
//
//  Created by Chaim Gingold on 6/8/17.
//
//

#include "TablaAppSettings.h"
#include "xml.h"

void TablaAppSettings::set( XmlTree xml )
{
	getXml(xml,"DimProjection",mDimProjection);
	getXml(xml,"DimProjectionOnBlack",mDimProjectionOnBlack);
}

XmlTree TablaAppSettings::get() const
{
	XmlTree xml("settings","");
 	
	xml.push_back( XmlTree("DimProjection", toString(mDimProjection)) );
	xml.push_back( XmlTree("DimProjectionOnBlack", toString(mDimProjectionOnBlack)) );
	
	return xml;
}