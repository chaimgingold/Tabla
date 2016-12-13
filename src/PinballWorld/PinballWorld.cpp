//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "glm/glm.hpp"
#include "PinballWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "Gamepad.h"
#include "xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;

PinballWorld::PinballWorld()
{
	setupSynthesis();

	mIsFlipperDown[0] = false;
	mIsFlipperDown[1] = false;
	
	mFlipperState[0] = 0.f;
	mFlipperState[1] = 0.f;
	
	auto button = [this]( unsigned int id, string postfix )
	{
		auto i = mGamepadButtons.find(id);
		if (i!=mGamepadButtons.end())
		{
			string key = i->second + postfix;
			
			cout << "-> " << key << endl;
			
			auto j = mGamepadFunctions.find(key);
			if (j!=mGamepadFunctions.end())
			{
				j->second();
			}
		}
	};
	
	mGamepadManager.mOnButtonDown = [this,button]( const GamepadManager::Event& event )
	{
		cout << "down " << event.mId << endl;
		button(event.mId,"-down");
		
		if (getBalls().empty()) serveBall();
	};

	mGamepadManager.mOnButtonUp = [this,button]( const GamepadManager::Event& event )
	{
		cout << "up "  << event.mId << endl;
		button(event.mId,"-up");
	};
	
	// inputs
	mInputToFunction["flippers-left"] = []()
	{
		cout << "flippers-left" << endl;
	};

	mInputToFunction["flippers-right"] = []()
	{
		cout << "flippers-right" << endl;
	};
	
	mGamepadFunctions["flippers-left-down"]  = [this]() { mIsFlipperDown[0] = true; cout << "boo" << endl; };
	mGamepadFunctions["flippers-left-up"]    = [this]() { mIsFlipperDown[0] = false; };
	mGamepadFunctions["flippers-right-down"] = [this]() { mIsFlipperDown[1] = true; };
	mGamepadFunctions["flippers-right-up"]   = [this]() { mIsFlipperDown[1] = false; };
}

PinballWorld::~PinballWorld()
{
	shutdownSynthesis();
}

