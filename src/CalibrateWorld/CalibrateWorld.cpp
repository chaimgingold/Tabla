//
//  CalibrateWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/1/16.
//
//

#include "CalibrateWorld.h"
#include "ocv.h"
#include "xml.h"

void CalibrateWorld::setParams( XmlTree xml )
{
	getXml(xml, "BoardCols", mBoardCols);
	getXml(xml, "BoardRows", mBoardRows);
	getXml(xml, "DrawBoard", mDrawBoard);
}

void CalibrateWorld::update()
{
}

void CalibrateWorld::updateCustomVision( Pipeline& pipeline )
{
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("clipped");
	if ( !world || world->mImageCV.empty() ) return;
	
	//
	cv::Size boardSize(mBoardCols,mBoardRows);
	
	
	//
	vector<cv::Point2f> corners; // in image space
	
	bool findResult = cv::findChessboardCorners( world->mImageCV, boardSize, corners, 0
		+ cv::CALIB_CB_ADAPTIVE_THRESH
//		+ cv::CALIB_CB_NORMALIZE_IMAGE
//		+ cv::CALIB_CB_FAST_CHECK
	);
	
	bool allCornersFound = findResult; // corners.size()==boardSize.width*boardSize.height;
		// sometimes findResult is false
	
	mLiveAllCornersFound = allCornersFound;
	
//	assert( findResult == allCornersFound );
	
	if ( corners.size()>0 )
	{
		mLastCornersSeenWhen = ci::app::getElapsedSeconds();
		
		if (mVerbose)
		{
//			cout << "corners: " << corners.size() << endl;
			if (allCornersFound) cout << "ALL corners found" << endl;
		}
		
		if ( allCornersFound )
		{
			cv::cornerSubPix( world->mImageCV, corners,
				cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1) );

			if ( areBoardCornersUnique(corners) )
			{
				mKnownBoards.push_back(corners);
				tryToSolveWithKnownBoards( cv::Size( world->mImageCV.cols, world->mImageCV.rows ) );
			}
		}

		// convert to world space for drawing...
		mLiveCorners = fromOcv(corners);
		for( auto &p : mLiveCorners )
		{
			p = vec2( world->mImageToWorld * vec4(p,0,1) ) ;
		}
	}
	else mLiveCorners.clear();
}

void CalibrateWorld::tryToSolveWithKnownBoards( cv::Size imageSize )
{
	if (mVerbose) cout << "solving... (" << mKnownBoards.size() << " boards)" << endl;
	
	{
		vector<vector<cv::Point3f>> objectPoints(mKnownBoards.size()); // in planar chessboard space
		cv::Mat cameraMatrix;
		cv::Mat distCoeffs;
		vector<cv::Mat> rvecs, tvecs;
		
		// objectPoints
		for( size_t i=0; i<mKnownBoards.size(); ++i )
		{
			// allocate entry
			objectPoints[i].resize(mKnownBoards[i].size());
			
			// fill it (redundant data; we could v.resize( n, v[0] ) to replicate it n times i think
			for( size_t j=0; j<mKnownBoards[i].size(); ++j )
			{
				objectPoints[i][j] = cv::Point3f(
					j%mBoardCols,
					j/mBoardCols,
					0.f
				);
			}
		}

		float err = cv::calibrateCamera(objectPoints, mKnownBoards, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs);
		
		//
		if (mVerbose) cout << "error: " << err << endl;
	}
}

bool CalibrateWorld::areBoardCornersUnique( vector<cv::Point2f> c ) const
{
	const float kErrThresh = 5.f;
	float bestErr=MAXFLOAT;
	
	for( const auto& c2 : mKnownBoards )
	{
		float err=0.f;
		
		if ( c.size() != c2.size() ) continue;
		
		for( size_t i=0; i<c.size(); ++i )
		{
			err += distance( fromOcv(c[i]), fromOcv(c2[i]) );
		}
		
		bestErr = min( err, bestErr );
	}
	
	if (mVerbose)
	{
		cout << "new board similarity measure: " << bestErr << endl ;
		if (bestErr>kErrThresh) cout << "unique; adding to set." << endl;
	}
	
	return bestErr > kErrThresh;
}

void CalibrateWorld::draw( DrawType drawType )
{
	vec2 center = getWorldBoundsPoly().calcCentroid();
	
	if ( !mLiveAllCornersFound )
	{
		bool promptDraw = false;
		
		if ( ci::app::getElapsedSeconds()-mLastCornersSeenWhen > 2.f )
		{
			float k = 4.f;
			promptDraw = fmod( ci::app::getElapsedSeconds(), k ) < k*.5 ;
		}
		
		if (mDrawBoard || promptDraw) drawChessboard( center, vec2(1,1)*40.f );
	}
	
	//
	if ( drawType != GameWorld::DrawType::UIPipelineThumb )
	{
		if (mLiveAllCornersFound) gl::color(1,0,0);
		else gl::color(0,1,0);
		
		for( auto p : mLiveCorners )
		{
			gl::drawSolidCircle(p,.5f,8);
		}
	}
}

void CalibrateWorld::drawChessboard( vec2 c, vec2 size  ) const
{
	vec2 squareSize = size / vec2(mBoardCols,mBoardRows);
	vec2 topleft = c - size*.5f;
	
	gl::color(1,1,1);
	
	for( int i=0; i<mBoardCols; ++i )
	for( int j=0; j<mBoardRows; ++j )
	{
		Rectf r( topleft, topleft + squareSize );
		r += vec2(i,j) * squareSize;
		
		if ( (i+j) % 2 )
			gl::drawSolidRect(r);
	}
}
