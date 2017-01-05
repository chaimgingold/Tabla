//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "PinballWorld.h"
#include "PinballView.h"

#include "PaperBounce3App.h"
#include "glm/glm.hpp"
#include "PinballParts.h"
#include "geom.h"
#include "cinder/rand.h"
#include "xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Pinball;

namespace Pinball
{

void PinballView::setup()
{
	{
		ci::geom::Sphere sphereGeom;
		sphereGeom.colors(false);
		sphereGeom.radius(1.f);
		sphereGeom.center(vec3(0,0,0));
		mBallMesh = gl::VboMesh::create(sphereGeom);
	}

	auto load = [this]( string name, gl::GlslProgRef* to, std::function<void(void)> f )
	{
		mFileWatch.loadShader(
			PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "pinball" / (name + ".vert") ),
			PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "pinball" / (name + ".frag") ),
			[this,to,f](gl::GlslProgRef prog)
		{
			*to = prog; // allows null, so we can easily see if we broke it
			if (f) f();
		});
	};
	
	load( "wall", &mWallShader, 0 );
	load( "ball", &mBallShader, 0 );
	load( "floor", &mFloorShader, 0 );
	load( "sky", &mSkyShader, 0 );
	load( "ball-shadow", &mBallShadowShader, 0 );
	
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath( fs::path("images") / "env_map.jpg" ),
		[this]( fs::path path ){
			try {
				mCubeMap = gl::TextureCubeMap::create( loadImage(path),
					gl::TextureCubeMap::Format().mipmap() );
			} catch (...) {
				mCubeMap = 0;
			}
		}
	);
}

void PinballView::setParams( XmlTree xml )
{	
	getXml(xml, "DebugDrawGeneratedContours", mDebugDrawGeneratedContours);
	getXml(xml, "DebugDrawAdjSpaceRays", mDebugDrawAdjSpaceRays );
	getXml(xml, "DebugDrawFlipperAccelHairs", mDebugDrawFlipperAccelHairs );
	getXml(xml, "DebugDrawCubeMaps", mDebugDrawCubeMaps);

	getXml(xml, "CircleMinVerts", mCircleMinVerts );
	getXml(xml, "CircleMaxVerts", mCircleMaxVerts );
	getXml(xml, "CircleVertsPerPerimCm", mCircleVertsPerPerimCm );

	getXml(xml, "3d/Enable", m3dEnable );
	getXml(xml, "3d/BackfaceCull", m3dBackfaceCull );
	getXml(xml, "3d/TableDepth", m3dTableDepth );
	getXml(xml, "3d/ZSkew", m3dZSkew );
	
	getXml(xml, "CubeMap/FrameSkip",mCubeMapFrameSkip);
	getXml(xml, "CubeMap/Size", mCubeMapSize );
	getXml(xml, "CubeMap/MaxCount",mCubeMapMaxCount);
	getXml(xml, "CubeMap/Dynamic", mCubeMapDynamic );

	getXml(xml, "CubeMap/DrawFloor", mCubeMapDrawFloor );
	getXml(xml, "CubeMap/DrawBalls", mCubeMapDrawBalls );
	getXml(xml, "CubeMap/DrawRibbons", mCubeMapDrawRibbons );
	getXml(xml, "CubeMap/EyeHeight", mCubeMapEyeHeight );
	getXml(xml, "CubeMap/LightHeight", mCubeMapLightHeight );
	getXml(xml, "CubeMap/LightColor", mCubeMapLightColor );
	
	getXml(xml, "SkyPipelineStageName", mSkyPipelineStageName);
	getXml(xml, "SkyHeight", mSkyHeight);
}

void PinballView::update()
{
	mFileWatch.update();
}

int PinballView::getNumCircleVerts( float r ) const
{
	return constrain( (int)(M_PI*r*r / mCircleVertsPerPerimCm), mCircleMinVerts, mCircleMaxVerts );
}

PolyLine2 PinballView::getCirclePoly( vec2 c, float r ) const
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

PolyLine2 PinballView::getCapsulePoly( vec2 c[2], float r[2] ) const
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

