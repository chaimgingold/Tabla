//
//  GameLibraryView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "GameLibraryView.h"
#include "TablaApp.h"

const vec2 kTextOffset(4,-4);

void GameLibraryView::layout( Rectf windowRect )
{
	vec2 lowerRight = windowRect.getLowerRight() + vec2(-8,-8);
	vec2 size(100,
		roundf(
		TablaApp::get()->mTextureFont->getAscent()
		+ TablaApp::get()->mTextureFont->getDescent()
		+ 4.f) );
	Rectf r( lowerRight - size, lowerRight );
	
	setFrame(r);
	setBounds(r - r.getUpperLeft());
}

void GameLibraryView::draw()
{
	if (!TablaApp::get()) return;
	
	const auto &app = *(TablaApp::get());
	const auto &games = app.mGameLibrary;

	gl::color(1,1,1,1);
	gl::drawSolidRect( getBounds() );
	
	gl::color(0,0,0, getHasMouseDown() ? .5 : 1.f );
	string worldName = app.mGameWorld ? app.mGameWorld->getUserName() : "——";
	app.mTextureFont->drawString( worldName, getBounds().getLowerLeft() + kTextOffset );

	
	if ( getHasMouseDown() )
	{
		const int highlight = getHighlightIndex( rootToChild(mMouseLoc) );
		
		const vec2 itemSize = getItemSize();
		const int   n = games.size();
		const Rectf menuRect = getMenuRect();
		
		gl::color(1,1,1,1);
		gl::drawSolidRect(menuRect);
		
		for( int i=0; i<n; ++i )
		{
			vec2 itemUL = getBounds().getUpperLeft() + vec2(0,-itemSize.y);
			
			Rectf r( itemUL, itemUL+itemSize );
			r += vec2(0,-(float)i*itemSize.y);
			
			if (highlight==i)
			{
				gl::color(.35,.35,.35,1.);
				gl::drawSolidRect(r);
				gl::color(0,0,0,1);
			}
			else {
				gl::color(0,0,0,1);
			}
			
			string worldName = games[i]->getUserName();
			app.mTextureFont->drawString( worldName, r.getLowerLeft() + kTextOffset );
		}
	}
}

void GameLibraryView::mouseDown( MouseEvent )
{

}

void GameLibraryView::mouseDrag( MouseEvent e )
{
	mMouseLoc = e.getPos();
}

void GameLibraryView::mouseUp  ( MouseEvent e )
{
	const int highlight = getHighlightIndex( rootToChild(e.getPos()) );

	// only do something if changed
	if (highlight != TablaApp::get()->mGameWorldCartridgeIndex )
	{
		TablaApp::get()->loadGame(highlight);
	}
}

vec2 GameLibraryView::getItemSize() const
{
	return getBounds().getSize();
}

Rectf GameLibraryView::getMenuRect() const
{
	const auto &app = *(TablaApp::get());
	const auto &games = app.mGameLibrary;
	
	const float itemwidth = getBounds().getWidth();
	const float itemheight = getBounds().getHeight();
	const int   n = games.size();

	const vec2 menuSize( itemwidth, (float)n * itemheight );
	const vec2 menuUL = getBounds().getUpperLeft() + vec2(0,-menuSize.y);
	Rectf menuRect( menuUL, menuUL + menuSize );

	return menuRect;
}

int GameLibraryView::getHighlightIndex( vec2 p ) const
{
	Rectf r = getMenuRect();
	
	if ( r.contains(p) )
	{
		p -= r.getLowerLeft();
		p.y /= getItemSize().y;
		
		return -p.y;
	}
	else return TablaApp::get()->mGameWorldCartridgeIndex;
}