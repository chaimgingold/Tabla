//
//  ProjectorCalibrateWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 1/12/17.
//
//

#ifndef ProjectorCalibrateWorld_hpp
#define ProjectorCalibrateWorld_hpp

#include "graycodepattern.hpp" // opencv
#include "GameWorld.h"

class ProjectorCalibrateWorld : public GameWorld
{
public:

	ProjectorCalibrateWorld();
	
	string getSystemName() const override { return "ProjectorCalibrateWorld"; }
	void setParams( XmlTree ) override;

	void update() override;
	void updateVision( const Vision::Output&, Pipeline& ) override;
	
	void draw( DrawType ) override;

private:

	// params
	bool	mVerbose=true;

	// patterns + captures
	ci::ivec2 mProjectorSize;
	cv::Ptr<cv::structured_light::GrayCodePattern> mGenerator;
	vector<cv::Mat> mPatterns;
	vector<cv::Mat> mCaptures;

	// optimization
	vector<gl::TextureRef> mPatternTextures;
	vector<gl::TextureRef> mCaptureTextures;
	
	// 
	void maybeMakePatterns( ivec2 size );
	void nextPattern();
	void capture( Pipeline::StageRef );
	
	int mShowPattern=-1;
	float mShowPatternWhen=0.f;
	float mShowPatternLength=.1f;
		
	Pipeline::StageRef mProjectorStage, mInputStage;
};

#endif /* ProjectorCalibrateWorld_hpp */
