//
//  Balls.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "PaperBounce3App.h"
#include "BallWorld.h"
#include "geom.h"
#include "cinder/Rand.h"
#include "xml.h"

BallWorld::BallWorld()
{
	mFileWatch.loadShader(
		PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "circle.vert" ),
		PaperBounce3App::get()->hotloadableAssetPath( fs::path("shaders") / "circle.frag" ),
		[this](gl::GlslProgRef prog)
	{
		mCircleShader = prog; // allows null, so we can easily see if we broke it
	});
}

void BallWorld::setParams( XmlTree xml )
{
	getXml(xml,"NumIntegrationSteps",mNumIntegrationSteps);
	getXml(xml,"DefaultNumBalls",mDefaultNumBalls);
	getXml(xml,"BallDefaultRadius",mBallDefaultRadius);
	getXml(xml,"BallDefaultMaxRadius",mBallDefaultMaxRadius);
	getXml(xml,"BallDefaultColor",mBallDefaultColor);
	getXml(xml,"BallMaxVel",mBallMaxVel);
	getXml(xml,"BallContourImpactNormalVelImpulse",mBallContourImpactNormalVelImpulse);
	getXml(xml,"BallContourCoeffOfRestitution",mBallContourCoeffOfRestitution);
	getXml(xml,"BallContourFrictionlessCoeff",mBallContourFrictionlessCoeff);

	getXml(xml,"Ribbon/Enabled",mRibbonEnabled);
	getXml(xml,"Ribbon/MaxLength",mRibbonMaxLength);
	getXml(xml,"Ribbon/SampleRate",mRibbonSampleRate);
	getXml(xml,"Ribbon/RadiusScale",mRibbonRadiusScale);
	getXml(xml,"Ribbon/RadiusExp",mRibbonRadiusExp);
	getXml(xml,"Ribbon/AlphaScale",mRibbonAlphaScale);
	getXml(xml,"Ribbon/AlphaExp",mRibbonAlphaExp);
	
	updateBallsWithRibbonParams();
}

void BallWorld::updateBallsWithRibbonParams()
{
	const int maxlen = getRibbonMaxLength();
		
	for( auto &b : mBalls )
	{
		int size = b.mHistory.max_size();
		
		if ( size > maxlen ) {
			b.mHistory.resize(maxlen);
		} else if ( size < maxlen ) {
			b.mHistory.set_capacity(maxlen);
		}
	}
}

void BallWorld::accumulateBallHistory()
{
	if ( ci::app::getElapsedFrames() % mRibbonSampleRate==0 )
	{
		for( auto &b : mBalls )
		{
			if ( b.mHistory.max_size() > 0 )
			{
				if ( b.mHistory.full() && !b.mHistory.empty() ) b.mHistory.pop_front();
				
				b.mHistory.push_back( b.mLoc );
			}
		}
	}
}

TriMeshRef BallWorld::getTriMeshForRibbons() const
{
	TriMesh mesh( TriMesh::Format().positions(2).colors(4) );
	
	auto getRadius = [this]( const Ball& b, int i ) -> float
	{
		float f = (float)i / (float)(b.mHistory.size()-1);
		f = powf(f,mRibbonRadiusExp);
		return b.mRadius * mRibbonRadiusScale * f;
	};

	auto getColor = [this]( const Ball& b, int i ) -> ColorA
	{
		float f = (float)i / (float)(b.mHistory.size()-1);
		f = powf(f,mRibbonAlphaExp);
		
		ColorA c = b.mColor;
		c.a *= f * mRibbonAlphaScale;
		return c;
		//return ColorA(1,1,1,.8f);
	};
	
	auto add = [&]( const Ball& ball, int a, int b, int v )
	{
		const auto &pts = ball.mHistory; 

		ColorA color = getColor(ball,v);
		
		vec2 d = normalize( pts[b] - pts[a] );
		vec2 p = perp(d) * getRadius(ball,v);
		
		mesh.appendPosition(pts[v] + p);
		mesh.appendPosition(pts[v] - p);
		mesh.appendColorRgba(color);
		mesh.appendColorRgba(color);
	};
	
	auto pushIndices = [&mesh]()
	{
		/*  0-2
			|/|  -->>
			1-3  */
		uint32_t i = mesh.getNumVertices()-4; 
		uint32_t ind[6] = {
			i+0, i+1, i+2,
			i+1, i+3, i+2
		};
		mesh.appendIndices(ind,6);
	};
	
	for( const auto &b : mBalls )
	{
		if ( b.mHistory.size() >= 2 )
		{
			// front
			add(b,0,1,0);
			
			// middle
			for( int i=1; i<b.mHistory.size()-1; ++i )
			{
				add(b,i-1,i+1,i);
				pushIndices();
			}
			
			// back
			{
				int n = b.mHistory.size()-1;

				add(b,n-1,n,n);
				pushIndices();
			}		
		}
	}
	
	return std::make_shared<TriMesh>(mesh);
}

