//
//  RectFinder.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/22/16.
//
//

#include "RectFinder.h"
#include "geom.h"
#include "xml.h"

void RectFinder::Params::set( XmlTree xml )
{
	getXml(xml,"AllowSubset",mAllowSubset);
	getXml(xml,"AllowSuperset",mAllowSuperset);

	if ( getXml(xml,"InteriorAngleMaxDelta",mInteriorAngleMaxDelta) ) {
		mInteriorAngleMaxDelta = toRadians(mInteriorAngleMaxDelta);
	}	

	getXml(xml,"MaxGainAreaFrac",mMaxGainAreaFrac);
	getXml(xml,"SubsetRectMinPerimOverlapFrac",mSubsetRectMinPerimOverlapFrac);
	getXml(xml,"EdgeOverlapDistAttenuate",mEdgeOverlapDistAttenuate);

	getXml(xml,"MinRectWidth",mMinRectWidth);
	getXml(xml,"MinRectArea",mMinRectArea);

}

bool RectFinder::getRectFromPoly( const PolyLine2& in, PolyLine2& out ) const
{
	// was it good to begin with?
	if ( in.size()==4 )
	{
		// size
		if ( !isSizeOK(in) ) return false;
		
		// angles
		if ( !areInteriorAnglesOK(in) ) return false;
		
		// OK
		out = in;
		return true;
	}

	// convex hull strategy
	if (mParams.mAllowSuperset)
	{
		PolyLine2 convexHull = getConvexHull(in);
		
		if ( checkIsConvexHullReasonable(convexHull,in) )
		{
			out=convexHull;
			return true;
		}
	}
	
	// theorize quads strategy
	if (mParams.mAllowSubset)
	{
		if ( trySubset(in,out) ) return true;
	}
	
	// fail
	return false;
}

float RectFinder::getPolyDiffArea( PolyLine2 a, PolyLine2 b, std::vector<PolyLine2>* diff )
{
	std::vector<PolyLine2> d1,d2;
	d1.push_back(a);
	d2.push_back(b);
	
	try
	{
		auto dout = PolyLine2::calcXor(d1,d2);
		if (diff) *diff = dout;

		float area=0.f;
		
		for( const auto &p : dout )
		{
			area += p.calcArea();
		}
		
		return area;

	} catch(...) { return MAXFLOAT; } // in case boost goes bananas on us
	// i suppose that's really us going bananas on it. :-)
}

static float getInteriorAngle( const PolyLine2& p, int index )
{
	int i = index==0 ? (p.size()-1) : (index-1);
	int j = index;
	int k = (index+1) % p.size();
	
	vec2 a = p.getPoints()[i];
	vec2 x = p.getPoints()[j];
	vec2 b = p.getPoints()[k];

	a -= x;
	b -= x;

	return acos( dot(a,b) / (length(a)*length(b)) );
}

bool RectFinder::areInteriorAnglesOK( const PolyLine2& p ) const
{
	for( int i=0; i<4; ++i )
	{
		if ( fabs(getInteriorAngle(p,i) - M_PI/2.f) > mParams.mInteriorAngleMaxDelta ) return false;
	}
	return true;
}

bool RectFinder::isSizeOK( const PolyLine2& p ) const
{
	// filter against size
	vec2 size(
		distance(p.getPoints()[0],p.getPoints()[1]),
		distance(p.getPoints()[1],p.getPoints()[2])
	 );
	 
	if ( min(size.x,size.y) < mParams.mMinRectWidth ) return false;
	if ( size.x * size.y    < mParams.mMinRectArea  ) return false;
	
	return true;
}

bool RectFinder::checkIsConvexHullReasonable( const PolyLine2& quad, const PolyLine2& source ) const
{
	// not quad
	if (quad.size()!=4) return false;
	
	// size
	if ( !isSizeOK(quad) ) return false;
	
	// max angle
	if ( !areInteriorAnglesOK(quad) ) return false;
	 
	// area changed too much
	if ( getPolyDiffArea(quad,source) > source.calcArea() * mParams.mMaxGainAreaFrac )
	{
		return false;
	}
	
	return true;
}

	
bool RectFinder::trySubsetQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const
{
	/*  o  k
		   |
		i--j
	*/
	
	int j = (i+1) % p.size();
	int k = (i+2) % p.size();
	
	const vec2 *v = &p.getPoints()[0]; 
	
	if ( (fabs(getInteriorAngle(p,j) - M_PI/2.f)) < angleDev )
	{
		vec2 jk = v[k] - v[j];
		vec2 ji = v[i] - v[j];		
		
		vec2 o = lerp( v[i] + jk, v[k] + ji, .5f );
		// split the difference between two projected fourth coordinate locations
		
		// filter based on size!
		const vec2 size( length(jk), length(ji) );
		const float area = size.x * size.y;
		{
			const float mindim = min(size.x,size.y);
				
			if ( mindim < mParams.mMinRectWidth ) return false;
			if ( area   < mParams.mMinRectArea  ) return false;
		}
		
		PolyLine2 theory;
		theory.push_back(v[i]);
		theory.push_back(v[j]);
		theory.push_back(v[k]);
		theory.push_back(o);
		theory.setClosed();
		
		// score heuristics
		const float score = calcPolyEdgeOverlapFrac(theory,p,mParams.mEdgeOverlapDistAttenuate);
		// TODO: consider relative area in relative ranking... we also want the biggest theorized quad!
		// the size filtering is giving us some of that, but it isn't principled enough...
		// trying this below: just using score as a minimum bar to cross, then comparing areas.
		// AND if we like this, we could move this filter into whomever calls theorize.
		// leaving it here leaves us flexible, though.
		
//		if ( score > ioBestScore )
		if ( score > ioBestScore && area > ioBestArea )
		{
//			ioBestScore = score; // just use input score as a floor
			ioBestArea  = area;
			result = theory;
			return true;
		}
		else
		{
			return false;				
		}			
	}
	
	return false;
};


bool RectFinder::trySubset( const PolyLine2& in, PolyLine2& out ) const
{
	float ioBestScore = mParams.mSubsetRectMinPerimOverlapFrac;
	float ioBestArea = 0.f;
	
	for( int i=0; i<in.size(); ++i )
	{
		trySubsetQuadFromEdge(in,i,out,ioBestScore,ioBestArea,mParams.mInteriorAngleMaxDelta);
	}
	
	return out.size()>0;
}