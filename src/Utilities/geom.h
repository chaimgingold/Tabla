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

vec2 closestPointOnLineSeg ( vec2 p, vec2 a, vec2 b );

vec2 closestPointOnPoly( vec2 pt, const PolyLine2& poly, size_t *ai=0, size_t *bi=0, float* dist=0 );

bool rayLineSegIntersection( vec2 rayOrigin, vec2 rayDirection, vec2 point1, vec2 point2, float *rayt=0 );

PolyLine2 getPointsAsPoly( const vec2* v, int n );

void setPointsFromPoly( vec2* v, int n, PolyLine2 vv );

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

void appendQuad( TriMesh& mesh, ColorA color, const vec2 v[4], const vec2 uv[4]=0 );

inline vec2 transformPoint(mat4 transform, vec2 point) {
	return vec2(transform * vec4(point,0,1));
}

mat4 getRectMappingAsMatrix( Rectf from, Rectf to );

bool rayIntersectPoly( const PolyLine2& poly, vec2 rayOrigin, vec2 rayVec, float *rayt=0, int* pt1index=0 );

bool getOrientedQuadFromPolyLine( PolyLine2 polyquad, vec2 xVec, vec2 quad[4] ); // returns false if polyquad.size()!=4
	/* Returns oriented like so:
	
     0---1
	 |   |
	 3---2

	 --> xVec	*/

PolyLine2 getConvexHull( const PolyLine2& ); // does the right thing with closed polys, unlike built-in cinder func.

float calcPolyEdgeOverlapFrac( const PolyLine2& a, const PolyLine2& b, float distanceAttenuate );

bool doPolygonsIntersect( const PolyLine2&, const PolyLine2& );

#endif /* geom_h */