gl::TextureCubeMapRef PinballView::getCubeMapForBall( int i ) const
{
	gl::TextureCubeMapRef env;

	if ( mCubeMapTextures.size() > i && mCubeMapTextures[i] ) {
		env = mCubeMapTextures[i];
	} else if (!mCubeMapTextures.empty()) {
		env = mCubeMapTextures[ i % mCubeMapTextures.size() ];
	}
	
	if (!env) env = mCubeMap;
	
	return env;
}

void PinballView::updateVision( Pipeline& p )
{
	// grab sky image
	auto stage = p.getStage(mSkyPipelineStageName);
	if (stage) {
		mSkyTexture = stage->getGLImage();
	}
	else mSkyTexture=0;
	
	// log cube maps
	appendToVisionPipeline(p);
}

void PinballView::appendToVisionPipeline( Pipeline& p ) const
{
	for( int i=0; i<mCubeMaps.size(); ++i )
	{
		if (mCubeMaps[i])
		{
			p.then( string("mCubeMaps[")+toString(i)+"]", mCubeMaps[i]->getTextureCubeMap() );
		}
	}
}
	
void PinballView::updateCubeMaps()
{
	// Optimization: move this state pushing/pulling to the caller of updateCubeMap
	gl::ScopedMatrices matrix;
	gl::ScopedDepthTest depth(true,GL_LESS);
	gl::ScopedFaceCulling cull(true,GL_BACK);
	gl::enableDepthRead();
	gl::enableDepthWrite();			
	gl::context()->pushFramebuffer();
	// we need to save the current FBO because we'll be calling bindFramebufferFace() below
	

	if (0)
	{
		mCubeMaps.resize(1);
		
		mCubeMaps[0] = updateCubeMap( mCubeMaps[0], vec3(mWorld.getWorldBoundsPoly().calcCentroid(),mWorld.getTableDepth()/2.f) );
	}
	else
	{
		// re-allocate
		int targetnum = min( mCubeMapMaxCount, (int)mWorld.getBalls().size() ); 
		
		if (targetnum != mCubeMaps.size()) mCubeMaps.resize( targetnum );
		
		mCubeMapTextures.resize(mCubeMaps.size(),0);
		
		// update
		for( int i=0; i<mCubeMaps.size(); ++i )
		{
			if ( mCubeMapFrameSkip<1 || ((ci::app::getElapsedFrames()+i) % mCubeMapFrameSkip) == 0 )
			{
				const auto& ball = mWorld.getBalls()[i];
				
				mCubeMaps[i] = updateCubeMap(
					mCubeMaps[i],
					vec3( ball.mLoc, mWorld.getTableDepth() - ball.mRadius ),
					i
					);
				
				// bake
				if (mCubeMaps[i]) {
					mCubeMapTextures[i] = mCubeMaps[i]->getTextureCubeMap();
				}
			}
		}
	}

	gl::context()->popFramebuffer();
	gl::enableDepthRead(false);
	gl::enableDepthWrite(false);
}

gl::FboCubeMapRef PinballView::updateCubeMap( gl::FboCubeMapRef fbo, vec3 eye, int skipBall ) const
{
	if ( !fbo || fbo->getSize() != ivec2(mCubeMapSize, mCubeMapSize) )
	{
		gl::FboCubeMap::Format format;
		format.textureCubeMapFormat(
			gl::TextureCubeMap::Format()
			.magFilter(GL_LINEAR)
			.minFilter(GL_LINEAR) );
		
		fbo = gl::FboCubeMap::create( mCubeMapSize, mCubeMapSize, format );
	}
	
	if (fbo)
	{
		gl::ScopedViewport PinballViewport( ivec2( 0, 0 ), fbo->getSize() );

		for( uint8_t dir = 0; dir < 6; ++dir )
		{
			ci::CameraPersp camera( fbo->getWidth(), fbo->getHeight(), 90.0f, 1, 1000 );
			gl::setProjectionMatrix( camera.getProjectionMatrix() );
			gl::setViewMatrix( fbo->calcViewMatrix( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, eye ) );
			
			fbo->bindFramebufferFace( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir );
			
			gl::clearDepth(1.f);
			gl::clear();
			
			if (mCubeMapDrawFloor) draw3dFloor();
			if (mCubeMapDrawBalls) draw3dBalls( eye, skipBall, fbo->getTextureCubeMap());
			if (mCubeMapDrawRibbons) draw3dRibbons(GameWorld::DrawType::CubeMap);
			draw3dScene();
			drawSky();
			if (mDebugDrawCubeMaps) drawBallOrientationMarkers();
		}
	}
	
	return fbo;
}