TriMeshRef BallWorld::getTriMeshForBalls() const
{
	const float inv_delta = mNumIntegrationSteps;
	
	TriMeshRef mesh = TriMesh::create( TriMesh::Format().positions(2).colors(4).texCoords0(2) );

	const vec2 uv[4] = {
		vec2(0,1),
		vec2(1,1),
		vec2(1,0),
		vec2(0,0)
	};

	for( const auto &b : mBalls )
	{
		if (0)
		{
			// just a circle
			vec2 v[4];

			Rectf r( b.mLoc - vec2(1,1)*b.mRadius,
					 b.mLoc + vec2(1,1)*b.mRadius );

			getRectCorners( r, v );
			
			appendQuad(*mesh, b.mColor, v, uv );
		}
		else
		{
			// squash and stretch
			mat4 x;
			
			float f;
			vec2 stretch;
			{
				vec2 vel    = b.getVel() * inv_delta;
				vec2 squash = b.mSquash  * inv_delta;
				
				float squashLen = min( length(squash) * 10.f, b.mRadius * .5f ) ;
				float velLen    = length(vel) ;
				
				float l ;
				
				if ( squashLen > velLen ) stretch = perp(squash), l=squashLen ;
				else stretch = vel, l = velLen ;
				
				f = .25f * (l / b.mRadius) ;
			}
			
			x *= glm::translate( vec3(b.mLoc,0) ) ;
			x *= glm::scale( vec3(b.mRadius,b.mRadius,1.f) );
			
			x *= glm::rotate( glm::atan( stretch.y, stretch.x ), vec3(0,0,1) );
			x *= glm::scale( vec3(1.f+f,1.f-f,1) );
			
			// just a circle
			vec2 v[4] = {
				vec2(-1,1),
				vec2(1,1),
				vec2(1,-1),
				vec2(-1,-1)
			};
			
			for( int i=0; i<4; ++i ) v[i] = vec2( x * vec4(v[i],0,1) );
			
			appendQuad(*mesh, b.mColor, v, uv);
		}
	}
	
	return mesh;
}

void BallWorld::prepareToDraw()
{
	mBallMesh = getTriMeshForBalls();
	mRibbonMesh = getTriMeshForRibbons();
}

void BallWorld::drawImmediate( bool lowPoly ) const
{
	for( auto b : mBalls )
	{
		gl::color(b.mColor) ;
		
		if (0)
		{
			// just a circle
			gl::drawSolidCircle( b.mLoc, b.mRadius ) ;
		}
		else
		{
			int numSegments;
			if ( lowPoly ) numSegments = -1 ; // should resolve to a small #
			else numSegments = 20;
			
			// squash + stretch
			gl::pushModelView() ;
			gl::translate( b.mLoc ) ;
			
			vec2  vel = b.getVel() ;
			
			float squashLen = min( length(b.mSquash) * 10.f, b.mRadius * .5f ) ;
			float velLen    = length(vel) ;
			
			vec2 stretch ;
			float l ;
			
			if ( squashLen > velLen ) stretch = perp(b.mSquash), l=squashLen ;
			else stretch = vel, l = velLen ;
			
			float f = .25f * (l / b.mRadius) ;
			
			gl::rotate( glm::atan( stretch.y, stretch.x ) ) ;
			gl::drawSolidEllipse( vec2(0,0), b.mRadius*(1.f+f), b.mRadius*(1.f-f), numSegments ) ;
			
			gl::popModelView() ;
		}
	}
}

void BallWorld::draw( DrawType drawType )
{
	if (1)
	{
		drawRibbons(drawType);
		drawBalls(drawType);
	}
	else
	{
		// draw immediate mode
		auto ribbons = getTriMeshForRibbons();
		if (ribbons) gl::draw(*ribbons);
		
		drawImmediate( drawType==DrawType::UIPipelineThumb );
	}	
}

