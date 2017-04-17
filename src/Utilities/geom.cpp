//
//  geom.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "geom.h"
#include "cinder/ConvexHull.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/multi.hpp>

using namespace cinder;
using namespace std;

vec2 closestPointOnLineSeg ( vec2 p, vec2 a, vec2 b )
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

vec2 closestPointOnPoly( vec2 pt, const PolyLine2& poly, size_t *ai, size_t *bi, float* dist )
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

bool rayLineSegIntersection( vec2 rayOrigin, vec2 rayDirection, vec2 point1, vec2 point2, float *rayt )
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

PolyLine2 getPointsAsPoly( const vec2* v, int n )
{
	return PolyLine2( vector<vec2>(v,v+n) );
}

void setPointsFromPoly( vec2* v, int n, PolyLine2 vv )
{
	assert( vv.getPoints().size()==n );
	for( int i=0; i<n; ++i ) v[i] = vv.getPoints()[i];
}

void appendQuad( TriMesh& mesh, ColorA color, const vec2 v[4], const vec2 uv[4] )
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

mat4 getRectMappingAsMatrix( Rectf from, Rectf to )
{
	mat4 m;

	m *= glm::translate( vec3(  to.getUpperLeft(), 0.f ) );

	m *= glm::scale( vec3(to.getWidth() / from.getWidth(), to.getHeight() / from.getHeight(), 1.f ) );

	m *= glm::translate( vec3( -from.getUpperLeft(), 0.f ) );

	return m;
}

bool rayIntersectPoly( const PolyLine2& poly, vec2 rayOrigin, vec2 rayVec, float *rayt, int* pt1index )
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
			if (pt1index) *pt1index = i;
			hit=true;
		}
	}
	
	return hit;	
}

bool getOrientedQuadFromPolyLine( PolyLine2 poly, vec2 xVec, vec2 quad[4] )
{
	// could have simplified a bit and just examined two bisecting lines. oh well. it works.
	// also calling this object 'Score' and the internal scores 'score' is a little confusing.

	if ( poly.size()==4 )
	{
		auto in = [&]( int i ) -> vec2
		{
			return poly.getPoints()[i%4];
		};

		auto scoreEdge = [&]( int from, int to )
		{
			return dot( xVec, normalize( in(to) - in(from) ) );
		};

		auto scoreSide = [&]( int side )
		{
			/* input pts:
			   0--1
			   |  |
			   3--2

			   side 0 : score of 0-->1, 3-->2
			*/

			return ( scoreEdge(side+0,side+1) + scoreEdge(side+3,side+2) ) / 2.f;
		};

		int   bestSide =0;
		float bestScore=0;

		for( int i=0; i<4; ++i )
		{
			float score = scoreSide(i);

			if ( score > bestScore )
			{
				bestScore = score ;
				bestSide  = i;
			}
		}

		// copy to quad
		quad[0] = in( bestSide+3 );
		quad[1] = in( bestSide+2 );
		quad[2] = in( bestSide+1 );
		quad[3] = in( bestSide+0 );
		// wtf i don't get the logic but it works
		// my confusion stems, i think, from the fact that y is inverted.
		// but it should be since we are likely in an upside down coordinate space,
		// so the ordering here is fixing that, whereas the right thing to do is handle it
		// explicitly elsewhere. but this works and stuff that uses it works, so not messing with now.

		return true ;
	}
	else return false;	
}

PolyLine2 getConvexHull( const PolyLine2 &in )
{
	PolyLine2 out = calcConvexHull(in);
	
	if (in.isClosed() && out.size()>0)
	{
		// cinder returns first and last point identical, which we don't want
		if (out.getPoints().back()==out.getPoints().front())
		{
			out.getPoints().pop_back();
		}
		out.setClosed( in.isClosed() );
	}
	
	return out;
}


