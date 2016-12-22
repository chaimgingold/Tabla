//
//  QuadTestWorld.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/22/16.
//
//

#ifndef QuadTestWorld_h
#define QuadTestWorld_h


#include <vector>
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Color.h"

#include "GameWorld.h"
#include "Contour.h"
#include "geom.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class QuadTestWorld : public GameWorld
{
public:
	QuadTestWorld();

	string getSystemName() const override { return "QuadTestWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output&, Pipeline& ) override;

	void update() override;
	void draw( DrawType ) override;

	void drawMouseDebugInfo( vec2 ) override;

private:
	vec2 mTimeVec=vec2(1,0); // for orienting quads; probably doesn't matter at all.
	float mMaxGainAreaFrac=.2f;
	float mInteriorAngleMaxDelta=10;
	float mTheorizedRectMinPerimOverlapFrac=.8f;
	
	float getFracOfEdgeCoveredByPoly( vec2 a, vec2 b, const PolyLine2& p, float& abLen ) const;
	float calcPolyEdgeOverlapFrac( const PolyLine2& fracOfA, const PolyLine2& isCoveredByB ) const; // 0..1 result
	
	bool checkIsQuadReasonable( const PolyLine2& quad, const PolyLine2& source ) const;
	bool areInteriorAnglesOK( const PolyLine2& p ) const;
	bool getQuadFromPoly( const PolyLine2& in, PolyLine2& out ) const;
	bool theorize( const PolyLine2& in, PolyLine2& out ) const;
	bool theorizeQuadFromEdge( const PolyLine2& p, int i, PolyLine2& result, float& ioBestScore, float &ioBestArea, float angleDev ) const;

	
	class Frame
	{
	public:
		bool mIsValid=false;
		
		PolyLine2 mContourPoly;
		PolyLine2 mContourPolyReduced;
		PolyLine2 mConvexHull;
		std::vector<PolyLine2> mReducedDiff;
		vec2 mQuad[4];
		
		float mOverlapScore;
		PolyLine2 getQuadAsPoly() const;
	};
	typedef vector<Frame> FrameVec; 
	FrameVec mFrames;
	
	FrameVec getFrames(
		const Pipeline::StageRef world,
		const ContourVector &contours,
		Pipeline& pipeline ) const;
	
};

class QuadTestWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "QuadTestWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<QuadTestWorld>();
	}
};


#endif /* QuadTestWorld_h */
