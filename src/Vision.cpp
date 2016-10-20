//
//  Vision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#include "Vision.h"
#include "xml.h"
#include "ocv.h"

void Vision::Params::set( XmlTree xml )
{
	getXml(xml,"ContourMinRadius",mContourMinRadius);
	getXml(xml,"ContourMinArea",mContourMinArea);
	getXml(xml,"ContourDPEpsilon",mContourDPEpsilon);
	getXml(xml,"ContourMinWidth",mContourMinWidth);
	getXml(xml,"CaptureAllPipelineStages",mCaptureAllPipelineStages);
}

template<class T>
vector<T> asVector( const T &d, int len )
{
	vector<T> v;
	
	for( int i=0; i<len; ++i ) v.push_back(d[i]);
	
	return v;
}

Rectf asBoundingRect( vec2 pts[4] )
{
	vector<vec2> v;
	for( int i=0; i<4; ++i ) v.push_back(pts[i]);
	return Rectf(v);
}

void Vision::processFrame( const Surface &surface, Pipeline& pipeline )
{
	// ---- Input ----
	
	// make cv frame
	cv::Mat input( toOcv( Channel( surface ) ) );
	cv::Mat clipped, output, gray, thresholded ;

	pipeline.then( "input", input );
	
	pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(
		mLightLink.mCaptureCoords,
		mLightLink.mCaptureWorldSpaceCoords ) );
	
	// ---- World Boundaries ----
	pipeline.then( "world-boundaries", vec2(4,4)*100.f ); // 4m^2 configurable area
	pipeline.setImageToWorldTransform( mat4() ); // identity; do it in world space
		// this is here just so it can be configured by the user.
	
	// ---- Clipped ----

	// clip
	float contourPixelToWorld = 1.f;
	
	{
		// gather transform parameters
		cv::Point2f srcpt[4], dstpt[4], dstpt_pixelspace[4];
		
		cv::Size outputSize;
		
		for( int i=0; i<4; ++i )
		{
			srcpt[i] = toOcv( mLightLink.mCaptureCoords[i] );
			dstpt[i] = toOcv( mLightLink.mCaptureWorldSpaceCoords[i] );
		}

		// compute output size pixel scaling factor
		const Rectf inputBounds  = asBoundingRect( mLightLink.mCaptureCoords );
		const Rectf outputBounds = asBoundingRect( mLightLink.mCaptureWorldSpaceCoords );

		const float pixelScale
			= max( inputBounds .getWidth(), inputBounds .getHeight() )
			/ max( outputBounds.getWidth(), outputBounds.getHeight() ) ;
		// more robust would be to process input edges: outputSize = [ max(l-edge,r-edge), max(topedge,botedge) ] 
		
		contourPixelToWorld = 1.f / pixelScale ;
		
		outputSize.width  = outputBounds.getWidth()  * pixelScale ;
		outputSize.height = outputBounds.getHeight() * pixelScale ;

		// compute dstpts in desired destination pixel space
		for( int i=0; i<4; ++i )
		{
			dstpt_pixelspace[i] = dstpt[i] * pixelScale ;
		}
		
		// do it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt, dstpt_pixelspace ) ;
		cv::warpPerspective(input, clipped, xform, outputSize );
		
		// log to pipeline
		pipeline.then( "clipped", clipped );
		
		glm::mat4 imageToWorld = glm::scale( vec3( 1.f / pixelScale, 1.f / pixelScale, 1.f ) );
		
		pipeline.setImageToWorldTransform( imageToWorld );
	}
	
	
	// blur
//	cv::GaussianBlur( clipped, gray, cv::Size(5,5), 0 );
//	pipeline.then( gray, "gray" );

	// threshold
//	cv::threshold( gray, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	cv::threshold( clipped, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	pipeline.then( "thresholded", thresholded );
	
	// contour detect
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	
	cv::findContours( thresholded, contours, hierarchy, cv::RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
	
	// transform contours to world space...
	// ideally we'd transform them in place, BUT since they are stored as integers
	// we can't do this without aliasing along world unit boundaries.
	// SO we need to keep the scaling ratio in mind as we go...
	// also IDEALLY we'd use the transform matrix above and not just a single scaling factor,
	// but this works.
	
	// filter and process contours into our format
	mContourOutput.clear() ;
	
	map<int,int> ocvContourIdxToMyContourIdx ;
	
	for( int i=0; i<contours.size(); ++i )
	{
		const auto &c = contours[i] ;
		
		cv::Point2f center ;
		float		radius ;
		
		cv::minEnclosingCircle( c, center, radius ) ;
		
		radius *= contourPixelToWorld ;
		center *= contourPixelToWorld ;
		
		float		area = cv::contourArea(c) * contourPixelToWorld ;
		
		cv::RotatedRect rotatedRect = minAreaRect(c) ; // TODO: output this
		
		if (	radius > mParams.mContourMinRadius &&
				area   > mParams.mContourMinArea   &&
				min( rotatedRect.size.width, rotatedRect.size.height ) > mParams.mContourMinWidth )
		{
			auto addContour = [&]( const vector<cv::Point>& c )
			{
				Contour myc ;
				
				myc.mPolyLine = fromOcv(c) ;

				// scale polyline to world space (from pixel space)
				for( auto &p : myc.mPolyLine.getPoints() ) p *= contourPixelToWorld ;

				myc.mRadius = radius ;
				myc.mCenter = fromOcv(center) ;
				myc.mArea   = area ;
				myc.mBoundingRect = Rectf( myc.mPolyLine.getPoints() ); // after scaling points!
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
			
			if ( mParams.mContourDPEpsilon > 0 )
			{
				// simplify
				vector<cv::Point> approx ;
				
				cv::approxPolyDP( c, approx, mParams.mContourDPEpsilon, true ) ;
				
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

