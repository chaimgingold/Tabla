//
//  CameraCalibrateWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/1/16.
//
//

#include "CameraCalibrateWorld.h"
#include "TablaApp.h" // for config
#include "ocv.h"
#include "xml.h"

static GameCartridgeSimple sCartridge("CameraCalibrateWorld", [](){
	return std::make_shared<CameraCalibrateWorld>();
});

void CameraCalibrateWorld::setParams( XmlTree xml )
{
	int cols=7,rows=7;
	getXml(xml, "BoardCols", cols);
	getXml(xml, "BoardRows", rows);
	mBoardNumCorners = ivec2( cols-1, rows-1 );
	
	getXml(xml, "DrawBoard", mDrawBoard);
	getXml(xml, "NumBoardsToSolve",mNumBoardsToSolve);
	
	getXml(xml, "UniqueBoardDiffThresh", mUniqueBoardDiffThresh);
	getXml(xml, "Verbose", mVerbose );
	
	getXml(xml, "MinSecsBetweenCameraSamples", mMinSecsBetweenCameraSamples );
}

void CameraCalibrateWorld::update()
{
}

void CameraCalibrateWorld::updateVision( const Vision::Output& visionOut, Pipeline& pipeline )
{
	// too many boards?
	if ( mKnownBoards.size() >= mNumBoardsToSolve ) return;
	
	// too soon?
	if ( mLiveAllCornersFound && ci::app::getElapsedSeconds() - mLastCornersSeenWhen < mMinSecsBetweenCameraSamples ) return ;
	
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("input");
	if ( !world || world->mImageCV.empty() ) return;
	
	mImageToWorld = world->mImageToWorld;
	
	//
	cv::Size boardSize(mBoardNumCorners.x,mBoardNumCorners.y);
	

	cv::UMat input_gray;
	cv::cvtColor( world->mImageCV, input_gray, CV_BGR2GRAY);
	pipeline.then( "input_gray", input_gray );
	
	//
	vector<cv::Point2f> corners; // in image space
	
	bool findResult = cv::findChessboardCorners( input_gray, boardSize, corners, 0
//		+ cv::CALIB_CB_ADAPTIVE_THRESH
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
			cout << "Found corners: " << corners.size();
			if (allCornersFound) cout << " [ALL]";
			cout << endl;
		}
		
		// found a full board?
		if (allCornersFound)
		{
			// refine
			cv::cornerSubPix( input_gray, corners,
				cv::Size(11, 11), cv::Size(-1, -1),
				cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1) );

			// store
			if ( areBoardCornersUnique(corners) )
			{
				if (mVerbose)
				{
					cout << "Unique board found ("
						<< mKnownBoards.size() << "/" << mNumBoardsToSolve << ")"
						<< endl;
				}
				mKnownBoards.push_back(corners);
				
				if ( mKnownBoards.size() >= mNumBoardsToSolve )
				{
					tryToSolveWithKnownBoards( cv::Size( input_gray.cols, input_gray.rows ) );
				}
			}
		}

		// convert to world space for drawing...
		mLiveCorners = fromOcv(corners);
		for( auto &p : mLiveCorners )
		{
			p = vec2( mImageToWorld * vec4(p,0,1) ) ;
		}
	}
	else if ( ci::app::getElapsedSeconds() - mLastCornersSeenWhen > 2.f )
	{
		mLiveCorners.clear();
	}
}