void PinballWorld::setParams( XmlTree xml )
{
	BallWorld::setParams(xml);
	
	getXml(xml, "UpVec", mUpVec );
	getXml(xml, "FlipperDistToEdge", mFlipperDistToEdge );
	getXml(xml, "BumperRadius", mBumperRadius );
	getXml(xml, "PartMaxContourRadius", mPartMaxContourRadius );
	getXml(xml, "Gravity", mGravity );
	getXml(xml, "BallReclaimAreaHeight", mBallReclaimAreaHeight );
	
	// gamepad
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

void PinballWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PinballWorld::update()
{
	// input
	mGamepadManager.tick();

	// gravity
	vector<Ball>& balls = getBalls();
	for( Ball &b : balls ) {
		b.mAccel += getGravityVec() * mGravity;
	}
	
	// balls
	cullBalls();
	BallWorld::update();
	
	// flippers
	tickFlippers();
}

void PinballWorld::serveBall()
{
	// for now, from the top
	float fx = Rand::randFloat();
	
	vec2 loc = fromPlayfieldSpace(
		vec2( lerp(mPlayfieldBoundingBox.x1,mPlayfieldBoundingBox.x2,fx), mPlayfieldBoundingBox.y2 )
		);
	
	newRandomBall(loc).mCollideWithContours = true;
}

void PinballWorld::cullBalls()
{
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
}

void PinballWorld::tickFlippers()
{
	for( int i=0; i<2; ++i )
	{
		float frac[2] = { .35f, .5f };
		int fraci = mIsFlipperDown[i] ? 0 : 1;
		
		mFlipperState[i] = lerp( mFlipperState[i], mIsFlipperDown[i] ? 1.f : 0.f, frac[fraci] );
	}
}

void PinballWorld::onBallBallCollide   ( const Ball&, const Ball& )
{
	mPureDataNode->sendBang("new-game");

	if (0) cout << "ball ball collide" << endl;
}

void PinballWorld::onBallContourCollide( const Ball&, const Contour& )
{
	mPureDataNode->sendBang("hit-object");

	if (0) cout << "ball contour collide" << endl;
}

void PinballWorld::onBallWorldBoundaryCollide	( const Ball& b )
{
	mPureDataNode->sendBang("hit-wall");

	if (0) cout << "ball world collide" << endl;
}

void PinballWorld::draw( DrawType drawType )
{
	// playfield markers
	gl::color(1,0,0);
	gl::drawLine(
		fromPlayfieldSpace(vec2(mPlayfieldBoundingBox.x1,mPlayfieldBallReclaimY)),
		fromPlayfieldSpace(vec2(mPlayfieldBoundingBox.x2,mPlayfieldBallReclaimY)) ) ;
	
	// balls
	BallWorld::draw(drawType);
	
	// flippers, etc...
	drawParts();



	// --- debugging/testing ---

	// world orientation debug info
	if (0)
	{
		vec2 c = getWorldBoundsPoly().calcCentroid();
		float l = 10.f;

		gl::color(1,0,0);
		gl::drawLine( c, c + getRightVec() * l );

		gl::color(0,1,0);
		gl::drawLine( c, c + getLeftVec() * l );

		gl::color(0,0,1);
		gl::drawLine( c, c + getUpVec() * l );
	}
	
	// test ray line seg...
	if (0) drawFlipperOrientationRays();

	// test contour generation
	if (0)
	{
		ContourVec cs = getContours();
		int i = cs.size(); // only start loop at new contours we append
		
		getContoursFromParts(mParts,cs);
		
		for( ; i<cs.size(); ++i )
		{
			const auto &c = cs[i];
			
			gl::color(0,.5,.5);
			gl::drawStrokedRect(c.mBoundingRect);

			gl::color(0,1,0);
			gl::draw(c.mPolyLine);
		}
	}
}

void PinballWorld::drawFlipperOrientationRays() const
{
	for( auto &p : mParts )
	{
		tAdjSpace space = getAdjacentLeftRightSpace(p.mLoc, getContours());
		
		gl::color(1,0,0);
		gl::drawLine(p.mLoc, p.mLoc + getRightVec() * space.mRight );

		gl::color(0,1,0);
		gl::drawLine(p.mLoc, p.mLoc + getLeftVec() * space.mLeft );
	}
}

void PinballWorld::worldBoundsPolyDidChange()
{
}

void PinballWorld::keyDown( KeyEvent event )
{
	char c = event.getChar();
	
	auto i = mKeyToInput.find(c);
	
	if (i!=mKeyToInput.end())
	{
		auto j = mInputToFunction.find(i->second);
		if (j!=mInputToFunction.end())
		{
			j->second();
		}
	}
}

void PinballWorld::drawParts() const
{
	for( const auto &p : mParts )
	{
		switch(p.mType)
		{
			case Part::Type::FlipperLeft:
			case Part::Type::FlipperRight:
			{
				gl::color( ColorA(0,1,1,1) );
				gl::drawSolid( p.mPoly );
			}
			break;
			
			case Part::Type::Bumper:
			{
				gl::color(1,0,0);
				gl::drawSolidCircle(p.mLoc,p.mRadius);
				gl::color(1,.8,0);
				gl::drawSolidCircle(p.mLoc,p.mRadius/2.f);
			}
			break;
		}
	}
}

// Vision
Rectf PinballWorld::getPlayfieldBoundingBox( const ContourVec& cs ) const
{
	vector<vec2> pts;
	
	for( const auto& c : cs )
	{
		if (c.mTreeDepth==0)
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

void PinballWorld::updateVision( const ContourVec &c, Pipeline& p )
{
	// playfield layout
	mPlayfieldBoundingBox = getPlayfieldBoundingBox(c);
//	cout << "min/max y: " << mPlayfieldBoundingBox.y1 << " " << mPlayfieldBoundingBox.y2 << endl;
	mPlayfieldBallReclaimY = mPlayfieldBoundingBox.y1 + mBallReclaimAreaHeight;
	
	// generate parts
	PartVec newParts = getPartsFromContours(c);
	mParts = mergeOldAndNewParts(mParts, newParts);

	//
	ContourVec contours = c;
	
	getContoursFromParts(mParts,contours);
	
	// tell ball world
	BallWorld::updateVision(contours,p);
}

PinballWorld::tAdjSpace
PinballWorld::getAdjacentLeftRightSpace( vec2 loc, const ContourVector& cs ) const
{
	const Contour* c = cs.findLeafContourContainingPoint(loc);

	if (c&&c->mIsHole) c = cs.getParent(c); // make sure we aren't the hole contour
	
	tAdjSpace result;
	
	if (c)
	{
		c->rayIntersection(loc,getRightVec(),&result.mRight);
		c->rayIntersection(loc,getLeftVec(),&result.mLeft);
		// 0.f result remains if hit fails
	}
	
	return result;
}

PinballWorld::Part
PinballWorld::getFlipperPart( vec2 pin, float contourRadius, Part::Type type ) const
{
	const float kFlipperMinRadius = 1.f;

	// angle is degrees from down (aka gravity vec)
	float angle;
	{
		int   flipperIndex = type==Part::Type::FlipperLeft ? 0 : 1;
		float angleSign[2] = {-1.f,1.f};
		angle = angleSign[flipperIndex] * toRadians(45.f + mFlipperState[flipperIndex]*90.f);
	}

	
	Part p;
	
	p.mType = type;
	p.mRadius = max( contourRadius, kFlipperMinRadius );
	p.mFlipperLength = max(5.f,p.mRadius) * 1.5f;
	
	p.mLoc = pin;
	p.mFlipperLoc2 = glm::rotate( getGravityVec() * p.mFlipperLength, angle) + p.mLoc;
	
	vec2 c[2] = { p.mLoc, p.mFlipperLoc2 };
	float r[2] = { p.mRadius, p.mRadius/2.f };
	p.mPoly = getCapsulePoly(c,r);
	
	return p;
}

PinballWorld::Part
PinballWorld::getBumperPart( vec2 pin, float contourRadius, tAdjSpace adjSpace ) const
{
	Part p;

	p.mLoc = pin;
	p.mType = Part::Type::Bumper;
	p.mRadius = min( max(contourRadius,mBumperRadius),
					 min(adjSpace.mLeft,adjSpace.mRight)
					 );
	
	p.mPoly = getCirclePoly(pin, p.mRadius);
	
	return p;
}

PinballWorld::PartVec PinballWorld::getPartsFromContours( const ContourVector& contours ) const
{
	PinballWorld::PartVec parts;
	
	for( const auto &c : contours )
	{
		if ( c.mTreeDepth>0 && c.mIsHole && c.mRadius < mPartMaxContourRadius )
		{
			// flipper orientation
			tAdjSpace adjSpace = getAdjacentLeftRightSpace(c.mCenter,contours);
			
			adjSpace.mLeft -= c.mRadius;
			adjSpace.mRight -= c.mRadius;
//			adjSpace.mLeft = max( adjSpace.mLeft, 0.f );
//			adjSpace.mRight = max( adjSpace.mRight, 0.f );
			
			if      (adjSpace.mRight < adjSpace.mLeft  && adjSpace.mRight < mFlipperDistToEdge)
			{
				parts.push_back(
					getFlipperPart(c.mCenter, c.mRadius, Part::Type::FlipperRight)
				);
			}
			else if (adjSpace.mLeft  < adjSpace.mRight && adjSpace.mLeft  < mFlipperDistToEdge)
			{
				parts.push_back(
					getFlipperPart(c.mCenter, c.mRadius, Part::Type::FlipperLeft)
				);
			}
//			else if (adjSpace.mLeft  < 1.f) // < epsilon
//			{
				// zero!
				// plunger?
				// something else?
//				p.mType = Part::Type::FlipperLeft;
//			}
			else
			{
				parts.push_back(
					getBumperPart( c.mCenter, c.mRadius, adjSpace )
				);
				
				// equal, and there really is space, so just pick something.
//				p.mType = Part::Type::FlipperLeft;
				
				// we could have a fuzzier sense of equal (left > right + kDelta),
				// which would allow us to have something different at the center of a space (where distance is equal-ish)
				// (like a bumper!)
				// or even a rule that said left is only when a left edge is within x space at left (eg), so very tight proximity rules.
			}
		}
	}
	
	return parts;
}

PinballWorld::PartVec
PinballWorld::mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const
{
	return newParts;
}

void PinballWorld::getContoursFromParts( const PinballWorld::PartVec& parts, ContourVec& contours ) const
{
	for( const Part &p : parts )
	{
		if (p.mPoly.size()>0)
		{
			addContourToVec( contourFromPoly( p.mPoly ), contours );
		}
	}
}

static int getNumCircleVerts( float r )
{
	const int kMinVerts=8;
	const int kMaxVerts=100;
	const float kVertPerPerimCm=2.f;
	
	return constrain( (int)(M_PI*r*r / kVertPerPerimCm), kMinVerts, kMaxVerts );
}

PolyLine2 PinballWorld::getCirclePoly( vec2 c, float r ) const
{
	const int n = getNumCircleVerts(r);

	PolyLine2 poly;
	
	for( int i=0; i<n; ++i )
	{
		float t = ((float)i/(float)(n-1)) * M_PI*2;
		poly.push_back( vec2(cos(t),sin(t)) );
	}
	
	poly.scale( vec2(1,1) * r );
	poly.offset(c);
	poly.setClosed();
	
	return poly;
}

PolyLine2 PinballWorld::getCapsulePoly( vec2 c[2], float r[2] ) const
{
	PolyLine2 poly;
	
	vec2 a2b = c[1] - c[0];
	vec2 a2bnorm = normalize(a2b);
	
	int numVerts[2] = { getNumCircleVerts(r[0]), getNumCircleVerts(r[1]) };

	// c0
	vec2 p1 = perp(a2bnorm) * r[0];
	mat4 r1 = glm::rotate( (float)(M_PI / ((float)numVerts[0])), vec3(0,0,1) );
	
	poly.push_back(p1+c[0]);
	for( int i=0; i<numVerts[0]; ++i )
	{
		p1 = vec2( r1 * vec4(p1,0,1) );
		poly.push_back(p1+c[0]);
	}

	// c1
	p1 = -perp(a2bnorm) * r[1];
	r1 = glm::rotate( (float)(M_PI / ((float)numVerts[1])), vec3(0,0,1) );
	
	poly.push_back(p1+c[1]);
	for( int i=0; i<numVerts[1]; ++i )
	{
		p1 = vec2( r1 * vec4(p1,0,1) );
		poly.push_back(p1+c[1]);
	}
	
	poly.setClosed();
	
	return poly;
}

Contour PinballWorld::contourFromPoly( PolyLine2 poly ) const
{
	Contour c;
	c.mPolyLine = poly;
	c.mCenter = poly.calcCentroid();
	c.mBoundingRect = Rectf( poly.getPoints() );
	c.mRadius = max( c.mBoundingRect.getWidth(), c.mBoundingRect.getHeight() ) * .5f ;
	c.mArea = M_PI * c.mRadius * c.mRadius; // use circle approximation for area
	return c;
}

void PinballWorld::addContourToVec( Contour c, ContourVec& contours ) const
{
	// stitch into polygon hierarchy
	c.mIsHole = true;
	Contour* parent = contours.findLeafContourContainingPoint(c.mCenter); // uh... a bit sloppy, but it should work enough
	if (parent) {
		c.mParent = parent - &contours[0]; // point to parent
		parent->mChild.push_back(contours.size()); // have parent point to where we will be
		contours.push_back(c);
	}
}

// Synthesis
void PinballWorld::setupSynthesis()
{
	mPureDataNode = cipd::PureDataNode::Global();
	
	// Load pong synthesis patch
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
}

void PinballWorld::shutdownSynthesis() {
	// Close pong synthesis patch
	mPureDataNode->closePatch(mPatch);
}