void BallWorld::drawBalls( DrawType )
{
	// draw using graphics we setup in prepareToDraw().
	if (mBallMesh && mCircleShader)
	{
		gl::ScopedGlslProg glslScp( mCircleShader );
		gl::draw(*mBallMesh);
	}
}

void BallWorld::drawRibbons( DrawType )
{
	if (mRibbonMesh) gl::draw(*mRibbonMesh);
}

void BallWorld::gameWillLoad()
{
	for ( int i=0; i<mDefaultNumBalls; ++i )
	{
		newRandomBall( getRandomPointInWorldBoundsPoly() );
	}
}

void BallWorld::update()
{
	mFileWatch.update();
	updatePhysics();
}

int BallWorld::getBallIndex( const Ball& b ) const
{
	int i = &b - &mBalls[0];
	if (i<0 || i>=mBalls.size()) i=-1;
	return i;
}

void BallWorld::onBallBallCollide			( const Ball& a, const Ball& b )
{
	mBallBallCollisions.push_back( BallBallCollision(getBallIndex(a),getBallIndex(b)));
}

void BallWorld::onBallContourCollide		( const Ball& a, const Contour& b )
{
	mBallContourCollisions.push_back( BallContourCollision(getBallIndex(a),mContours.getIndex(b)));
}

void BallWorld::onBallWorldBoundaryCollide	( const Ball& a )
{
	mBallWorldCollisions.push_back( BallWorldCollision(getBallIndex(a)));
}

void BallWorld::scaleBallVelsForIntegrationSteps( int oldSteps, int newSteps )
{
	if (oldSteps==newSteps) return;
	
	float scale = (float)oldSteps / (float)newSteps;
	
	for( auto &b : mBalls )
	{
		b.setVel( b.getVel() * scale );
	}
}

void BallWorld::updatePhysics()
{
	// wipe collisions
	mBallBallCollisions.clear();
	mBallContourCollisions.clear();
	mBallWorldCollisions.clear();
	
	//
	scaleBallVelsForIntegrationSteps( mLastNumIntegrationSteps, mNumIntegrationSteps );
	mLastNumIntegrationSteps=mNumIntegrationSteps;
	
	const int   steps = mNumIntegrationSteps ;
	const float delta = 1.f / (float)steps ;
	
	for( int step=0; step<steps; ++step )
	{
		// accelerate
		for( auto &b : mBalls )
		{
			b.mLoc += (b.mAccel * (float)steps) * delta*delta ;
			b.mAccel = vec2(0,0) ;
		}

		// ball <> contour collisions
		resolveBallContourCollisions();
		
		// ball <> ball collisions
		resolveBallCollisions() ;
		
		// cap velocity
		// (i think this is mostly to compensate for aggressive contour<>ball collisions in which balls get pushed in super fast;
		// alternative would be to cap impulse there)
		if (1)
		{
			const float maxVel = mBallMaxVel * delta;
			
			for( auto &b : mBalls )
			{
				vec2 v = b.getVel() ;
				
				if ( length(v) > maxVel )
				{
					b.setVel( normalize(v) * maxVel ) ;
				}
			}
		}
		
		// inertia
		for( auto &b : mBalls )
		{
			vec2 vel = b.getVel() ; // rewriting mLastLoc will stomp vel, so get it first
			b.mLastLoc = b.mLoc ;
			b.mLoc += vel ;
		}
	}

	// un-squash
	for( auto &b : mBalls )
	{
		b.mSquash *= .7f ;
	}
	
	// history
	if (mRibbonEnabled) accumulateBallHistory();
}

