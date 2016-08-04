//
//  Vision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "Vision.h"

namespace cinder {
	
	PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

}

gl::Texture getImageSubarea( gl::Texture from, vec2 fromCoords[4], vec2 toSize )
{
	// from is the input camera image
	// fromCoords are the topleft, topright, b-r, b-l area we are going to cut out
	//	- specified in from's coordinate space
	// return value:
	//  - is the resulting image,
	//  - whose size is toSize
	
	// fbo (which we'll want to cache between frames :P)
	// etc
}

void Vision::processFrame( const Surface &surface )
{
	mOCVPipelineTrace = Pipeline() ;
	mOCVPipelineTrace.setQuery("input");
	
	// make cv frame
	cv::Mat input( toOcv( Channel( surface ) ) );
	cv::Mat output, gray, thresholded ;

	mOCVPipelineTrace.then( input, "input" );

	
	// blur
	cv::GaussianBlur( input, gray, cv::Size(5,5), 0 );
	mOCVPipelineTrace.then( gray, "gray" );

	// threshold
	cv::threshold( gray, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	mOCVPipelineTrace.then( thresholded, "thresholded" );
	
	// contour detect
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	
	cv::findContours( thresholded, contours, hierarchy, cv::RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
	
	// filter and process contours into our format
	mContourOutput.clear() ;
	
	map<int,int> ocvContourIdxToMyContourIdx ;
	
	for( int i=0; i<contours.size(); ++i )
	{
		const auto &c = contours[i] ;
		
		cv::Point2f center ;
		float		radius ;
		
		cv::minEnclosingCircle( c, center, radius ) ;
		
		float		area = cv::contourArea(c) ;
		
		cv::RotatedRect rotatedRect = minAreaRect(c) ;
		
		if (	radius > kContourMinRadius &&
				area > kContourMinArea &&
				min( rotatedRect.size.width, rotatedRect.size.height ) > kContourMinWidth )
		{
			auto addContour = [&]( const vector<cv::Point>& c )
			{
				Contour myc ;
				
				myc.mPolyLine = fromOcv(c) ;
				myc.mRadius = radius ;
				myc.mCenter = fromOcv(center) ;
				myc.mArea   = area ;
				myc.mBoundingRect = Rectf( myc.mPolyLine.getPoints() );
				myc.mOcvContourIndex = i ;
				
				myc.mTreeDepth = 0 ;
				{
					int n = i ;
					while ( (n = hierarchy[n][3]) > 0 )
					{
						myc.mTreeDepth++ ;
					}
				}
				myc.mIsHole = ( myc.mTreeDepth % 2 ) ; // odd depth # children are holes
				myc.mIsLeaf = hierarchy[i][2] < 0 ;
				
				mContourOutput.push_back( myc );
				
				// store my index mapping
				ocvContourIdxToMyContourIdx[i] = mContourOutput.size()-1 ;
			} ;
			
			if (1)
			{
				// simplify
				vector<cv::Point> approx ;
				
				cv::approxPolyDP( c, approx, kContourDPEpislon, true ) ;
				
				addContour(approx);
			}
			else addContour(c) ;
		}
	}
	
	// add contour topology metadata
	// (this might be screwed up because of how we cull contours;
	// a simple fix might be to find orphaned contours--those with parents that don't exist anymore--
	// and strip them, too. but this will, in turn, force us to rebuild indices.
	// it might be simplest to just "hide" rejected contours, ignore them, but keep them around.
	for ( size_t i=0; i<mContourOutput.size(); ++i )
	{
		Contour &c = mContourOutput[i] ;
		
		if ( hierarchy[c.mOcvContourIndex][3] >= 0 )
		{
			c.mParent = ocvContourIdxToMyContourIdx[ hierarchy[c.mOcvContourIndex][3] ] ;
			
//				assert( myc.mParent is valid ) ;
			
			assert( c.mParent >= 0 && c.mParent < mContourOutput.size() ) ;
			
			mContourOutput[c.mParent].mChild.push_back( i ) ;
		}
	}
}

