//
//  CaptureProfileMenuView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/3/17.
//
//

#ifndef CaptureProfileMenuView_hpp
#define CaptureProfileMenuView_hpp

#include "PopUpMenuView.h"
#include "LightLink.h"

class CaptureProfileMenuView : public PopUpMenuView
{
protected:
	virtual int getDefaultItem() const override;
	virtual vector<string> getItems() const override;
	virtual string getDefaultItemName() const override;
	virtual void userChoseItem( int ) override;
	
private:
	string getDisplayName( const LightLink::CaptureProfile& ) const;
	
};

#endif /* CaptureProfileMenuView_hpp */
