//
//  PongWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "PongWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"

PongWorld::PongWorld()
{
	mStateEnterTime = ci::app::getElapsedSeconds();
	
	mState = GameState::Attract;
	
	mPlayerColor[0] = Color(1,0,0);
	mPlayerColor[1] = Color(0,0,1);
	
	mPlayerScore[0] = mPlayerScore[1] = 0;
	
	mMaxScore=5;

	setupSynthesis();
}

vec2 PongWorld::fieldCorner( int i ) const
{
	int j = i + mFieldPlayer0LeftCornerIndex;
	
	return getWorldBoundsPoly().getPoints()[ j % getWorldBoundsPoly().getPoints().size() ];
}

void PongWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PongWorld::update()
{
	switch (mState)
	{
		// start a game?
		case GameState::Attract:
		{
			goToState( GameState::Serve );
		}
		break;
		
		// serve?
		case GameState::Serve:
		{
			if ( getSecsInState() > .5f )
			{
				// clear old paused ball
				clearBalls();

				if ( getBalls().empty() )
				{
					serve();
					goToState( GameState::Play );
				}
			}
		}
		break;

		//
		case GameState::Play:
		{
			BallWorld::update();
			// collision handlers will catch scoring
		}
		break;
		
		case GameState::Score:
		{
			strobeBalls();

			if ( getSecsInState() > 1.5f )
			{
				if ( mPlayerScore[0] == mMaxScore || mPlayerScore[1] == mMaxScore )
				{
					goToState( GameState::Over );
				}
				else
				{
					goToState( GameState::Serve );
				}
			}
		}
		break;
		
		case GameState::Over:
		{
			clearBalls();
			
			if ( getSecsInState() > 2.f )
			{
				goToState( GameState::Attract );
			}
		}
		break;
	}
}

string PongWorld::getStateName( GameState s ) const
{
	switch (s)
	{
		case GameState::Attract: return "Attract"; break;
		case GameState::Serve: return "Serve"; break;
		case GameState::Play: return "Play"; break;
		case GameState::Score: return "Score"; break;
		case GameState::Over: return "Over"; break;
	}
}

float PongWorld::getSecsInState() const
{
	return ci::app::getElapsedSeconds() - mStateEnterTime;
}

void PongWorld::goToState( GameState s )
{
	GameState old = mState;
	
	mState = s;
	mStateEnterTime = ci::app::getElapsedSeconds();
	
	stateDidChange(old,s);
}

void PongWorld::stateDidChange( GameState old, GameState newState )
{
	if (1) cout << "stateDidChange " << getStateName(old) << " => " << getStateName(newState) << endl;
	
	switch (newState)
	{
		case GameState::Attract:
		{
			mPlayerScore[0]=0;
			mPlayerScore[1]=0;
		}
		break;
		
		case GameState::Serve:
		{
		}
		break;

		case GameState::Play:
		{
		}
		break;
		
		case GameState::Score:
		{
		}
		break;
		
		case GameState::Over:
		{
			mPureDataNode->sendBang("new-game");
		}
		break;
	}
}

void PongWorld::onBallBallCollide   ( const Ball&, const Ball& )
{

	if (0) cout << "ball ball collide" << endl;
}

void PongWorld::onBallContourCollide( const Ball&, const Contour& )
{
	mPureDataNode->sendBang("hit-object");

	if (0) cout << "ball contour collide" << endl;
}

void PongWorld::onBallWorldBoundaryCollide	( const Ball& b )
{
	mPureDataNode->sendBang("hit-wall");

	if (0) cout << "ball world collide" << endl;

	// which player side did it hit?
	for( int i=0; i<2; i++ )
	{
		vec2 c = closestPointOnLineSeg( b.mLoc, mPlayerSide[i][0], mPlayerSide[i][1] );
		
		// in goal
		if ( glm::distance(c,b.mLoc) <= b.mRadius * 2.f ) // a little hacky, but it works.
		{
			didScore( 1 - i );
		}
	}
}

void PongWorld::didScore( int player )
{
	mPureDataNode->sendFloat("scored-point", player);

	cout << "didScore " << player << endl;

	mPlayerScore[player] = ( mPlayerScore[player] + 1 ) % (mMaxScore + 1);
	
	mPlayerJustScored=player;
	goToState(GameState::Score);
}

void PongWorld::serve()
{
	Ball ball ;
	
	ball.mColor = Color(1,0,0);
	ball.setLoc( mCenter ) ;
	ball.mRadius = 2.f;
	ball.setMass( M_PI * powf(ball.mRadius,3.f) ) ;
	
	ball.mCollideWithContours = false;
	
	ball.setVel( Rand::randVec2() * ball.mRadius/2.f ) ;
	
	getBalls().push_back( ball ) ;
}

