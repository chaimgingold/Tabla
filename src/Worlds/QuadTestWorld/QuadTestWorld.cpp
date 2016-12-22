//
//  QuadTestWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "PaperBounce3App.h" // for text
#include "QuadTestWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

#include "cinder/ConvexHull.h"

using namespace std;

PolyLine2 QuadTestWorld::Frame::getQuadAsPoly() const
{
	PolyLine2 p;
	
	for( int i=0; i<4; ++i ) p.push_back(mQuad[i]);
	
	p.setClosed();
	return p;
}

QuadTestWorld::QuadTestWorld()
{
}

void QuadTestWorld::setParams( XmlTree xml )
{
	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"MaxGainAreaFrac",mMaxGainAreaFrac);
	getXml(xml,"InteriorAngleMaxDelta",mInteriorAngleMaxDelta);
	getXml(xml,"TheorizedRectMinPerimOverlapFrac",mTheorizedRectMinPerimOverlapFrac);
}

void QuadTestWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	const Pipeline::StageRef source = pipeline.getStage("clipped");
	if ( !source || source->mImageCV.empty() ) return;

	mFrames = getFrames( source, visionOut.mContours, pipeline );
}

PolyLine2 getConvexHull( PolyLine2 in )
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

float getPolyDiffArea( PolyLine2 a, PolyLine2 b, std::vector<PolyLine2>* diff=0 )
{
	std::vector<PolyLine2> d1,d2;
	d1.push_back(a);
	d2.push_back(b);
	
	try
	{
		auto dout = PolyLine2::calcXor(d1,d2);
		if (diff) *diff = dout;

		float area=0.f;
		
		for( const auto &p : dout )
		{
			area += p.calcArea();
		}
		
		return area;

	} catch(...) { return MAXFLOAT; } // in case boost goes bananas on us
	// i suppose that's really us going bananas on it. :-)
}

float QuadTestWorld::getFracOfEdgeCoveredByPoly( vec2 a, vec2 b, const PolyLine2& p, float& out_abLen ) const
{
	const float kDistanceAttentuate = 1.f;
	
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
		const float distance_score = 1.f - min(1.f, distance / kDistanceAttentuate);
		
		// 
		const float score = angle_overlap * distance_score ;
		edgefrac += overlap * score;
	}
	
	// finish
	out_abLen = ablen;
	return min( edgefrac, ablen ); // don't let us exceed the length of this edge
}

float QuadTestWorld::calcPolyEdgeOverlapFrac( const PolyLine2& a, const PolyLine2& b ) const
{
	// what frac-of-A is covered-by-B ?
	
	const int an = a.isClosed() ? a.size() : a.size()-1; 
	
	float totalfrac=0.f;
	float totallen =0.f;
	
	for( int i=0; i<an; ++i )
	{
		int j = (i+1) % a.size();
		
		float len;
		float frac = getFracOfEdgeCoveredByPoly( a.getPoints()[i], a.getPoints()[j], b, len );
		
		// integrate
//		totalfrac = (totalfrac * totallen + edgefrac * pji_len) / (totallen + pji_len);
		totalfrac += frac;
		totallen  += len;
	}
	
	return totalfrac / totallen;
}

static float getInteriorAngle( const PolyLine2& p, int index )
{
	int i = index==0 ? (p.size()-1) : (index-1);
	int j = index;
	int k = (index+1) % p.size();
	
	vec2 a = p.getPoints()[i];
	vec2 x = p.getPoints()[j];
	vec2 b = p.getPoints()[k];

	a -= x;
	b -= x;

	return acos( dot(a,b) / (length(a)*length(b)) );
}

bool QuadTestWorld::areInteriorAnglesOK( const PolyLine2& p ) const
{
	for( int i=0; i<4; ++i )
	{
		if ( fabs(getInteriorAngle(p,i) - M_PI/2.f) > toRadians(mInteriorAngleMaxDelta) ) return false;
	}
	return true;
}

bool QuadTestWorld::checkIsQuadReasonable( const PolyLine2& quad, const PolyLine2& source ) const
{
	// not quad
	if (quad.size()!=4) return false;
	
	// max angle
	if ( !areInteriorAnglesOK(quad) ) return false;
	 
	// area changed too much
	if ( getPolyDiffArea(quad,source) > source.calcArea() * mMaxGainAreaFrac )
	{
		return false;
	}
	
	return true;
}

	
bool QuadTestWorld::theorizeQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const
{
	/*  o  k
		   |
		i--j
	*/
	
	int j = (i+1) % p.size();
	int k = (i+2) % p.size();
	
	const vec2 *v = &p.getPoints()[0]; 
	
	if ( (fabs(getInteriorAngle(p,j) - M_PI/2.f)) < angleDev )
	{
		vec2 jk = v[k] - v[j];
		vec2 ji = v[i] - v[j];		
		
		vec2 o = lerp( v[i] + jk, v[k] + ji, .5f );
		// split the difference between two projected fourth coordinate locations
		
		// filter based on size!
		const vec2 size( length(jk), length(ji) );
		const float area = size.x * size.y;
		{
			const float mindim = min(size.x,size.y);
			
			float thresh = min(
				getVisionParams().mContourVisionParams.mContourMinWidth,
				getVisionParams().mContourVisionParams.mContourMinRadius*2.f );
				
			if ( mindim < thresh ) return false;
			if ( area   < getVisionParams().mContourVisionParams.mContourMinArea ) return false;
		}
		
		PolyLine2 theory;
		theory.push_back(v[i]);
		theory.push_back(v[j]);
		theory.push_back(v[k]);
		theory.push_back(o);
		theory.setClosed();
		
		// score heuristics
		const float score = calcPolyEdgeOverlapFrac(theory,p);
		// TODO: consider relative area in relative ranking... we also want the biggest theorized quad!
		// the size filtering is giving us some of that, but it isn't principled enough...
		// trying this below: just using score as a minimum bar to cross, then comparing areas.
		// AND if we like this, we could move this filter into whomever calls theorize.
		// leaving it here leaves us flexible, though.
		
//		if ( score > ioBestScore )
		if ( score > ioBestScore && area > ioBestArea )
		{
//			ioBestScore = score; // just use input score as a floor
			ioBestArea  = area;
			result = theory;
			return true;
		}
		else
		{
			return false;				
		}			
	}
	
	return false;
};


