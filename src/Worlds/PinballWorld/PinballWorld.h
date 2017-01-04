//
//  PinballWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#ifndef PinballWorld_hpp
#define PinballWorld_hpp

#include "BallWorld.h"
#include "PureDataNode.h"
#include "GamepadManager.h"
#include "PinballParts.h"
#include "FileWatch.h"

namespace Pinball
{

class Scene
{
public:
	
	struct Obj
	{
		Obj();
		Obj( TriMeshRef m ) : mMesh(m) {}
		Obj( TriMeshRef m, mat4 x ) : mMesh(m), mTransform(x) {}
		
		TriMeshRef mMesh;
		mat4	   mTransform;
	};
	
	typedef vector<Obj> Meshes;
	
	// by shader
	Meshes mWalls;
	
};

class PinballWorld : public BallWorld
{
public:

	PinballWorld();
	~PinballWorld();

	void setParams( XmlTree ) override;
	
	string getSystemName() const override { return "PinballWorld"; }

	void gameWillLoad() override;
	void update() override;
	void updateVision( const Vision::Output&, Pipeline& ) override;
	void prepareToDraw() override;
	void draw( DrawType ) override;
	
	void worldBoundsPolyDidChange() override;

	void keyDown( KeyEvent ) override;
	void keyUp( KeyEvent ) override;
	void mouseClick( vec2 ) override;

public:

	// world orientation
	vec2 getUpVec() const { return mUpVec; }
	vec2 getLeftVec() const { return vec2(cross(vec3(mUpVec,0),vec3(0,0,1))); }
	vec2 getRightVec() const { return -getLeftVec(); }
	vec2 getDownVec() const { return -mUpVec; }
	vec2 getGravityVec() const { return getDownVec(); } // sugar
	
	float getTableDepth() const { return m3dTableDepth; }
	
	// part params
	float mBumperMinRadius = 5.f;
	float mBumperContourRadiusScale = 1.5f;
	float mBumperKickAccel = 1.f;
	ColorA mBumperOuterColor = ColorA(1,0,0,1);
	ColorA mBumperInnerColor = ColorA(1,.8,0,1);
	ColorA mBumperStrobeColor = ColorA(0,.8,1,1);
	ColorA mBumperOnColor = ColorA(0,.8,1,1);
	
	float mFlipperMinLength=5.f;
	float mFlipperMaxLength=10.f;
	float mFlipperRadiusToLengthScale=5.f;	
	ColorA mFlipperColor = ColorA(0,1,1,1);

	ColorA mRolloverTargetOnColor=Color(1,0,0);
	ColorA mRolloverTargetOffColor=Color(0,1,0);
	ColorA mRolloverTargetStrobeColor=ColorA(1,0,1);

	float mRolloverTargetRadius=1.f;
	float mRolloverTargetMinWallDist=1.f;
	bool  mRolloverTargetDynamicRadius=false;	
	
	// inter-frame coherence params
	float mPartTrackLocMaxDist = 1.f;
	float mPartTrackRadiusMaxDist = .5f;
	float mDejitterContourMaxDist = 0.f;
	
	// debug params
	bool mDebugDrawFlipperAccelHairs=false;
	bool mDebugDrawCubeMaps=false;
	bool mDebugDrawAdjSpaceRays=false;
	bool mDebugDrawGeneratedContours=false;
	
	// state
	void sendGameEvent( GameEvent );
	
	float getTime() const { return ci::app::getElapsedSeconds(); } // use this time so we can locally modulate it (eg slow down, pause, etc...)
	
	float getStrobeTime() const { return getTime(); }
	float getStrobe( float phase, float freq ) const;
	
	float getFlipperState( int side ) const { assert(side==0||side==1); return mFlipperState[side]; }
	float getFlipperAngularVel( int side ) const; // TODO: make radians per second

	const PartVec& getParts() const { return mParts; }

	cipd::PureDataNodeRef getPd() { return mPd; }


	// geometry
	PolyLine2 getCirclePoly ( vec2 c, float r ) const;
	PolyLine2 getCapsulePoly( vec2 c[2], float r[2] ) const;

	Shape2d polyToShape( const PolyLine2& ) const;
	TriMeshRef get3dMeshForPoly( const PolyLine2&, float znear, float zfar ) const; // e.g. 0..1, from tabletop in 1cm	
	
	
private:
	
	FileWatch mFileWatch;
		
	// params
	vec2  mUpVec = vec2(0,1);
	float mGravity=0.1f;
	float mBallReclaimAreaHeight = 10.f;
		
	// playfield layout
	vec2  toPlayfieldSpace  ( vec2 p ) const { return vec2( dot(p,getRightVec()), dot(p,getUpVec()) ); }
	vec2  fromPlayfieldSpace( vec2 p ) const { return getRightVec() * p.x + getUpVec() * p.y ; }
		// could make this into two matrices
	
