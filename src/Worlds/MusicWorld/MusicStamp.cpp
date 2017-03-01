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
	sway.mGradientFreq = 0;
	sway.mGradientSpeed = 0;
	sway.mGradientCenter = vec2(0,0);
	
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
	sway.mGradientFreq = 0;
	sway.mGradientSpeed = 0;
	sway.mGradientCenter = vec2(0,0);
	
	return sway;
}

void MusicStamp::draw( bool debugDrawSearch ) const
{
	if ( getState()==State::Lost && mDrawLastBoundToScorePoly>0.f )
	{
		gl::color(1,1,1,mDrawLastBoundToScorePoly*.5f);
		gl::drawSolid(mLastBoundToScorePoly);

		gl::color(0,1,1,mDrawLastBoundToScorePoly);
		gl::draw(mLastBoundToScorePoly);
	}
	
	drawInstrumentIcon( mXAxis, mIconPose );
	
	if (debugDrawSearch)
	{
		// debug search
		gl::color(0,0,1);
		gl::drawSolidCircle(mSearchForPaperLoc, 1.f);
		gl::color(1,0,0);
		gl::drawLine(mLoc, mSearchForPaperLoc);

		if ( mLastBoundToScorePoly.size()>0 )
		{
			gl::color(0,1,1);
			gl::draw(mLastBoundToScorePoly);
		}
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
		mGradientClock += dt * mIconPose.mGradientSpeed*mIconPose.mGradientFreq;
	}

	tIconAnimState oldPose = mIconPose;
	mIconPose = lerp( mIconPose, mIconPoseTarget, .5f );
	
	// slow decay of gradient params
	auto slowDecay = []( float old, float newv ) -> float
	{
		float f = .5f;
		if ( old > newv ) f = .05f;
		return lerp( old, newv, f );
	};
	
	mIconPose.mGradientSpeed = slowDecay( oldPose.mGradientSpeed, mIconPoseTarget.mGradientSpeed );
	mIconPose.mGradientFreq  = slowDecay( oldPose.mGradientFreq, mIconPoseTarget.mGradientFreq );
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
		mRainbowShader->uniform( "uGradientFreq", pose.mGradientFreq );
		
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

float MusicStamp::getInStateLength() const
{
	return ci::app::getElapsedSeconds() - mStateEnterTime;
}
	
void MusicStamp::goToState( State s )
{
	mState = s;
	mStateEnterTime = ci::app::getElapsedSeconds();
}

void MusicStamp::goHome()
{
	mLoc = mSearchForPaperLoc = mHomeLoc;
	setLastBoundToScorePoly( PolyLine2() );
	goToState( MusicStamp::State::Home );
}

void MusicStampVec::draw()
{
	for( const auto &s : *this )
	{
		bool drawIt=true;
		
		if ( areTokensEnabled() && !s.mHasScore ) drawIt=false;
		
		if (drawIt) s.draw(mDebugDrawSearch);
	}
}

void MusicStampVec::setParams( XmlTree& xml )
{
	getXml(xml,"IconWidth",mStampIconWidth);
	getXml(xml,"PaletteGutter",mStampPaletteGutter);
	getXml(xml,"DoCircleLayout",mDoCircleLayout);
	getXml(xml,"DebugDrawSearch",mDebugDrawSearch);
	getXml(xml,"SnapHomeWhenLost",mSnapHomeWhenLost);
	getXml(xml,"DoLostPolySearch",mDoLostPolySearch);
	getXml(xml,"DoLostPolySearchTime",mDoLostPolySearchTime);
	getXml(xml,"EnableTokens",mEnableTokens);
}

void MusicStampVec::setup( const map<string,InstrumentRef>& instruments, PolyLine2 worldBounds, vec2 timeVec, gl::GlslProgRef rainbowShader )
{
	mTimeVec = timeVec;
	mWorldBoundsPoly = worldBounds;
	this->clear();
	
	// make stamps
	int index = 0;
	for( auto i : instruments )
	{
		MusicStamp s;
		
		s.mRainbowShader = rainbowShader;

		s.mXAxis = mTimeVec;
		s.mIconWidth = mStampIconWidth;
		s.mInstrument = i.second;
		s.mGradientSeed = index++;
		
		this->push_back(s);
	}
	
	//
	if (mDoCircleLayout) setupStartLocsInCircle();
	else setupStartLocsInLine();
}

void MusicStampVec::setupStartLocsInCircle()
{
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
	float circum = stampDist * (float)size();
	float r = circum / (2.f * M_PI); // 2πr
	r = min(r,minr-10.f); // what r to put them at?
	// this could be based on the number of stamps we want in the ring

	float angle=0.f;
	float anglestep = M_PI*2.f / (float)size() ;
	
	for( MusicStamp &s : *this )
	{
		s.mLoc = c + vec2(glm::rotate( vec3(r,0,0), angle, vec3(0,0,1) ));
		s.mHomeLoc = s.mLoc;
		s.mSearchForPaperLoc = s.mLoc;
		
		angle += anglestep;
	}	
}

void MusicStampVec::setupStartLocsInLine()
{
	const float gutter = mStampPaletteGutter + mStampIconWidth/2;
	const vec2  worldc = mWorldBoundsPoly.calcCentroid();
	const vec2  up = -perp(mTimeVec);
	float upt;
	
	if ( !rayIntersectPoly( mWorldBoundsPoly, worldc, up, &upt ) ) return;
	
	const vec2 topc = worldc + up * (upt - gutter);
	
	float leftt,rightt;
	if ( !rayIntersectPoly( mWorldBoundsPoly, topc,  mTimeVec, &rightt ) ) return;
	if ( !rayIntersectPoly( mWorldBoundsPoly, topc, -mTimeVec, &leftt  ) ) return;
	
	const vec2 right = topc + mTimeVec * ( rightt - gutter ); 
	const vec2 left  = topc - mTimeVec * ( leftt  - gutter );
	
	if (empty())
	{
		MusicStamp& s = (*this)[0];
		
		s.mLoc = s.mSearchForPaperLoc = s.mHomeLoc = topc;
	}
	else
	{
		for ( int i=0; i<size(); ++i )
		{
			float f = (float)i / (float)(size()-1);
			
			MusicStamp& s = (*this)[i];
			
			s.mLoc = s.mSearchForPaperLoc = s.mHomeLoc = lerp(left,right,f);			
		}
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

MusicStamp*
MusicStampVec::getStampByInstrumentName( string name )
{
	for( auto &i : *this )
	{
		if (i.mInstrument->mName==name) return &i;
	}
	return 0;
}

const MusicStamp*
MusicStampVec::getStampByInstrumentName( string name ) const
{
	for( auto &i : *this )
	{
		if (i.mInstrument->mName==name) return &i;
	}
	return 0;
}

void MusicStampVec::updateWithTokens( const TokenMatchVec& tokens )
{
	return;
	bool verbose = false;
	
	for ( auto t : tokens )
	{
		auto stamp = getStampByInstrumentName(t.getName());
		
		if (stamp)
		{
			if (verbose) cout << "Token! " << t.getName() << endl;
			stamp->mLoc = stamp->mSearchForPaperLoc = stamp->mHomeLoc = t.getPoly().calcCentroid();
			stamp->goHome();
		}
		else /*if (verbose)*/ cout << "No Instrument for Token: '" << t.getName() << "'" << endl;
	}
}

void MusicStampVec::tick(
	const ScoreVec& scores,
	const ContourVector& contours,
	float globalPhase, float globalBeatDuration )
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
		for( auto instrument : score.mInstruments )
		{	
			MusicStamp* stamp = getStampByInstrument( instrument );

			if (stamp)
			{
				// mark it
				stamp->mHasScore = true;
				stamp->setLastBoundToScorePoly( score.getPolyLine() );
				
				// update search: new score to attach to?
				if ( !stamp->mLastHasScore /* new! */ && findContour(contours,score.getCentroid()) /* not a zombie */ )
				{
					stamp->mSearchForPaperLoc = score.getCentroid();
				}

				// anim
				updateAnimWithScore(*stamp,score);
			}
		}
	}

	if ( !areTokensEnabled() )
	{
		// update search loc
		updateSearch(contours);
		
		// idle dance
		updateIdleAnims(globalPhase, globalBeatDuration);
		
		// De-collide them
		decollide();
		decollideScores(scores);
		
		// Update state
		updateBoundState();
		
		// Snap home?
		if (mSnapHomeWhenLost) snapHomeIfLost();
		
		updateLostPolyDraw();
	}
	
	// Tick stamps
	for( auto &stamp : *this ) stamp.tick();
}

void MusicStampVec::updateLostPolyDraw()
{
	for( auto &s : *this )
	{
		float f;
		
		if ( s.getState()==MusicStamp::State::Lost && mDoLostPolySearch )
		{
			f = s.getInStateLength() / mDoLostPolySearchTime;
			
			f = constrain( f, 0.f, 1.f);
			f = 1.f - f;
			
//			const float k = .1f;
//			
//			if ( f < k ) f *= 1.f / k;
//			else f = 1.f - powf( (f - k)*((1.f-k)/1.f), 2.f );
		}
		else f = 0.f;
		
		s.setDrawLastBoundToScorePoly(f);
	}
}

void MusicStampVec::updateBoundState()
{
	for( auto &s : *this )
	{
		const bool isBoundNow = s.mHasScore || s.mHasContour ;
		
		switch ( s.getState() )
		{
			case MusicStamp::State::Lost:
			case MusicStamp::State::Home:
				if ( isBoundNow ) s.goToState( MusicStamp::State::Bound );
				break;
				
			case MusicStamp::State::Bound:
				if ( !isBoundNow ) s.goToState( MusicStamp::State::Lost );
				break;
		}
	}
}

void MusicStampVec::updateAnimWithScore( MusicStamp& stamp, const Score& score ) const
{
	stamp.mXAxis = score.getPlayheadVec();
	stamp.mLoc = lerp( stamp.mLoc, score.getCentroid(), .5f );
	stamp.mIconPoseTarget = score.getIconPoseFromScore(stamp.mInstrument,score.getPlayheadFrac());
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
			vec2 searchLoc;
			
			if ( mDoLostPolySearch && stamp.getLastBoundToScorePoly().size()>0 ) {
				searchLoc = stamp.getLastBoundToScorePoly().calcCentroid();
			} else {
				searchLoc = stamp.mSearchForPaperLoc;
			}
			
			const Contour* contains = findContour(contours,searchLoc);
			
			// failed?
			if ( !contains )
			{
				if ( mDoLostPolySearch && stamp.getLastBoundToScorePoly().size()>0 )
				{
					// look for intersection with old score...
					for( auto &i : contours )
					{
						if ( doPolygonsIntersect( stamp.getLastBoundToScorePoly(), i.mPolyLine ) )
						{
							contains = &i;
							break;
						}
					}
				}
				else
				{
					// find a close one instead...
					float dist;
					contains = contours.findClosestContour(stamp.mSearchForPaperLoc,0,&dist);
					float maxDist = stamp.mIconWidth * ((stamp.mLastHasContour || stamp.mLastHasScore) ? 5.f : 1.f ); 
					if (dist>maxDist) contains=0;
				}
			}
			
			// check it and bind
			if ( contains )
			{
				// ensure:
				// 1. the centroid is still in the polygon!, and
				// 2. it isn't in a score [Not doing this; Should we?]
				// 3. we aren't reusing an ocv contour >1x
				
				vec2 goTo = contains->mCenter;
				
				if ( !contains->contains(goTo) )
				{
					const float kGoInsideAmount = .5f;

					float dist;
					vec2 closest = closestPointOnPoly(goTo,contains->mPolyLine,0,0,&dist);					
					goTo += normalize(closest-goTo) * (dist+kGoInsideAmount);
				}
				
				if (   contains->contains(goTo) /* To be 100% sure; not like it really matters */
					&& contoursUsed.find(contains->mOcvContourIndex) == contoursUsed.end() )
				{
					stamp.mHasContour = true;
					stamp.mSearchForPaperLoc = goTo;
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
			
			if ( s && !s->mInstruments.empty() )
			{
	//			stamp.mLoc = lerp( stamp.mLoc, stamp.mHomeLoc, .5f );
				// problem: this doesn't complete properly, it works, but stamp just goes to the edge...

				if (mDoLostPolySearch)
				{
					stamp.goHome();
				}
				else
				{
					// just do it all the way
					stamp.mLoc = stamp.mHomeLoc;
					stamp.mSearchForPaperLoc = stamp.mLoc;
				}
			}
		}
	}
}

void MusicStampVec::snapHomeIfLost()
{
	if (mDoLostPolySearch)
	{
		for( auto &stamp : *this )
		{
			if ( stamp.getState()==MusicStamp::State::Lost )
			{
				// make sure we are in
				if ( stamp.getLastBoundToScorePoly().size() > 0 )
				{
					stamp.mLoc = stamp.mSearchForPaperLoc = stamp.getLastBoundToScorePoly().calcCentroid();
				}
				
				if ( stamp.getInStateLength() > mDoLostPolySearchTime )
				{
					stamp.goHome();
				}
			}
		}
	}
	else
	{
		for( auto &stamp : *this )
		{
			if ( stamp.getState()==MusicStamp::State::Lost )
			{
				stamp.goHome();
			}
		}
	}
}