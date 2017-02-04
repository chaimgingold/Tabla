//
//  CaptureProfileMenuView.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/3/17.
//
//

#include "CaptureProfileMenuView.h"
#include "TablaApp.h"

int CaptureProfileMenuView::getDefaultItem() const
{
	string name = getDefaultItemName();
	auto lib = TablaApp::get()->getLightLink().mCaptureProfiles;
	auto i = lib.begin();

	int item=0;
	
	while ( i != lib.end() && i->first != name )
	{
		++i;
		item++;
	}
	
	return item;
}

vector<string> CaptureProfileMenuView::getItems() const
{
	const auto &app = *(TablaApp::get());

	vector<string> items;
	
	for( auto p : app.getLightLink().mCaptureProfiles )
	{
		items.push_back( getDisplayName(p.second) );
	}
	
	return items;
}

void CaptureProfileMenuView::userChoseItem( int item )
{
	TablaApp::get()->setCaptureProfile( getItems()[item] );
}

string CaptureProfileMenuView::getDefaultItemName() const
{
	const auto &app = *(TablaApp::get());
	
	return getDisplayName( app.getLightLink().getCaptureProfile() );
}

string CaptureProfileMenuView::getDisplayName( const LightLink::CaptureProfile& p ) const
{
	return p.mName;
}
