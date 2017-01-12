//
//  PinballVision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/8/17.
//
//

#include "PinballVision.h"
#include "PinballWorld.h"

namespace Pinball
{

void PinballVision::setParams( XmlTree xml )
{
	getXml(xml, "FlipperDistToEdge", mFlipperDistToEdge );	
	getXml(xml, "PartMaxContourRadius", mPartMaxContourRadius );
	getXml(xml, "PartMaxContourAspectRatio", mPartMaxContourAspectRatio );
	getXml(xml, "HolePartMaxContourRadius",mHolePartMaxContourRadius);
	
	getXml(xml, "PartTrackLocMaxDist", mPartTrackLocMaxDist );
	getXml(xml, "PartTrackRadiusMaxDist", mPartTrackRadiusMaxDist );
	getXml(xml, "DejitterContourMaxDist", mDejitterContourMaxDist );
}


ContourVec PinballVision::dejitterVisionContours( ContourVec in, ContourVec old ) const
{
	// To really properly work this needs to fit new points into old line segments, not just old vertices.

	ContourVec out = in;
	
	for( auto &c : out )
	{
		// find closest contour to compare ourselves against
		float bestscore = MAXFLOAT;
		const Contour* match = 0;
		
		for( auto &o : old )
		{
			if (o.mIsHole != c.mIsHole) continue;
			if (o.mTreeDepth != c.mTreeDepth) continue;
			
			float score = distance( c.mCenter, o.mCenter )
				+ fabs( c.mRadius - o.mRadius )
				;
			
			if (score < bestscore)
			{
				bestscore=score;
				match = &o;
			}
		}
		
		// match points
		if (match)
		{
			for( auto &p : c.mPolyLine.getPoints() )
			{
				float bestscore = mDejitterContourMaxDist;
				
				for ( auto op : match->mPolyLine.getPoints() )
				{
					float score = distance(p,op);
					if (score<bestscore)
					{
						bestscore = score;
						p = op;
					}
				}
			}
		}
	}
	
	return out;
}

PinballVision::Output
PinballVision::update(
	const Vision::Output& visionOut,
	Pipeline& p,
	const PartVec& oldParts,
	const ContourVec* oldVisionContours )
{
	Output out;
	
	// capture contours, so we can later pass them into BallWorld (after merging in part shapes)
	if ( mDejitterContourMaxDist > 0.f && oldVisionContours ) {
		out.mVisionContours = dejitterVisionContours( visionOut.mContours, *oldVisionContours );
	} else {
		out.mVisionContours = visionOut.mContours;
	}
	
	// generate parts
	PartVec newParts = getPartsFromContours(out.mVisionContours);
	out.mParts = mergeOldAndNewParts( oldParts, newParts );
	
	return out;
}

bool PinballVision::shouldContourBeAPart( const Contour& c, const ContourVec& cs ) const
{
	// no children?
//	if (!c.mChild.empty()) return false; // seems logical; or does it matter? is this just annoying?
	
	// round enough?
	{
		float lo = c.mRotatedBoundingRect.mSize.x;
		float hi = c.mRotatedBoundingRect.mSize.y;
		
		if (hi<lo) swap(lo,hi);
		
		// not round enough
		if (hi>lo*mPartMaxContourAspectRatio) return false;
	}
	
	if ( c.mIsHole ) {
		return c.mTreeDepth>0 && c.mRadius < mPartMaxContourRadius;
	}
	else {
		bool depthOK;
		
		if (c.mTreeDepth==0) depthOK=true; // ok
		else if (c.mTreeDepth==2) // allow them to be nested 1 deep, but only if parent contour isn't a part!
		{
			auto p = cs.getParent(c);
			assert(p);
			depthOK = !shouldContourBeAPart(*p,cs);
		}
		else depthOK=false;
		
		return depthOK && c.mRadius < mHolePartMaxContourRadius;
	}
}

AdjSpace
PinballVision::getAdjacentSpace( vec2 loc, const ContourVector& cs ) const
{
	const Contour* leaf = cs.findLeafContourContainingPoint(loc);
	
	return getAdjacentSpace(leaf, loc, cs);
}

AdjSpace
PinballVision::getAdjacentSpace( const Contour* leaf, vec2 loc, const ContourVector& cs ) const
{
	AdjSpace result;

	if (leaf)
	{
		float far = leaf->mRadius * 2.f;
		
		auto calcSize = [&]( vec2 vec, float& result )
		{
			float t;
			if ( leaf->rayIntersection(loc + vec * far, -vec, &t) ) {
				result = far - t;
			}
		};
		
		calcSize( mWorld.getLeftVec(), result.mWidthLeft );
		calcSize( mWorld.getRightVec(), result.mWidthRight );
		calcSize( mWorld.getUpVec(), result.mWidthUp );
		calcSize( mWorld.getDownVec(), result.mWidthDown );
	}
	
	auto filter = [&]( const Contour& c ) -> bool
	{
		// 1. not self
		//if ( c.mIsHole && c.contains(loc) ) return false;
		if ( &c == leaf ) return false; // supposed to be optimized version of c.mIsHole && c.contains(loc)
		// 2. not other parts
		else if ( shouldContourBeAPart(c,cs) ) return false; // could be a part
		// OK
		else return true;
	};
	
	cs.rayIntersection( loc, mWorld.getRightVec(), &result.mRight, filter );
	cs.rayIntersection( loc, mWorld.getLeftVec (), &result.mLeft , filter );
	cs.rayIntersection( loc, mWorld.getUpVec(),    &result.mUp , filter );
	cs.rayIntersection( loc, mWorld.getDownVec(),  &result.mDown , filter );
	
	// bake in contour width into adjacent space calc
	result.mLeft  -= result.mWidthLeft;
	result.mRight -= result.mWidthRight;
	result.mUp    -= result.mWidthUp;
	result.mDown  -= result.mWidthDown;
	
	return result;
}

PartVec PinballVision::getPartsFromContours( const ContourVector& contours )
{
	PartVec parts;
	
	for( const auto &c : contours )
	{
		if ( !shouldContourBeAPart(c,contours) ) continue;


		AdjSpace adjSpace = getAdjacentSpace(&c,c.mCenter,contours);

		auto add = [&c,&parts,adjSpace]( Part* p )
		{
			p->mContourLoc = c.mCenter;
			p->mContourRadius = c.mRadius;
			parts.push_back( PartRef(p) );
		};
		

		if ( c.mIsHole )
		{
			// flipper orientation
			const bool closeToRight  = adjSpace.mRight < mFlipperDistToEdge;
			const bool closeToLeft   = adjSpace.mLeft  < mFlipperDistToEdge;
//			const bool closeToBottom = adjSpace.mDown  < mFlipperDistToEdge;
			
			if ( closeToRight && !closeToLeft )
			{
				add( new Flipper(mWorld, c.mCenter, c.mRadius, PartType::FlipperRight) );
			}
			else if ( closeToLeft && !closeToRight )
			{
				add( new Flipper(mWorld, c.mCenter, c.mRadius, PartType::FlipperLeft) );
			}
//			else if ( closeToLeft && closeToRight && closeToBottom )
//			{
//				 add( new Plunger( *this, c.mCenter, c.mRadius ) );
//			}
			else
			{
				add( new Bumper( mWorld, c.mCenter, c.mRadius, adjSpace ) );
			}
		} // hole
		else
		{
			// non-hole:
			
			// make target
			auto filter = [this,contours]( const Contour& c ) -> bool {
				return !shouldContourBeAPart(c,contours);
			};
			
			float dist;
			vec2 closestPt;
			
			
			
			vec2 lightLoc = c.mCenter;
			float r = mWorld.mPartParams.mTargetRadius;
			
			const float kMaxDist = mWorld.mPartParams.mTargetDynamicRadius*4.f;
			
			if ( contours.findClosestContour(c.mCenter,&closestPt,&dist,filter)
			  && closestPt != c.mCenter
			  && dist < kMaxDist )
			{
				vec2 dir = normalize( closestPt - c.mCenter );

				float far=0.f;
				
				if (mWorld.mPartParams.mTargetDynamicRadius)
				{
					float r = distance(closestPt,c.mCenter) - c.mRadius;
					
					r = constrain(r, mWorld.mPartParams.mTargetRadius, mWorld.mPartParams.mTargetRadius*4.f );
					r = mWorld.mPartParams.mTargetRadius;
					
					far = r;
				}
				
				lightLoc = closestPt + dir * (r+far);
				
				vec2 triggerLoc = closestPt;
				vec2 triggerVec = dir;
				
				auto rt = new Target(mWorld,triggerLoc,triggerVec,lightLoc,r);
				rt->mContourPoly = c.mPolyLine;
				
				add( rt );
			}
		}
		
	} // for
	
	return parts;
}

PartVec
PinballVision::mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const
{
	PartVec parts = newParts;
	
	// match old parts to new ones
	for( PartRef &p : parts )
	{
		// does it match an old part?
		bool wasMatched = false;
		
		for( const PartRef& old : oldParts )
		{
			if ( !old->getShouldAlwaysPersist() && p->getShouldMergeWithOldPart(old) )
			{
				// matched.
				bool replace=true;
				
				// but...
				// replace=false
				
				// replace with old.
				// (we are simply shifting pointers rather than copying contents, but i think this is fine)
				if (replace) {
					p = old;
					wasMatched = true;
				}
			}
		}
		
		//
		if (!wasMatched) p->onGameEvent(GameEvent::NewPart);
	}
	
	// carry forward any old parts that should be
	for( const PartRef &p : oldParts )
	{
		if (p->getShouldAlwaysPersist())
		{
			bool doIt=true;
			
			// are we an expired trigger?
			/*
			if ( p->getType()==PartType::Target )
			{
				auto rt = dynamic_cast<Target*>(p.get());
				if ( rt && !isValidRolloverLoc( rt->mLoc, rt->mRadius, oldParts ) )
				{
//					doIt=false;
				}
			}*/
			// this whole concept of procedurally generating and retiring them is deprecated
			
			if (doIt) parts.push_back(p);
		}
	}
	
	return parts;
}

} // ns Pinball