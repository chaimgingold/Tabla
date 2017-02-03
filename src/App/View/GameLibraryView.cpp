//
//  GameLibraryView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "GameLibraryView.h"
#include "TablaApp.h"

void GameLibraryView::layout( Rectf windowRect )
{
	// TODO: move some of this to PopUpMenuView
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

int GameLibraryView::getDefaultItem() const
{
	auto lib = GameCartridge::getLibrary();
	auto i = lib.begin();

	int item=0;
	
	while ( i != lib.end() && i->first != TablaApp::get()->mGameWorldCartridgeName )
	{
		++i;
		item++;
	}
	
	return item;
}

vector<string> GameLibraryView::getItems() const
{
	const auto &games = GameCartridge::getLibrary();
	
	vector<string> items;
	
	for( auto g : games ) items.push_back( g.first );
	
	return items;
}

void GameLibraryView::userChoseItem( int item )
{
	auto lib = GameCartridge::getLibrary();
	auto i = lib.begin();
	
	for( int j=0; j<item && i !=lib.end(); ++j ) ++i;
	
	if ( i!=lib.end() ) TablaApp::get()->loadGame(i->first);
}

string GameLibraryView::getDefaultItemName() const
{
	const auto &app = *(TablaApp::get());
	return app.mGameWorld ? app.mGameWorld->getUserName() : "——";
}
