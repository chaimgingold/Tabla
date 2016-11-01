//
//  CalibrateWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/1/16.
//
//

#ifndef CalibrateWorld_hpp
#define CalibrateWorld_hpp

#include "BallWorld.h"

class CalibrateWorld : public BallWorld
{
public:

//	CalibrateWorld();
//	~CalibrateWorld();
	
	string getSystemName() const override { return "CalibrateWorld"; }

//	void gameWillLoad() override;
	void update() override;
	void updateCustomVision( Pipeline& pipeline );
	
	void draw( DrawType ) override;

private:
	vector<vec2> mCorners;
	
};

class CalibrateWorldCartridge : public GameCartridge
{
public:
	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<CalibrateWorld>();
	}
};

#endif /* CalibrateWorld_hpp */
