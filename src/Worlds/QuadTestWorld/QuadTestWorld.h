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
#include "RectFinder.h"

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
	
	class Frame
	{
	public:
		PolyLine2 mContourPoly;
		PolyLine2 mContourPolyResult;
		RectFinder::CandidateVec mCandidates;
		bool mIsOK;
	};
	typedef vector<Frame> FrameVec; 
	FrameVec mFrames;
	
	FrameVec getFrames(
		const ContourVector &contours,
		Pipeline& pipeline ) const;
	
	RectFinder mRectFinder;
};

#endif /* QuadTestWorld_h */
