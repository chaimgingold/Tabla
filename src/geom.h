//
//  geom.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef geom_h
#define geom_h

#include "cinder/PolyLine.h"

inline vec2 perp( vec2 p )
{
	vec3 cross = glm::cross( vec3(p,0), vec3(0,0,1) ) ;
	return vec2( cross.x, cross.y ) ;
}

inline vec2 closestPointOnLineSeg ( vec2 p, vec2 a, vec2 b )
{
	vec2 ap = p - a ;
	vec2 ab = b - a ;
	
	float ab2 = ab.x*ab.x + ab.y*ab.y;
	float ap_ab = ap.x*ab.x + ap.y*ab.y;
	float t = ap_ab / ab2;
	
	if (t < 0.0f) t = 0.0f;
	else if (t > 1.0f) t = 1.0f;
	
	vec2 x = a + ab * t;
	return x ;
}

inline vec2 closestPointOnPoly( vec2 pt, const PolyLine2& poly, size_t *ai=0, size_t *bi=0, float* dist=0 )
{
	float best = MAXFLOAT ;
	vec2 result = pt ;
	
	// assume poly is closed
	for( size_t i=0; i<poly.size(); ++i )
	{
		size_t j = (i+1) % poly.size() ;
		
		vec2 a = poly.getPoints()[i];
		vec2 b = poly.getPoints()[j];

		vec2 x = closestPointOnLineSeg(pt, a, b);
		
		float dist = glm::distance(pt,x) ; // could eliminate sqrt

		if ( dist < best )
		{
			best = dist ;
			result = x ;
			if (ai) *ai = i ;
			if (bi) *bi = j ;
		}
	}
	
	if (dist) *dist = best ;
	return result ;
}

PolyLine2 getPointsAsPoly( const vec2* v, int n )
{
	return PolyLine2( vector<vec2>(v,v+n) );
};

void setPointsFromPoly( vec2* v, int n, PolyLine2 vv )
{
	assert( vv.getPoints().size()==n );
	for( int i=0; i<n; ++i ) v[i] = vv.getPoints()[i];
};


#endif /* geom_h */
