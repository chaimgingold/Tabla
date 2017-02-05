//
//  PolyEditView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#ifndef PolyEditView_hpp
#define PolyEditView_hpp

#include "Pipeline.h"
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

	PolyEditView( Pipeline&, function<PolyLine2()>, string polyCoordSpace );
	
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
	
private:
	
	std::shared_ptr<MainImageView> mMainImageView;
	
	int pickPoint( vec2 ) const; // image coord space
	Rectf getPointControlRect( vec2 ) const; // image coord space

	mat4 getPolyToImageTransform() const;
	mat4 getImageToPolyTransform() const;
	
	Pipeline& mPipeline;

	string mPolyCoordSpace;
	
	function<PolyLine2()> mGetPolyFunc;
	function<void(const PolyLine2&)> mSetPolyFunc;
	
	PolyLine2 getPolyInImageSpace( bool withDrag=true ) const;
	PolyLine2 getPolyInPolySpace ( bool withDrag=true ) const;
	
	int mDragPointIndex=-1;
	vec2 mDragStartMousePos; // should really be in view manager; put it there when we do backlinks
	
	vec2 mDragStartPoint; // where poly point started (in poly space)
	vec2 mDragAtPoint; // where dragged poly point is (")
	
	bool mDoLiveUpdate=true;
	
	vector<string> mEditableInStages;
};

#endif /* PolyEditView_hpp */
