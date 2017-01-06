//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "PaperBounce3App.h"
#include "glm/glm.hpp"
#include "PinballWorld.h"
#include "PinballParts.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "Gamepad.h"
#include "xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Pinball;

//const bool kLogButtons = true; // we will need them to configure new controllers :P

PinballWorld::PinballWorld()
	: mView(*this)
{
	setupSynthesis();
	setupControls();
	mView.setup();
}

void PinballWorld::setupControls()
{
	mIsFlipperDown[0] = false;
	mIsFlipperDown[1] = false;
	
	mFlipperState[0] = 0.f;
	mFlipperState[1] = 0.f;
	
	auto button = [this]( unsigned int id, string postfix )
	{
		auto i = mGamepadButtons.find(id);
		if (i!=mGamepadButtons.end())
		{			
			processInputEvent( i->second + postfix );
		}
	};
	
	
	mGamepadManager.mOnButtonDown = [this,button]( const GamepadManager::Event& event )
	{
		cout << "down " << event.mId << endl;
		button(event.mId,"-down");
	};

	mGamepadManager.mOnButtonUp = [this,button]( const GamepadManager::Event& event )
	{
		cout << "up "  << event.mId << endl;
		button(event.mId,"-up");
	};
	
	// inputs
	auto flipperChange = [this]( int side, int state )
	{
		mIsFlipperDown[side] = state;
		
		if (getBalls().empty() && state==1 && !getIsInGameOverState() ) serveBall();
		
		if ( mPartCensus.getPop( flipperIndexToType(side) ) > 0 )
		{
			mPd->sendFloat("flipper-change", state);
			
			if (state) this->addScreenShake(mFlipperScreenShake);
		}
	};
	
	mInputToFunction["flippers-left-down"]  = [this,flipperChange]() { flipperChange(0,1); };
	mInputToFunction["flippers-left-up"]    = [this,flipperChange]() { flipperChange(0,0); };
	mInputToFunction["flippers-right-down"] = [this,flipperChange]() { flipperChange(1,1); };
	mInputToFunction["flippers-right-up"]   = [this,flipperChange]() { flipperChange(1,0); };
	mInputToFunction["pause-ball-world-down"]    = [this](){ mPauseBallWorld = !mPauseBallWorld; };
	mInputToFunction["serve-multiball-down"]     = [this](){ serveBall(); }; 
}

PinballWorld::~PinballWorld()
{
	shutdownSynthesis();
}

void PinballWorld::setParams( XmlTree xml )
{
	if ( xml.hasChild("Parts") ) {
		mPartParams.set(xml.getChild("Parts"));
	}
	
	if ( xml.hasChild("BallWorld") ) {
		BallWorld::setParams( xml.getChild("BallWorld") );
	}
	
	if ( xml.hasChild("View") ) {
		mView.setParams(xml.getChild("View"));
	}
	
	bool wasSoundEnabled = mSoundEnabled;
	getXml(xml, "SoundEnabled", mSoundEnabled);
	if (wasSoundEnabled!=mSoundEnabled) setupSynthesis();

	getXml(xml, "FlipperScreenShake", mFlipperScreenShake );
	getXml(xml, "BallVelScreenShakeK", mBallVelScreenShakeK );
	getXml(xml, "MaxScreenShake", mMaxScreenShake);
	
	// playfield
	getXml(xml, "UpVec", mUpVec );	
	getXml(xml, "Gravity", mGravity );
	getXml(xml, "BallReclaimAreaHeight", mBallReclaimAreaHeight );

	// vision
	getXml(xml, "FlipperDistToEdge", mFlipperDistToEdge );	
	getXml(xml, "PartMaxContourRadius", mPartMaxContourRadius );
	getXml(xml, "PartMaxContourAspectRatio", mPartMaxContourAspectRatio );
	getXml(xml, "HolePartMaxContourRadius",mHolePartMaxContourRadius);
	
	getXml(xml, "PartTrackLocMaxDist", mPartTrackLocMaxDist );
	getXml(xml, "PartTrackRadiusMaxDist", mPartTrackRadiusMaxDist );
	getXml(xml, "DejitterContourMaxDist", mDejitterContourMaxDist );
	
	// gamepad
	mGamepadButtons.clear();
	if (xml.hasChild("Gamepad"))
	{
		XmlTree keys = xml.getChild("Gamepad");
		
		for( auto item = keys.begin("button"); item != keys.end(); ++item )
		{
			if (item->hasAttribute("id") && item->hasAttribute("do"))
			{
				unsigned int id = item->getAttributeValue<unsigned int>("id");
				string _do = item->getAttributeValue<string>("do");
				
				cout << id << " -> " << _do << endl;
				
				mGamepadButtons[id] = _do;
			}
		}
	}
	
	// keyboard
	mKeyToInput.clear();
	if (xml.hasChild("KeyMap"))
	{
		XmlTree keys = xml.getChild("KeyMap");
		
		for( auto item = keys.begin("Key"); item != keys.end(); ++item )
		{
			if ( item->hasChild("char") && item->hasChild("input") )
			{
				string	charkey	= item->getChild("char").getValue<string>();
				string	input	= item->getChild("input").getValue();
				
				char ckey = charkey.front();
				
				cout << ckey << ", " << input << endl;

				mKeyToInput[ckey] = input;
			}
		}
	}
}

