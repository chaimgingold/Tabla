//
//  RectFinder.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/22/16.
//
//

#include "RectFinder.h"
#include "geom.h"
#include "ocv.h"
#include "xml.h"

void RectFinder::Params::set( XmlTree xml )
{
	getXml(xml,"AllowSubset",mAllowSubset);
	getXml(xml,"AllowSuperset",mAllowSuperset);
	getXml(xml,"AllowFragment",mAllowFragment);
	
	mApproxPolyDP=0.f;
	getXml(xml,"ApproxPolyDP",mApproxPolyDP);
	
	if ( getXml(xml,"InteriorAngleMaxDelta",mInteriorAngleMaxDelta) ) {
		mInteriorAngleMaxDelta = toRadians(mInteriorAngleMaxDelta);
		
		if (0)
		{
			cout << "mInteriorAngleMaxDelta: " << endl
				<< "\t" << mInteriorAngleMaxDelta << endl
				<< "\tcos = " << cos(mInteriorAngleMaxDelta) << endl
				<< "\tacos = " << acos(mInteriorAngleMaxDelta) << endl
				;
		}
	}	

	getXml(xml,"MaxGainAreaFrac",mMaxGainAreaFrac);
	getXml(xml,"SubsetRectMinPerimOverlapFrac",mSubsetRectMinPerimOverlapFrac);
	getXml(xml,"EdgeOverlapDistAttenuate",mEdgeOverlapDistAttenuate);
	getXml(xml,"FragmentParallelLinesMaxLengthRatio", mFragmentParallelLinesMaxLengthRatio );

	getXml(xml,"MinRectWidth",mMinRectWidth);
	getXml(xml,"MinRectArea",mMinRectArea);
}

bool RectFinder::getRectFromPoly( const PolyLine2& in_raw, PolyLine2& out, CandidateVec* oCandidates ) const
{
	// approx
	PolyLine2 in;
	if ( mParams.mApproxPolyDP > 0.f ) in = approxPolyDP(in_raw, mParams.mApproxPolyDP );
	else in = in_raw;

	// already ok?
	if ( getRectFromPoly_Intrinsic(in_raw, out, oCandidates) ) return true;
	
	// convex hull strategy
	if ( mParams.mAllowSuperset )
	{
		if ( getRectFromPoly_Superset(in, out,oCandidates) ) return true;
	}
	
	// theorize quads strategy
	if ( mParams.mAllowSubset )
	{
		if ( getRectFromPoly_Subset(in, out,oCandidates) ) return true;
	}
	
	// fragment
	if ( mParams.mAllowFragment )
	{
		if ( getRectFromPoly_Fragment( in, out, oCandidates ) ) {
			return true;
		}
	}
	
	// fail
	return false;		
}

bool RectFinder::getRectFromPoly_Intrinsic( const PolyLine2& poly, PolyLine2& rect, CandidateVec* oCandidates ) const
{
	// intrinsically ok?
	if ( isOK(poly) )
	{
		Candidate c;
		
		for( int i=0; i<4; ++i ) c.mV[i] = poly.getPoints()[i];
		
		c.mStrategy = Candidate::Strategy::Intrinsic;
		c.mArea = c.getAsPoly().calcArea();
		c.mSourcePolyArea = poly.calcArea();
		c.mDiffArea=0.f;
		c.mPerimScore=1.f;
		
		rect = poly;
		
		if (oCandidates) oCandidates->push_back(c);
		return true;
	}	
	
	return false;
}

bool RectFinder::getRectFromPoly_Subset  ( const PolyLine2& in, PolyLine2& out, CandidateVec* candidates ) const
{
	// TODO: add things we try to candidates
	if ( trySubset(in,out) ) return true;
	else return false;
}

bool RectFinder::getRectFromPoly_Superset( const PolyLine2& in, PolyLine2& out, CandidateVec* candidates ) const
{
	PolyLine2 convexHull = getConvexHull(in);
	
	if (candidates)
	{
		// TODO: show all the diffs, other params...
		Candidate c;
		c.mStrategy = Candidate::Strategy::Superset;
		for( int i=0; i<4; ++i ) c.mV[i] = in.getPoints()[i];
		
		candidates->push_back(c);
	}
	
	if ( checkIsConvexHullReasonable(convexHull,in) )
	{
		out=convexHull;
		return true;
	}
	else return false;
}
	
