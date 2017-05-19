//
//  VectorEditView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 5/18/17.
//
//

#include "VectorEditView.h"
#include "TablaApp.h"
#include "cinder/gl/draw.h"

VectorEditView::VectorEditView()
{
}

void VectorEditView::draw()
{
	gl::color(0,0,0);
	gl::drawSolidCircle( mCenter, mRadius );

	gl::color(0,1,1,.5f);
	gl::drawStrokedCircle( mCenter, mRadius );

	vec2 tip = mCenter + getVec() * mRadius;

	gl::color(0,1,1);
	gl::drawLine( mCenter, tip );
	gl::drawSolidCircle( tip, 2.f );
}

bool VectorEditView::pick( vec2 p )
{
	return distance(p,mCenter) <= mRadius;
}

void VectorEditView::mouseDown( MouseEvent event )
{
	updateWithMouse(event);
}

void VectorEditView::mouseUp  ( MouseEvent event )
{
	updateWithMouse(event);
}

void VectorEditView::mouseDrag( MouseEvent event )
{
	updateWithMouse(event);
}

void VectorEditView::updateWithMouse( MouseEvent event )
{
	const vec2 pos = rootToChild(event.getPos());
	
	vec2 v = pos - mCenter;
	
	// normalized only
	if ( v!=vec2(0,0) )
	{
		v = normalize(v);
		setVec(v);
	}
}

vec2 VectorEditView::getVec() const
{
	auto game = TablaApp::get()->getGameWorld();
	
	if (game)
	{
		auto vecs = game->getOrientationVecs();

		auto v = vecs.find( getVecName() );
		
		if ( v != vecs.end() ) return v->second;
	}
	
	// nothing here...
	return vec2(0,0);
}

void VectorEditView::setVec( vec2 v )
{
	auto game = TablaApp::get()->getGameWorld();
	
	if (game)
	{
		game->setOrientationVec( getVecName(), v );
	}
}

string VectorEditView::getVecName() const
{
	auto game = TablaApp::get()->getGameWorld();
	
	if (game)
	{
		auto vecs = game->getOrientationVecs();
		if ( !vecs.empty() ) return vecs.begin()->first; // just do the first...
	}
	
	return string("");
}

/*
mat4 WorldUITransformer::getVecToImageTransform() const
{
	return getPipeline().getCoordSpaceTransform(
		mVecCoordSpace,
		mMainImageView ? mMainImageView->getPipelineStageName() : "world"
		);
}

mat4 WorldUITransformer::getImageToVecTransform() const
{
	return getPipeline().getCoordSpaceTransform(
		mMainImageView ? mMainImageView->getPipelineStageName() : "world",
		mVecCoordSpace
		);
	// should be inverse of getPolyToImageTransform()
}
*/
