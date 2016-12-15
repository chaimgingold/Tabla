//
//  RibbonWorld.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#include "RibbonWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;

RibbonWorld::RibbonWorld()
{

}

void RibbonWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	mWorld = pipeline.getStage("clipped");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	mContours = visionOut.mContours;

	cout << "**************************" << endl;
	for (auto c : mContours ) {
		vec2 center = c.mPolyLine.calcCentroid();

		float bestDistance = 9999.0;
		Ribbon &bestRibbon = mRibbons.back();

		for (auto &r : mRibbons) {


			float pointDistance = distance(center, r.points.getPoints().back());

			if (pointDistance < bestDistance) {
				bestDistance = pointDistance;
				bestRibbon = r;
			}
		}
		// Don't create redundant points
		if (bestDistance < 1) {
			continue;;
		}
		if (bestDistance < 10.0) {
			bestRibbon.points.push_back(center);
		} else {
			Ribbon newRibbon;
			newRibbon.ID = mRibbons.size();
			newRibbon.points.push_back(center);
			mRibbons.push_back(newRibbon);

			for (auto &r : mRibbons) {
				cout << "Ribbon " << r.ID << " size: " << r.points.size() << endl;
			}
		}
	}



}

void RibbonWorld::update()
{

}

void RibbonWorld::draw( DrawType drawType )
{
	// Draw polyline

	for (auto c : mRibbons) {
		auto polyLine = c.points;
		gl::color(ColorAf(1,0,1));
		gl::draw(polyLine);
	}
}

void RibbonWorld::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_TAB:
		{

		}
		break;
		case KeyEvent::KEY_BACKQUOTE:
		{

		}
		default:
			break;
	}
}
