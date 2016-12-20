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
	Rectf		mBoundingRect ;
	float		mRadius ;
	float		mArea ;
	
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

	bool rayIntersection( vec2 rayOrigin, vec2 rayVec, float *rayt=0 ) const; // returns first rayt, if any
	
};

class ContourVec : public vector<Contour>
{
public:

	// physics/geometry helpers
	const Contour* findClosestContour ( vec2 point, vec2* closestPoint=0, float* closestDist=0, ContourKind kind = ContourKind::Any ) const ; // assumes findLeafContourContainingPoint failed

	const Contour* findLeafContourContainingPoint( vec2 point ) const ;
	Contour* findLeafContourContainingPoint( vec2 point );

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
		float *rayt=0,
		std::function<bool(const Contour&)> = 0 // optional filter function
		) const;
	 // returns rayt of first edge hit, if any
	
};
typedef ContourVec ContourVector;

#endif /* Contour_hpp */