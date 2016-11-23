//
//  MusicStamp.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/21/16.
//
//

#include "MusicStamp.h"
#include "geom.h"

tIconAnimState
tIconAnimState::getSwayForScore( float playheadFrac )
{
	tIconAnimState sway;
	
	float jump = sin( playheadFrac*4.f * M_PI*2.f );

	sway.mRotate = powf( cos( playheadFrac * M_PI*2.f ), 3.f ) * toRadians(25.);
	sway.mTranslate.y = jump * .1f;
	
	// zero so we can be added
	sway.mScale = vec2(0,0);
	sway.mColor = ColorA(0,0,0,0);
	
	return sway;
}

tIconAnimState
tIconAnimState::getIdleSway( float phaseInBeats, float beatDuration )
{
	tIconAnimState sway;
	
	float duration = beatDuration * 4.f;
	float f = fmod(phaseInBeats,duration) / duration;
	
	sway.mRotate = cos( f * M_PI*2.f ) * toRadians(25.);
//			float jump = sin( score.getPlayheadFrac()*4.f * M_PI*2.f );
//			sway.mTranslate.y = jump * .1f;

	// zero so we can be added
	sway.mScale = vec2(0,0);
	sway.mColor = ColorA(0,0,0,0);
	
	return sway;
}

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