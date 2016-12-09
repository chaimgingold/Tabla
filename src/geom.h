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

using namespace cinder;
using namespace std;

inline vec2 perp( vec2 p )
{
	vec3 cross = glm::cross( vec3(p,0), -vec3(0,0,1) ) ;
	return vec2(cross) ;
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

inline bool rayLineSegIntersection( vec2 rayOrigin, vec2 rayVec, vec2 line0, vec2 line1, float *rayt=0 )
{
	rayVec *= -1.f;
	// this negative sign is not what the recipe calls for, but recipe
	// seems to return bogus results.
	
	// https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d/
	vec2 v1 = rayOrigin - line0;
	vec2 v2 = line1 - line0;
	vec2 v3( -rayVec.y, rayVec.x );
	
	float t1 = length( glm::cross(vec3(v2,0),vec3(v1,0)) ) / dot(v2,v3);
	float t2 = dot(v1,v3) / dot(v2,v3);
	
	if (rayt) *rayt = t1;
	
	return (t2 >= 0.f && t2 <= 1.f) && t1>=0 ;
}

inline PolyLine2 getPointsAsPoly( const vec2* v, int n )
{
	return PolyLine2( vector<vec2>(v,v+n) );
}

inline void setPointsFromPoly( vec2* v, int n, PolyLine2 vv )
{
	assert( vv.getPoints().size()==n );
	for( int i=0; i<n; ++i ) v[i] = vv.getPoints()[i];
}

inline void getRectCorners( Rectf r, vec2 v[4] )
{
	// clockwise order
	// 0-1
	// | |
	// 3-2
	v[0] = r.getUpperLeft();
	v[1] = r.getUpperRight();
	v[2] = r.getLowerRight();
	v[3] = r.getLowerLeft();
}

inline void appendQuad( TriMesh& mesh, ColorA color, const vec2 v[4], const vec2 uv[4]=0 )
{
	/*  0--1
	    |  |
		3--2
	*/
	
	int i = mesh.getNumVertices();

	ColorA colors[4] = {color,color,color,color};
	
	mesh.appendPositions(v,4);
	mesh.appendColors(colors,4);
	if (uv) mesh.appendTexCoords0(uv,4);
	
	mesh.appendTriangle(i+0,i+1,i+3);
	mesh.appendTriangle(i+3,i+1,i+2);
}

inline vec2 transformPoint(mat4 transform, vec2 point) {
	return vec2(transform * vec4(point,0,1));
}

inline mat4 getRectMappingAsMatrix( Rectf from, Rectf to )
{
	mat4 m;

	m *= glm::translate( vec3(  to.getUpperLeft(), 0.f ) );

	m *= glm::scale( vec3(to.getWidth() / from.getWidth(), to.getHeight() / from.getHeight(), 1.f ) );

	m *= glm::translate( vec3( -from.getUpperLeft(), 0.f ) );

	return m;
}

#endif /* geom_h */
