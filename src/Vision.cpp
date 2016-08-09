//
//  Vision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "Vision.h"
#include "xml.h"

namespace cinder {
	
	PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

}

string  Pipeline::getFirstStageName() const
{
	if ( mStageNames.empty() ) return "" ;
	else return mStageNames.front();
}

string  Pipeline::getLastStageName () const
{
	if ( mStageNames.empty() ) return "" ;
	else return mStageNames.back();
}

string Pipeline::getAdjStageName( string name, int adj ) const
{
	for ( size_t i=0; i<mStageNames.size(); ++i )
	{
		if ( mStageNames[i]==name )
		{
			int k = (int)i + adj ;
			
			k = min( k, (int)mStageNames.size()-1 );
			k = max( k, 0 );
			
			return mStageNames[k];
		}
	}
	
	return getFirstStageName();
}

void Pipeline::start()
{
	mStageNames.clear();
	mFrame = gl::TextureRef();
}

void Vision::setParams( XmlTree xml )
{
	getXml(xml,"ResScale",mResScale);
	getXml(xml,"ContourMinRadius",mContourMinRadius);
	getXml(xml,"ContourMinArea",mContourMinArea);
	getXml(xml,"ContourDPEpsilon",mContourDPEpsilon);
	getXml(xml,"ContourMinWidth",mContourMinWidth);

	mContourMinRadius	*= mResScale;
	mContourMinArea		*= mResScale;
	mContourDPEpsilon	*= mResScale;
	mContourMinWidth	*= mResScale;
}

void Vision::processFrame( const Surface &surface )
{
//	mOCVPipelineTrace = Pipeline() ;
//	mOCVPipelineTrace.setQuery("input");
	mOCVPipelineTrace.start();
	if ( mOCVPipelineTrace.getQuery() == "" ) mOCVPipelineTrace.setQuery("input");
	
	// make cv frame
	cv::Mat input( toOcv( Channel( surface ) ) );
	cv::Mat clipped, output, gray, thresholded ;

	mOCVPipelineTrace.then( input, "input" );

	
	// clip
	{
		// gather transform parameters
		cv::Mat xform( 2, 4, CV_32FC1 );
		
		cv::Point2f srcpt[4], dstpt[4];
		
		cv::Size outputSize;
		
		for( int i=0; i<4; ++i )
		{
			srcpt[i] = toOcv( mLightLink.mCaptureCoords[i] );
			dstpt[i] = toOcv( mLightLink.mCaptureWorldSpaceCoords[i] );

			outputSize.width  = max( outputSize.width,  (int)dstpt[i].x );
			outputSize.height = max( outputSize.height, (int)dstpt[i].y );
		}

		// compute output size
//		if (0)
//		{
//			// This doesn't work BECAUSE our dstpt[] coords are still in world space
//			// so this code just doesn't do what we want at all. but it is still an important clue...
//			
//			ivec2 minsrc = fromOcv(srcpt[0]), maxsrc = fromOcv(srcpt[0]);
//			
//			for( int i=1; i<4; ++i )
//			{
//				minsrc = glm::min( ivec2(fromOcv(srcpt[i])), minsrc ) ;
//				maxsrc = glm::max( ivec2(fromOcv(srcpt[i])), maxsrc ) ;
//			}
//
//			outputSize = cv::Size( maxsrc.x - minsrc.x, maxsrc.y - minsrc.y );
//		}

		// do it
		xform = cv::getPerspectiveTransform( srcpt, dstpt ) ;
		cv::warpPerspective(input, clipped, xform, outputSize );
		
		mOCVPipelineTrace.then( clipped, "clipped" );
	}
	
	// blur
	cv::GaussianBlur( clipped, gray, cv::Size(5,5), 0 );
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
		
		if (	radius > mContourMinRadius &&
				area > mContourMinArea &&
				min( rotatedRect.size.width, rotatedRect.size.height ) > mContourMinWidth )
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
				
				cv::approxPolyDP( c, approx, mContourDPEpsilon, true ) ;
				
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

