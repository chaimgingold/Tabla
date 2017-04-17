//
//  Contour.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef Contour_hpp
#define Contour_hpp

#include "cinder/PolyLine.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

enum class ContourKind {
	Any,
	Holes,
	NonHoles
} ;

class Contour {
public:
	PolyLine2	mPolyLine ;
	vec2		mCenter ;
	Rectf		mBoundingRect=Rectf(0,0,0,0) ;
	float		mRadius=0.f;
	float		mArea=0.f;
	float		mPerimeter=0.f;
	
	struct tRotatedRect
	{
		vec2  mCenter;
		vec2  mSize;
		float mAngle; // same semantics as ocv
	} mRotatedBoundingRect;
	
	bool		mIsHole = false ;
	bool		mIsLeaf = true ;
	int			mParent = -1 ; // index into the contour we in
	vector<int> mChild ; // indices into mContour of contours which are in me
	int			mTreeDepth = 0 ;
	
	int			mIndex = -1;
	int			mOcvContourIndex = -1 ;
	
	bool		isKind ( ContourKind kind ) const
	{
		switch(kind)
		{
			case ContourKind::NonHoles:	return !mIsHole ;
			case ContourKind::Holes:	return  mIsHole ;
			case ContourKind::Any:
			default:					return  true ;
		}
	}
	
	bool		contains( vec2 point ) const
	{
		return mBoundingRect.contains(point) && mPolyLine.contains(point) ;
	}

	bool rayIntersection( vec2 rayOrigin, vec2 rayVec, float *rayt=0, int* pt1index=0 ) const;
	// returns first rayt, if any
	// and index of first vertex of line hit
	
};

class ContourVec : public vector<Contour>
{
public:

	typedef std::function<bool(const Contour&)> Filter;
	typedef vector<bool> Mask;
	
	Mask getMask( Filter f ) const
	{
		Mask m(size());
		for( int i=0; i<size(); ++i ) {
			m[i] = f((*this)[i]);
		}
		return m;
	}
	
	static Filter getMaskFilter( const Mask m ) {
		return [m](const Contour& c) -> bool
		{
			int i = c.mIndex;
			if (i > m.size()-1) return false;
			return m[i];
		};
	}
	
	static Filter getKindFilter( ContourKind kind ) {
		return [kind](const Contour& c) -> bool {
			return c.isKind(kind);
		};
	}
	
	static Filter getAndFilter( Filter f1, Filter f2 ) {
		if (!f2) return f1;
		if (!f1) return f2;
		return [f1,f2](const Contour& c) -> bool {
			return f1(c) && f2(c);
		};
	}
	
	// physics/geometry helpers
	const Contour* findClosestContour ( vec2 point, vec2* closestPoint=0, float* closestDist=0, Filter=0 ) const ; // assumes findLeafContourContainingPoint failed

	const Contour* findLeafContourContainingPoint( vec2 point, Filter=0 ) const ;
	Contour* findLeafContourContainingPoint( vec2 point, Filter=0 );

	const Contour* getParent( const Contour& c ) const {
		if (c.mParent!=-1) return &((*this)[c.mParent]);
		else return 0;
	}

	Contour* getParent( const Contour& c ) {
		if (c.mParent!=-1) return &((*this)[c.mParent]);
		else return 0;
	}
	
	int getIndex( const Contour& c ) const {
		int i = &c - &( (*this)[0] );
		if (i<0 || i>=size()) i=-1;
		return i;
	}
	
	ContourVec& operator+=(const ContourVec& rhs);
	ContourVec operator+(const ContourVec& rhs) const { ContourVec c = *this; c += rhs; return c; }
	
	const Contour* rayIntersection( vec2 rayOrigin, vec2 rayVec,
		float* rayt=0,
		int*   pt1index=0, // optional index of first vertex of line segment hit
		Filter = 0 // optional filter function
		) const;
	 // returns rayt of first edge hit, if any
	
};
typedef ContourVec ContourVector;

#endif /* Contour_hpp */
