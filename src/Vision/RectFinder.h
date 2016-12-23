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

		// filters we always apply
		float mInteriorAngleMaxDelta = ci::toRadians(10.f); // how strict are corner angles? (0 = most strict)
		float mMinRectWidth=0.f;
		float mMinRectArea=0.f;
		
		// tuning
		float mMaxGainAreaFrac=.2f; // when doing superset, how much bigger can rect be?
		float mSubsetRectMinPerimOverlapFrac=.8f; // when doing subset, how much do edge perimeter need to overlap?
		float mEdgeOverlapDistAttenuate=1.f; // when doing subset edge perimeter test, how far is far away?
	};
	Params mParams;
	
	bool getRectFromPoly( const PolyLine2& poly, PolyLine2& rect ) const;

	// For debugging visualization (basically a sugar-coat wrapper on cinder poly xor)
	static float getPolyDiffArea( PolyLine2 a, PolyLine2 b, std::vector<PolyLine2>* diff=0 );

private:
	bool checkIsConvexHullReasonable( const PolyLine2& quad, const PolyLine2& source ) const;
	bool areInteriorAnglesOK( const PolyLine2& p ) const;
	bool isSizeOK( const PolyLine2& p ) const;
	
	bool trySubset( const PolyLine2& in, PolyLine2& out ) const;
	bool trySubsetQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const;
	
	
};

#endif /* RectFinder_hpp */