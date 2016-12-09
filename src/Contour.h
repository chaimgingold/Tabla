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
	// TODO: rotated rect
	
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

class ContourVector : public vector<Contour>
{
public:

	// physics/geometry helpers
	const Contour* findClosestContour ( vec2 point, vec2* closestPoint=0, float* closestDist=0, ContourKind kind = ContourKind::Any ) const ; // assumes findLeafContourContainingPoint failed

	const Contour* findLeafContourContainingPoint( vec2 point ) const ;

	const Contour* getParent( const Contour* c ) const {
		if (c->mParent!=-1) return &((*this)[c->mParent]);
		else return 0;
	}

	Contour* getParent( const Contour* c ) {
		if (c->mParent!=-1) return &((*this)[c->mParent]);
		else return 0;
	}
	
};


#endif /* Contour_hpp */
