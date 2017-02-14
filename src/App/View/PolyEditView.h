//
//  PolyEditView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#ifndef PolyEditView_hpp
#define PolyEditView_hpp

#include "GameWorld.h"
#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class TablaApp;
class MainImageView;

class PolyEditView : public View
{
public:

	PolyEditView( TablaApp&, function<PolyLine2()>, string polyCoordSpace );
	
	void draw() override;
	bool pick( vec2 ) override;

	void mouseDown( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;

	void setMainImageView( std::shared_ptr<MainImageView> ); // becomes the parent
	
	void setSetPolyFunc( function<void(const PolyLine2&)> f ) {mSetPolyFunc=f;}

	void setDoLiveUpdate( bool v ) { mDoLiveUpdate=v; }
	bool getDoLiveUpdate() const { return true; }

	// what stages are we editable in?
	// by default (if empty), then all
	vector<string>& getEditableInStages() { return mEditableInStages; }
	const vector<string>& getEditableInStages() const { return mEditableInStages; }
	
	bool isEditable() const ;
	
	void setQuantizeToUnit ( float f ) { mQuantizeToUnit=f; } // 0 for off
	void setConstrainToRect( bool constrain=true );
	void setConstrainToRectWithAspectRatioOfPipelineStage( string );
	void setCanEditVertexMask( vector<bool> m ) { mCanEditVertexMask=m; }
	void setDrawSize( bool v=true ) { mDrawSize=v; }
	void setDrawPipelineStage( string s ) { mDrawPipelineStage=s; } 
	
private:
	
	int pickPoint( vec2 ) const; // image coord space
	Rectf getPointControlRect( vec2 ) const; // image coord space

	mat4 getPolyToImageTransform() const;
	mat4 getImageToPolyTransform() const;
	
	PolyLine2 getPolyInImageSpace( bool withDrag=true ) const;
	PolyLine2 getPolyInPolySpace ( bool withDrag=true ) const;

	vec2  quantize( vec2 v ) const { return vec2(quantize(v.x),quantize(v.y));}
	float quantize( float  ) const;

	PolyLine2 constrainToRect( PolyLine2 p, int fromIndex ) const;
	PolyLine2 constrainAspectRatio( PolyLine2 p, int fromIndex ) const;

	const Pipeline& getPipeline() const;	
	bool canEditVertex( int ) const;
	
	std::shared_ptr<MainImageView> mMainImageView;	
	
	TablaApp& mApp;
	
	string mPolyCoordSpace;
	
	function<PolyLine2()> mGetPolyFunc;
	function<void(const PolyLine2&)> mSetPolyFunc;
	vector<bool> mCanEditVertexMask; // empty for all are ok
	
	int mDragPointIndex=-1;
	vec2 mDragStartMousePos; // should really be in view manager; put it there when we do backlinks
	
	vec2 mDragStartPoint; // where poly point started (in poly space)
	vec2 mDragAtPoint; // where dragged poly point is (")
	
	bool mDoLiveUpdate=true;
	
	vector<string> mEditableInStages;
	
	float mQuantizeToUnit=0.f;
	bool  mConstrainToRect=false;
	string mConstrainToRectWithAspectRatioOfPipelineStage;
	
	bool  mDrawSize=false;
	string mDrawPipelineStage;
};

#endif /* PolyEditView_hpp */
