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

	
bool theorizeQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& inOutBestScore, float angleDev )
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
		
		PolyLine2 theory;
		theory.push_back(v[i]);
		theory.push_back(v[j]);
		theory.push_back(v[k]);
		theory.push_back(o);
		theory.setClosed();
		
		// score heuristics
		float score = getPolyDiffArea(p,theory);
			// this heuristic isn't appropriate for this quad generator.
			// TODO:
			// what we really need to do is look at degree of perimeter overlap--what % of the
			// theorized quad perimeter isn't on the perimeter of the input poly. 
		 
		if ( score < inOutBestScore )
		{
			inOutBestScore = score;
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
	float inOutBestScore = in.calcArea() * mMaxGainAreaFrac;
		
	for( int i=0; i<in.size(); ++i )
	{
		theorizeQuadFromEdge(in,i,out,inOutBestScore,mInteriorAngleMaxDelta);
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

		getPolyDiffArea(frame.mConvexHull,frame.mContourPoly,&frame.mReducedDiff);
		// use convex hull, since reduced may be present for invalid frames
		
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
			gl::color(1,.5,0);
			gl::draw(f.mConvexHull);
			
//			if ( !f.mIsValid && f.mConvexHull.size()>0 )
			if (0)
			{
				PaperBounce3App::get()->mTextureFont->drawString(
					toString(f.mConvexHull.size()) + " " + toString(f.mContourPoly.calcArea()),
					f.mConvexHull.getPoints()[0] + vec2(0,-1),
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