void PinballView::drawAdjSpaceRays( const PartVec& parts ) const
{
	for( auto &p : parts )
	{
		vec2 loc = p->getAdjSpaceOrigin();
		
		const AdjSpace space = mWorld.getAdjacentSpace(loc,mWorld.getVisionContours());

		auto draw = [&]( ColorA color, vec2 vec, float width, float m )
		{
			gl::color(color);
			gl::drawLine(
				loc + vec *  width,
				loc + vec * (width + m) );
		};
		
		draw( Color(1,0,0), mWorld.getRightVec(), space.mWidthRight, space.mRight );
		draw( Color(0,1,0), mWorld.getLeftVec(), space.mWidthLeft, space.mLeft );
		draw( Color(1,0,0), mWorld.getUpVec(), space.mWidthUp, space.mUp );
		draw( Color(0,1,0), mWorld.getDownVec(), space.mWidthDown, space.mDown );


		gl::color(0,0,1);
		gl::drawLine(
			loc + mWorld.getLeftVec ()  * space.mWidthLeft,
			loc + mWorld.getRightVec()  * space.mWidthRight );

		gl::color(0,0,1);
		gl::drawLine(
			loc + mWorld.getUpVec  ()  * space.mWidthUp,
			loc + mWorld.getDownVec()  * space.mWidthDown );
	}
}

void PinballView::drawBallCullLine( float z ) const
{
	vec2 p[2];
	mWorld.getCullLine(p);
	
	// playfield markers
	gl::color(1,0,0);
	gl::drawLine( vec3(p[0],z),vec3(p[1],z));
}

void PinballView::prepareToDraw()
{
	if (m3dEnable) {
		prepare3dScene();
		if (mCubeMapDynamic) updateCubeMaps();
		else mCubeMaps.clear();
	}
}

