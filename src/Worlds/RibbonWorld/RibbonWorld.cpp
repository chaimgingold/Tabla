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
#include "PaperBounce3App.h"

using namespace std;

const int MAX_RIBBONS = 60;
const int MAX_RIBBON_LENGTH = 500;
const int MAX_DISTANCE_UNTIL_CONSIDERED_NEW_RIBBON = 10.0;
const int MIN_DISTANCE_TO_ADD_POINT_ON_RIBBON = 1;

RibbonWorld::RibbonWorld()
{
	mFileWatch.loadShader(
						  PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "ribbon.vert" ),
						  PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "ribbon.frag" ),
						  [this](gl::GlslProgRef prog)
						  {
							  mRibbonShader = prog; // allows null, so we can easily see if we broke it
						  });
}

void RibbonWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	mWorld = pipeline.getStage("clipped");
	if ( !mWorld || mWorld->mImageCV.empty() ) return;

	mContours = visionOut.mContours;


	if (mRibbons.size() > MAX_RIBBONS) {
		mRibbons.erase(mRibbons.begin(), mRibbons.begin() + mRibbons.size() - MAX_RIBBONS);
	}

	for (auto &c : mContours) {
		vec2 newPoint = c.mPolyLine.calcCentroid();

		float bestDistance = 999999.0;
		Ribbon *bestRibbon;

		for (auto it = mRibbons.begin(); it != mRibbons.end(); ++it) {

			// Compare to each ribbon's tip point and find the closest one.
			float tipDistance = distance(newPoint, it->lastPoint);
//			cout << "Tip distance: " << it->ID << ": " << tipDistance << endl;
			if (tipDistance < bestDistance) {
				bestDistance = tipDistance;
				bestRibbon = &(*it);
			}
		}

		// Don't create redundant points
		if (bestDistance < MIN_DISTANCE_TO_ADD_POINT_ON_RIBBON) {
			continue;
		}

		// If tip of closest ribbon is within range, extend that ribbon
		if (bestDistance < MAX_DISTANCE_UNTIL_CONSIDERED_NEW_RIBBON) {
			vec2 lastPoint = bestRibbon->lastPoint;
			vec2 pointDiff = newPoint - lastPoint;
			float mag = length(pointDiff);

			float progress    = ((float)bestRibbon->numPoints)   / 100.0;
			float oldProgress = ((float)bestRibbon->numPoints-1) / 100.0;

			// Vary width with sine wave
			float width = (sin(progress*10) + 1.5) * 1;

			// Rotate two spread points in direction of new point
			float angle = atan2(pointDiff.y, pointDiff.x) + M_PI/2;
			mat4 rotator = rotate(mat4(1), angle, vec3(0, 0, 1));

			vec2 p1 = lastPoint + vec2(rotator * vec4(-width, -mag, 0, 1));
			vec2 p2 = lastPoint + vec2(rotator * vec4( width, -mag, 0, 1));

			// Tri1
			bestRibbon->triMesh->appendPosition(bestRibbon->lastP1);
			bestRibbon->triMesh->appendPosition(bestRibbon->lastP2);
			bestRibbon->triMesh->appendPosition(p1);
			bestRibbon->triMesh->appendTexCoord(vec2(0, oldProgress));
			bestRibbon->triMesh->appendTexCoord(vec2(1, oldProgress));
			bestRibbon->triMesh->appendTexCoord(vec2(0, progress));

			// Tri2
			bestRibbon->triMesh->appendPosition(bestRibbon->lastP2);
			bestRibbon->triMesh->appendPosition(p1);
			bestRibbon->triMesh->appendPosition(p2);
			bestRibbon->triMesh->appendTexCoord(vec2(1, oldProgress));
			bestRibbon->triMesh->appendTexCoord(vec2(0, progress));
			bestRibbon->triMesh->appendTexCoord(vec2(1, progress));

			bestRibbon->lastPoint = newPoint;
			bestRibbon->lastP1 = p1;
			bestRibbon->lastP2 = p2;
			bestRibbon->numPoints++;

			// Erase oldest positions
			if (bestRibbon->numPoints > MAX_RIBBON_LENGTH) {
				auto &positions = bestRibbon->triMesh->getBufferPositions();
				positions.erase(positions.begin(), positions.begin() + 6);

				auto &texCoords = bestRibbon->triMesh->getBufferTexCoords0();
				texCoords.erase(texCoords.begin(), texCoords.begin() + 6);
			}
		} else {
			// Otherwise, create a new one
			Ribbon newRibbon;
			newRibbon.ID = mRibbons.size();
			newRibbon.lastPoint = newPoint;
			newRibbon.lastP1 = newPoint;
			newRibbon.lastP2 = newPoint;
			newRibbon.numPoints = 1;

			newRibbon.triMesh = TriMesh::create( TriMesh::Format().positions(2).texCoords0(2) );

			mRibbons.push_back(newRibbon);
		}
	}
}

void RibbonWorld::update()
{
	mFileWatch.scanFiles();
}

void RibbonWorld::draw( DrawType drawType )
{
	gl::ScopedGlslProg glslScp( mRibbonShader );
	mRibbonShader->uniform( "uTime", (float)ci::app::getElapsedSeconds() );

	for (auto &r : mRibbons) {
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
