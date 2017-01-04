//
//  PinballWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#ifndef PinballView_hpp
#define PinballView_hpp

#include "FileWatch.h"
#include "PinballParts.h"
#include "GameWorld.h"

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

class PinballView
{
public:
	PinballView( const PinballWorld& world ) : mWorld(world) {} // world hasn't been initialized yet, so don't use it!
	
	void setup();
	
	void setParams( XmlTree );
	
	void update();
	void prepareToDraw();
	void draw( GameWorld::DrawType );

	void drawAdjSpaceRays( const PartVec& ) const;
	
	// debug params
	bool mDebugDrawFlipperAccelHairs=false;
	bool mDebugDrawCubeMaps=false;
	bool mDebugDrawAdjSpaceRays=false;
	bool mDebugDrawGeneratedContours=false;
	
	// geometry
	PolyLine2 getCirclePoly ( vec2 c, float r ) const;
	PolyLine2 getCapsulePoly( vec2 c[2], float r[2] ) const;

	Shape2d polyToShape( const PolyLine2& ) const;
	TriMeshRef get3dMeshForPoly( const PolyLine2&, float znear, float zfar ) const; // e.g. 0..1, from tabletop in 1cm	
	
	
private:
	
	const Pinball::PinballWorld& mWorld;
	
	friend class PinballWorld;
	
	FileWatch mFileWatch;
		
	void draw2d( GameWorld::DrawType );
	
	void prepare3dScene();
	void draw3d( GameWorld::DrawType );
	void beginDraw3d() const;
	void endDraw3d() const;

	void draw3dFloor() const;
	void draw3dScene() const;
	void draw3dBalls() const;
	void draw3dRibbons( GameWorld::DrawType ) const;
	
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


#endif /* PinballView_hpp */