	Rectf getPlayfieldBoundingBox( const ContourVec& ) const; // min/max of all points in playfield space
	Rectf toPlayfieldBoundingBox ( const PolyLine2& ) const; // just for one poly
	
	Rectf mPlayfieldBoundingBox; // min/max of non-hole contours in playfield coordinate space (up/right vectors)
	
	float mPlayfieldBallReclaimY; // at what playfield y do we reclaim balls?
	float mPlayfieldBallReclaimX[2]; // left, right edges for drawing...
	
	void updatePlayfieldLayout( const ContourVec& );
	
	
	// parts
	typedef map<int,PartRef> ContourToPartMap;
	
	PartVec mParts;
	ContourToPartMap mContoursToParts;

	void getContoursFromParts( const PartVec&, ContourVec& contours, ContourToPartMap& ) const; // for physics simulation
	void drawParts() const;
	
	PartRef findPartForContour( const Contour& ) const;
	PartRef findPartForContour( int contourIndex ) const;
	
	void rolloverTest();
	bool isValidRolloverLoc( vec2 loc, float r, const PartVec& ) const;
	
	// simulation
	void updateBallWorldContours();
	void processCollisions();
	void serveBall();
	void cullBalls(); // cull dropped balls
	
	// drawing
	void drawAdjSpaceRays() const;
	
	
	// --- Vision ---
	//

	// - params
	float mPartMaxContourRadius = 5.f; // contour radius lt => part
	float mHolePartMaxContourRadius = 2.f;
	float mFlipperDistToEdge = 10.f; // how close to the edge does a flipper appear?

	// - do it
	PartVec getPartsFromContours( const ContourVector& ); // only reason this is non-const is b/c parts point to the world
	bool shouldContourBeAPart( const Contour&, const ContourVector& ) const;
	PartVec mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const;
	AdjSpace getAdjacentSpace( const Contour*, vec2, const ContourVector& ) const ;
	AdjSpace getAdjacentSpace( vec2, const ContourVector& ) const ; // how much adjacent space is to the left, right?

	ContourVec dejitterVisionContours( ContourVec in, ContourVec old ) const;
	
	// - state
	ContourVec mVisionContours;
	Contour contourFromPoly( PolyLine2 ) const; // area, radius, center, bounds, etc... is approximate
	void addContourToVec( Contour, ContourVec& ) const; // for accumulating physics contours from Parts
	
	
	// --- Input ---
	//
	
	// are flippers depressed
	bool  mIsFlipperDown[2]; // left, right
	float mFlipperState[2]; // left, right; 0..1
	
	void tickFlipperState();	

	// keymap
	map<char,string> mKeyToInput; // maps keystrokes to input names
	map<string,function<void()>> mInputToFunction; // maps input names to code handlers
	void processKeyEvent( KeyEvent, string suffix );
	
	// game pad
	void setupControls();
	GamepadManager mGamepadManager;
	map<unsigned int,string> mGamepadButtons;
	map<string,function<void()>> mGamepadFunctions;
	
	
	// --- Sound Synthesis ---
	cipd::PureDataNodeRef	mPd;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis();
	void shutdownSynthesis();
	
	void updateBallSynthesis();
	
	
	
	// --- Graphics ---
	//
	// TODO: Move into a different object
	void setupGraphics();

	//
	void draw2d( DrawType );
	
	void prepare3dScene();
	void draw3d( DrawType );
	void beginDraw3d() const;
	void endDraw3d() const;

	void draw3dFloor() const;
	void draw3dScene() const;
	void draw3dBalls() const;
	void draw3dRibbons( DrawType drawType ) const;
	
	void drawBallCullLine() const;
	
	Scene mDrawScene;

	// - params
	int mCircleMinVerts=8;
	int mCircleMaxVerts=100;
	float mCircleVertsPerPerimCm=1.f;

	int mCubeMapSize = 256;
	int mMaxCubeMaps = 10;
	int mCubeMapFrameSkip = 0;
	
	bool  m3dEnable      = false;
	bool  m3dBackfaceCull= false;
	float m3dTableDepth  = 10.f;
	float m3dZSkew       = .5f;
	bool  m3dDynamicCubeMap = false;

	int getNumCircleVerts( float r ) const;

	// - cube maps
	gl::TextureCubeMapRef mCubeMap; // static
	vector<gl::FboCubeMapRef> mCubeMaps; // dynamic

	void updateCubeMaps();
	gl::FboCubeMapRef updateCubeMap( gl::FboCubeMapRef, vec3 eye ) const;
	gl::TextureCubeMapRef getCubeMapForBall( int ball ) const;
	
	// - shaders
	gl::GlslProgRef mWallShader;
	gl::GlslProgRef mBallShader;
	gl::GlslProgRef mFloorShader;
	gl::GlslProgRef mBallShadowShader;
	
	// - meshes
	gl::VboMeshRef mBallMesh;
};

} // namespace Pinball


class PinballWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "PinballWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<Pinball::PinballWorld>();
	}
};

#endif /* PinballWorld_hpp */
