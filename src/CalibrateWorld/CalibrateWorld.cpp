//
//  CalibrateWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/1/16.
//
//

#include "CalibrateWorld.h"
#include "ocv.h"

void CalibrateWorld::update()
{
}

void CalibrateWorld::updateCustomVision( Pipeline& pipeline )
{
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("clipped");
	if ( !world || world->mImageCV.empty() ) return;
	
	//
	cv::Size boardSize(7,7);
	
	
	//
	vector<cv::Point2f> corners; // in image space
	cv::findChessboardCorners( world->mImageCV, boardSize, corners,
		cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK
	);

//	cout << "corners: " << corners.size() << endl;;

	if ( corners.size()==boardSize.width*boardSize.height )
	{
		cv::cornerSubPix( world->mImageCV, corners,
			cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1) );
	}

	// convert to world space for drawing...
	mCorners = fromOcv(corners);
	for( auto &p : mCorners )
	{
		p = vec2( world->mImageToWorld * vec4(p,0,1) ) ;
	}
}

void CalibrateWorld::draw( DrawType drawType )
{
	gl::color(1,0,0);
//	gl::draw( getWorldBoundsPoly() );
	
	gl::drawStrokedCircle( getWorldBoundsPoly().calcCentroid(), 10.f, 20 );
	
//	vec2 c = getWorldBoundsPoly().calcCentroid();
	
//	for( auto p : getWorldBoundsPoly().getPoints() )
//	{
//		gl::drawLine( c, p );
//	}
	
	//
	if ( drawType != GameWorld::DrawType::UIPipelineThumb )
	{
		gl::color(0,1,0);
		for( auto p : mCorners )
		{
			gl::drawSolidCircle(p,.5f,8);
		}
	}
}
