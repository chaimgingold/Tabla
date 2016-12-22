//
//  RectFinder.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/22/16.
//
//

#ifndef RectFinder_hpp
#define RectFinder_hpp

#include "cinder/Xml.h"
#include "Contour.h"

class RectFinder
{
public:
	
	class Params
	{
	public:
		void set( XmlTree );
		
	};
	Params mParams;
	
	bool getRectFromPoly( const PolyLine2&, vec2 quad[4], vec2 xaxis ) const;
	
private:
	
	
};

#endif /* RectFinder_hpp */