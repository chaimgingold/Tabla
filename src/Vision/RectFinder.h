//
//  RectFinder.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/22/16.
//
//

#ifndef RectFinder_hpp
#define RectFinder_hpp

#include "cinder/Xml.h"
#include "Contour.h"

class RectFinder
{
public:
	
	class Params
	{
	public:
		void set( XmlTree );

		bool mAllowSubset=false;
		bool mAllowSuperset=false;
		bool mAllowFragment=false;
		
		// filters we always apply
		float mApproxPolyDP = 0.f;
		float mInteriorAngleMaxDelta = ci::toRadians(10.f); // how strict are corner angles? (0 = most strict)
		float mMinRectWidth=0.f;
		float mMinRectArea=0.f;
		
		// tuning
		float mMaxGainAreaFrac=.2f; // when doing superset, how much bigger can rect be?
		float mSubsetRectMinPerimOverlapFrac=.8f; // when doing subset, how much do edge perimeter need to overlap?
		float mEdgeOverlapDistAttenuate=1.f; // when doing subset edge perimeter test, how far is far away?
		float mFragmentParallelLinesMaxLengthRatio = 5.f; 
		

	};
	Params mParams;
	
	// new api
	struct Candidate
	{
	public:
		// output quad verts, clockwise order
		vec2 mV[4];
		PolyLine2 getAsPoly() const {
			PolyLine2 p;
			p.push_back(mV[0]);
			p.push_back(mV[1]);
			p.push_back(mV[2]);
			p.push_back(mV[3]);
			p.setClosed();
			return p;
		}
		
		// internal data for debugging/visualization
		enum class Strategy
		{
			Intrinsic,
			Subset,
			Superset,
			Fragment
		};
		Strategy mStrategy;
		  
		std::vector<PolyLine2> mPolyDiff;
		vec2  mSize;
		float mArea=0;
		float mSourcePolyArea=0.f;
		float mDiffArea=MAXFLOAT;
		float mPerimScore=0.f;
		bool  mAllowed=false;
		float mScore=0.f;
	};
	typedef vector<Candidate> CandidateVec;
	
	bool getRectFromPoly( const PolyLine2& poly, PolyLine2& rect, CandidateVec* candidates=0 ) const;
	bool getRectFromPoly( const PolyLine2& poly, PolyLine2& rect, CandidateVec& cv ) const {
		return getRectFromPoly(poly,rect,&cv);
	}
	
private:

	bool getRectFromPoly_Intrinsic( const PolyLine2& poly, PolyLine2& rect, CandidateVec* candidates ) const;
	bool getRectFromPoly_Subset  ( const PolyLine2& poly, PolyLine2& rect, CandidateVec* candidates ) const;
	bool getRectFromPoly_Superset( const PolyLine2& poly, PolyLine2& rect, CandidateVec* candidates ) const;
	bool getRectFromPoly_Fragment( const PolyLine2& poly, PolyLine2& rect, CandidateVec* candidates ) const;
	// TODO: Subset and Superset strategies don't show their work yet in candidates (need to append to it)
	
	bool checkIsConvexHullReasonable( const PolyLine2& quad, const PolyLine2& source ) const;
	bool areInteriorAnglesOK( const PolyLine2& p ) const;
	bool isSizeOK( const PolyLine2& p ) const;	
	bool isOK( const PolyLine2& p ) const;
	
	bool trySubset( const PolyLine2& in, PolyLine2& out ) const;
	bool trySubsetQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const;

	static float getPolyDiffArea( PolyLine2 a, PolyLine2 b, std::vector<PolyLine2>* diff=0 );
	// basically a sugar-coat wrapper on cinder poly xor
	
	CandidateVec getFragmentCandidates( const PolyLine2& ) const;
	
};

#endif /* RectFinder_hpp */