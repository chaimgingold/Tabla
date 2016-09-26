//
//  Balls.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "BallWorld.h"
#include "geom.h"
#include "cinder/Rand.h"
#include "xml.h"

void BallWorld::setParams( XmlTree xml )
{
	getXml(xml,"BallDefaultRadius",mBallDefaultRadius);
	getXml(xml,"BallDefaultMaxRadius",mBallDefaultMaxRadius);
	getXml(xml,"BallDefaultColor",mBallDefaultColor);
	getXml(xml,"BallMaxVel",mBallMaxVel);
}

void BallWorld::draw( bool highQuality )
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
			int numSegments = -1 ;
			
			if (highQuality) numSegments = 20;
			
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

void BallWorld::update()
{
	int   steps = 1 ;
	float delta = 1.f / (float)steps ;
	
	for( int step=0; step<steps; ++step )
	{
		// accelerate
		for( auto &b : mBalls )
		{
			b.mLoc += b.mAccel * delta*delta ;
			b.mAccel = vec2(0,0) ;
		}

		// ball <> contour collisions
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
				
				b.setVel(
					  glm::reflect( oldVel, surfaceNormal ) // transfer old velocity, but reflected
//						+ normalize(newLoc - oldLoc) * max( distance(newLoc,oldLoc), b.mRadius * .1f )
					+ normalize(newLoc - oldLoc) * .1f
						// accumulate energy from impact
						// would be cool to use optic flow for this, and each contour can have a velocity
					) ;

				// squash?
				b.noteSquashImpact( surfaceNormal * length(b.getVel()) ) ; //newLoc - oldLoc ) ;
			}
		}

		// ball <> ball collisions
		resolveBallCollisions() ;
		
		// cap velocity
		// (i think this is mostly to compensate for aggressive contour<>ball collisions in which balls get pushed in super fast;
		// alternative would be to cap impulse there)
		if (1)
		{
			for( auto &b : mBalls )
			{
				vec2 v = b.getVel() ;
				
				if ( length(v) > mBallMaxVel )
				{
					b.setVel( normalize(v) * mBallMaxVel ) ;
				}
			}
		}
		
		// squash
		for( auto &b : mBalls )
		{
			b.mSquash *= .7f ;
		}
		
		// inertia
		for( auto &b : mBalls )
		{
			vec2 vel = b.getVel() ; // rewriting mLastLoc will stomp vel, so get it first
			b.mLastLoc = b.mLoc ;
			b.mLoc += vel ;
		}
	}
}

void BallWorld::newRandomBall ( vec2 loc )
{
	Ball ball ;
	
	ball.mColor = mBallDefaultColor;
	ball.setLoc( loc ) ;
	ball.mRadius = Rand::randFloat(mBallDefaultRadius,mBallDefaultMaxRadius) ;
	ball.setMass( M_PI * powf(ball.mRadius,3.f) ) ;
	
	ball.mCollideWithContours = randBool();
	if (!ball.mCollideWithContours) ball.mColor = Color(0,0,1);
	
	ball.setVel( Rand::randVec2() * mBallDefaultRadius/2.f ) ;
	
	mBalls.push_back( ball ) ;
}

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
}

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
	
	const Contour * nearestHole = mContours.findClosestContour( p, &x, &dist, kind ) ;
	
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
	const Contour* in = mContours.findLeafContourContainingPoint(point) ;
	
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
		
		const Contour* nearest = mContours.findClosestContour( point, &x, 0, ContourKind::NonHoles );
		
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
	const Contour* in = mContours.findLeafContourContainingPoint(point) ;
	
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
			
			const Contour* interiorHole = mContours.findClosestContour(point,&x2,&x2dist,ContourKind::Holes);
			
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
		const Contour * nearest = mContours.findClosestContour( point, &x, &dist, ContourKind::NonHoles ) ;
		
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