Rectf PinballWorld::toPlayfieldBoundingBox ( const PolyLine2 &poly ) const
{
	std::vector<vec2> pts = poly.getPoints();
	
	for( auto &p : pts )
	{
		p = toPlayfieldSpace(p);
	}
	
	return Rectf(pts);
}

vec2 PinballWorld::normalizeToPlayfieldBBox( vec2 worldSpace ) const
{
	vec2 p = toPlayfieldSpace(worldSpace);
	
	p.x = (p.x - mPlayfieldBoundingBox.x1) / mPlayfieldBoundingBox.getWidth();
	p.y = (p.y - mPlayfieldBoundingBox.y1) / mPlayfieldBoundingBox.getHeight();
	
	return p;
}

vec2 PinballWorld::fromNormalizedPlayfieldBBox( vec2 p ) const
{
	p.x = mPlayfieldBoundingBox.x1 + mPlayfieldBoundingBox.getWidth()*p.x;
	p.y = mPlayfieldBoundingBox.y1 + mPlayfieldBoundingBox.getWidth()*p.y;
	
	return fromPlayfieldSpace(p);
}

void PinballWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PinballWorld::sendGameEvent( GameEvent e )
{
	onGameEvent(e);
	
	for( auto p : mParts ) {
		p->onGameEvent(e);
	}
}

void PinballWorld::onGameEvent( GameEvent e )
{
	switch(e)
	{
		case GameEvent::LostBall:
//			addScreenShake(1.f);
			break;
		case GameEvent::LostLastMultiBall:
			mPd->sendBang("game-over");
			beginParty(0);
			mTargetCount=0;
			break;
			
		case GameEvent::ServeMultiBall:
			mPd->sendBang("multi-ball");
			beginParty(1); // trigger shader effect
			break;
			
		case GameEvent::ServeBall:
			mPd->sendBang("serve-ball");
			break;

		case GameEvent::ATargetTurnedOn:
			mTargetCount++;
			mPd->sendFloat("rollover-count", mTargetCount);
			break;
			
		default:break;
	}
}
void PinballWorld::beginParty( int type ) {
	mPartyBegan = (float)ci::app::getElapsedSeconds();
	mPartyType = type;
}

vec2 PinballWorld::getPartyLoc() const
{
	if (mPartyType==0) return vec2(1, 0.5);
	else return vec2(0,.5);
}

float PinballWorld::getPartyProgress() const {
	float timeSinceGameOver = (float)ci::app::getElapsedSeconds() - mPartyBegan;
	return timeSinceGameOver < 2 ? timeSinceGameOver : -1;
}

void PinballWorld::addScreenShake( float intensity )
{
	mScreenShakeValue += intensity;
	mScreenShakeValue = min( mScreenShakeValue, mMaxScreenShake );
//	mScreenShakeValue = min( 1.f, mScreenShakeValue + intensity) ;
}

void PinballWorld::updateScreenShake()
{
	mScreenShakeValue *= .9f;
	mScreenShakeVec *= .5f;
	
	if ( randFloat() < mScreenShakeValue )
	{
		mScreenShakeVec += randVec2() * mScreenShakeValue;
	}
}