void PinballView::draw( GameWorld::DrawType drawType )
{
	// world
	if (m3dEnable && (drawType==GameWorld::DrawType::UIMain || drawType==GameWorld::DrawType::Projector) ) draw3d(drawType);
	else draw2d(drawType);

	// --- debugging/testing ---
	
	// world orientation debug info
	if (0)
	{
		vec2 c = mWorld.getWorldBoundsPoly().calcCentroid();
		float l = 10.f;

		gl::color(1,0,0);
		gl::drawLine( c, c + mWorld.getRightVec() * l );

		gl::color(0,1,0);
		gl::drawLine( c, c + mWorld.getLeftVec() * l );

		gl::color(0,0,1);
		gl::drawLine( c, c + mWorld.getUpVec() * l );
	}
	
	// test ray line seg...
	if (mDebugDrawAdjSpaceRays) drawAdjSpaceRays(mWorld.getParts());

	// test contour generation
	if (mDebugDrawGeneratedContours)
	{
		ContourVec cs = mWorld.getContours();
		int i = cs.size(); // only start loop at new contours we append
		
		mWorld.getContoursFromParts(mWorld.getParts(),cs);
		
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

void PinballView::draw2d( GameWorld::DrawType drawType )
{
	drawBallCullLine();
	
	// flippers, etc...
	for( const auto &p : mWorld.getParts() ) {
		p->draw();
	}

	// balls
	mWorld.BallWorld::drawRibbons(drawType);
	mWorld.BallWorld::drawBalls(drawType);
}

void PinballView::beginDraw3d() const
{
	// all of this may be for naught as it seems that it won't hold up to the
	// deformations we do for the projector transform.
	// so the right way to do this--without doing something really transformationally clever with
	// the ogl pipeline i can barely even get my brain to think of the possibility--would be to
	// render to a texture and then project onto the table. 
	
	// alternatives: we use a stencil to cut out the transparent part of the tabletop.
	// i think our 3d effect might still be a bit messed, but we might be able to adjust the skew
	// to compensate for that.
	// that actually could get us quite close.
	
	// i think +z is away from the camera (doh!), and i don't quite have that right yet...
	// also, projector view is quite messed compared to UI window :P, but UI is a start...
	

	
	// back face culling (a little weird, and not necessary)
	if (m3dBackfaceCull)
	{
		const bool isViewFlipped = (gl::getViewMatrix() * vec4(0,0,1,1)).w < 1.f; 
	//	cout << gl::getViewMatrix() * vec4(0,0,1,1) << endl;	
			// this insanity is to capture when things have turned inside out on us, and
			// we need to reverse culling... (e.g. for reversed projection mapping)
			// I have **NO** idea how robust this test is; it just seems to work given the one
			// test case I did. I'm sure there is a right way to do this, I just don't know what it is.
			

		gl::enableFaceCulling();
		gl::cullFace( isViewFlipped ? GL_FRONT : GL_BACK );
	}
	
	// depth
	gl::enableDepthRead();
	gl::enableDepthWrite();

	if (1)
	{
		const bool kDebugVizDepth = false;
		
		if (kDebugVizDepth) {
			gl::color(.5,.5,.5);
		} else {
			gl::colorMask(false, false, false, false);
		}
		
		glDepthFunc(GL_ALWAYS);
		
		// write 0 everywhere for tabletop
		gl::clearDepth(0.f);
		gl::clear(GL_DEPTH_BUFFER_BIT);

		// punch out holes where paper is at table floor, and tabletops for holes inside of that
		std::function<void(const Contour& c)> recurseTree;
		
		recurseTree = [&]( const Contour& c ) -> void
		{
			if ( !mWorld.shouldContourBeAPart(c,mWorld.getVisionContours()) )
			{
				// draw me
				const bool punchOut = !c.mIsHole;
				
				if (punchOut) {
					// if punching out, go in; otherwise we stay at 0 (and fill)
					gl::pushModelView();
					gl::translate(vec3(0,0,m3dTableDepth*2.f)); // go beyond floor, because we will draw it in later
					if (kDebugVizDepth) gl::color(1,1,1);
				}
				else if (kDebugVizDepth) gl::color(.5,.5,.5);
				
				gl::drawSolid(c.mPolyLine);
				
				if (punchOut) gl::popModelView();
				
				// childers
				for( int childIndex : c.mChild ) {
					recurseTree(mWorld.getVisionContours()[childIndex]);
				}
			}
		};
		
		for( const auto &c : mWorld.getVisionContours() )
		{
			if (c.mTreeDepth==0) recurseTree(c);
		}
		gl::colorMask(true,true,true,true);
		glDepthFunc(GL_LESS);
	}

	// 3d transform
	{
		mat4 skew;
		skew[2][0] = mWorld.getGravityVec().x * m3dZSkew;
		skew[2][1] = mWorld.getGravityVec().y * m3dZSkew;
		
		skew *= glm::translate( vec3( mWorld.getScreenShake(), 0 ) );
		
		gl::pushModelView();
//		gl::setViewMatrix( gl::getModelView()
		gl::multModelMatrix(skew);
		
//		gl::pushmode
	}
}

void PinballView::endDraw3d() const
{
	gl::popModelView();

	gl::enableDepthRead(false);
	gl::enableDepthWrite(false);
	
	if (m3dBackfaceCull) {
		gl::enableFaceCulling(false);
	}
}

Shape2d PinballView::polyToShape( const PolyLine2& poly ) const
{
	Shape2d shape;
	shape.moveTo(poly.getPoints()[0]);
	for( int i=1; i<poly.size(); ++i ) shape.lineTo(poly.getPoints()[i]);
	if (poly.isClosed()) shape.close(); // so we can generalize poly -> shape
	return shape;
}

TriMeshRef PinballView::get3dMeshForPoly( const PolyLine2& poly, float znear, float zfar ) const
{
	float extrudeDepth = zfar - znear;
	
	std::function<Colorf(vec3)> posToColor = [&]( vec3 v ) -> Colorf
	{
		return Colorf(1,1,1);
		
		return lerp(
//			Colorf(0,1,0),
//			Colorf(1,0,0),
			Colorf(1,1,1),
//			Colorf(0,0,0),
			Colorf(.5,.5,.5),
			constrain( (v.z-znear)/(zfar-znear), 0.f, 1.f ) );
			// near -> far (from viewer)
	};
	
//	PolyLine2 poly=c.mPolyLine;
//			poly.reverse(); // turn it inside out, so normals face inward
	
	return TriMesh::create(
		   geom::Extrude( polyToShape(poly), extrudeDepth ).caps(false).subdivisions( 1 )
		>> geom::Translate(0,0,extrudeDepth/2+znear)
		>> geom::ColorFromAttrib( geom::POSITION, posToColor )
	);
}

void PinballView::prepare3dScene()
{
	mDrawScene = Scene();
	
	// 3d contours
	for( const auto &c : mWorld.getVisionContours() )
	{
		if ( !mWorld.shouldContourBeAPart(c,mWorld.getVisionContours()) )
		{
			mDrawScene.mWalls.push_back( get3dMeshForPoly(c.mPolyLine,0.f,m3dTableDepth) );
		}
	}

	// 3d part sides
	for( auto &p : mWorld.getParts() )
	{
		p->addTo3dScene(mDrawScene);
	}
}

void PinballView::draw3dFloor() const
{
	// floor
	if (mFloorShader)
	{
		gl::ScopedGlslProg glslScp(mFloorShader);
		
		mFloorShader->uniform( "uTime", (float)ci::app::getElapsedSeconds() );

		gl::pushModelView();
		gl::translate(0,0,m3dTableDepth);
		// we don't z fight with depth punch out because it is beyond the floor;
		// we are building a floor for things to clip against here.
		
		if (1)
		{
			gl::ScopedFaceCulling cull(false); // world bounds poly not guaranteed to be proper winding :P (should be though)
			
			// let z-buffer clip it for us
			// (but it doesn't render anymore in the fbo... but it's on the bottom of the ball, so whatever)
			gl::drawSolid( mWorld.getWorldBoundsPoly() );
		}
		else
		{
			// only fill in precisely the right places
			for( const auto &c : mWorld.getVisionContours() )
			{
				if ( !c.mIsHole && !mWorld.shouldContourBeAPart(c,mWorld.getVisionContours()) )
				{
					gl::drawSolid(c.mPolyLine);
				}
			}
		}
		
		gl::popModelView();
	}
}

void PinballView::drawSky() const
{
	if (mSkyTexture) //&&mSkyShader)
	{
		gl::ScopedFaceCulling cull(false);
		
//		mSkyTexture->bind();
//		mDrawScene.mSky
		
		gl::ScopedModelMatrix mat;
		gl::translate(0,0,-mSkyHeight);
		
		Rectf r(  mWorld.getWorldBoundsPoly().getPoints() );
		
		gl::draw(mSkyTexture, r );
	}
}

void PinballView::draw3dScene() const
{
	auto drawSceneSegment = [this](
		gl::GlslProgRef shader,
		const Scene::Meshes& meshes,
		function<void()> f=0 )
	{
		if (!shader) return;
		
		gl::ScopedGlslProg glslScp(shader);
		
		if (f) f();
		
		// mDrawScene is assembled in prepare3dScene
		for( auto w : meshes ) {
			if (w.mMesh) {
				gl::ScopedModelMatrix trans;
				gl::multModelMatrix(w.mTransform);
				gl::draw(*w.mMesh);
			}
		}
	};
	
	drawSceneSegment(mWallShader,mDrawScene.mWalls,[this](){
		mWallShader->uniform("uTime",(float)ci::app::getElapsedSeconds());
	});
}

void PinballView::draw3dBalls( vec3 eyeLoc, int skipBall, gl::TextureCubeMapRef skipMap ) const
{
	if (mBallShader)
	{
		gl::ScopedGlslProg glslScp(mBallShader);
		
		if (mCubeMap && !mCubeMapDynamic) {
			mCubeMap->bind();
			mBallShader->uniform( "uCubeMapTex", 0 );
		}
		
		vec3 lightLoc = vec3( mWorld.getWorldBoundsPoly().calcCentroid(), -mCubeMapLightHeight ) ;
		
		mBallShader->uniform( "uCubeMapTex", 0 );
		mBallShader->uniform( "inLightLoc", lightLoc);
		mBallShader->uniform( "inEyeLoc", eyeLoc);
		mBallShader->uniform( "uLightColor", mCubeMapLightColor );
		
		for( int i=0; i<mWorld.getBalls().size(); ++i )
		{
			if (i==skipBall) continue;
			
			if (mCubeMapDynamic)
			{
				gl::TextureCubeMapRef env = getCubeMapForBall(i);
				if (env&&env!=skipMap) env->bind();
			}
			
			const Ball& b = mWorld.getBalls()[i];
			
			float ballz = m3dTableDepth-b.mRadius/2;
			
			mBallShader->uniform("inBallLoc", vec3( b.mLoc, ballz ) );
			
			gl::ScopedModelMatrix model;
			if (!mCubeMapDynamic) gl::multModelMatrix( mWorld.getBallTransform(b) );
			else
			{
				// no deform, for testing
				gl::multModelMatrix(
					glm::translate(vec3(b.mLoc,0))
					*  glm::scale( vec3(1,1,1)*b.mRadius )
				);
			}
			gl::translate(0,0,ballz);
			gl::draw(mBallMesh);
		}
	}
}

void PinballView::drawBallOrientationMarkers() const
{
	const int n = 8; 
	
	for( auto &ball : mWorld.getBalls() )
	{
		const vec3 ballc = vec3(ball.mLoc,mWorld.getTableDepth()-ball.mRadius);
		const float r = ball.mRadius*.5f;
		const float dist = ball.mRadius*2.f;
		
		for( int i=0; i<n; ++i )
		{
			vec3 v;
			
			float a = (float)i / (float)n * M_PI*2.f;
			mat4 rotate = glm::rotate( a, vec3(0.f,0.f,-1.f) );
			v = vec3( vec4(1,0,0,1) * rotate ); 
			
			gl::color(v.x,v.y,0.f);
			gl::drawSphere( ballc + v * dist, r );
		}
		
		gl::color(0,0,1);
		gl::drawSphere( ballc + vec3(0,0,-1)*dist, r );		
	}
}

void PinballView::draw3dRibbons( GameWorld::DrawType drawType ) const
{
	gl::ScopedModelMatrix trans;
	gl::translate(0,0,m3dTableDepth - mWorld.mBallDefaultRadius*.5f);
	
	gl::enableDepthWrite(false);
	mWorld.drawRibbons(drawType);
	gl::enableDepthWrite(true);
}

void PinballView::draw3d( GameWorld::DrawType drawType )
{
	beginDraw3d();

	draw3dFloor();
	
	// collected 3d scene
	draw3dScene();

	// axes
	if (0)
	{
		vec3 c = vec3( mWorld.getWorldBoundsPoly().calcCentroid(), -2 );
		float l = 10.f;

		gl::color(1,0,0);
		gl::drawLine( c, c + vec3(1,0,0) * l );

		gl::color(0,1,0);
		gl::drawLine( c, c + vec3(0,1,0) * l );

		gl::color(0,0,1);
		gl::drawLine( c, c + vec3(0,0,1) * l );
	}
	

	// ball shadows
	if (0&&mBallShadowShader)
	{
		gl::ScopedModelMatrix trans;
		gl::translate(0,0,m3dTableDepth - .01f);
		
		mWorld.drawBalls(drawType,mBallShadowShader);
	}
	
	// parts (2d)
	for( const auto &p : mWorld.getParts() ) {
		p->draw();
	}

	// red line
	drawBallCullLine(m3dTableDepth - .01f);

	// balls
	vec3 eyeLoc = 
		  vec3( mWorld.getWorldBoundsPoly().calcCentroid(), -mCubeMapEyeHeight )
		+ vec3( mWorld.getDownVec(), 0 ) * mCubeMapEyeHeight * m3dZSkew ;
		// some slight of hand here... 
		
	draw3dBalls(eyeLoc);
	draw3dRibbons(drawType);
	if (mDebugDrawCubeMaps) drawBallOrientationMarkers();

	// done with 3d
	endDraw3d();
}


} // ns Pinball
