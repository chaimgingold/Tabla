//
//  GameLibraryView.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#ifndef GameLibraryView_hpp
#define GameLibraryView_hpp

#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class TablaApp;

class GameLibraryView : public View
{
public:

	void layout( Rectf windowRect ) ;
	
	void draw() override;
	void mouseDown( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	
private:
	vec2 mMouseLoc;
	
	vec2 getItemSize() const;
	Rectf getMenuRect() const;
	int getHighlightIndex( vec2 ) const;
	
};

#endif /* GameLibraryView_hpp */