void PinballWorld::update()
{
	mFileWatch.update();

	// take census
	mPartCensus = PartCensus();
	for( auto p : mParts ) p->updateCensus(mPartCensus);
	
	// enter multiball?
	{
		int numTargets = getPartCensus().getPop(PartType::Target);
		
		if ( numTargets>0 && getPartCensus().mNumTargetsOn==numTargets ) {
			// start multi-ball
			serveBall();
		}
	}
	
	// input
	mGamepadManager.tick();
	tickFlipperState();

	if (!mPauseBallWorld) updateScreenShake();
	
	// tick parts
	for( auto p : mParts ) p->tick();

	// update contours, merging in physics shapes from our parts
	updateBallWorldContours();

	// gravity
	vector<Ball>& balls = getBalls();
	for( Ball &b : balls ) {
		b.mAccel += getGravityVec() * mGravity;
	}
	
	// sim balls
	cullBalls();
	if (!mPauseBallWorld) BallWorld::update();
		// TODO: To make pinball flippers super robust in terms of tunneling (especially at the tips),
		// we should make attachments for BallWorld contours (a parallel vector)
		// that specifies angular rotation (center, radians per second)--basically what
		// Flipper::getAccelForBall does, and have BallWorld do more fine grained rotations of
		// the contour itself. So we'd set the flipper to its rotation for the last frame,
		// and specify the rotation to be the rotation that will get us to its rotation for this frame,
		// and BallWorld will handle the granular integration itself.
	
	// respond to collisions
	processCollisions();
	
	updateBallSynthesis();
	
	mView.update();
}

void PinballWorld::updateBallSynthesis() {
	vector<Ball>& balls = getBalls();
	
	auto ballVels = pd::List();

	// Send no velocities when paused to keep balls silent
	bool shouldSynthesizeBalls = !mPauseBallWorld;

	if (shouldSynthesizeBalls) {
		for( Ball &b : balls ) {
			ballVels.addFloat(length(b.getVel()) * 10);
		}
	}
	
	mPd->sendList("ball-velocities", ballVels );
}

void PinballWorld::serveBall()
{
	const int oldCount = getBalls().size();

	// for now, from the top
	float fx = Rand::randFloat();
	
	vec2 loc = fromPlayfieldSpace(
		vec2( lerp(mPlayfieldBoundingBox.x1,mPlayfieldBoundingBox.x2,fx), mPlayfieldBoundingBox.y2 )
		);
	
	Ball& ball = newRandomBall(loc);
	
	ball.mCollideWithContours = true;
	ball.mColor = mBallDefaultColor; // no random colors that are set entirely in code, i think. :P
	
	sendGameEvent(GameEvent::ServeBall);
	if (oldCount>0) sendGameEvent(GameEvent::ServeMultiBall);
	if (oldCount==0) sendGameEvent(GameEvent::NewGame);
}

void PinballWorld::cullBalls()
{
	const int oldCount = getBalls().size();
	
	// remove?
	vector<Ball>& balls = getBalls();
	vector<Ball> newBalls;
	
	for( const auto &b : balls )
	{
		if ( toPlayfieldSpace(b.mLoc).y + b.mRadius < mPlayfieldBallReclaimY )
		{
			// kill
		}
		else
		{
			// keep
			newBalls.push_back(b);
		}
	}
	
	balls = newBalls;
	
	// events
	const int newCount = balls.size();
	
	if (newCount==0 && oldCount>0) sendGameEvent(GameEvent::LostLastMultiBall);
	if (newCount < oldCount) sendGameEvent(GameEvent::LostBall);
}

void PinballWorld::tickFlipperState()
{
	for( int i=0; i<2; ++i )
	{
		if (0)
		{
			// non-linear, creates lots of tunneling
			float frac[2] = { .35f, .5f };
			int fraci = mIsFlipperDown[i] ? 0 : 1;
			
			mFlipperState[i] = lerp( mFlipperState[i], mIsFlipperDown[i] ? 1.f : 0.f, frac[fraci] );
		}
		else
		{
			
			// linear, to help me debug physics
			float step = (1.f / 60.f) * ( 1.f / .1f );
			
			if ( mIsFlipperDown[i] ) mFlipperState[i] += step;
			else mFlipperState[i] -= step;
			
			mFlipperState[i] = constrain( mFlipperState[i], 0.f, 1.f );
		}
	}
}

