//
//  Contour.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "Contour.h"
#include "geom.h"

bool Contour::rayIntersection( vec2 rayOrigin, vec2 rayVec, float *rayt, int* pt1index ) const
{
	return rayIntersectPoly(mPolyLine, rayOrigin, rayVec, rayt );
}

const Contour* ContourVector::findClosestContour ( vec2 point, vec2* closestPoint, float* closestDist, Filter filter ) const
{
	float best = MAXFLOAT ;
	const Contour* result = 0 ;
	
	// can optimize this by using bounding boxes as heuristic, but whatev for now.
	for ( const auto &c : *this )
	{
		if ( !filter || filter(c) )
		{
			float dist ;
			
			vec2 x = closestPointOnPoly( point, c.mPolyLine, 0, 0, &dist ) ;
			
			if ( dist < best )
			{
				best = dist ;
				result = &c ;
				if (closestPoint) *closestPoint = x ;
				if (closestDist ) *closestDist  = dist ;
			}
		}
	}
	
	return result ;
}

const Contour* ContourVector::findLeafContourContainingPoint( vec2 point, Filter filter ) const
{
	function<const Contour*(const Contour&)> search = [&]( const Contour& at ) -> const Contour*
	{
		if ( filter && !filter(at) ) return 0;
		
		if ( at.contains(point) )
		{
			for( auto childIndex : at.mChild )
			{
				const Contour* x = search( (*this)[childIndex] ) ;
				
				if (x) return x ;
			}
			
			return &at ;
		}
		
		return 0 ;
	} ;

	for( const auto &c : *this )
	{
		if ( c.mTreeDepth == 0 )
		{
			const Contour* x = search(c) ;
			
			if (x) return x ;
		}
	}
	
	return 0 ;
}

// same as above, just without const...
Contour* ContourVector::findLeafContourContainingPoint( vec2 point, Filter filter )
{
	function<Contour*(Contour&)> search = [&]( Contour& at ) -> Contour*
	{
		if ( filter && !filter(at) ) return 0;

		if ( at.contains(point) )
		{
			for( auto childIndex : at.mChild )
			{
				Contour* x = search( (*this)[childIndex] ) ;
				
				if (x) return x ;
			}
			
			return &at ;
		}
		
		return 0 ;
	} ;

	for( auto &c : *this )
	{
		if ( c.mTreeDepth == 0 )
		{
			Contour* x = search(c) ;
			
			if (x) return x ;
		}
	}
	
	return 0 ;
}

ContourVector& ContourVector::operator+=(const ContourVector& rhs)
{
	int offset = size();
	
	for( auto c : rhs )
	{
		if (c.mParent != -1) c.mParent += offset;
		for ( auto &ch : c.mChild ) ch += offset;
		push_back(c);
	}
	
	return *this;
}

const Contour* ContourVector::rayIntersection(
	vec2 rayOrigin, vec2 rayVec,
	float *rayt, int* pt1index,
	Filter filter ) const
{
	float m = MAXFLOAT;
	const Contour* hit=0;
	
	for( const auto &c : *this )
	{
		if ( filter && !filter(c) ) continue;
		
		float d;
		if ( c.rayIntersection(rayOrigin,rayVec,&d,pt1index) && d < m )
		{
			m=d;
			hit=&c;
		}
	}
	
	if (hit&&rayt) *rayt = m;
	return hit;
}