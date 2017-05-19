//
//  VectorEditView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 5/18/17.
//
//

#ifndef VectorEditView_hpp
#define VectorEditView_hpp

#include "View.h"

class TablaApp;

/*
class WorldUITransformer
{
public:
	WorldUITransformer( string myCoordSpace );

	mat4 getMeToImageTransform( TablaApp& ) const;
	mat4 getImageToMeTransform( TablaApp& ) const;
	
private:
	string mMeCoordSpace;
	
};*/


class VectorEditView : public View
{
public:
	VectorEditView();
	
	void setCenter( vec2  c ) { mCenter=c; }
	void setRadius( float r ) { mRadius=r; }
	
	void draw() override;
	bool pick( vec2 ) override;

	void mouseDown( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;
	
private:
	void updateWithMouse( MouseEvent );
	
	vec2 getVec() const;
	void setVec( vec2 v );
	string getVecName() const;
	
	vec2  mCenter;
	float mRadius=10.f;
	
};

#endif /* VectorEditView_hpp */