float PinballWorld::getStrobe( float phase, float freq ) const
{
	float strobe;
	strobe = getStrobeTime();
	strobe = fmod( strobe, freq ) / freq;
	strobe = (cos( (strobe + phase)*M_PI*2.f) + 1.f) / 2.f;
	return strobe;
}

float PinballWorld::getFlipperAngularVel( int side ) const
{
	const float eps = .1f;
	const float radPerSec = (M_PI/2.f) / 6.f; // assume 6 steps per 90 deg of motion
	const float sign[2] = {1.f,-1.f};
	
	if ( mIsFlipperDown[side] && mFlipperState[side] < 1.f-eps ) return radPerSec * sign[side];
	else if ( !mIsFlipperDown[side] && mFlipperState[side] > eps ) return -radPerSec * sign[side];
	else return 0.f;
}

void PinballWorld::processCollisions()
{
	float screenShake=0.f;
	
	auto getBall = [this]( int i ) -> Ball*
	{
		Ball* ball=0;
		if (i>=0 && i<=getBalls().size())
		{
			ball = &getBalls()[i];
		}
		return ball;
	};
	
	auto shakeWithBall = [&]( int i )
	{
		Ball* b = getBall(i);
		if (b) {
			screenShake += length(getDenoisedBallVel(*b)) * mBallVelScreenShakeK;
		}
	};
	
	for( const auto &c : getBallBallCollisions() )
	{
	//	mPd->sendBang("new-game");

		if (0) cout << "ball ball collide (" << c.mBallIndex[0] << ", " << c.mBallIndex[1] << ")" << endl;
		
		shakeWithBall(c.mBallIndex[0]);
		shakeWithBall(c.mBallIndex[1]);
	}
	
	for( const auto &c : getBallWorldCollisions() )
	{
		if (0) cout << "ball world collide (" << c.mBallIndex << ")" << endl;
	}

	for( const auto &c : getBallContourCollisions() )
	{
		shakeWithBall(c.mBallIndex);

		Ball* ball = getBall(c.mBallIndex);
		
		if (0) cout << "ball contour collide (ball=" << c.mBallIndex << ", " << c.mContourIndex << ")" << endl;
		
		// tell part
		PartRef p = findPartForContour(c.mContourIndex);
		if (p) {
	//		cout << "collide part" << endl;
			if (ball) p->onBallCollide(*ball);
		}
		else {
			if (ball) {
				mPd->sendFloat("hit-wall", length( getDenoisedBallVel(*ball) ) * 10);
			}
	//		cout << "collide wall" << endl;
		}
	}
	
	this->addScreenShake(screenShake);
}

void PinballWorld::prepareToDraw()
{
	BallWorld::prepareToDraw();	

	mView.prepareToDraw();
}

void PinballWorld::draw( DrawType dt )
{
	mView.draw(dt);
}

void PinballWorld::worldBoundsPolyDidChange()
{
}

void PinballWorld::processInputEvent( string name )
{
	cout << "-> " << name << endl;

	auto j = mInputToFunction.find(name);
	if (j!=mInputToFunction.end())
	{
		j->second();
	}
}

void PinballWorld::processKeyEvent( KeyEvent event, string suffix )
{
	char c = event.getChar();
	
	auto i = mKeyToInput.find(c);
	
	if (i!=mKeyToInput.end())
	{
		processInputEvent(i->second + suffix);
	}
}

void PinballWorld::keyDown( KeyEvent event )
{
	processKeyEvent(event, "-down");
}

void PinballWorld::keyUp( KeyEvent event )
{
	processKeyEvent(event, "-up");
}

void PinballWorld::mouseClick( vec2 loc )
{
}

void PinballWorld::getCullLine( vec2 v[2] ) const
{
	v[0] = fromPlayfieldSpace(vec2(mPlayfieldBallReclaimX[0],mPlayfieldBallReclaimY));
	v[1] = fromPlayfieldSpace(vec2(mPlayfieldBallReclaimX[1],mPlayfieldBallReclaimY));
}