void BallWorld::resolveBallContourCollisions()
{
	for( auto &b : mBalls )
	{
		vec2 oldVel = b.getVel() ;
		vec2 oldLoc = b.mLoc ;
		
		vec2 newLoc;
		
		if (b.mCollideWithContours)	newLoc = resolveCollisionWithContours		( b.mLoc, b.mRadius, &b ) ;
		else						newLoc = resolveCollisionWithInverseContours( b.mLoc, b.mRadius, &b ) ;
		
		// update?
		if ( newLoc != oldLoc )
		{
			// update loc
			b.mLoc = newLoc ;
			
			// update vel
			vec2 surfaceNormal = glm::normalize( newLoc - oldLoc ) ;
				// not as accurate as it might be, but seems to work fine.
				// also, this gets an approximate normal for collision with >1 edges
			
			vec2 newVel = glm::reflect( oldVel, surfaceNormal );
			
			if (mBallContourCoeffOfRestitution < 1.f || mBallContourFrictionlessCoeff < 1.f)
			{
				// modulate it with inelastic collision + maybe friction
				vec2 normalVel  = surfaceNormal * dot(newVel,surfaceNormal); // for inelastic
				vec2 tangentVel = newVel - normalVel; // for friction
				
				tangentVel *= mBallContourFrictionlessCoeff;
				
				newVel = tangentVel + normalVel * mBallContourCoeffOfRestitution;
			}
				
			//newVel += surfaceNormal * mBallContourImpactNormalVelImpulse;
			b.mAccel += surfaceNormal * mBallContourImpactNormalVelImpulse;
				// accumulate energy from impact
				// would be cool to use optic flow for this, and each contour can have a velocity
			
			b.setVel(newVel);

			// squash?
			b.noteSquashImpact( surfaceNormal * length(b.getVel()) ) ; //newLoc - oldLoc ) ;
		}
	}
}

Ball& BallWorld::newRandomBall ( vec2 loc )
{
	Ball ball ;
	
	ball.mColor = mBallDefaultColor;
	
	ball.setLoc( loc ) ;
	ball.mRadius = Rand::randFloat(mBallDefaultRadius,mBallDefaultMaxRadius) ;
	ball.setMass( M_PI * powf(ball.mRadius,3.f) ) ;
	
	ball.mCollideWithContours = randBool();
	if (!ball.mCollideWithContours) ball.mColor = Color(0,0,1);

	if ( Rand::randBool() )
	{
		if (ball.mCollideWithContours) ball.mColor = Color(0,1,0);
		else ball.mColor = Color(.5,0,.5);
	}
	
	ball.mAccel = Rand::randVec2() * mBallDefaultRadius/2.f;
//	ball.setVel( Rand::randVec2() * mBallDefaultRadius/2.f * (float)mNumIntegrationSteps ) ;
	// ideally we use mAccel here, but not sure how to get that to have the same effect,
	// so just sticking with this for now.
	
	ball.mHistory.set_capacity(getRibbonMaxLength());
	
	mBalls.push_back( ball ) ;
	
	return mBalls.back();
}

/*
vec2 BallWorld::resolveCollisionWithBalls ( vec2 p, float r, Ball* ignore, float correctionFraction ) const
{
	for ( const auto &b : mBalls )
	{
		if ( &b==ignore ) continue ;
		
		float d = glm::distance(p,b.mLoc) ;
		
		float rs = r + b.mRadius ;
		
		if ( d < rs )
		{
			// just update p
			vec2 correctionVec ;
			
			if (d==0.f) correctionVec = Rand::randVec2() ; // oops on top of one another; pick random direction
			else correctionVec = glm::normalize( p - b.mLoc ) ;
			
			p = correctionVec * lerp( d, rs, correctionFraction ) + b.mLoc ;
		}
	}
	
	return p ;
}*/

