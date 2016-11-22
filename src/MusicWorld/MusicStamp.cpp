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
	tIconAnimState pose = mIconPose;
//	pose.mColor = ColorA(1,1,1,1);
	
	drawInstrumentIcon(mXAxis, pose );
}

void MusicStamp::tick()
{
	mIconPose = lerp( mIconPose, mIconPoseTarget, .5f );
}

void MusicStamp::drawInstrumentIcon( vec2 worldx, tIconAnimState pose ) const
{
	// old code to orient next to score
//	vec2 iconLoc = fracToQuad(vec2(0,.5f));
//	iconLoc -= playheadVec * (kIconGutter + kIconWidth*.5f);
//	iconLoc += pose.mTranslate * vec2(kIconWidth,kIconWidth) ;

	vec2 worldy = perp(worldx);
	vec3 worldz = cross(vec3(worldx,0),vec3(worldy,0));
	
	//
	Rectf r(-.5f,-.5f,.5f,.5f);
	
	gl::pushModelMatrix();
	gl::translate( mLoc - r.getCenter() );
	gl::scale( vec2(1,1)*mIconWidth * pose.mScale );

	gl::translate( pose.mTranslate );
	gl::multModelMatrix( mat4( ci::mat3(
		vec3(worldx,0),
		vec3(worldy,0),
		worldz
		)) );
	gl::rotate( pose.mRotate );
	
	
	gl::color( pose.mColor );
	
	if (mInstrument && mInstrument->mIcon) gl::draw( mInstrument->mIcon, r );
	else gl::drawSolidRect(r);
	
	gl::popModelMatrix();
}