Rectf PinballWorld::getPlayfieldBoundingBox( const ContourVec& cs ) const
{
	vector<vec2> pts;
	
	for( const auto& c : cs )
	{
		if (c.mTreeDepth==0 && !shouldContourBeAPart(c,cs))
		{
			pts.insert(pts.end(),c.mPolyLine.getPoints().begin(), c.mPolyLine.end());
		}
	}
	
	for( auto &p : pts )
	{
		p = toPlayfieldSpace(p);
	}
	
	return Rectf(pts);
}

void PinballWorld::updatePlayfieldLayout( const ContourVec& contours )
{
	mPlayfieldBoundingBox = getPlayfieldBoundingBox(contours);
//	cout << "min/max y: " << mPlayfieldBoundingBox.y1 << " " << mPlayfieldBoundingBox.y2 << endl;
	mPlayfieldBallReclaimY = mPlayfieldBoundingBox.y1 + mBallReclaimAreaHeight;
	
	mPlayfieldBallReclaimX[0] = mPlayfieldBoundingBox.x1;
	mPlayfieldBallReclaimX[1] = mPlayfieldBoundingBox.x2;
	
	float t;
	vec2 vleft  = vec2(mPlayfieldBallReclaimX[0],mPlayfieldBallReclaimY);
	vec2 vright = vec2(mPlayfieldBallReclaimX[1],mPlayfieldBallReclaimY);
	if ( contours.rayIntersection( fromPlayfieldSpace(vleft), getRightVec(), &t ) )
	{
		mPlayfieldBallReclaimX[0] = (vleft + toPlayfieldSpace(getRightVec() * t)).x;
	}
	
	if ( contours.rayIntersection( fromPlayfieldSpace(vright), getLeftVec(), &t ) )
	{
		mPlayfieldBallReclaimX[1] = (vright + toPlayfieldSpace(getLeftVec() * t)).x;
	}
}

void PinballWorld::updateBallWorldContours()
{
	// modify vision out for ball world
	// (we add contours to convey the new shapes to simulate)
	ContourVec physicsContours = mVisionContours;

	getContoursFromParts( mParts, physicsContours, &mContoursToParts );
	
	// which contours to use?
	// (we could move this computation into updateVision(), but its cheap and convenient to do it here)
	ContourVec::Mask mask(physicsContours.size(),true); // all true...
	for( int i=0; i<mVisionContours.size(); ++i )
	{
		// ...except vision contours that became parts
		mask[i] = !shouldContourBeAPart(physicsContours[i],physicsContours);
	}
	
	// tell ball world
	BallWorld::setContours(physicsContours,ContourVec::getMaskFilter(mask));
}

ContourVec PinballWorld::dejitterVisionContours( ContourVec in, ContourVec old ) const
{
	ContourVec out = in;
	
	for( auto &c : out )
	{
		// find closest contour to compare ourselves against
		float bestscore = MAXFLOAT;
		const Contour* match = 0;
		
		for( auto &o : old )
		{
			if (o.mIsHole != c.mIsHole) continue;
			if (o.mTreeDepth != c.mTreeDepth) continue;
			
			float score = distance( c.mCenter, o.mCenter )
				+ fabs( c.mRadius - o.mRadius )
				;
			
			if (score < bestscore)
			{
				bestscore=score;
				match = &o;
			}
		}
		
		// match points
		if (match)
		{
			for( auto &p : c.mPolyLine.getPoints() )
			{
				float bestscore = mDejitterContourMaxDist;
				
				for ( auto op : match->mPolyLine.getPoints() )
				{
					float score = distance(p,op);
					if (score<bestscore)
					{
						bestscore = score;
						p = op;
					}
				}
			}
		}
	}
	
	return out;
}

void PinballWorld::updateVision( const Vision::Output& visionOut, Pipeline& p )
{
	// capture contours, so we can later pass them into BallWorld (after merging in part shapes)
	if ( mDejitterContourMaxDist > 0.f ) {
		mVisionContours = dejitterVisionContours( visionOut.mContours, mVisionContours );
	} else {
		mVisionContours = visionOut.mContours;
	}
	
	// playfield layout
	updatePlayfieldLayout(visionOut.mContours);
	
	// generate parts
	PartVec newParts = getPartsFromContours(visionOut.mContours);
	mParts = mergeOldAndNewParts(mParts, newParts);
	
	// log cube maps, allow it to capture images
	mView.updateVision(p);
}

