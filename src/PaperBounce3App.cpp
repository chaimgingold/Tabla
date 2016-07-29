#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

#include <map>

using namespace ci;
using namespace ci::app;
using namespace std;


const float kResScale = 1.f ;
const float kContourMinRadius = 3.f  * kResScale ;
const float kContourMinArea   = 100.f * kResScale ;
const float kContourDPEpislon = 5.f  * kResScale ;
const float kContourMinWidth  = 5.f  * kResScale ;


const float kBallDefaultRadius = 8.f ;

//const vec2 kCaptureSize = vec2( 640, 480 ) ;
const vec2 kCaptureSize = vec2( 16.f/9.f * 480.f, 480 ) ;


const bool kDebug = false ;

const bool kAutoFullScreenProjector	= !kDebug ; // default: true
const bool kDrawCameraImage			= false  ; // default: false
const bool kDrawContoursFilled		= kDebug ;  // default: false
const bool kDrawMouseDebugInfo		= kDebug ;
const bool kDrawPolyBoundingRect	= kDebug ;
const bool kDrawContourTree			= kDebug ;

namespace cinder {
	
	PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

}

class cWindowData {
public:
	bool mIsAux = false ;
};

class cBall {
	
public:
	vec2 mLoc ;
	vec2 mLastLoc ;
	vec2 mAccel ;
	
//	vec2 mVel ;
	
	float mRadius ;
	ColorAf mColor ;
	
	void setLoc( vec2 l ) { mLoc=mLastLoc=l; }
	void setVel( vec2 v ) { mLastLoc = mLoc - v ; }
	vec2 getVel() const { return mLoc - mLastLoc ; }
};

enum class ContourKind {
	Any,
	Holes,
	NonHoles
} ;

class cContour {
public:
	PolyLine2	mPolyLine ;
	vec2		mCenter ;
	Rectf		mBoundingRect ;
	float		mRadius ;
	float		mArea ;
	
	bool		mIsHole = false ;
	bool		mIsLeaf = true ;
	int			mParent = -1 ; // index into the contour we in
	vector<int> mChild ; // indices into mContour of contours which are in me
	int			mTreeDepth = 0 ;
	
	int			mOcvContourIndex = -1 ;
	
	bool		isKind ( ContourKind kind ) const
	{
		switch(kind)
		{
			case ContourKind::NonHoles:	return !mIsHole ;
			case ContourKind::Holes:	return  mIsHole ;
			case ContourKind::Any:
			default:					return  true ;
		}
	}
	
	bool		contains( vec2 point ) const
	{
		return mBoundingRect.contains(point) && mPolyLine.contains(point) ;
	}
	
};

class PaperBounce3App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override { mMousePos = event.getPos(); }
	void update() override;
	void draw() override;
	void resize() override;
	void keyDown( KeyEvent event ) override;
	
	void drawProjectorWindow() ;
	void drawAuxWindow() ;

	void updateVision(); // updates mCameraTexture, mContours
	void updateBalls() ;
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	
	CaptureRef			mCapture;
	gl::TextureRef		mCameraTexture;
	
	vector<cContour>	mContours;
	
	vector<cBall>		mBalls ;
	
	app::WindowRef		mAuxWindow ; // for other debug info, on computer screen
	
	double				mLastFrameTime = 0. ;
	vec2				mMousePos ;
	
	// physics/geometry helpers
	const cContour* findClosestContour ( vec2 point, vec2* closestPoint=0, float* closestDist=0, ContourKind kind = ContourKind::Any ) const ; // assumes findLeafContourContainingPoint failed
	
	const cContour* findLeafContourContainingPoint( vec2 point ) const ;
	
	vec2 resolveCollisionWithContours	( vec2 p, float r ) const ; // returns pinned version of point
	vec2 resolveCollisionWithBalls		( vec2 p, float r, cBall* ignore=0, float correctionFraction=1.f ) const ;
		// simple pushes p out of overlapping balls.
		// but might take multiple iterations to respond to all of them
		// fraction is (0,1], how much of the collision correction to do.
	
	vec2 resolveBallCollisions			() ;
	
	
	// for main window, the projector display
	vec2 mouseToWorld( vec2 );
	void updateWindowMapping();
	
	float	mOrthoRect[4]; // points for glOrtho
};

