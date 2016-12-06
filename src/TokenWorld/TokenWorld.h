//
//  TokenWorld.h
//  PaperBounce3
//
//  Created by Luke Iannini on 12/5/16.
//
//

#ifndef TokenWorld_h
#define TokenWorld_h


#include <vector>
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Color.h"

#include "GameWorld.h"
#include "Contour.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Token {

public:
	vec2 mLoc;
};


class TokenWorld : public GameWorld
{
public:

	string getSystemName() const override { return "TokenWorld"; }

	void setParams( XmlTree ) override;
	void updateVision( const ContourVector &c, Pipeline& ) override {  }

	void gameWillLoad() override;
	void update() override;
	void draw( DrawType ) override;


	void mouseClick( vec2 p ) override {  }
	void keyDown( KeyEvent ) override;

protected:


private:


};

class TokenWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "TokenWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<TokenWorld>();
	}
};


#endif /* TokenWorld_h */
