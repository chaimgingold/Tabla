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

//	cout << "*****************" << endl;
//	cout << "Checking " << mContours.size() << " contours" << endl;
	for (auto &c : mContours) {
		vec2 newPoint = c.mPolyLine.calcCentroid();

		float bestDistance = 999999.0;
		Ribbon *bestRibbon;

//		cout << "Checking " << mRibbons.size() << " ribbons" << endl;
//		cout << "Ribbons are now:" << endl;
//		for (auto& r : mRibbons) {
//			cout << "Ribbon " << r.ID << " size: " << r.points.size() << endl;
//		}

		for (auto it = mRibbons.begin(); it != mRibbons.end(); ++it) {

			// Compare to each ribbon's tip point and find the closest one.
			float tipDistance = distance(newPoint, it->points.getPoints().back());
//			cout << "Tip distance: " << it->ID << ": " << tipDistance << endl;
			if (tipDistance < bestDistance) {
				bestDistance = tipDistance;
				bestRibbon = &(*it);
			}
		}

		// Don't create redundant points
		if (bestDistance < 1) {
			continue;
		}

		// If tip of closest ribbon is within range, extend that ribbon
		if (bestDistance < 10.0) {
//			cout << "Extending ribbon " << bestRibbon->ID << endl;
			vec2 lastPoint = bestRibbon->points.getPoints().back();
			vec2 pointDiff = newPoint - lastPoint;
			float mag = length(pointDiff);

			// FIXME: do when mag > 0.01
			float angle = atan2(pointDiff.y, pointDiff.x) + M_PI/2;
			float width = 1.0;

			float progress = (float)bestRibbon->points.size() / 100.0;

			mat4 rotator = rotate(mat4(1), angle, vec3(0, 0, 1));

			vec2 p1 = lastPoint + vec2(rotator * vec4(-width, -mag, 0, 1));
			vec2 p2 = lastPoint + vec2(rotator * vec4( width, -mag, 0, 1));

			bestRibbon->triMesh->appendPosition(p1);
			bestRibbon->triMesh->appendPosition(p2);
			bestRibbon->triMesh->appendTexCoord(vec2(0, progress));
			bestRibbon->triMesh->appendTexCoord(vec2(1, progress));

			bestRibbon->points.push_back(newPoint);
		} else {
			// Otherwise, create a new one
			Ribbon newRibbon;
			newRibbon.ID = mRibbons.size();
			newRibbon.points.push_back(newPoint);

			newRibbon.triMesh = TriMesh::create( TriMesh::Format().positions(2).texCoords0(2) );
//			cout << "Created ribbon with ID: " << newRibbon.ID << endl;
			mRibbons.push_back(newRibbon);

//			cout << "Ribbons are now:" << endl;
//			for (auto &r : mRibbons) {
//				cout << "Ribbon " << r.ID << " size: " << r.points.size() << endl;
//			}
		}
	}



}

void RibbonWorld::update()
{

}

void RibbonWorld::draw( DrawType drawType )
{
	// Draw polyline

	for (auto &r : mRibbons) {
		auto polyLine = r.points;
		gl::color(ColorAf(1,0,1));
		gl::draw(polyLine);

		gl::draw(*r.triMesh);
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