bool PinballWorld::shouldContourBeAPart( const Contour& c, const ContourVec& cs ) const
{
	{
		float lo = c.mRotatedBoundingRect.mSize.x;
		float hi = c.mRotatedBoundingRect.mSize.y;
		
		if (hi<lo) swap(lo,hi);
		
		// not round enough
		if (hi>lo*mPartMaxContourAspectRatio) return false;
	}
	
	if ( c.mIsHole ) {
		return c.mTreeDepth>0 && c.mRadius < mPartMaxContourRadius;
	}
	else {
		bool depthOK;
		
		if (c.mTreeDepth==0) depthOK=true; // ok
		else if (c.mTreeDepth==2) // allow them to be nested 1 deep, but only if parent contour isn't a part!
		{
			auto p = cs.getParent(c);
			assert(p);
			depthOK = !shouldContourBeAPart(*p,cs);
		}
		else depthOK=false;
		
		return depthOK && c.mRadius < mHolePartMaxContourRadius;
	}
}

AdjSpace
PinballWorld::getAdjacentSpace( vec2 loc, const ContourVector& cs ) const
{
	const Contour* leaf = cs.findLeafContourContainingPoint(loc);
	
	return getAdjacentSpace(leaf, loc, cs);
}

AdjSpace
PinballWorld::getAdjacentSpace( const Contour* leaf, vec2 loc, const ContourVector& cs ) const
{
	AdjSpace result;

	if (leaf)
	{
		float far = leaf->mRadius * 2.f;
		
		auto calcSize = [&]( vec2 vec, float& result )
		{
			float t;
			if ( leaf->rayIntersection(loc + vec * far, -vec, &t) ) {
				result = far - t;
			}
		};
		
		calcSize( getLeftVec(), result.mWidthLeft );
		calcSize( getRightVec(), result.mWidthRight );
		calcSize( getUpVec(), result.mWidthUp );
		calcSize( getDownVec(), result.mWidthDown );
	}
	
	auto filter = [&]( const Contour& c ) -> bool
	{
		// 1. not self
		//if ( c.mIsHole && c.contains(loc) ) return false;
		if ( &c == leaf ) return false; // supposed to be optimized version of c.mIsHole && c.contains(loc)
		// 2. not other parts
		else if ( shouldContourBeAPart(c,cs) ) return false; // could be a part
		// OK
		else return true;
	};
	
	cs.rayIntersection( loc, getRightVec(), &result.mRight, filter );
	cs.rayIntersection( loc, getLeftVec (), &result.mLeft , filter );
	cs.rayIntersection( loc, getUpVec(),    &result.mUp , filter );
	cs.rayIntersection( loc, getDownVec(),  &result.mDown , filter );
	
	// bake in contour width into adjacent space calc
	result.mLeft  -= result.mWidthLeft;
	result.mRight -= result.mWidthRight;
	result.mUp    -= result.mWidthUp;
	result.mDown  -= result.mWidthDown;
	
	return result;
}