bool RectFinder::getRectFromPoly_Fragment( const PolyLine2& poly, PolyLine2& rect, CandidateVec* oCandidates ) const
{
	CandidateVec cand;

	const float sourcePolyArea = poly.calcArea();
	
	{
		cand = getFragmentCandidates(poly);
		
		// score them
		for( auto &c : cand )
		{
			PolyLine2 quadPoly = c.getAsPoly();

			std::vector<PolyLine2>* keepPolyDiff = oCandidates ? &c.mPolyDiff : 0;
			
			c.mArea = quadPoly.calcArea();
			c.mSourcePolyArea = sourcePolyArea;
			c.mDiffArea = getPolyDiffArea(quadPoly,poly,keepPolyDiff);
			c.mPerimScore = calcPolyEdgeOverlapFrac(quadPoly,poly,mParams.mEdgeOverlapDistAttenuate);
		}
	}

	// calc sizes + filter
	for( auto &c : cand )
	{
		c.mStrategy = Candidate::Strategy::Fragment;
		
		c.mSize = vec2(
			distance(c.mV[0], c.mV[1]),
			distance(c.mV[1], c.mV[2])
		); // assuming square-ish
		
		c.mAllowed =
			 c.mPerimScore > mParams.mSubsetRectMinPerimOverlapFrac
		  && c.mArea > mParams.mMinRectArea
		  && min(c.mSize.x,c.mSize.y) > mParams.mMinRectWidth
		  && c.mDiffArea < c.mSourcePolyArea * mParams.mMaxGainAreaFrac
		  ;
	}
	
	// pick a winner?
	int winner=-1;
	float bestScore=0.f;
	for( int i=0; i<cand.size(); ++i )
	{
		auto &c = cand[i];
		
		if ( c.mAllowed ) c.mScore = c.mArea;
		else c.mScore=0.f;
		
		if (c.mScore > bestScore)
		{
			bestScore = c.mScore;
			winner=i;
		}
	}
	
	// 

	// return internal scores?
	if (oCandidates) {
		oCandidates->insert(oCandidates->begin(), cand.begin(), cand.end());
	}
	
	// return result
	if (winner==-1)
	{
		return false;
	}
	else
	{
		rect = cand[winner].getAsPoly();
		return true;
	}
}

bool RectFinder::isOK( const PolyLine2& p ) const
{
	if ( p.size()==4 )
	{
		// size
		if ( !isSizeOK(p) ) return false;
		
		// angles
		if ( !areInteriorAnglesOK(p) ) return false;
		
		// OK
		return true;
	}
	else return false;	
}

float RectFinder::getPolyDiffArea( PolyLine2 a, PolyLine2 b, std::vector<PolyLine2>* diff )
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

bool RectFinder::areInteriorAnglesOK( const PolyLine2& p ) const
{
	for( int i=0; i<4; ++i )
	{
		if ( fabs(getInteriorAngle(p,i) - M_PI/2.f) > mParams.mInteriorAngleMaxDelta ) return false;
	}
	return true;
}

bool RectFinder::isSizeOK( const PolyLine2& p ) const
{
	// filter against size
	vec2 size(
		distance(p.getPoints()[0],p.getPoints()[1]),
		distance(p.getPoints()[1],p.getPoints()[2])
	 );
	 
	if ( min(size.x,size.y) < mParams.mMinRectWidth ) return false;
	if ( size.x * size.y    < mParams.mMinRectArea  ) return false;
	
	return true;
}

bool RectFinder::checkIsConvexHullReasonable( const PolyLine2& quad, const PolyLine2& source ) const
{
	// not quad
	if (quad.size()!=4) return false;
	
	// size
	if ( !isSizeOK(quad) ) return false;
	
	// max angle
	if ( !areInteriorAnglesOK(quad) ) return false;
	 
	// area changed too much
	if ( getPolyDiffArea(quad,source) > source.calcArea() * mParams.mMaxGainAreaFrac )
	{
		return false;
	}
	
	return true;
}

	
bool RectFinder::trySubsetQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const
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
				
			if ( mindim < mParams.mMinRectWidth ) return false;
			if ( area   < mParams.mMinRectArea  ) return false;
		}
		
		PolyLine2 theory;
		theory.push_back(v[i]);
		theory.push_back(v[j]);
		theory.push_back(v[k]);
		theory.push_back(o);
		theory.setClosed();
		
		// score heuristics
		const float score = calcPolyEdgeOverlapFrac(theory,p,mParams.mEdgeOverlapDistAttenuate);
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


bool RectFinder::trySubset( const PolyLine2& in, PolyLine2& out ) const
{
	float ioBestScore = mParams.mSubsetRectMinPerimOverlapFrac;
	float ioBestArea = 0.f;
	
	for( int i=0; i<in.size(); ++i )
	{
		trySubsetQuadFromEdge(in,i,out,ioBestScore,ioBestArea,mParams.mInteriorAngleMaxDelta);
	}
	
	return out.size()>0;
}

