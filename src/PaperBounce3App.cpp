#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class PaperBounce3App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	gl::TextureRef			mTexture;
	CaptureRef				mCapture;

};

void PaperBounce3App::setup()
{
	mCapture = Capture::create( 640, 480 );
	mCapture->start();
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
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
		
		cv::findContours( thresholded, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
		
//		cv::findContours(<#InputOutputArray image#>, <#OutputArrayOfArrays contours#>, <#OutputArray hierarchy#>, <#int mode#>, <#int method#>) ;
		
		// convert to texture
//		mTexture = gl::Texture::create( fromOcv( output ), gl::Texture::Format().loadTopDown() );
//		mTexture = gl::Texture::create( fromOcv( input ), gl::Texture::Format().loadTopDown() );
	}
}

void PaperBounce3App::draw()
{
	if( ( ! mTexture ) ) return;
	
	gl::clear();
	gl::enableAlphaBlending();
	
	gl::setMatricesWindow( getWindowSize() );
	gl::color( 1, 1, 1 );
	gl::draw( mTexture );
}

CINDER_APP( PaperBounce3App, RendererGl )