PartVec PinballWorld::getPartsFromContours( const ContourVector& contours )
{
	PartVec parts;
	
	for( const auto &c : contours )
	{
		if ( !shouldContourBeAPart(c,contours) ) continue;


		AdjSpace adjSpace = getAdjacentSpace(&c,c.mCenter,contours);

		auto add = [&c,&parts,adjSpace]( Part* p )
		{
			p->mContourLoc = c.mCenter;
			p->mContourRadius = c.mRadius;
			parts.push_back( PartRef(p) );
		};
		

		if ( c.mIsHole )
		{
			// flipper orientation
			const bool closeToRight  = adjSpace.mRight < mFlipperDistToEdge;
			const bool closeToLeft   = adjSpace.mLeft  < mFlipperDistToEdge;
//			const bool closeToBottom = adjSpace.mDown  < mFlipperDistToEdge;
			
			if ( closeToRight && !closeToLeft )
			{
				add( new Flipper(*this, c.mCenter, c.mRadius, PartType::FlipperRight) );
			}
			else if ( closeToLeft && !closeToRight )
			{
				add( new Flipper(*this, c.mCenter, c.mRadius, PartType::FlipperLeft) );
			}
//			else if ( closeToLeft && closeToRight && closeToBottom )
//			{
//				 add( new Plunger( *this, c.mCenter, c.mRadius ) );
//			}
			else
			{
				add( new Bumper( *this, c.mCenter, c.mRadius, adjSpace ) );
			}
		} // hole
		else
		{
			// non-hole:
			
			// make target
			auto filter = [this,contours]( const Contour& c ) -> bool {
				return !shouldContourBeAPart(c,contours);
			};
			
			float dist;
			vec2 closestPt;
			
			
			
			vec2 lightLoc = c.mCenter;
			float r = mPartParams.mTargetRadius;
			
			const float kMaxDist = mPartParams.mTargetDynamicRadius*4.f;
			
			if ( contours.findClosestContour(c.mCenter,&closestPt,&dist,filter)
			  && closestPt != c.mCenter
			  && dist < kMaxDist )
			{
				vec2 dir = normalize( closestPt - c.mCenter );

				float far=0.f;
				
				if (mPartParams.mTargetDynamicRadius)
				{
					float r = distance(closestPt,c.mCenter) - c.mRadius;
					
					r = constrain(r, mPartParams.mTargetRadius, mPartParams.mTargetRadius*4.f );
					r = mPartParams.mTargetRadius;
					
					far = r;
				}
				
				lightLoc = closestPt + dir * (r+far);
				
				vec2 triggerLoc = closestPt;
				vec2 triggerVec = dir;
				
				auto rt = new Target(*this,triggerLoc,triggerVec,lightLoc,r);
				rt->mContourPoly = c.mPolyLine;
				
				add( rt );
			}
		}
		
	} // for
	
	return parts;
}

PartRef PinballWorld::findPartForContour( const Contour& c ) const
{
	return findPartForContour( getContours().getIndex(c) );
}

PartRef PinballWorld::findPartForContour( int contouridx ) const
{
	auto i = mContoursToParts.find(contouridx);
	
	if (i==mContoursToParts.end()) return 0;
	else return i->second;
}

PartVec
PinballWorld::mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const
{
	PartVec parts = newParts;
	
	// match old parts to new ones
	for( PartRef &p : parts )
	{
		// does it match an old part?
		bool wasMatched = false;
		
		for( const PartRef& old : oldParts )
		{
			if ( !old->getShouldAlwaysPersist() && p->getShouldMergeWithOldPart(old) )
			{
				// matched.
				bool replace=true;
				
				// but...
				// replace=false
				
				// replace with old.
				// (we are simply shifting pointers rather than copying contents, but i think this is fine)
				if (replace) {
					p = old;
					wasMatched = true;
				}
			}
		}
		
		//
		if (!wasMatched) p->onGameEvent(GameEvent::NewPart);
	}
	
	// carry forward any old parts that should be
	for( const PartRef &p : oldParts )
	{
		if (p->getShouldAlwaysPersist())
		{
			bool doIt=true;
			
			// are we an expired trigger?
			/*
			if ( p->getType()==PartType::Target )
			{
				auto rt = dynamic_cast<Target*>(p.get());
				if ( rt && !isValidRolloverLoc( rt->mLoc, rt->mRadius, oldParts ) )
				{
//					doIt=false;
				}
			}*/
			// this whole concept of procedurally generating and retiring them is deprecated
			
			if (doIt) parts.push_back(p);
		}
	}
	
	return parts;
}

