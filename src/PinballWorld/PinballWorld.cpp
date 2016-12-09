//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "PinballWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "Gamepad.h"
#include "xml.h"


PinballWorld::PinballWorld()
{
	setupSynthesis();

	mIsFlipperDown[0] = false;
	mIsFlipperDown[1] = false;
	
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

void PinballWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PinballWorld::update()
{
	mGamepadManager.tick();

	BallWorld::update();
	
	if ( getBalls().size() < 2 )
	{
		newRandomBall( getRandomPointInWorldBoundsPoly() ).mCollideWithContours = true;
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
	// balls
	BallWorld::draw(drawType);
	
	// flippers, etc...
	drawParts();

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
//	drawFlipperOrientationRays();
}

void PinballWorld::drawFlipperOrientationRays() const
{
	for( auto &p : mParts )
	{
		pair<float,float> space = getAdjacentLeftRightSpace(p.mLoc, getContours());
		
		gl::color(1,0,0);
		gl::drawLine(p.mLoc, p.mLoc + getRightVec() * space.second );

		gl::color(0,1,0);
		gl::drawLine(p.mLoc, p.mLoc + getLeftVec() * space.first );
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
	const float kFlipperLength = 5.f;
	
	for( const auto &p : mParts )
	{
		switch(p.mType)
		{
			case Part::Type::FlipperLeft:
			case Part::Type::FlipperRight:
			{
				bool isDown = false;
				vec2 f2;
				float flipperLength = max( p.mRadius*2.f, kFlipperLength );
				
				if ( p.mType == Part::Type::FlipperLeft)
				{
					isDown = mIsFlipperDown[0];
					f2 = p.mLoc + getRightVec() * flipperLength;
				}
				else
				{
					isDown = mIsFlipperDown[1];
					f2 = p.mLoc + getLeftVec() * flipperLength;
				}

				if (isDown) f2 += getUpVec() * flipperLength;
				
				gl::color( ColorA(0,1,1,1) );
				gl::drawSolidCircle(p.mLoc, p.mRadius);
				gl::drawLine(p.mLoc,f2);
			}
			break;
			
			case Part::Type::Bumper:
			{
				gl::color(1,0,0);
				gl::drawStrokedCircle(p.mLoc,p.mRadius);
			}
			break;
		}
		
	}
}

// Vision
void PinballWorld::updateVision( const ContourVector &c, Pipeline& p )
{
	BallWorld::updateVision(c,p);
	
	mParts = getPartsFromContours(c);
}

pair<float,float> PinballWorld::getAdjacentLeftRightSpace( vec2 loc, const ContourVector& cs ) const
{
	const Contour* c = cs.findLeafContourContainingPoint(loc);

	if (c&&c->mIsHole) c = cs.getParent(c); // make sure we aren't the hole contour
	
	pair<float,float> result(0.f,0.f);
	
	if (c)
	{
		c->rayIntersection(loc,getRightVec(),&result.second);
		c->rayIntersection(loc,getLeftVec(),&result.first);
		// 0.f result remains if hit fails
	}
	
	return result;
}

PinballWorld::PartVec PinballWorld::getPartsFromContours( const ContourVector& contours ) const
{
	const float kFlipperMinRadius = 1.f;
	
	PinballWorld::PartVec parts;
	
	for( const auto &c : contours )
	{
		if ( c.mIsHole && c.mRadius < mPartMaxContourRadius )
		{
			Part p;
			
			// location
			p.mLoc = c.mCenter;
			p.mRadius = c.mRadius;
			
			// flipper orientation
			pair<float,float> adjSpace = getAdjacentLeftRightSpace(p.mLoc,contours);
			adjSpace.first -= c.mRadius;
			adjSpace.second -= c.mRadius;
			
			if      (adjSpace.second < adjSpace.first  && adjSpace.second < mFlipperDistToEdge)
			{
				p.mType = Part::Type::FlipperRight;
				p.mRadius = max( p.mRadius, kFlipperMinRadius );
//				p.mRadius = kFlipperMinRadius;
			}
			else if (adjSpace.first  < adjSpace.second && adjSpace.first  < mFlipperDistToEdge)
			{
				p.mType = Part::Type::FlipperLeft;
//				p.mRadius = max( p.mRadius, kFlipperMinRadius );
//				p.mRadius = kFlipperMinRadius;
			}
//			else if (adjSpace.first  < 1.f) // < epsilon
//			{
				// zero!
				// plunger?
				// something else?
//				p.mType = Part::Type::FlipperLeft;
//			}
			else
			{
				p.mType = Part::Type::Bumper;
				p.mRadius = min( max(p.mRadius,mBumperRadius),
								 min(adjSpace.first,adjSpace.second)
								 );
				
				// equal, and there really is space, so just pick something.
//				p.mType = Part::Type::FlipperLeft;
				
				// we could have a fuzzier sense of equal (left > right + kDelta),
				// which would allow us to have something different at the center of a space (where distance is equal-ish)
				// (like a bumper!)
				// or even a rule that said left is only when a left edge is within x space at left (eg), so very tight proximity rules.
			}
			
			parts.push_back(p);
		}
	}
	
	return parts;
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