bool QuadTestWorld::theorize( const PolyLine2& in, PolyLine2& out ) const
{
	float ioBestScore = mTheorizedRectMinPerimOverlapFrac;
	float ioBestArea = 0.f;
	
	for( int i=0; i<in.size(); ++i )
	{
		theorizeQuadFromEdge(in,i,out,ioBestScore,ioBestArea,mInteriorAngleMaxDelta);
	}
	
	return out.size()>0;
}

bool QuadTestWorld::getQuadFromPoly( const PolyLine2& in, PolyLine2& out ) const
{
	// was it good to begin with?
	if ( in.size()==4 )
	{
		if ( areInteriorAnglesOK(in) )
		{
			out = in;
			return true;
		}
		else return false;
	}

	// convex hull strategy
	{
		PolyLine2 convexHull = getConvexHull(in);
		
		if ( checkIsQuadReasonable(convexHull,in) )
		{
			out=convexHull;
			return true;
		}
	}
	
	// theorize quads strategy
	{
		if ( theorize(in,out) ) return true;
	}
	
	// fail
	return false;
}

QuadTestWorld::FrameVec QuadTestWorld::getFrames(
	const Pipeline::StageRef world,
	const ContourVector &contours,
	Pipeline& pipeline ) const
{
	FrameVec frames;
	if ( !world || world->mImageCV.empty() ) return frames;

	for ( auto c : contours )
	{
		if ( c.mIsHole || c.mTreeDepth>0 ) continue;

		// do it
		Frame frame;
		frame.mContourPoly = c.mPolyLine;
		frame.mIsValid = getQuadFromPoly(frame.mContourPoly,frame.mContourPolyReduced);
		frame.mConvexHull = getConvexHull(frame.mContourPoly);
		
		PolyLine2* diffPoly=0;
		
		if ( frame.mContourPolyReduced.size() > 0 ) {
			diffPoly = &frame.mContourPolyReduced; 
		} else {
			diffPoly = &frame.mConvexHull;
		}

		frame.mOverlapScore = calcPolyEdgeOverlapFrac(*diffPoly,frame.mContourPoly);
				
		getPolyDiffArea(*diffPoly,frame.mContourPoly,&frame.mReducedDiff);
		
		if (frame.mIsValid) {
			getOrientedQuadFromPolyLine(frame.mContourPolyReduced, mTimeVec, frame.mQuad);
		}
		
		frames.push_back(frame);
	}
	return frames;	
}

void QuadTestWorld::update()
{
}

void QuadTestWorld::draw( DrawType drawType )
{
	for( const auto &f : mFrames )
	{
		// fill
		if (1)
		{
			gl::color(0,1,1,.5);
			gl::drawSolid( f.getQuadAsPoly() );
		}
		
		// diff
		if (1)
		{
			gl::color(1, 0, 0, .6f);
			for( const auto &p : f.mReducedDiff )
			{
				gl::drawSolid(p);
			}
		}
		
		// frame
		if (1)
		{
			gl::color(1,0,0);
			gl::draw( f.mContourPolyReduced.size()==0 ? f.mConvexHull : f.mContourPolyReduced );
			
//			if ( !f.mIsValid && f.mConvexHull.size()>0 )
			if (1)
			{
				PaperBounce3App::get()->mTextureFont->drawString(
				//	toString(f.mConvexHull.size()) + " " + toString(f.mContourPoly.calcArea()),	
					toString(floorf(f.mOverlapScore*100.f)) + "%",
					//f.mConvexHull.getPoints()[0] + vec2(0,-1),
					f.mContourPoly.calcCentroid(),
					gl::TextureFont::DrawOptions().scale(1.f/8.f).pixelSnap(false)
					);
			}
		}
		
		// frame corners 
		if (f.mIsValid)
		{
			Color c[4] = { Color(1,0,0), Color(0,1,0), Color(0,0,1), Color(1,1,1) };
			
			for( int i=0; i<4; ++i )
			{
				gl::color(c[i]);
				gl::drawSolidCircle(f.mQuad[i],1.f);
			}
		}
	}
}

void QuadTestWorld::drawMouseDebugInfo( vec2 v )
{
//	cout << v << " => " << mGlobalToLocal * v << endl;
}