void BallWorld::resolveBallCollisions()
{
	if ( mBalls.size()==0 ) return ; // wtf, i have some stupid logic error below...
	
	for( size_t i=0  ; i<mBalls.size()-1; i++ )
	for( size_t j=i+1; j<mBalls.size()  ; j++ )
	{
		auto &a = mBalls[i] ;
		auto &b = mBalls[j] ;
		
		float d  = glm::distance(a.mLoc,b.mLoc) ;
		float rs = a.mRadius + b.mRadius ;
		
		if ( d < rs )
		{
			vec2 a2b ;
			
			if (d==0.f) a2b = Rand::randVec2() ; // oops on top of one another; pick random direction
			else a2b = glm::normalize( b.mLoc - a.mLoc ) ;
			
			float overlap = rs - d ;
			
			// get velocities
			const vec2 avel = a.getVel() ;
			const vec2 bvel = b.getVel() ;
			
			// get masses
			const float ma = a.getMass() ;
			const float mb = b.getMass() ;

			const float amass_frac = ma / (ma+mb) ; // a's % of total mass
			const float bmass_frac = 1.f - amass_frac ; // b's % of total mass
			
			// correct position (proportional to masses)
			b.mLoc +=  a2b * overlap * amass_frac ;
			a.mLoc += -a2b * overlap * bmass_frac ;
			
			// get velocities along collision axis (a2b)
			const float avelp = dot( avel, a2b ) ;
			const float bvelp = dot( bvel, a2b ) ;
			
			// ...computations for new velocities
			float avelp_new ;
			float bvelp_new ;
			
			if (0)
			{
				// swap velocities along axis of collision
				// (old way)
				avelp_new = bvelp ;
				bvelp_new = avelp ;
			}
			else
			{
				// new way:
				// - do relative mass interactions
				// - can dial elasticity
				
				float cr = 1.f ; // 0..1
					// coefficient of restitution:
					// 0 is elastic
					// 1 is inelastic
					// https://en.wikipedia.org/wiki/Inelastic_collision
				
				avelp_new = (cr * mb * (bvelp - avelp) + ma*avelp + mb*bvelp) / (ma+mb) ;
				bvelp_new = (cr * ma * (avelp - bvelp) + ma*avelp + mb*bvelp) / (ma+mb) ;
					// we'll let the compiler simplify that
					// (though if we cache inverse mass we can plug that in directly;
					// uh... i'm blanking on the algebra for this. whatev.)
			}
			
			// compute new velocities
			const vec2 avel_new = avel + a2b * ( avelp_new - avelp ) ;
			const vec2 bvel_new = bvel + a2b * ( bvelp_new - bvelp ) ;
			
			// set velocities
			a.setVel(avel_new) ;
			b.setVel(bvel_new) ;

			// squash it
//			a.noteSquashImpact( -a2b * overlap * bmass_frac ) ;
//			b.noteSquashImpact(  a2b * overlap * amass_frac ) ;

			a.noteSquashImpact( avel_new - avel ) ;
			b.noteSquashImpact( bvel_new - bvel ) ;
				// *cough* just undoing some of the comptuation i did earlier. compiler can figure this out,
				// but the point is that we just want the velocities along the axis of collision.
			
			// note it
			onBallBallCollide(a,b);
		}
	}
}

vec2 BallWorld::unlapEdge( vec2 p, float r, const Contour& poly, const Ball* b )
{
	float dist ;

	vec2 x = closestPointOnPoly( p, poly.mPolyLine, 0, 0, &dist );

	if ( dist < r )
	{
		if (b) onBallContourCollide( *b, poly );
		
		return glm::normalize( p - x ) * r + x ;
	}
	else return p ;
}

vec2 BallWorld::unlapHoles( vec2 p, float r, ContourKind kind, const Ball* b )
{
	float dist ;
	vec2 x ;
	
	const Contour * nearestHole = mContours.findClosestContour( p, &x, &dist,
		ContourVec::getAndFilter( ContourVec::getKindFilter(kind), mContourFilter ) ) ;
	
	if ( nearestHole && dist < r && !nearestHole->mPolyLine.contains(p) )
		// ensure we aren't actually in this hole or that would be bad...
	{
		if (b) onBallContourCollide( *b, *nearestHole );

		return glm::normalize( p - x ) * r + x ;
	}
	else return p ;
}

/*	Marc ten Bosch suggests we could refactor this into a giant pile of edges.
	That would easily handle both insides and outsides, and allow us to ignore tree topology (I think, though maybe not).
*/

vec2 BallWorld::resolveCollisionWithContours ( vec2 point, float radius, const Ball* b )
{
	// inside a poly?
	const Contour* in = mContours.findLeafContourContainingPoint(point,mContourFilter) ;
	
	if (in)
	{
		if ( !in->mIsHole )
		{
			// we are in paper
			vec2 p = point ;
			
			p = unlapEdge ( p, radius, *in, b ) ; // get off an edge we might be on
			p = unlapHoles( p, radius, ContourKind::Holes, b ) ; // make sure we aren't in a hole
			
			// done
			return p ;
		}
		else
		{
			// push us out of this hole
			vec2 x = closestPointOnPoly(point, in->mPolyLine) ;

			if (b) onBallContourCollide( *b, *in );
			
			return glm::normalize( x - point ) * radius + x ;
		}
	}
	else
	{
		// inside of no contour
		
		// push us into nearest paper
		vec2 x ;
		
		const Contour* nearest = mContours.findClosestContour(
			point, &x, 0,
			ContourVec::getAndFilter( ContourVec::getKindFilter(ContourKind::NonHoles), mContourFilter )
		);
		
		if ( nearest )
		{
			if (b) onBallContourCollide( *b, *nearest );
			
			return glm::normalize( x - point ) * radius + x ;
		}
		else return point; // ah! no constraints.
	}
}

