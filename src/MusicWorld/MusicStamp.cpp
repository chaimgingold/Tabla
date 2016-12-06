//
//  MusicStamp.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/21/16.
//
//

#include <queue>
#include "cinder/Rand.h"

#include "MusicStamp.h"
#include "Score.h"
#include "Instrument.h"
#include "geom.h"
#include "xml.h"

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
	drawInstrumentIcon( mXAxis, mIconPose );
	
	if (0)
	{
		// debug search
		gl::color(0,0,1);
		gl::drawSolidCircle(mSearchForPaperLoc, 1.f);
		gl::color(1,0,0);
		gl::drawLine(mLoc, mSearchForPaperLoc);
	}
}

void MusicStamp::tick()
{
	const float now = ci::app::getElapsedSeconds();
	const float dt = now - mLastFrameTime;
	mLastFrameTime = now;

	// Advance gradient based on how many notes are on
	// TODO: modulate by tempo?
	if (mInstrument) {
		mGradientClock += dt * mIconPose.mGradientSpeed;
	}

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
	
	
	if (mRainbowShader && mInstrument && mInstrument->mIcon)
	{
		gl::ScopedGlslProg glslScp( mRainbowShader );
		gl::ScopedTextureBind texScp( mInstrument->mIcon );
		
		mRainbowShader->uniform( "uTex0", 0 );
		mRainbowShader->uniform( "uTime", mGradientClock );
		mRainbowShader->uniform( "uGradientCenter", pose.mGradientCenter );
		mRainbowShader->uniform( "uSeed", mGradientSeed );
		mRainbowShader->uniform( "uGradientFreq", (float)mInstrument->mOnNotes.size() );
		
		gl::drawSolidRect(r,vec2(0,1),vec2(1,0));
	}
	else
	{
		gl::color( pose.mColor );
		
		if (mInstrument && mInstrument->mIcon) gl::draw( mInstrument->mIcon, r );
		else gl::drawSolidRect(r);
	}
	
	gl::popModelMatrix();
}

void MusicStampVec::setParams( XmlTree& xml )
{
	getXml(xml,"IconWidth",mStampIconWidth);
	getXml(xml,"PaletteGutter",mStampPaletteGutter);
}

void MusicStampVec::setup( const map<string,InstrumentRef>& instruments, PolyLine2 worldBounds, vec2 timeVec, gl::GlslProgRef rainbowShader )
{
	mTimeVec = timeVec;
	mWorldBoundsPoly = worldBounds;
	this->clear();
	
	// get info about shape of world
	vec2 c = mWorldBoundsPoly.calcCentroid();
	float minr = MAXFLOAT, maxr = 0.f;
	
	for( auto p : mWorldBoundsPoly )
	{
		float d = distance(c,p);
		minr = min( minr, d );
		maxr = max( maxr, d );
	}
	
	// compute layout info
	float stampDist = mStampPaletteGutter + mStampIconWidth ;
	float circum = stampDist * (float) instruments.size();
	float r = circum / (2.f * M_PI); // 2πr
	r = min(r,minr-10.f); // what r to put them at?
	// this could be based on the number of stamps we want in the ring
	
	// make stamps
	float angle=0.f;
	float anglestep = M_PI*2.f / (float)instruments.size() ;

	int index = 0;
	for( auto i : instruments )
	{
		MusicStamp s;
		
		s.mRainbowShader = rainbowShader;
		s.mLoc = c + vec2(glm::rotate( vec3(r,0,0), angle, vec3(0,0,1) ));
		s.mHomeLoc = s.mLoc;
		s.mSearchForPaperLoc = s.mLoc;

		s.mXAxis = mTimeVec;
		s.mIconWidth = mStampIconWidth;
		s.mInstrument = i.second;
		s.mGradientSeed = index++;
		
		angle += anglestep;
		
		this->push_back(s);
	}
}

MusicStamp*
MusicStampVec::getStampByInstrument( InstrumentRef instr )
{
	for( auto &i : *this )
	{
		if (i.mInstrument==instr) return &i;
	}
	return 0;
}

void MusicStampVec::tick( const ScoreVec& scores, const ContourVector& contours, float globalPhase, float globalBeatDuration )
{
	// Forget stamps' scores
	for( auto &stamp : *this )
	{
		stamp.mLastHasScore = stamp.mHasScore;
		stamp.mHasScore = false;
		
		stamp.mLastHasContour = stamp.mHasContour;
		stamp.mHasContour = false;
	}

	// Note scores => stamps
	for( const auto &score : scores )
	{
		MusicStamp* stamp = getStampByInstrument( score.mInstrument );

		if (stamp)
		{
			// mark it
			stamp->mHasScore = true;
			
			// update search: new score to attach to?
			if ( !stamp->mLastHasScore /* new! */ && findContour(contours,score.getCentroid()) /* not a zombie */ )
			{
				stamp->mSearchForPaperLoc = score.getCentroid();
			}

			// anim
			updateAnimWithScore(*stamp,score);
		}
	}

	// update search loc
	updateSearch(contours);
	
	// idle dance
	updateIdleAnims(globalPhase, globalBeatDuration);
	
	// De-collide them
	decollide();
	decollideScores(scores);
	
	// Tick stamps
	for( auto &stamp : *this ) stamp.tick();
}

void MusicStampVec::updateAnimWithScore( MusicStamp& stamp, const Score& score ) const
{
	stamp.mXAxis = score.getPlayheadVec();
	stamp.mLoc = lerp( stamp.mLoc, score.getCentroid(), .5f );
	stamp.mIconPoseTarget = score.getIconPoseFromScore(score.getPlayheadFrac());
	stamp.mIconPoseTarget += tIconAnimState::getSwayForScore(
		fmod( score.getPlayheadFrac() * score.mMeasureCount, 1.f )
		); // blend in sway
}

