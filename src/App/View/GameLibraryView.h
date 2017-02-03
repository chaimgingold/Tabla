//
//  GameLibraryView.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#ifndef GameLibraryView_hpp
#define GameLibraryView_hpp

#include "PopUpMenuView.h"

using namespace std;
using namespace ci;

class GameLibraryView : public PopUpMenuView
{
protected:
	virtual int getDefaultItem() const override;
	virtual vector<string> getItems() const override;
	virtual string getDefaultItemName() const override;
	virtual void userChoseItem( int ) override;
};

#endif /* GameLibraryView_hpp */
