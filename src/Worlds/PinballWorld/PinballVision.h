//
//  PinballVision.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/8/17.
//
//

#ifndef PinballVision_hpp
#define PinballVision_hpp

#include "PinballParts.h"

namespace Pinball
{

class PinballWorld;

class PinballVision
{
public:

	PinballVision( PinballWorld& w ) : mWorld(w) {}
	// non-const because Parts get non-const refs to world
	
	void setParams( XmlTree );
	
	class Output
	{
	public:
		PartVec mParts;
		ContourVec mVisionContours;
	};
	
	PinballVision::Output update(
		const Vision::Output& visionOut,
		Pipeline& p,
		const PartVec& oldParts,
		const ContourVec* oldVisionContours=0 );
	
	AdjSpace getAdjacentSpace( const Contour*, vec2, const ContourVector& ) const ;
	AdjSpace getAdjacentSpace( vec2, const ContourVector& ) const ; // how much adjacent space is to the left, right?

	bool shouldContourBeAPart( const Contour&, const ContourVector& ) const;

	// - inter-frame coherence params
	float mPartTrackLocMaxDist = 1.f;
	float mPartTrackRadiusMaxDist = .5f;
	
private:
	float mDejitterContourMaxDist = 0.f;	

	// - params
	float mPartMinContourRadius = 0.f;
	float mPartMaxContourRadius = 5.f; // contour radius lt => part
	float mPartMaxContourAspectRatio = 2.f;
	float mHolePartMinContourRadius = 0.f;
	float mHolePartMaxContourRadius = 2.f;
	float mFlipperDistToEdge = 10.f; // how close to the edge does a flipper appear?
	
	// - do it
	PartVec getPartsFromContours( const ContourVector& ); // only reason this is non-const is b/c parts point to the world
	PartVec mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const;
	
	ContourVec dejitterVisionContours( ContourVec in, ContourVec old ) const;
	
	PinballWorld& mWorld;
	
};

} // ns Pinball

#endif /* PinballVision_hpp */