void MusicStampVec::updateIdleAnims( float globalPhase, float globalBeatDuration )
{
	for( auto &stamp : *this )
	{
		if ( !stamp.mHasScore )
		{
			// idle sway animation
			stamp.mXAxis = mTimeVec;
			stamp.mIconPoseTarget = tIconAnimState() + tIconAnimState::getIdleSway(globalPhase,globalBeatDuration);
			
			// shrink away?
			if ( !stamp.mInstrument || !stamp.mInstrument->isAvailable() )
			{
				stamp.mIconPoseTarget.mScale = vec2(0,0);
			}
			
			// color: same as note-off
			if (stamp.mInstrument) stamp.mIconPoseTarget.mColor = stamp.mInstrument->mNoteOffColor;
		}
	}
}

void MusicStampVec::updateSearch( const ContourVector& contours )
{
	// sort stamps by priority when binding to and chasing contours
	auto cmp = [](MusicStamp* left, MusicStamp* right)
	{
		auto priority = []( MusicStamp& s ) -> int
		{
			if (s.mHasScore)		return 5;
			if (s.mLastHasScore)	return 4;
			if (s.mLastHasContour)	return 3;
			if (s.mHasContour)		return 2;
			return 0;
		};
		
		return priority(*left) < priority(*right);
	};
	priority_queue<MusicStamp*, std::vector<MusicStamp*>, decltype(cmp)> updateQueue(cmp);

	for( auto &s : *this ) updateQueue.push(&s);
	
	
	// update
	set<int> contoursUsed; // prevent stamps from doubling up by marking them as used

	while ( !updateQueue.empty() )
	{
		// get next
		MusicStamp& stamp = *(updateQueue.top());
		updateQueue.pop();
		
		// update search loc
		if ( stamp.isInstrumentAvailable() || stamp.mHasScore ) // very rare we don't ever try
		{
			// find  a contour to latch on to
			const Contour* contains = findContour(contours,stamp.mSearchForPaperLoc);
			
			// failed? find a close one instead...
			if ( !contains )
			{
				float dist;
				contains = contours.findClosestContour(stamp.mSearchForPaperLoc,0,&dist);
				if (dist>stamp.mIconWidth) contains=0;
			}
			
			// check it and bind
			if ( contains )
			{
				// ensure:
				// 1. the centroid is still in the polygon!, and
				// 2. it isn't in a score [Not doing this; Should we?]
				// 3. we aren't reusing an ocv contour >1x
				
				if ( contains->contains(contains->mCenter) && contoursUsed.find(contains->mOcvContourIndex) == contoursUsed.end() )
				{
					stamp.mHasContour = true;
					stamp.mSearchForPaperLoc = contains->mCenter;
					contoursUsed.insert(contains->mOcvContourIndex);
				}
			}
		}
		
		// go to search location?
		if ( !stamp.mHasScore )
		{
			stamp.mLoc = lerp( stamp.mLoc, stamp.mSearchForPaperLoc, .5f );
		}
	}
}

const Contour* MusicStampVec::findContour( const ContourVector& contours, vec2 p ) const
{
	const Contour* contains = contours.findLeafContourContainingPoint(p);
	
	if ( contains && !contains->mIsHole && contains->mPolyLine.contains(contains->mCenter) )
	{
		return contains;
	}
	else return 0;
}

void MusicStampVec::decollide()
{
	const float decollideFrac = .5f;  

	for( int i=0  ; i<this->size(); ++i )
	for( int j=i+1; j<this->size(); ++j )
	{
		auto &s1 = (*this)[i];
		auto &s2 = (*this)[j];
		
		float d = distance(s1.mSearchForPaperLoc,s2.mSearchForPaperLoc);
		float mind = (s1.mIconWidth + s2.mIconWidth) * .5f;
		float overlap = mind - d ;
		
		if ( overlap>0 )
		{
			vec2 v = s1.mSearchForPaperLoc - s2.mSearchForPaperLoc;
			if (v==vec2(0,0)) v = randVec2();
			else v = normalize(v);
			
			float df1 = decollideFrac ;
			float df2 = decollideFrac ;
			
			// prioritize them
			int p1 = (s1.mHasScore ? 3 : ( s1.mHasContour ? 2 : 1 ) ); 
			int p2 = (s2.mHasScore ? 3 : ( s2.mHasContour ? 2 : 1 ) ); 
			
			if (p1>p2) df1 = 0.f;
			else if (p1<p2) df2 = 0.f;
			
			// move
			s1.mSearchForPaperLoc += v * df1 * overlap * .5f;
			s2.mSearchForPaperLoc -= v * df2 * overlap * .5f; 
		}
	}
}

void MusicStampVec::decollideScores( const ScoreVec& scores )
{
	for( auto &stamp : *this )
	{
		if ( !stamp.mHasScore )
		{
			// did we collide with an active (has an instrument) score?
			const Score* s = scores.pick(stamp.mLoc);
			
			if ( s && s->mInstrument )
			{
	//			stamp.mLoc = lerp( stamp.mLoc, stamp.mHomeLoc, .5f );
				// problem: this doesn't complete properly, it works, but stamp just goes to the edge...

				// just do it all the way
				stamp.mLoc = stamp.mHomeLoc;
				stamp.mSearchForPaperLoc = stamp.mLoc;
			}
		}
	}
}
