//
//  PopUpMenuView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/2/17.
//
//

#include "PopUpMenuView.h"
#include "TablaApp.h"

const vec2 kTextOffset(4,-4);

void PopUpMenuView::layout( Rectf r )
{
	setFrame(r);
	setBounds(r - r.getUpperLeft());
}

float PopUpMenuView::getHeightWhenClosed()
{
	return roundf(
		TablaApp::get()->getFont()->getAscent()
		+ TablaApp::get()->getFont()->getDescent()
		+ 4.f);
}

string PopUpMenuView::getDefaultItemName() const
{
	// slower than what you might override and do, but it works.
	vector<string> items = getItems();
	const int defaultItem = getDefaultItem();
	
	string selectionName="——";
	if (defaultItem>=0 && defaultItem < items.size()) selectionName = items[defaultItem];
	return selectionName;
}

void PopUpMenuView::draw()
{
	if (!TablaApp::get()) return;
	
	const auto &app = *(TablaApp::get());
	vector<string> items = getItems();
	
	gl::color(1,1,1,1);
	gl::drawSolidRect( getBounds() );
	
	gl::color(0,0,0, getHasMouseDown() ? .5 : 1.f );
	{
		gl::ScopedScissor scissor( getScissorLowerLeft(), getScissorSize() );
		app.getFont()->drawString( getDefaultItemName(), getBounds().getLowerLeft() + kTextOffset );
	}
	
	if ( getHasMouseDown() )
	{
		const int highlight = getHighlightIndex( rootToChild(mMouseLoc) );
		
		const vec2 itemSize = getItemSize();
		const Rectf menuRect = getMenuRect();

//		gl::ScopedScissor scissor( getScissorLowerLeft(menuRect), getScissorSize(menuRect) );
// doesn't work right for some reason

		gl::color(1,1,1,1);
		gl::drawSolidRect(menuRect);
		
		auto c = items.begin();
		
		for( int i=0;
			 c != items.end();
			 ++i, ++c )
		{
			vec2 itemUL = getBounds().getUpperLeft() + vec2(0,-itemSize.y);
			
			Rectf r( itemUL, itemUL+itemSize );
			r += vec2(0,-(float)i*itemSize.y);
			
			if (highlight==i)
			{
				gl::color(.25,.5,.5,1.);
				gl::drawSolidRect(r);
				gl::color(1,1,1,1);
			}
			else {
				gl::color(0,0,0,1);
			}
			
			string worldName = *c;
			app.getFont()->drawString( worldName, r.getLowerLeft() + kTextOffset );
		}
	}
}

void PopUpMenuView::mouseDown( MouseEvent )
{
	mMouseDownIndex = getDefaultItem();
}

void PopUpMenuView::mouseDrag( MouseEvent e )
{
	mMouseLoc = e.getPos();
}

void PopUpMenuView::mouseUp  ( MouseEvent e )
{
	const int highlight = getHighlightIndex( rootToChild(e.getPos()) );

	// only do something if changed
	if (highlight != mMouseDownIndex)
	{
		userChoseItem(highlight);
	}
}

vec2 PopUpMenuView::getItemSize() const
{
	return getBounds().getSize();
}

Rectf PopUpMenuView::getMenuRect() const
{
	const float itemwidth = getBounds().getWidth();
	const float itemheight = getBounds().getHeight();
	const int   n = getItems().size();

	const vec2 menuSize( itemwidth, (float)n * itemheight );
	const vec2 menuUL = getBounds().getUpperLeft() + vec2(0,-menuSize.y);
	Rectf menuRect( menuUL, menuUL + menuSize );

	return menuRect;
}

int PopUpMenuView::getHighlightIndex( vec2 p ) const
{
	Rectf r = getMenuRect();
	
	if ( r.contains(p) )
	{
		p -= r.getLowerLeft();
		p.y /= getItemSize().y;
		
		return -p.y;
	}
	else return mMouseDownIndex;
}