void PaperBounce3App::setup()
{
	mLastFrameTime = getElapsedSeconds() ;
	
	auto displays = Display::getDisplays() ;
	cout << displays.size() << " Displays" << endl ;
	for ( auto d : displays )
	{
		cout << "\t" << d->getBounds() << endl ;
	}
	
	mCapture = Capture::create( kCaptureSize.x, kCaptureSize.y );
	mCapture->start();

	// Fullscreen main window in second display
	if ( displays.size()>1 && kAutoFullScreenProjector )
	{
		getWindow()->setPos( displays[1]->getBounds().getUL() );
		
		getWindow()->setFullScreen(true) ;
	}

	// aux display
	if (0)
	{
		// for some reason this seems to create three windows once we fullscreen the main window :P
		app::WindowRef mAuxWindow = createWindow( Window::Format().size( kCaptureSize.x, kCaptureSize.y ) );
		
		cWindowData* data = new cWindowData ;
		data->mIsAux=true ;
		mAuxWindow->setUserData(data);
		
		if (displays.size()==1)
		{
			// move it out of the way on one screen
			mAuxWindow->setPos( getWindow()->getBounds().getUL() + ivec2(0,getWindow()->getBounds().getHeight() + 16.f) ) ;
		}
	}
	
	// text
	mTextureFont = gl::TextureFont::create( Font("Avenir",12) );
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	cBall ball ;
	
	ball.mColor = ColorAf::hex(0xC62D41);
	ball.setLoc( mouseToWorld( event.getPos() ) ) ;
	ball.mRadius = kBallDefaultRadius ;
	
	ball.mRadius += Rand::randFloat(0.f,kBallDefaultRadius);
	
	ball.setVel( Rand::randVec2() * 2.f ) ;
	
	mBalls.push_back( ball ) ;
}

void PaperBounce3App::update()
{
	updateVision();
	updateBalls();
}

