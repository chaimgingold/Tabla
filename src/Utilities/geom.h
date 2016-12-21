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

inline bool rayLineSegIntersection( vec2 rayOrigin, vec2 rayDirection, vec2 point1, vec2 point2, float *rayt=0 )
{
	// https://gist.github.com/danieljfarrell/faf7c4cafd683db13cbc
	
	vec2 v1 = rayOrigin - point1;
    vec2 v2 = point2 - point1;
    vec2 v3 = vec2(-rayDirection[1], rayDirection[0]) ;
    float t1 = cross(vec3(v2,0), vec3(v1,0)).z / dot(v2, v3);
    float t2 = dot(v1, v3) / dot(v2, v3);
	
    if ( t1 >= 0.0 && t2 >= 0.0 && t2 <= 1.f )
	{
		if (rayt) *rayt = t1;
		return true;
	}
	return false;
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

inline bool rayIntersectPoly( const PolyLine2& poly, vec2 rayOrigin, vec2 rayVec, float *rayt )
{
	bool hit=false;
	int n = poly.isClosed() ? poly.size() : poly.size()-1; 
	
	for( int i=0; i<n; ++i )
	{
		int j = (i+1) % poly.size();
		
		float t;
		
		bool h = rayLineSegIntersection(rayOrigin, rayVec, poly.getPoints()[i], poly.getPoints()[j], &t );
		
		if (h)
		{
			if (rayt)
			{
				if (hit) *rayt = min( *rayt, t ); // min of all
				else *rayt = t; // first
			}
			hit=true;
		}
	}
	
	return hit;	
}

#endif /* geom_h */