vec2 BallWorld::resolveCollisionWithInverseContours ( vec2 point, float radius, const Ball* b )
{
	// inside a poly?
	const Contour* in = mContours.findLeafContourContainingPoint(point,mContourFilter) ;
	
	// ok, find closest
	if (in)
	{
		if ( !in->mIsHole )
		{
			// in paper

			// push us out of this contour
			bool pushOut = true ;
			
			vec2 x1 = closestPointOnPoly(point, in->mPolyLine) ;
			
			// but what if it's better to get us into a smaller hole inside?
			vec2 x2 ;
			float x2dist;
			
			const Contour* interiorHole = mContours.findClosestContour(
				point,&x2,&x2dist,
				ContourVec::getAndFilter( ContourVec::getKindFilter(ContourKind::Holes), mContourFilter )
				);
			
			if ( interiorHole )
			{
				// this is closer, so replace x1
				if ( x2dist < glm::distance(x1,point) )
				{
					pushOut = false;
					x1 = x2;
				}
			}
			
			// compute fix
			point = glm::normalize( x1 - point ) * radius + x1 ;
			
			// note
			if (b) onBallContourCollide(*b, pushOut ? *in : *interiorHole );
		}
		else
		{
			// in hole			
			vec2 p = point ;
			
			p = unlapEdge ( p, radius, *in, b ) ;
			p = unlapHoles( p, radius, ContourKind::NonHoles, b ) ;
			
			// done
			point = p ;
		}
	}
	else
	{
		// in empty space
		
		// make sure we aren't grazing a contour
		vec2 x;
		float dist;
		const Contour * nearest = mContours.findClosestContour(
			point, &x, &dist,
			ContourVec::getAndFilter( ContourVec::getKindFilter(ContourKind::NonHoles), mContourFilter )
			);
		
		if ( nearest && dist < radius )
		{
			point = glm::normalize( point - x ) * radius + x ;
			
			if (b) onBallContourCollide(*b,*nearest);
		}
		
		// make sure we are inside the world (not floating away)
		if ( getWorldBoundsPoly().size() > 0 )
		{
			vec2 x1 = closestPointOnPoly( point, getWorldBoundsPoly(), 0, 0, &dist );

			if ( getWorldBoundsPoly().contains(point) )
			{
				if ( dist < radius )
				{
					point = glm::normalize( point - x1 ) * radius + x1 ;
				
					if (b) onBallWorldBoundaryCollide(*b);
				}
			}
			else
			{
				point = glm::normalize( x1 - point ) * radius + x1 ;
				
				if (b) onBallWorldBoundaryCollide(*b);
			}
		}
	}
	
	// return
	return point;
}

void BallWorld::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_b:
		{
			newRandomBall( getRandomPointInWorldBoundsPoly() );
			// we could traverse paper hierarchy and pick a random point on paper...
			// might be a useful function, randomPointOnPaper()
		}
		break ;
			
		case KeyEvent::KEY_c:
			clearBalls() ;
			break ;
	}
}

void BallWorld::drawMouseDebugInfo( vec2 mouseInWorld )
{
	// test collision logic
	const float r = getBallDefaultRadius() ;
	
	vec2 fixed = resolveCollisionWithContours(mouseInWorld,r);
//		vec2 fixed = mBallWorld.resolveCollisionWithInverseContours(mouseInWorld,r);
	
	gl::color( ColorAf(0.f,0.f,1.f) ) ;
	gl::drawStrokedCircle(fixed,r);
	
	gl::color( ColorAf(0.f,1.f,0.f) ) ;
	gl::drawLine(mouseInWorld, fixed);
}

// Synthesis
void onBallBallCollide			( const Ball&, const Ball& )
{

}
void onBallContourCollide		( const Ball&, const Contour& )
{

}
void onBallWorldBoundaryCollide	( const Ball& )
{

}
