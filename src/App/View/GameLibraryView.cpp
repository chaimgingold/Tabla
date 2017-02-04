//
//  GameLibraryView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "GameLibraryView.h"
#include "TablaApp.h"

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
