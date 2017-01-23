//
//  PinballVision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/8/17.
//
//

#include "PinballVision.h"
#include "PinballWorld.h"
#include "geom.h"

namespace Pinball
{

void PinballVision::setParams( XmlTree xml )
{
	getXml(xml, "FlipperDistToEdge", mFlipperDistToEdge );	
	getXml(xml, "PartMinContourRadius", mPartMinContourRadius );
	getXml(xml, "PartMaxContourRadius", mPartMaxContourRadius );
	getXml(xml, "PartMaxContourAspectRatio", mPartMaxContourAspectRatio );
	
	getXml(xml, "HolePartMaxContourRadius", mHolePartMaxContourRadius);
	getXml(xml, "HolePartMinContourRadius", mHolePartMinContourRadius);
	
	getXml(xml, "PartTrackLocMaxDist", mPartTrackLocMaxDist );
	getXml(xml, "PartTrackRadiusMaxDist", mPartTrackRadiusMaxDist );
	getXml(xml, "DejitterContourMaxDist", mDejitterContourMaxDist );
	getXml(xml, "DejitterContourLerpFrac", mDejitterContourLerpFrac );

	getXml(xml, "EnableUI", mEnableUI );
	
	if ( xml.hasChild("RectFinder") ) {
		mRectFinder.mParams.set( xml.getChild("RectFinder") );
	}
}

ContourVec PinballVision::dejitterContours( ContourVec in, ContourVec old ) const
{
	ContourVec out = in;

	// Limitation of this function is that it doesn't fix up area, bounding rect, rotated bounding rect, etc...
	// But for our purposes, this should be fine.
	// There might be some weird edge cases where BallWorld uses bounding box for optimization and we should update that...
	
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
				float dist;
				
				vec2 x = closestPointOnPoly( p, match->mPolyLine, 0, 0, &dist );
				
				if ( dist < mDejitterContourMaxDist ) p = lerp( p, x, mDejitterContourLerpFrac ) ;
			}
			
			// patch up some params, this one might be important to BallWorld's heuristics
			c.mBoundingRect = Rectf( c.mPolyLine.getPoints() );
		}
	}
	
	return out;
}

PinballVision::Output
PinballVision::update(
	const Vision::Output& visionOut,
	Pipeline& p,
	Output& lastOutput )
{
	Output out;
	
	// dejitter (optional)
	if ( mDejitterContourMaxDist > 0.f && !lastOutput.mContours.empty() ) {
		out.mContours = dejitterContours( visionOut.mContours, lastOutput.mContours );
	} else {
		out.mContours = visionOut.mContours;
	}
	
	// classify contours
	out.mContourTypes = classifyContours(out.mContours);
	
	// generate parts
	PartVec newParts = getPartsFromContours(out.mContours,out.mContourTypes);
	out.mParts = mergeOldAndNewParts( lastOutput.mParts, newParts );
	
	// get ui components
	UIBoxes uiboxes = getUIBoxesFromContours(out.mContours,out.mContourTypes);
	out.mUI = uiboxes;
	
	return out;
}

PinballVision::ContourTypes
PinballVision::classifyContours( const ContourVec& cs ) const
{
	ContourTypes ct;
	
	ct.resize(cs.size());
	
	for( int i=0; i<ct.size(); ++i )
	{
		ContourType t;

		if      ( shouldContourBeAPart (cs[i],cs) ) t = ContourType::Part;
		else if ( shouldContourBeAUI   (cs[i])    ) t = ContourType::UI;
		else if ( shouldContourBeASpace(cs[i])    ) t = ContourType::Space;
		else t = ContourType::Ignore;
		
		ct[i] = t;
	}
	
	return ct;
}

bool PinballVision::shouldContourBeAUI( const Contour& c, UIBox* out ) const
{
	// enabled?
	if ( !mEnableUI ) return false;
	
	// root
	if ( c.mTreeDepth > 0 ) return false;
	
	// no children
	if ( c.mChild.size() > 0 ) return false; 
	
	// 
	if ( c.mIsHole ) return false;

	// is rectangle
	PolyLine2 quadpoly;
	if ( !mRectFinder.getRectFromPoly(c.mPolyLine,quadpoly) ) return false;
	
	// check rectangle
	UIBox box;
	box.mQuad.getPoints().resize(4);
	vec2 *quad = &box.mQuad.getPoints()[0];
	if ( !getOrientedQuadFromPolyLine( quadpoly, mWorld.getRightVec(), quad ) ) return false; 	
	
	// not too tilted
	if ( dot( normalize(quad[2]-quad[3]), mWorld.getRightVec() ) < .0f ) return false;
	
	// good aspect ratio
	box.mSize = vec2( distance(quad[3],quad[2]), distance(quad[0],quad[3]) );
	if ( box.mSize.y > box.mSize.x * 2.f ) return false;
	
	// size big enough
	//if ( size.y > mWorld. )
	
	// not bigger than X% of world
	
	// above or below major components 
	// (allow anywhere?)
	
	// OK!
	if (out)
	{
		box.mXAxis = normalize( quad[2] - quad[3] );
		box.mYAxis = normalize( quad[0] - quad[3] );
		*out = box;
	}

	return true;
}