void PongWorld::draw( bool highQuality )
{
	//
	// Draw field layout markings
	//
	{
		gl::color(0,.8,0,1.f);
		
		PolyLine2 wb = getWorldBoundsPoly();
		wb.setClosed();
		gl::draw( wb );
		
		gl::drawLine( mCenterLine[0], mCenterLine[1] );
		
		gl::drawStrokedCircle(mCenter, mFieldCenterCircleRadius);
		
		gl::color(1,0,0);
		for( int i=0; i<2; ++i )
		{
			gl::drawLine(mPlayerSide[i][0] + mPlayerVec[1-i], mPlayerSide[i][1] + mPlayerVec[1-i]);
		}
	}
	
	//
	// Draw world
	//
	BallWorld::draw(highQuality);
	
	//
	// Draw Score/UI
	//
	for( int i=0; i<2; ++i )
	{
		drawScore( i, mScoreDotStart[i], mScoreDotStep[i], mScoreDotRadius, mPlayerScore[i], mMaxScore );
	}
}

void PongWorld::worldBoundsPolyDidChange()
{
	computeFieldLayout();
}

void PongWorld::computeFieldLayout()
{
	assert( getWorldBoundsPoly().size()==4 );
	
	mFieldPlayer0LeftCornerIndex=1;
		// hard code this for now, since it is just right
		// we should probably want to move this to xml :P
		// whatever, it's all hacky.
	
	mCenterLine[0] = lerp(fieldCorner(0), fieldCorner(1), .5f);
	mCenterLine[1] = lerp(fieldCorner(2), fieldCorner(3), .5f);
	mCenter = lerp( mCenterLine[0], mCenterLine[1], .5f );
	
	mCenterVec = normalize( mCenterLine[0] - mCenterLine[1] );
	
	mPlayerSide[0][0] = fieldCorner(0);
	mPlayerSide[0][1] = fieldCorner(3);
	mPlayerSide[1][0] = fieldCorner(1);
	mPlayerSide[1][1] = fieldCorner(2);
	
	mPlayerGoal[0] = lerp(fieldCorner(0), fieldCorner(3), .5f);
	mPlayerGoal[1] = lerp(fieldCorner(1), fieldCorner(2), .5f);
	
	mPlayerVec[1] = normalize( mPlayerGoal[1] - mPlayerGoal[0] );
	mPlayerVec[0] = -mPlayerVec[1];
	
	mFieldCenterCircleRadius = glm::distance( mCenterLine[0], mCenterLine[1] ) * .15f;


	mScoreDotRadius = 1.f; // 1mm
	float scoreDotGutter = mScoreDotRadius * 1.5f ;
	
	for ( int i=0; i<2; ++i )
	{
		mScoreDotStep[i] = mPlayerVec[i] * (mScoreDotRadius + scoreDotGutter);

		mScoreDotStart[i] = mCenterLine[0]
			+ mScoreDotStep[i]
			- mCenterVec    * (mScoreDotRadius + scoreDotGutter);
		
//		mScoreDotStart[0] = mCenterLine[1] + mPlayerVec[0] * mScoreDotStep[0].x + mCenterVec * mScoreDotStep[0].y;
//		mScoreDotStart[1] = mCenterLine[1] + mPlayerVec[1] * mScoreDotStep[1].x + mCenterVec * mScoreDotStep[1].y;
	}
}

void PongWorld::drawScore( int player, vec2 dotStart, vec2 dotStep, float dotRadius, int score, int maxScore ) const
{
	float a=1.f;
	
	if ( ( mState==GameState::Score && player==mPlayerJustScored ) ||
	     ( mState==GameState::Over) )
	{
		float kFreq = .35f;
		
		float t = getSecsInState();
		
		a = (t/kFreq) - floorf(t/kFreq) ;
		
		if ( a<.5 ) a=0.f;
	}


	const int   kNumSegments = 12;
	const float kStrokeWidth = .2f;
	
	gl::color( ColorA( mPlayerColor[player],a) );
	
	for( int i=0; i<maxScore; ++i )
	{
		vec2 c = dotStart + dotStep * (float)i;
		
		// full
		if ( i <= score-1 )
		{
			gl::drawSolidCircle(c,dotRadius,kNumSegments);
		}
		// empty
		else
		{
			gl::drawStrokedCircle(c,dotRadius,kStrokeWidth,kNumSegments);
		}
	}
}

void PongWorld::strobeBalls()
{
	float kFreq = .35f;
	
	float t = getSecsInState();
	
	float a = (t/kFreq) - floorf(t/kFreq) ;
//	float a = (t - roundf( t / kFreq )) > .5f ? 1.f : 0.f ;
	
	for( auto &b : getBalls() )
	{
		b.mColor.a = a;
	}
}

// Synthesis
void PongWorld::setupSynthesis()
{
	auto ctx = audio::master();
	
	// Create the synth engine
	mPureDataNode = ctx->makeNode( new cipd::PureDataNode( audio::Node::Format().autoEnable() ) );
	
	// Enable Cinder audio
	ctx->enable();
	
	// Load pong synthesis
	mPureDataNode->disconnectAll();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
	
	// Connect synth to master output
	mPureDataNode >> audio::master()->getOutput();
}

PongWorld::~PongWorld() {
	// Clean up synth engine
	mPureDataNode->disconnectAll();
	mPureDataNode->closePatch(mPatch);
}