void PaperBounce3App::updateVision()
{
	if( mCapture->checkNewFrame() )
	{
		// Get surface data
		Surface surface( *mCapture->getSurface() );
		mCameraTexture = gl::Texture::create( surface );
		
		// make cv frame
		cv::Mat input( toOcv( Channel( surface ) ) );
		cv::Mat output, gray, thresholded ;
		
//		cv::Sobel( input, output, CV_8U, 1, 0 );
		
		//		cv::threshold( input, output, 128, 255, CV_8U );
		//		cv::Laplacian( input, output, CV_8U );
		//		cv::circle( output, toOcv( Vec2f(200, 200) ), 300, toOcv( Color( 0, 0.5f, 1 ) ), -1 );
		//		cv::line( output, cv::Point( 1, 1 ), cv::Point( 30, 30 ), toOcv( Color( 1, 0.5f, 0 ) ) );

		
		// gray image
		if (1)
		{
//			cv::cvtColor( input, gray, cv::COLOR_BGR2GRAY ); // already grayscale
			
			cv::GaussianBlur( input, gray, cv::Size(5,5), 0 );

			cv::threshold( gray, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
		}
		
		// contours
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		
		cv::findContours( thresholded, contours, hierarchy, cv::RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
		
		// filter and process contours into our format
		mContours.clear() ;
		
		map<int,int> ocvContourIdxToMyContourIdx ;
		
		for( int i=0; i<contours.size(); ++i )
		{
			const auto &c = contours[i] ;
			
			cv::Point2f center ;
			float		radius ;
			
			cv::minEnclosingCircle( c, center, radius ) ;
			
			float		area = cv::contourArea(c) ;
			
			cv::RotatedRect rotatedRect = minAreaRect(c) ;
			
			if (	radius > kContourMinRadius &&
					area > kContourMinArea &&
					min( rotatedRect.size.width, rotatedRect.size.height ) > kContourMinWidth )
			{
				auto addContour = [&]( const vector<cv::Point>& c )
				{
					cContour myc ;
					
					myc.mPolyLine = fromOcv(c) ;
					myc.mRadius = radius ;
					myc.mCenter = fromOcv(center) ;
					myc.mArea   = area ;
//					myc.mIsHole = hierarchy[i][3] != -1 ;
					myc.mBoundingRect = Rectf( myc.mPolyLine.getPoints() );
					myc.mOcvContourIndex = i ;
					
					myc.mTreeDepth = 0 ;
					{
						int n = i ;
						while ( (n = hierarchy[n][3]) > 0 )
						{
							myc.mTreeDepth++ ;
						}
					}
					myc.mIsHole = ( myc.mTreeDepth % 2 ) ; // odd depth # children are holes
					myc.mIsLeaf = hierarchy[i][2] < 0 ;
					
					mContours.push_back( myc );
					
					// store my index mapping
					ocvContourIdxToMyContourIdx[i] = mContours.size()-1 ;
				} ;
				
				if (1)
				{
					// simplify
					vector<cv::Point> approx ;
					
					cv::approxPolyDP( c, approx, kContourDPEpislon, true ) ;
					
					addContour(approx);
				}
				else addContour(c) ;
			}
		}
		
		// add contour topology metadata
		// (this might be screwed up because of how we cull contours;
		// a simple fix might be to find orphaned contours--those with parents that don't exist anymore--
		// and strip them, too. but this will, in turn, force us to rebuild indices.
		// it might be simplest to just "hide" rejected contours, ignore them, but keep them around.
		for ( size_t i=0; i<mContours.size(); ++i )
		{
			cContour &c = mContours[i] ;
			
			if ( hierarchy[c.mOcvContourIndex][3] >= 0 )
			{
				c.mParent = ocvContourIdxToMyContourIdx[ hierarchy[c.mOcvContourIndex][3] ] ;
				
//				assert( myc.mParent is valid ) ;
				
				assert( c.mParent >= 0 && c.mParent < mContours.size() ) ;
				
				mContours[c.mParent].mChild.push_back( i ) ;
			}
		}
		
		// convert to texture
		mCameraTexture = gl::Texture::create( fromOcv( thresholded ), gl::Texture::Format().loadTopDown() );
//		mCameraTexture = gl::Texture::create( fromOcv( input ), gl::Texture::Format().loadTopDown() );
	}
}

void PaperBounce3App::updateBalls()
{
	const float kMaxBallVel = kBallDefaultRadius ;
	
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

		// collisions
		for( auto &b : mBalls )
		{
			{
				vec2 oldVel = b.getVel() ;
				vec2 oldLoc = b.mLoc ;
				vec2 newLoc = resolveCollisionWithContours( b.mLoc, b.mRadius ) ;
				
				// update loc
				b.mLoc = newLoc ;

				// update vel
				if ( newLoc != oldLoc )
				{
					vec2 surfaceNormal = glm::normalize( newLoc - oldLoc ) ;
					
					b.setVel(
						  glm::reflect( oldVel, surfaceNormal ) // transfer old velocity, but reflected
//						+ normalize(newLoc - oldLoc) * max( distance(newLoc,oldLoc), b.mRadius * .1f )
						+ normalize(newLoc - oldLoc) * .1f
							// accumulate energy from impact
							// would be cool to use optic flow for this, and each contour can have a velocity
						) ;
				}
			}
			
			
			{
				b.mLoc = resolveCollisionWithBalls   ( b.mLoc, b.mRadius, &b, .5f ) ;
			}
		}
		
		// cap velocity
		for( auto &b : mBalls )
		{
			vec2 v = b.getVel() ;
			
			if ( length(v) > kMaxBallVel )
			{
				b.setVel( normalize(v) * kMaxBallVel ) ;
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
}

vec2 closestPointOnLineSeg ( vec2 p, vec2 a, vec2 b )
{
	vec2 ap = p - a ;
	vec2 ab = b - a ;
	
	float ab2 = ab.x*ab.x + ab.y*ab.y;
	float ap_ab = ap.x*ab.x + ap.y*ab.y;
	float t = ap_ab / ab2;
	
	if (t < 0.0f) t = 0.0f;
	else if (t > 1.0f) t = 1.0f;
	
	vec2 x = a + ab * t;
	return x ;
}

vec2 closestPointOnPoly( vec2 pt, const PolyLine2& poly, size_t *ai=0, size_t *bi=0, float* dist=0 )
{
	float best = MAXFLOAT ;
	vec2 result = pt ;
	
	// assume poly is closed
	for( size_t i=0; i<poly.size(); ++i )
	{
		size_t j = (i+1) % poly.size() ;
		
		vec2 a = poly.getPoints()[i];
		vec2 b = poly.getPoints()[j];

		vec2 x = closestPointOnLineSeg(pt, a, b);
		
		float dist = glm::distance(pt,x) ; // could eliminate sqrt

		if ( dist < best )
		{
			best = dist ;
			result = x ;
			if (ai) *ai = i ;
			if (bi) *bi = j ;
		}
	}
	
	if (dist) *dist = best ;
	return result ;
}

const cContour* PaperBounce3App::findClosestContour ( vec2 point, vec2* closestPoint, float* closestDist, ContourKind kind ) const
{
	float best = MAXFLOAT ;
	const cContour* result = 0 ;
	
	// can optimize this by using bounding boxes as heuristic, but whatev for now.
	for ( const auto &c : mContours )
	{
		if ( c.isKind(kind) )
		{
			float dist ;
			
			vec2 x = closestPointOnPoly( point, c.mPolyLine, 0, 0, &dist ) ;
			
			if ( dist < best )
			{
				best = dist ;
				result = &c ;
				if (closestPoint) *closestPoint = x ;
				if (closestDist ) *closestDist  = dist ;
			}
		}
	}
	
	return result ;
}

const cContour* PaperBounce3App::findLeafContourContainingPoint( vec2 point ) const
{
	function<const cContour*(const cContour&)> search = [&]( const cContour& at ) -> const cContour*
	{
		if ( at.contains(point) )
		{
			for( auto childIndex : at.mChild )
			{
				const cContour* x = search( mContours[childIndex] ) ;
				
				if (x) return x ;
			}
			
			return &at ;
		}
		
		return 0 ;
	} ;

	for( const auto &c : mContours )
	{
		if ( c.mTreeDepth == 0 )
		{
			const cContour* x = search(c) ;
			
			if (x) return x ;
		}
	}
	
	return 0 ;
}

vec2 PaperBounce3App::resolveCollisionWithContours ( vec2 point, float radius ) const
{
//	size_t ai=-1, bi=-1 ; // line segment indices we collide with
	
	const cContour* inHole=0 ;
	const cContour* inPoly=0 ;
	// being inside a poly means we're OK (inside a piece of paper)
	// BUT we then should still test against holes to make sure...

	/*
	auto doNormal = [&]( const cContour& c, size_t ai, size_t bi )
	{
		if (surfaceNormal)
		{
			if (ai==-1) *surfaceNormal = vec2(0,0) ;
			else
			{
				vec2 a2b = c.mPolyLine.getPoints()[ai] - c.mPolyLine.getPoints()[bi] ;
				
				vec3 cross = glm::cross( vec3(a2b,0), vec3(0,0,1) ) ;
				
				*surfaceNormal = glm::normalize( vec2(cross.x,cross.y) ) ;
			}
		}
	};*/
	
	// inside a poly?
	const cContour* in = findLeafContourContainingPoint(point) ;
	
	if (in)
	{
		if ( in->mIsHole )	inHole = in ;
		else				inPoly = in ;
	}
	
	// ok, find closest
	if (inPoly)
	{
		// in paper

		// 1. make sure we aren't overlapping the edge
		auto unlapEdge = []( vec2 p, float r, const cContour& poly ) -> vec2
		{
			float dist ;

			vec2 x = closestPointOnPoly( p, poly.mPolyLine, 0, 0, &dist );

			if ( dist < r ) return glm::normalize( p - x ) * r + x ;
			else return p ;
		} ;
		
		// 2. make sure we aren't overlapping a hole
		auto unlapHole = [this]( vec2 p, float r ) -> vec2
		{
			float dist ;
			vec2 x ;
			
			const cContour * nearestHole = findClosestContour( p, &x, &dist, ContourKind::Holes ) ;
			
			if ( nearestHole && dist < r && !nearestHole->mPolyLine.contains(p) )
				// ensure we aren't actually in this hole or that would be bad...
			{
				return glm::normalize( p - x ) * r + x ;
			}
			else return p ;
		} ;
		
		// combine
		vec2 p = point ;
		
		p = unlapEdge( p, radius, *inPoly ) ;
		p = unlapHole( p, radius ) ;
		
		// done
		return p ;
	}
	else if ( inHole )
	{
		// push us out of this hole
		vec2 x = closestPointOnPoly(point, inHole->mPolyLine) ;
		
		return glm::normalize( x - point ) * radius + x ;
	}
	else
	{
		// inside of no contour
		
		// push us into nearest paper
		vec2 x ;
		findClosestContour( point, &x, 0, ContourKind::NonHoles ) ;
		
		return glm::normalize( x - point ) * radius + x ;
	}
}

vec2 PaperBounce3App::resolveCollisionWithBalls ( vec2 p, float r, cBall* ignore, float correctionFraction ) const
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

vec2 PaperBounce3App::resolveBallCollisions()
{
/*	for ( const auto &b1 : mBalls )
	{
		for ( const auto &b2 : mBalls )
		{
			if ( &b1==&b2 ) continue ;
			
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
	}*/
}

void PaperBounce3App::resize()
{
	updateWindowMapping();
}

void PaperBounce3App::updateWindowMapping()
{
	auto ortho = [&]( float a, float b, float c, float d )
	{
		mOrthoRect[0] = a ;
		mOrthoRect[1] = b ;
		mOrthoRect[2] = c ;
		mOrthoRect[3] = d ;
	};
	
	// set window transform
	{
		//
		const float captureAspectRatio = kCaptureSize.x / kCaptureSize.y ;
		const float windowAspectRatio  = (float)getWindowSize().x / (float)getWindowSize().y ;
		
		if ( captureAspectRatio < windowAspectRatio )
		{
			// vertical black bars
			float w = windowAspectRatio * kCaptureSize.y ;
			float barsw = w - kCaptureSize.x ;
			
			ortho( -barsw/2, kCaptureSize.x + barsw/2, kCaptureSize.y, 0.f ) ;
		}
		else if ( captureAspectRatio > windowAspectRatio )
		{
			// horizontal black bars
			float h = (1.f / windowAspectRatio) * kCaptureSize.x ;
			float barsh = h - kCaptureSize.y ;
			
			ortho( 0.f, kCaptureSize.x, kCaptureSize.y + barsh/2, -barsh/2 ) ;
		}
		else ortho( 0.f, kCaptureSize.x, kCaptureSize.y, 0.f ) ;
	}
}

vec2 PaperBounce3App::mouseToWorld( vec2 p )
{
	RectMapping rm( Rectf( 0.f, 0.f, getWindowSize().x, getWindowSize().y ),
					Rectf( mOrthoRect[0], mOrthoRect[3], mOrthoRect[1], mOrthoRect[2] ) ) ;
	
	return rm.map(p) ;
}

void PaperBounce3App::drawProjectorWindow()
{
	// set window transform
	gl::setMatrices( CameraOrtho( mOrthoRect[0], mOrthoRect[1], mOrthoRect[2], mOrthoRect[3], 0.f, 1.f ) ) ;
	
	// camera image
	if ( mCameraTexture && kDrawCameraImage )
	{
		gl::color( 1, 1, 1 );
		gl::draw( mCameraTexture );
	}
	
	// draw frame
	if (1)
	{
		gl::color( 1, 1, 1 );
		gl::drawStrokedRect( Rectf(0,0,kCaptureSize.x,kCaptureSize.y).inflated( vec2(-1,-1) ) ) ;
	}

	// draw contour bounding boxes, etc...
	if (kDrawPolyBoundingRect)
	{
		for( auto c : mContours )
		{
			if ( !c.mIsHole ) gl::color( 1.f, 1.f, 1.f, .2f ) ;
			else gl::color( 1.f, 0.f, .3f, .3f ) ;

			gl::drawStrokedRect(c.mBoundingRect);
		}
	}

	// draw contours
	{
		// filled
		if ( kDrawContoursFilled )
		{
			// recursive tree
			if (1)
			{
				function<void(const cContour&)> drawOne = [&]( const cContour& c )
				{
					if ( c.mIsHole ) gl::color(.0f,.0f,.0f,.8f);
					else gl::color(.2f,.2f,.4f,.5f);
					
					gl::drawSolid(c.mPolyLine);
					
					for( auto childIndex : c.mChild )
					{
						drawOne( mContours[childIndex] ) ;
					}
				};
				
				for( auto const &c : mContours )
				{
					if ( c.mTreeDepth==0 )
					{
						drawOne(c) ;
					}
				}
			}
			else
			// flat... (should work just the same, hmm)
			{
				// solids
				for( auto c : mContours )
				{
					if ( !c.mIsHole )
					{
						gl::color(.2f,.2f,.4f,.5f);
						gl::drawSolid(c.mPolyLine);
					}
				}
	
				// holes
				for( auto c : mContours )
				{
					if ( c.mIsHole )
					{
						gl::color(.0f,.0f,.0f,.8f);
						gl::drawSolid(c.mPolyLine);
					}
				}
			}
			
			// picked highlight
			vec2 mousePos = mouseToWorld(mMousePos) ;

			const cContour* picked = findLeafContourContainingPoint( mousePos ) ;
			
			if (picked)
			{
				if (picked->mIsHole) gl::color(6.f,.4f,.2f);
				else gl::color(.2f,.6f,.4f,.8f);
				
				gl::draw(picked->mPolyLine);
				
				if (0)
				{
					vec2 x = closestPointOnPoly(mousePos,picked->mPolyLine);
					gl::color(.5f,.1f,.3f,.9f);
					gl::drawSolidCircle(x, 5.f);
				}
			}
		}
		// outlines
		else
		{
			for( auto c : mContours )
			{
				ColorAf color ;
				
				if ( !c.mIsHole ) color = ColorAf(1,1,1); //color = ColorAf::hex( 0x43730F ) ;
				else color = ColorAf::hex( 0xF19878 ) ;
				
	//			color = ColorAf(1,1,1);
				
				gl::color(color) ;
				gl::draw(c.mPolyLine) ;
			}
		}
	}
	
	// draw balls
	{
		for( auto b : mBalls )
		{
			gl::color(b.mColor) ;
			gl::drawSolidCircle( b.mLoc, b.mRadius ) ;
		}
	}
	
	// test collision logic
	if ( kDrawMouseDebugInfo && getWindowBounds().contains(mMousePos) )
	{
		vec2 pt = mouseToWorld( mMousePos ) ;
		
		float r = kBallDefaultRadius ;
		
//		vec2 surfaceNormal ;
		
		vec2 fixed = resolveCollisionWithContours(pt,r);
		
		gl::color( ColorAf(0.f,0.f,1.f) ) ;
		gl::drawStrokedCircle(fixed,r);
		
		gl::color( ColorAf(0.f,1.f,0.f) ) ;
		gl::drawLine(pt, fixed);
		
//		gl::color( ColorAf(.8f,.2f,.3f,.5f) ) ;
//		gl::drawLine( fixed, fixed + surfaceNormal * r * 2.f ) ;
	}
	
	// draw contour debug info
	if (kDrawContourTree)
	{
		const float k = 16.f ;
		
		for ( size_t i=0; i<mContours.size(); ++i )
		{
			const auto& c = mContours[i] ;
			
			Rectf r = c.mBoundingRect ;
			
			r.y1 = ( c.mTreeDepth + 1 ) * k ;
			r.y2 = r.y1 + k ;
			
			if (c.mIsHole) gl::color(0,0,.3);
			else gl::color(0,.3,.4);
			
			gl::drawSolidRect(r) ;
			
			gl::color(.8,.8,.7);
			mTextureFont->drawString( to_string(i) + " (" + to_string(c.mParent) + ")",
				r.getLowerLeft() + vec2(2,-mTextureFont->getDescent()) ) ;
		}
	}
}

void PaperBounce3App::drawAuxWindow()
{
	gl::setMatricesWindow( kCaptureSize.x, kCaptureSize.y );
	gl::color( 1, 1, 1 );
	gl::draw( mCameraTexture );
}

void PaperBounce3App::draw()
{
	gl::clear();
	gl::enableAlphaBlending();
	gl::enable( GL_LINE_SMOOTH );
	gl::enable( GL_POLYGON_SMOOTH );
	//	gl::enable( gl_point_smooth );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	//	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	

	cWindowData* winData = getWindow()->getUserData<cWindowData>() ;
	
	if ( winData && winData->mIsAux )
	{
		drawAuxWindow();
	}
	else
	{
		drawProjectorWindow();
	}
}

void PaperBounce3App::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			cout << "Frame rate: " << getFrameRate() << endl ;
			break ;

		case KeyEvent::KEY_b:
			// make a random ball
			{
				cBall ball ;
				
				ball.mColor = ColorAf::hex(0xC62D41);
//				ball.mColor = ColorAf( Rand::randFloat(), Rand::randFloat(), Rand::randFloat() ) ;
					// they recognize themselves, confusing openCV. -- it's kind of awesome on its own.
				ball.setLoc( randVec2() * kCaptureSize ) ; // UH using as proxy for world.
				ball.mRadius = kBallDefaultRadius ;
				
				ball.mRadius += Rand::randFloat(0.f,kBallDefaultRadius);
				
				mBalls.push_back( ball ) ;
			}
			break ;
			
		case KeyEvent::KEY_c:
			mBalls.clear() ;
			break ;
	}
}


CINDER_APP( PaperBounce3App, RendererGl, [&]( App::Settings *settings ) {
	settings->setFrameRate(120.f);
	settings->setWindowSize(kCaptureSize);
	settings->setTitle("See Paper") ;
})