void CameraCalibrateWorld::tryToSolveWithKnownBoards( cv::Size imageSize )
{
	if (mVerbose) cout << "solving... (" << mKnownBoards.size() << " boards)" << endl;
	
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
				j%mBoardNumCorners.x,
				j/mBoardNumCorners.x,
				0.f
			);
		}
	}

	float err = cv::calibrateCamera(objectPoints, mKnownBoards, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs);
	
	if (mVerbose)
	{
		cout << "cv::calibrateCamera" << endl;
		cout << "\t inputs:" << endl;
		cout << "\timageSize: " << imageSize << endl;
		cout << "\t outputs:" << endl;
		cout << "\tdistCoeffs: " << distCoeffs.cols << " x " << distCoeffs.rows << endl;
		cout << "\tdistCoeffs: " << distCoeffs << endl;
		cout << "\tcameraMatrix: " << cameraMatrix << endl;
		cout << "\timageSize: " << imageSize << endl;
//			cout << "\trvecs: " << rvecs << endl;
//			cout << "\ttvecs: " << tvecs << endl;
		cout << "\terror: " << err << endl;
	}
	
	if ( mKnownBoards.size() >= mNumBoardsToSolve )
	{
		if ( err > 0.f && err < 1.f )
		{
			mIsDone = true;
			cout << "*** Calibration done! ***" << endl;
			
			// call out to app to configure
			auto app = TablaApp::get();
			if (app)
			{
				app->getLightLink().getCaptureProfile().mDistCoeffs = distCoeffs;
				app->getLightLink().getCaptureProfile().mCameraMatrix = cameraMatrix;
				app->lightLinkDidChange();
			}
		}
		// start over?
	//	if ( mIsDone && err > 1.f )
		else
		{
			cout << "*** Starting over... ***" << endl;
			mKnownBoards.clear();
			mIsDone = false;
		}
	}
}

bool CameraCalibrateWorld::areBoardCornersUnique( vector<cv::Point2f> c ) const
{
	const float kErrThresh = mUniqueBoardDiffThresh * (float)c.size();
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
		cout << "New board similarity measure: " << bestErr;
		if (bestErr>kErrThresh) cout << "[UNIQUE] ";
		cout << " (thresh = " << kErrThresh << ")" << endl;
	}
	
	return bestErr > kErrThresh;
}

void CameraCalibrateWorld::draw( DrawType drawType )
{
	vec2 center = getWorldBoundsPoly().calcCentroid();
	
	if ( !mLiveAllCornersFound || mIsDone )
	{
		bool promptDraw = false;
		
		if ( ci::app::getElapsedSeconds()-mLastCornersSeenWhen > 2.f )
		{
			float k = 4.f;
			promptDraw = fmod( ci::app::getElapsedSeconds(), k ) < k*.5 ;
		}
		
		if (mDrawBoard || promptDraw || mIsDone) drawChessboard( center, vec2(1,1)*40.f );
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
		
		// drawn known boards
		gl::pushViewMatrix();
		gl::multViewMatrix(mImageToWorld);
		
		if (0)
		{
			// style #1: Points for each board
			for( int i=0; i<mKnownBoards.size(); ++i )
			{
				gl::color(0,0+((float)i/(float)mKnownBoards.size()),1);
				
				for( auto p : mKnownBoards[i] )
				{
					gl::drawSolidCircle(fromOcv(p),.25f,8);
				}
			}
		}
		else if (1)
		{
			// style #2: Lines between boards
			int n=0;
			if ( mKnownBoards.size()>0 ) n=mKnownBoards[0].size();
			
			for( int i=0; i<n; ++i )
			{
				int iy = i%mBoardNumCorners.x;
				int ix = (i-iy);
				
				gl::color( 0,
					lerp( .2, 1., (float)iy/(float)mBoardNumCorners.y ),
					lerp( .2, 1., (float)ix/(float)mBoardNumCorners.x) );
				
				for( int j=0; j<mKnownBoards.size()-1; ++j )
				{
					if ( i < mKnownBoards[j].size() && i < mKnownBoards[i+1].size() )
					{
						gl::drawLine( fromOcv(mKnownBoards[j][i]), fromOcv(mKnownBoards[j+1][i]) );
					}
				}
			}
		}
		
		gl::popViewMatrix();
	}
}

void CameraCalibrateWorld::drawChessboard( vec2 c, vec2 size  ) const
{
	vec2 squareSize = size / vec2(mBoardNumCorners.x,mBoardNumCorners.y);
	vec2 topleft = c - size*.5f;
	
	if (mIsDone) gl::color(0,1,0);
	else gl::color(1,1,1);
	
	for( int i=0; i<mBoardNumCorners.x; ++i )
	for( int j=0; j<mBoardNumCorners.y; ++j )
	{
		Rectf r( topleft, topleft + squareSize );
		r += vec2(i,j) * squareSize;
		
		if ( (i+j) % 2 )
			gl::drawSolidRect(r);
	}
}