static float getFracOfEdgeCoveredByPoly( vec2 a, vec2 b, const PolyLine2& p, float& out_abLen, float distanceAttenuate )
{
	const vec2  ab = b - a;
	const vec2  abnorm = normalize(ab);
	const vec2  abnormperp = perp(abnorm);
	const float ablen  = length(ab);
	
	float edgefrac = 0.f;
	
	const int pn = p.isClosed() ? p.size() : p.size()-1; 
	for( int i=0; i<pn; ++i )
	{
		const int j = (i+1) % p.size();
		
		const vec2 pi = p.getPoints()[i];
		const vec2 pj = p.getPoints()[j];
		const vec2 pij = pj - pi;
		const vec2 pijnorm = normalize(pij);
		
		// pi,pj projected onto ab sits from t0..t1 (ab goes from 0...ablen)
		float t0 = dot(pi-a,abnorm);
		float t1 = dot(pj-a,abnorm);
		if (t0>t1) swap(t0,t1);
		
		float overlap;
		
		if ( t1<0.f || t0>ablen ) overlap=0.f;
		else
		{
			const float t0c = max(0.f,t0);
			const float t1c = min(ablen,t1);
			
			overlap = t1c - t0c;
		}
		
		// get how much angles coincide
		const float angle_overlap = pow( max( 0.f, dot(abnorm,pijnorm)), 2.f );
		
		// how far away is it?
		const float distance = fabs( dot(pi-a,abnormperp) ) + fabs( dot(pj-a,abnormperp) ) ;
		const float distance_score = 1.f - min(1.f, distance / distanceAttenuate);
		
		// 
		const float score = angle_overlap * distance_score ;
		edgefrac += overlap * score;
	}
	
	// finish
	out_abLen = ablen;
	return min( edgefrac, ablen ); // don't let us exceed the length of this edge
}

float calcPolyEdgeOverlapFrac( const PolyLine2& a, const PolyLine2& b, float distanceAttenuate )
{
	// what frac-of-A is covered-by-B ?
	
	const int an = a.isClosed() ? a.size() : a.size()-1; 
	
	float totalfrac=0.f;
	float totallen =0.f;
	
	for( int i=0; i<an; ++i )
	{
		int j = (i+1) % a.size();
		
		float len;
		float frac = getFracOfEdgeCoveredByPoly( a.getPoints()[i], a.getPoints()[j], b, len, distanceAttenuate );
		
		// integrate
//		totalfrac = (totalfrac * totallen + edgefrac * pji_len) / (totallen + pji_len);
		totalfrac += frac;
		totallen  += len;
	}
	
	return totalfrac / totallen;
}

// from ci::PolyLine2.cpp
namespace {
typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > polygon;

template<typename T>
polygon convertPolyLinesToBoostGeometry( const PolyLineT<T> &a )
{
	polygon result;
	
	for( auto ptIt = a.getPoints().begin(); ptIt != a.getPoints().end(); ++ptIt )
		result.outer().push_back( boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( ptIt->x, ptIt->y ) );
	/*
	for( typename std::vector<PolyLineT<T> >::const_iterator plIt = a.begin() + 1; plIt != a.end(); ++plIt ) {
		polygon::ring_type ring;
		for( typename std::vector<T>::const_iterator ptIt = plIt->getPoints().begin(); ptIt != plIt->getPoints().end(); ++ptIt )
			ring.push_back( boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( ptIt->x, ptIt->y ) );
		result.inners().push_back( ring );
	}*/
	
//	boost::geometry::correct( result ); // not needed since we have no inner geometry
	
	return result;
}
}

bool doPolygonsIntersect( const PolyLine2& a, const PolyLine2& b )
{
	if (1)
	{
		try
		{
			std::vector<PolyLine2> d1,d2;
			d1.push_back(a);
			d2.push_back(b);
			auto c = PolyLine2::calcIntersection(d1,d2);
			return ( !c.empty() && c[0].size()>0 );
		} catch (...) {
			return false;
		}
	}
	else
	{
		return boost::geometry::intersects(
			convertPolyLinesToBoostGeometry(a),
			convertPolyLinesToBoostGeometry(b) );
	}
}