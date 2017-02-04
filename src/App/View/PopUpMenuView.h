//
//  PopUpMenuView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/2/17.
//
//

#ifndef PopUpMenuView_hpp
#define PopUpMenuView_hpp

#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class TablaApp;

class PopUpMenuView : public View
{
public:

	void layout( Rectf windowRect );
	static float getHeightWhenClosed();
	
	void draw() override;
	void mouseDown( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	
protected:
	virtual int getDefaultItem() const = 0;
	virtual vector<string> getItems() const = 0;
	virtual void userChoseItem( int ) = 0;
	virtual string getDefaultItemName() const; // uses getItems() + getDefaultItem(); override to speed up
	
private:
	vec2 mMouseLoc;
	
	vec2 getItemSize() const;
	Rectf getMenuRect() const;
	int getHighlightIndex( vec2 ) const;
	
	int mMouseDownIndex=-1;
	
};

#endif
