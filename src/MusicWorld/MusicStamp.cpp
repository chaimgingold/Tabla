//
//  MusicStamp.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/21/16.
//
//

#include "MusicStamp.h"
#include "geom.h"

void MusicStamp::draw() const
{
	tInstrumentIconAnimState pose;
	pose.mColor = ColorA(1,1,1,1);
	
	drawInstrumentIcon(mTimeVec, pose );
}

void MusicStamp::drawInstrumentIcon( vec2 worldx, tInstrumentIconAnimState pose ) const
{
	// draw
//	const float kIconWidth  = 5.f ;
//	const float kIconGutter = 1.f ;
	// in world space
	
	//
//	const vec2 playheadVec = getPlayheadVec();
	
//	vec2 iconLoc = fracToQuad(vec2(0,.5f));
//	iconLoc -= playheadVec * (kIconGutter + kIconWidth*.5f);
//	iconLoc += pose.mTranslate * vec2(kIconWidth,kIconWidth) ;
	
	//
	Rectf r(-.5f,-.5f,.5f,.5f);
	
	gl::pushModelMatrix();
	gl::translate( mLoc - r.getCenter() );
	gl::scale( vec2(1,1)*mIconWidth * pose.mScale );
	
	vec2 worldy = perp(worldx);
	vec3 worldz = cross(vec3(worldx,0),vec3(worldy,0));
	
	gl::multModelMatrix( mat4( ci::mat3(
		vec3(worldx,0),
		vec3(worldy,0),
		worldz
		)) );
	gl::rotate( pose.mRotate );
	
	gl::color( pose.mColor );
	
	if (mIcon) gl::draw( mIcon, r );
	else gl::drawSolidRect(r);
	
	gl::popModelMatrix();
}