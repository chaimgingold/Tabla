//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "TablaApp.h"
#include "glm/glm.hpp"
#include "PinballWorld.h"
#include "PinballParts.h"
#include "geom.h"
#include "cinder/Rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Pinball;

//const bool kLogButtons = true; // we will need them to configure new controllers :P

static GameCartridgeSimple sCartridge("PinballWorld", [](){
	return std::make_shared<PinballWorld>();
});

PinballWorld::PinballWorld()
	: mView(*this)
	, mVision(*this)
	, mInput(*this)
{
	setupSynthesis();
	mView.setup();

	mFlipperState[0] = 0.f;
	mFlipperState[1] = 0.f;
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
	
	if ( xml.hasChild("PinballVision") ) {
		mVision.setParams( xml.getChild("PinballVision") );
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

	
	if ( xml.hasChild("Input") ) {
		mInput.setParams( xml.getChild("Input") );
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
	
	for( auto p : mVisionOutput.mParts ) {
		p->onGameEvent(e);
	}
}

void PinballWorld::onGameEvent( GameEvent e )
{
	switch(e)
	{
		case GameEvent::NewGame:
			mScore=0;
			mBallsLeft=3;
			mTargetCount=0;
			break;
		
		case GameEvent::GameOver:
			break;
		
		case GameEvent::LostBall:
//			addScreenShake(1.f);
			break;
			
		case GameEvent::LostLastMultiBall:
			mPd->sendBang("game-over");
			beginParty(PartyType::GameOver);
			if (mBallsLeft==0) sendGameEvent(GameEvent::GameOver);
			break;
			
		case GameEvent::ServeMultiBall:
			addToScore(10000);
			mPd->sendBang("multi-ball");
			beginParty(PartyType::Multiball); // trigger shader effect
			break;
			
		case GameEvent::ServeBall:
			if (mBallsLeft>0) mBallsLeft--;
			else sendGameEvent(GameEvent::NewGame); 
			mPd->sendBang("serve-ball");
			break;

		case GameEvent::ATargetTurnedOn:
			mTargetCount++;
			mPd->sendFloat("rollover-count", mTargetCount);
			break;
			
		default:break;
	}
}

void PinballWorld::addToScore( int pts )
{
	mScore += pts;
	
	mHighScore = max( mScore, mHighScore );
}

void PinballWorld::beginParty( PartyType type ) {
	mPartyBegan = (float)ci::app::getElapsedSeconds();
	mPartyType = type;
}

vec2 PinballWorld::getPartyLoc() const
{
	if (mPartyType==PartyType::GameOver) return vec2(1, 0.5);
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
	for( auto p : mVisionOutput.mParts ) p->updateCensus(mPartCensus);
	
	// input
	mInput.tick();
	tickFlipperState();

	// sim
	if (!isPaused())
	{
		// enter multiball?
		{
			int numTargets = getPartCensus().getPop(PartType::Target);
			
			if ( numTargets>0 && getPartCensus().mNumTargetsOn==numTargets ) {
				// start multi-ball
				serveBall();
			}
		}
	
		if (!isPaused()) updateScreenShake();
		
		// tick parts
		for( auto p : mVisionOutput.mParts ) p->tick();

		// update contours, merging in physics shapes from our parts
		updateBallWorldContours();
	
		//
		cullBalls();

		// gravity
		vector<Ball>& balls = getBalls();
		for( Ball &b : balls ) {
			b.mAccel += getGravityVec() * mGravity;
		}
		
		// balls
		BallWorld::update();

		// respond to collisions
		processCollisions();
	}
		// TODO: To make pinball flippers super robust in terms of tunneling (especially at the tips),
		// we should make attachments for BallWorld contours (a parallel vector)
		// that specifies angular rotation (center, radians per second)--basically what
		// Flipper::getAccelForBall does, and have BallWorld do more fine grained rotations of
		// the contour itself. So we'd set the flipper to its rotation for the last frame,
		// and specify the rotation to be the rotation that will get us to its rotation for this frame,
		// and BallWorld will handle the granular integration itself.
		
	updateBallSynthesis();
	
	mView.update();
}

void PinballWorld::updateBallSynthesis() {
	vector<Ball>& balls = getBalls();
	
	auto ballVels = pd::List();

	// Send no velocities when paused to keep balls silent
	bool shouldSynthesizeBalls = !isPaused();

//	const float scale = 10.f / (float)balls.size();
	const float scale = 10.f;
	
	if (shouldSynthesizeBalls) {
		for( Ball &b : balls ) {
//			cout << length(getDenoisedBallVel(b)) << endl;
			ballVels.addFloat(length(getDenoisedBallVel(b)) * scale);
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
}

void PinballWorld::serveBallCheat()
{
	serveBall();
}

void PinballWorld::serveBallIfNone()
{
	if ( getBalls().empty() && !getIsInGameOverState() ) 
	{
		serveBall();
	}
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
			int fraci = mInput.isFlipperDown(i) ? 0 : 1;
			
			mFlipperState[i] = lerp( mFlipperState[i], mInput.isFlipperDown(i) ? 1.f : 0.f, frac[fraci] );
		}
		else
		{
			
			// linear, to help me debug physics
			float step = (1.f / 60.f) * ( 1.f / .1f );
			
			if ( mInput.isFlipperDown(i) ) mFlipperState[i] += step;
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
	const float radPerFrame = (M_PI/2.f) / 6.f; // assume 6 steps per 90 deg of motion
	const float sign[2] = {1.f,-1.f};
	
	if ( mInput.isFlipperDown(side) && mFlipperState[side] < 1.f-eps ) return radPerFrame * sign[side];
	else if ( !mInput.isFlipperDown(side) && mFlipperState[side] > eps ) return -radPerFrame * sign[side];
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

void PinballWorld::keyDown( KeyEvent event )
{
	mInput.keyDown(event);
}

void PinballWorld::keyUp( KeyEvent event )
{
	mInput.keyUp(event);
}

void PinballWorld::mouseClick( vec2 loc )
{
}

void PinballWorld::updateVision( const Vision::Output& visionOut, Pipeline& p )
{
	// run our vision
	mVisionOutput = mVision.update( visionOut, p, mVisionOutput );
	
	// playfield layout
	updatePlayfieldLayoutWithVisionContours();	
	
	// log cube maps, allow it to capture images
	mView.updateVision(p);
}

PinballVision::ContourType
PinballWorld::getVisionContourType( const Contour& c ) const
{
	assert( c.mIndex >= 0 && c.mIndex < mVisionOutput.mContours.size() );
	
	return mVisionOutput.mContourTypes[c.mIndex];
}

void PinballWorld::getCullLine( vec2 v[2] ) const
{
	v[0] = fromPlayfieldSpace(vec2(mPlayfieldBallReclaimX[0],mPlayfieldBallReclaimY));
	v[1] = fromPlayfieldSpace(vec2(mPlayfieldBallReclaimX[1],mPlayfieldBallReclaimY));
}

Rectf PinballWorld::calcPlayfieldBoundingBox() const
{
	vector<vec2> pts;
	
	for( const auto& c : mVisionOutput.mContours )
	{
		if (c.mTreeDepth==0 && getVisionContourType(c)==PinballVision::ContourType::Space)
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

void PinballWorld::updatePlayfieldLayoutWithVisionContours()
{
	mPlayfieldBoundingBox = calcPlayfieldBoundingBox();
//	cout << "min/max y: " << mPlayfieldBoundingBox.y1 << " " << mPlayfieldBoundingBox.y2 << endl;
	mPlayfieldBallReclaimY = mPlayfieldBoundingBox.y1 + mBallReclaimAreaHeight;
	
	mPlayfieldBallReclaimX[0] = mPlayfieldBoundingBox.x1;
	mPlayfieldBallReclaimX[1] = mPlayfieldBoundingBox.x2;
	
	float t;
	vec2 vleft  = vec2(mPlayfieldBallReclaimX[0],mPlayfieldBallReclaimY);
	vec2 vright = vec2(mPlayfieldBallReclaimX[1],mPlayfieldBallReclaimY);
	if ( mVisionOutput.mContours.rayIntersection( fromPlayfieldSpace(vleft), getRightVec(), &t ) )
	{
		mPlayfieldBallReclaimX[0] = (vleft + toPlayfieldSpace(getRightVec() * t)).x;
	}
	
	if ( mVisionOutput.mContours.rayIntersection( fromPlayfieldSpace(vright), getLeftVec(), &t ) )
	{
		mPlayfieldBallReclaimX[1] = (vright + toPlayfieldSpace(getLeftVec() * t)).x;
	}
}

void PinballWorld::updateBallWorldContours()
{
	// modify vision out for ball world
	// (we add contours to convey the new shapes to simulate)
	ContourVec physicsContours = mVisionOutput.mContours;

	getContoursFromParts( mVisionOutput.mParts, physicsContours, &mContoursToParts );
	
	// which contours to use?
	// (we could move this computation into updateVision(), but its cheap and convenient to do it here)
	ContourVec::Mask mask(physicsContours.size(),true); // all true...
	for( int i=0; i<mVisionOutput.mContours.size(); ++i )
	{
		// only contours that define space
		mask[i] = getVisionContourType(mVisionOutput.mContours[i]) == PinballVision::ContourType::Space;
	}
	
	// tell ball world
	BallWorld::setContours(physicsContours,ContourVec::getMaskFilter(mask));
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

Contour PinballWorld::contourFromPoly( PolyLine2 poly ) const
{
	Contour c;
	c.mPolyLine = poly;
	c.mCenter = poly.calcCentroid();
	c.mBoundingRect = Rectf( poly.getPoints() );
	c.mRadius = max( c.mBoundingRect.getWidth(), c.mBoundingRect.getHeight() ) * .5f ;
	c.mArea = M_PI * c.mRadius * c.mRadius; // use circle approximation for area
	c.mPerimeter = 1.f ; // pinball world doesn't care about this... :D
	
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
	auto app = TablaApp::get();

	mPd = app->getPd();

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