void PinballWorld::getContoursFromParts( const PartVec& parts, ContourVec& contours, ContourToPartMap* c2p ) const
{
	if (c2p) c2p->clear();
	
	for( const PartRef p : parts )
	{
		PolyLine2 poly = p->getCollisionPoly();
		
		if (poly.size()>0)
		{
			addContourToVec( contourFromPoly(poly), contours );
			
			if (c2p) (*c2p)[contours.size()-1] = p;
		}
	}
}
/*
bool PinballWorld::isValidRolloverLoc( vec2 loc, float r, const PartVec& parts ) const
{
	// outside?
	if ( ! mVisionContours.findLeafContourContainingPoint(loc) ) {
		return false;
	}

	// too close to edge?
	float closestContourDist;
	if ( mVisionContours.findClosestContour(loc,0,&closestContourDist)
	  && closestContourDist < mPartParams.mTargetMinWallDist + r )
	{
		return false;
	}
	
	// parts
	for( const auto &p : parts ) {
		if ( !p->isValidLocForTarget(loc,r) ) return false;
	}
	
	// ok
	return true;
}

void PinballWorld::rolloverTest()
{
	const Rectf playfieldbb = toPlayfieldBoundingBox(getWorldBoundsPoly());
	
	Rand rgen(0);
	
	for ( int i=0; i<200; ++i )
	{
		vec2 p( rgen.nextFloat(), rgen.nextFloat() );
		
		p.x = lerp( playfieldbb.x1, playfieldbb.x2, p.x );
		p.y = lerp( playfieldbb.y1, playfieldbb.y2, p.y );
		
		p = fromPlayfieldSpace(p);
		float r = mPartParams.mTargetRadius;
		
		gl::color( isValidRolloverLoc(p, r, mParts) ? Color(0,1,0) : Color(1,0,0) );
		gl::drawSolidCircle(p, r);
	}
}*/

Contour PinballWorld::contourFromPoly( PolyLine2 poly ) const
{
	Contour c;
	c.mPolyLine = poly;
	c.mCenter = poly.calcCentroid();
	c.mBoundingRect = Rectf( poly.getPoints() );
	c.mRadius = max( c.mBoundingRect.getWidth(), c.mBoundingRect.getHeight() ) * .5f ;
	c.mArea = M_PI * c.mRadius * c.mRadius; // use circle approximation for area

	c.mPolyLine.setClosed(); // ensure this is true in case any parts fail to do this right (ray casting cares) 
	
	// TODO: rotated bounding box correctly
	// just put in a coarse approximation for now in case it starts to matter
	c.mRotatedBoundingRect.mCenter = c.mBoundingRect.getCenter();
	c.mRotatedBoundingRect.mAngle  = 0.f;
	c.mRotatedBoundingRect.mSize = c.mBoundingRect.getSize(); // not sure if i got rotation/size semantics right
	
	return c;
}

void PinballWorld::addContourToVec( Contour c, ContourVec& contours ) const
{
	// ideally, BallWorld has an intermediate input representation of shapes to simulate, and we add to that.
	// but for now, we do this and use contour vec itself
	
	// stitch into polygon hierarchy
	c.mIsHole = true;
	Contour* parent = contours.findLeafContourContainingPoint(c.mCenter); // uh... a bit sloppy, but it should work enough
	
	// make sure we are putting a hole in a non-hole
	// (e.g. flipper is made by a hole, and so we don't want to be a non-hole in that hole, but a sibling to it)
	if (parent && parent->mIsHole) {
		parent = contours.getParent(*parent);
	}

	// link it in
	if (parent) {
		c.mParent = parent - &contours[0]; // point to parent
		c.mIsHole = true;
		c.mTreeDepth = parent->mTreeDepth + 1;
		c.mIsLeaf = true;
		c.mIndex = contours.size();
		parent->mChild.push_back(contours.size()); // have parent point to where we will be
		contours.push_back(c);
	}
}

// Synthesis
void PinballWorld::setupSynthesis()
{
	// Register file-watchers for all the major pd patch components
	auto app = PaperBounce3App::get();

	mPd = app->mPd;

	std::vector<fs::path> paths =
	{
		app->hotloadableAssetPath("synths/PinballWorld/pinball-world.pd")
	};

	// Load pinball synthesis patch
	if (mSoundEnabled)
	{
		mFileWatch.load( paths, [this,app]()
		{
			// Reload the root patch
			auto rootPatch = app->hotloadableAssetPath("synths/PinballWorld/pinball-world.pd");
			mPd->closePatch(mPatch);
			mPatch = mPd->loadPatch( DataSourcePath::create(rootPatch) ).get();
		});
	}
	else
	{
		mPd->closePatch(mPatch);
		mPatch=0;
	}
}

void PinballWorld::shutdownSynthesis() {
	// Close pinball synthesis patch
	mPd->closePatch(mPatch);
}
