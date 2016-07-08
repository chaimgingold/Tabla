#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PaperBounce3App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void PaperBounce3App::setup()
{
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
}

void PaperBounce3App::update()
{
}

void PaperBounce3App::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( PaperBounce3App, RendererGl )