RectFinder::CandidateVec
RectFinder::getFragmentCandidates( const PolyLine2& poly ) const
{
	// TODO: Optimize by eliminating this case:
	//
	// i  j
	// +  +
	// |  | 
	// +  + 
	//         +--+ k
	//       +--+ l
	// for a first edge i we need to find the combined range along i for i and j
	// then we need to project k,l onto that and ensure they are within that range.
	// easy. 
	// 
	// auto isBracketed = []() {...}
	
//	const float kDotEps = 1.f - cos(mParams.mInteriorAngleMaxDelta);
	// is this right??? 
	const float kDotEps = .025f;


	const vec2* pts = &poly.getPoints()[0];
	const int   n   = poly.size();

	CandidateVec result;
	
	vector<vec2> norms(n);
	vector<float> lengths(n);
	for( int i=0; i<n; ++i )
	{
		const int j = (i+1) % n;
		lengths[i] = distance(pts[i],pts[j]);
		norms[i] = normalize( pts[j] - pts[i] ); 
	}

	set<int> uniqueIndices;
	
	auto isUnique = [&]( int i, int j, int k, int l ) -> bool {
		// this effectively cuts candidates in half,
		// eliminating the same box, but rotated 90-degrees
		// (note: i,j and k,l should always be in the same order, so really
		// it is redundant (ij, kl) and (kl, ij) orderings we need to eliminate--
		// the below is a bit overkill)   
		vector<int> v;
		v.push_back(i);
		v.push_back(j);
		v.push_back(k);
		v.push_back(l);
		sort(v.begin(),v.end());
		
		int index = v[0] + v[1]*n + v[2]*n*n + v[3]*n*n*n;
		
		 if (uniqueIndices.find(index)==uniqueIndices.end())
		 {
			uniqueIndices.insert(index);
			return true;
		 }
		 else return false;
	};

	auto isParallel = [kDotEps]( vec2 n1, vec2 n2 ) -> bool {
		return fabs( dot(n1,n2) ) > 1.f - kDotEps;
	};
	
	auto isOrthogonal = [kDotEps]( vec2 n1, vec2 n2 ) -> bool {
		return fabs( dot(n1,n2) ) < kDotEps;
	};

	auto isValidWidth = [&]( int i, int j ) -> bool
	{
		const vec2 p = perp(norms[i]);
		
		float d = max(
			fabs( dot( pts[j]       - pts[i], p ) ),
			fabs( dot( pts[(j+1)%n] - pts[i], p ) )
			); 
			
		return d >= mParams.mMinRectWidth;
	};
	
	auto isProportional = [&]( int i, int j ) -> bool
	{
		float lo = lengths[i];
		float hi = lengths[j];
		
		if (lo>hi) swap(lo,hi);
		
		return (hi / lo < mParams.mFragmentParallelLinesMaxLengthRatio);
	};

	auto isBracketed = [&]( int i, int j, int c ) -> bool
	{
		vec2 v = -perp( norms[j] );
		
		float x = dot( pts[i]     - pts[j], v );
		// [i,j] interval goes from [x,0]
		
		float w1 = dot( pts[c] - pts[j], v );
		float w2 = dot( pts[(c+1)%n] - pts[j], v );

		if (x<0) {
			// necessary?
			x  *= -1.f;
			w1 *= -1.f;
			w2 *= -1.f;
		}
		
		float eps = 1.f;
		
		return w1 <= x+eps  && w2 <= x+eps
		    && w1 >= -eps   && w2 >= -eps;
	};
	
	auto getCorner = [&]( int i, int j ) -> vec2
	{
		// https://gist.github.com/danieljfarrell/faf7c4cafd683db13cbc
		
		const vec2& rayOrigin = pts[i];
		const vec2& rayDirection = norms[i];
		const vec2& point1 = pts[j];
		const vec2& point2 = pts[(j+1)%n];
		
		vec2 v1 = rayOrigin - point1;
		vec2 v2 = point2 - point1;
		vec2 v3 = vec2(-rayDirection[1], rayDirection[0]) ;
		float t1 = cross(vec3(v2,0), vec3(v1,0)).z / dot(v2, v3);
//		float t2 = dot(v1, v3) / dot(v2, v3);
		
		return rayOrigin + rayDirection * t1 ;
//		if ( t1 >= 0.0 && t2 >= 0.0 && t2 <= 1.f )
//		{
//			if (rayt) *rayt = t1;
//			return true;
//		}
//		return false;
	};	
	
	for( int i=0;   i<n-1; ++i )
	for( int j=i+1; j<n  ; ++j )
	{
		// i,j parallel?
		if ( isParallel(norms[i],norms[j])
		  && isValidWidth(i,j)
		  && isProportional(i,j) )
		{
			for( int k=0; k<n-1; ++k )
			{
				// k orthogonal?
				if ( isOrthogonal(norms[i],norms[k])
				  && isBracketed(i,j,k) )
				{
					for( int l=k+1; l<n; ++l )
					{
						// l orthogonal?
						if ( isParallel(norms[k],norms[l])
						  && isValidWidth(k,l)
						  && isBracketed(i,j,l)
						  && isProportional(k,l)
						  )
						{
							if ( isUnique(i,j,k,l) )
							{
								// reconstruct corners of quad
								Candidate q;
								
								q.mV[0] = getCorner(i,k);
								q.mV[1] = getCorner(k,j);
								q.mV[2] = getCorner(j,l);
								q.mV[3] = getCorner(l,i);

								// use convex hull to ensure proper clockwise ordering
								if (0) // doesn't seem to be necessary...
								{
									PolyLine2 hull = getConvexHull(q.getAsPoly());
									assert(hull.size()==4);
									for( int i=0; i<4; ++i ) q.mV[i]=hull.getPoints()[i];
								}
								
								result.push_back(q);
							}
						}
					} // l
				}
			} // k
		}
	} // i,j
	
	return result;
}