#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;


//const cv::RetrievalModes kContourHierStyle = cv::RETR_TREE ;      // whole enchilada
const cv::RetrievalModes kContourHierStyle = cv::RETR_CCOMP ;     // is just two level, what we want
//const cv::RetrievalModes kContourHierStyle = cv::RETR_EXTERNAL ;

const float kResScale = 1.f ;
const float kContourMinRadius = 3.f  * kResScale ;
const float kContourMinArea   = 100.f * kResScale ;
const float kContourDPEpislon = 5.f  * kResScale ;
const float kContourMinWidth  = 5.f  * kResScale ;


const float kBallDefaultRadius = 8.f ;

//const vec2 kCaptureSize = vec2( 640, 480 ) ;
const vec2 kCaptureSize = vec2( 16.f/9.f * 480.f, 480 ) ;

namespace cinder {
	
	PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

}

class cBall {
	
public:
//	cBall();
	
	vec2 mLoc ;
	vec2 mVel ;
	float mRadius ;
	ColorAf mColor ;
};

class cContour {
public:
	PolyLine2	mPolyLine ;
	vec2		mCenter ;
	float		mRadius ;
	float		mArea ;
	
	bool		mIsInside = false ;
};

class PaperBounce3App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	void tickBalls() ;
	

	gl::TextureRef			mTexture;
	CaptureRef				mCapture;
	
	vector<cContour>		mContours;

	std::vector<cBall>		mBalls ;
};

void PaperBounce3App::setup()
{
	mCapture = Capture::create( kCaptureSize.x, kCaptureSize.y );
	mCapture->start();
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	cBall ball ;
	
	ball.mColor = ColorAf::hex(0xC62D41);
	ball.mLoc   = event.getPos() ;
	ball.mRadius = kBallDefaultRadius ;
	
	mBalls.push_back( ball ) ;
}

void PaperBounce3App::update()
{
	if( mCapture->checkNewFrame() )
	{
		// Get surface data
		Surface surface( *mCapture->getSurface() );
		mTexture = gl::Texture::create( surface );
		
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
		
		cv::findContours( thresholded, contours, hierarchy, kContourHierStyle, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
		
		// filter and process contours into our format
		mContours.clear() ;
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
					std::min( rotatedRect.size.width, rotatedRect.size.height ) > kContourMinWidth )
			{
				auto addContour = [&]( const vector<cv::Point>& c )
				{
					cContour myc ;
					
					myc.mPolyLine = fromOcv(c) ;
					myc.mRadius = radius ;
					myc.mCenter = fromOcv(center) ;
					myc.mArea   = area ;
					myc.mIsInside = hierarchy[i][3] != -1 ;
					
					mContours.push_back( myc );
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
		
		// convert to texture
//		mTexture = gl::Texture::create( fromOcv( output ), gl::Texture::Format().loadTopDown() );
//		mTexture = gl::Texture::create( fromOcv( input ), gl::Texture::Format().loadTopDown() );
	}
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
			
			gl::setMatrices( CameraOrtho( -barsw/2, kCaptureSize.x + barsw/2, kCaptureSize.y, 0.f, 0.f, 1.f ) ) ;
		}
		else if ( captureAspectRatio > windowAspectRatio )
		{
			// horizontal black bars
			float h = (1.f / windowAspectRatio) * kCaptureSize.x ;
			float barsh = h - kCaptureSize.y ;

			gl::setMatrices( CameraOrtho( 0.f, kCaptureSize.x, kCaptureSize.y + barsh/2, -barsh/2, 0.f, 1.f ) ) ;
		}
		else gl::setMatricesWindow( kCaptureSize.x, kCaptureSize.y ) ;
		
		// scale up window
//		float width = (kCaptureSize.y / getWindowSize().y) * (kCaptureSize.x) ;
//		float centerx = getWindowSize().x / 2.f ;
//		float left = centerx - width/2 ;
//		float right = left + width ;
//
//		
//		gl::setMatricesWindow( width, kCaptureSize.y ); // just at origin; doesn't work for scaled windows

		//		left = 0.f ;
//		right =
//		gl::setMatrices( CameraOrtho( 0, getWindowSize().x, getWindowSize().y, 0.f, 0.f, 1.f ) ) ;
			// <- this, but don't deform it.
	}

	// camera image
	if ( mTexture )
	{
		gl::color( 1, 1, 1 );
		gl::draw( mTexture );
	}
	
	// draw contours
	{
		for( auto c : mContours )
		{
			ColorAf color ;
			
			if ( c.mIsInside ) color = ColorAf::hex( 0x43730F ) ;
			else color = ColorAf::hex( 0xF19878 ) ;
			
			gl::color(color) ;
			gl::draw(c.mPolyLine) ;
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
}

CINDER_APP( PaperBounce3App, RendererGl, [&]( App::Settings *settings ) {
	settings->setWindowSize(kCaptureSize);
	settings->setTitle("See Paper") ;
})