bool PinballVision::shouldContourBeASpace( const Contour& c ) const
{
	const float ballRadius = mWorld.getBallRadius();

	// holes are always OK (small walls are fine!)
	if ( c.mIsHole ) return true;
	
	// minimum dimension
	{
		const float minDim = min(c.mRotatedBoundingRect.mSize.x,c.mRotatedBoundingRect.mSize.y);
		
		if ( minDim < ballRadius*2.f ) return false;
	}
	
	// area to perimeter ratio
	// (not sure this is ever false)
	// ...there is probably a better way to do this heuristic. AND we should account for area of holes inside
	// (not sure how often that ever really happens though).
	if (0)
 	{
		const float ballArea = M_PI * ballRadius * ballRadius;
		const float ballPerim = 2.f * M_PI * ballRadius;
		const float maxq = ballPerim / ballArea;
		// TODO: precompute this! (but who cares?)
		// And do we even need ballRadius in here?

		const float q = c.mPerimeter / c.mArea;
		
		if ( q > maxq ) return false;
	}
	
	// perimeter test #2
	{
		// If we take c's area, and imagine it as a rectangle that
		// tightly holds a set of boxes...
		
		/* +------+
		   |( )( )|
		   +------+
		*/

		// What size box is needed to hold these balls?
		// What is the perimeter of that box? If our perimeter is greater,
		// than this should, in theory, be too small of a space to hold the balls. 
		
		const float netArea = c.mArea; // TODO: use actual net area
		const float est_height = (ballRadius*2.f);   
		const float est_width  = netArea / est_height;
		const float est_perim  = (est_height + est_width) * 2.f;
		
		if ( c.mPerimeter > est_perim ) return false;
	}
	
	// TODO: try some random sampling inside, see if ball will even fit!
	// In effect, we would like to run the simulation AS IF a ball is inside,
	// and see if it freaks out. If it freaks out, then we want to reject the shape.
	// Maybe OpenCV has been contour analytical techniques for us...
	
	return true;
}

bool PinballVision::shouldContourBeAPart( const Contour& c, const ContourVec& cs ) const
{
	// no children?
//	if (!c.mChild.empty()) return false; // seems logical; or does it matter? is this just annoying?
	// TBD design question. We should try this and see if it's problematic.
	
	// round enough?
	{
		float lo = c.mRotatedBoundingRect.mSize.x;
		float hi = c.mRotatedBoundingRect.mSize.y;
		
		if (hi<lo) swap(lo,hi);
		
		// not round enough
		if (hi>lo*mPartMaxContourAspectRatio) return false;
	}
	
	if ( c.mIsHole ) {
		return c.mTreeDepth>0 && c.mRadius > mPartMinContourRadius && c.mRadius < mPartMaxContourRadius;
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
		
		return depthOK && c.mRadius > mHolePartMinContourRadius && c.mRadius < mHolePartMaxContourRadius;
	}
}

AdjSpace
PinballVision::getAdjacentSpace( const Contour* leaf, vec2 loc, const ContourVector& cs, const ContourTypes& ctypes ) const
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
		return ctypes[c.mIndex]==ContourType::Space;
		
		/*
		// 1. not self
		//if ( c.mIsHole && c.contains(loc) ) return false;
		if ( &c == leaf ) return false; // supposed to be optimized version of c.mIsHole && c.contains(loc)
		// 2. not other parts
		else if ( ctypes[c.mIndex]==ContourType::Part ) return false; // could be a part
		// OK
		else return true;
		*/
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

PinballVision::UIBoxes
PinballVision::getUIBoxesFromContours( const ContourVec& cs, const ContourTypes& ct ) const
{
	UIBoxes ui;
	
	for( int i=0; i<ct.size(); ++i )
	{
		if ( ct[i] == ContourType::UI )
		{
			UIBox e;
			if ( shouldContourBeAUI(cs[i],&e) ) // this test is redundant, but, whatever...
			{
				ui[i] = e ;
			}
		}
	}
	
	return ui;
}

PartVec PinballVision::getPartsFromContours( const ContourVector& contours, const ContourTypes& ctypes ) const
{
	PartVec parts;
	
	for( const auto &c : contours )
	{
		if ( ctypes[c.mIndex] != ContourType::Part ) continue;

		AdjSpace adjSpace = getAdjacentSpace(&c,c.mCenter,contours,ctypes);

		auto add = [&c,&parts,adjSpace]( Part* p )
		{
			p->setSourceContour( c.mCenter, c.mRadius );
			p->setAdjSpace( adjSpace );
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
			auto filter = [this,contours,ctypes]( const Contour& c ) -> bool {
				return ctypes[c.mIndex] == ContourType::Space;
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

				if (mWorld.mPartParams.mTargetDynamicRadius)
				{
					float r = distance(closestPt,c.mCenter) - c.mRadius;
					
					r = constrain(r, mWorld.mPartParams.mTargetRadius, mWorld.mPartParams.mTargetRadius*4.f );
				}
				else
				{
					r = mWorld.mPartParams.mTargetRadius;
				}
				
				vec2 triggerLoc = closestPt;
				vec2 triggerVec = dir;
				
				auto rt = new Target(mWorld,triggerLoc,triggerVec,r);
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