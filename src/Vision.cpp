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

	glm::mat3x3 fromOcvMat3x3( const cv::Mat& m )
	{
		glm::mat3x3 r;
		
		for( int i=0; i<3; ++i )
		{
			for( int j=0; j<3; ++j )
			{
				r[j][i] = m.at<double>(i,j);
				// opencv: (row,column)
				// glm:    (column,row)
			}
		}
		
		return r;
	}
}

void Pipeline::start()
{
	mStages.clear();
//	mQueryFrame = gl::TextureRef();
	mQueryIndex = -1;
}

void Pipeline::then( Surface &img, string name )
{
	then( [&](){
			return gl::Texture::create( img );
		}, name ) ;
}

void Pipeline::then( cv::Mat &img, string name )
{
	then( [&](){
			return gl::Texture::create( fromOcv(img), gl::Texture::Format().loadTopDown() );
		}, name ) ;
}

void Pipeline::then( gl::Texture2dRef ref, string name )
{
	then( [&](){
			return ref;
		}, name ) ;
}

void Pipeline::then( function<gl::Texture2dRef()> func, string name )
{
	Stage s;
	s.mName = name ;
	
	if ( !mStages.empty() ) //s.mImageToWorld = glm::mat3x3(); // identity
//	else
	{
		// inherit from last
		s.mImageToWorld = mStages.back().mImageToWorld;
		s.mWorldToImage = mStages.back().mWorldToImage;
		
		// (by default it will be the identity matrix, i think)
	}
	
	mStages.push_back(s);
	
	if ( name==mQuery )
	{
		mStages.back().mFrame = func();
		mQueryIndex = mStages.size()-1;
	}
}

void Pipeline::setImageToWorldTransform( const cv::Mat& m )
{
	setImageToWorldTransform( fromOcvMat3x3(m) );
}

void Pipeline::setImageToWorldTransform( const glm::mat3x3& m )
{
	assert( !empty() );
	mStages.back().mImageToWorld = m;
	mStages.back().mWorldToImage = inverse(m);
}

string  Pipeline::getFirstStageName() const
{
	if ( empty() ) return "" ;
	else return mStages.front().mName;
}

string  Pipeline::getLastStageName () const
{
	if ( empty() ) return "" ;
	else return mStages.back().mName;
}

string Pipeline::getAdjStageName( string name, int adj ) const
{
	for ( size_t i=0; i<mStages.size(); ++i )
	{
		if ( mStages[i].mName==name )
		{
			int k = (int)i + adj ;
			
			k = min( k, (int)mStages.size()-1 );
			k = max( k, 0 );
			
			return mStages[k].mName;
		}
	}
	
	return getFirstStageName();
}

//Rectf Pipeline::Stage::getBoundsInWorld() const
//{
//	if (mFrame)
//	{
//		ivec2 frameSize = mFrame->getSize();
//		
//		vec2 tl = vec2( mImageToWorld * vec3(0,0,1) ) ;
//		vec2 br = vec2( mImageToWorld * vec3(frameSize.x,frameSize.y,1) ) ;
//		
//		return Rectf(tl,br);
//	}
//	else return Rectf(0,0,0,0);
//}


void Vision::setParams( XmlTree xml )
{
	getXml(xml,"ContourMinRadius",mContourMinRadius);
	getXml(xml,"ContourMinArea",mContourMinArea);
	getXml(xml,"ContourDPEpsilon",mContourDPEpsilon);
	getXml(xml,"ContourMinWidth",mContourMinWidth);
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

void Vision::processFrame( const Surface &surface )
{
	mOCVPipelineTrace.start();
	if ( mOCVPipelineTrace.getQuery() == "" ) mOCVPipelineTrace.setQuery("clipped");
	
	// make cv frame
	cv::Mat input( toOcv( Channel( surface ) ) );
	cv::Mat clipped, output, gray, thresholded ;

	mOCVPipelineTrace.then( input, "input" );

	
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

		// store this transformation
		cv::Mat inputImageToWorld = cv::getPerspectiveTransform( srcpt, dstpt ) ;
//		cout << inputImageToWorld << endl;
		mOCVPipelineTrace.setImageToWorldTransform( inputImageToWorld );
			// this isn't coming out quite right; looks like we are missing translation somehow.
		
		// compute output size pixel scaling factor
		const Rectf inputBounds  = asBoundingRect( mLightLink.mCaptureCoords );
		const Rectf outputBounds = asBoundingRect( mLightLink.mCaptureWorldSpaceCoords );

		const float pixelScale
			= max( inputBounds .getWidth(), inputBounds .getHeight() )
			/ max( outputBounds.getWidth(), outputBounds.getHeight() )
			* 1 ; // bump it up
		
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
		mOCVPipelineTrace.then( clipped, "clipped" );
		
		glm::mat3x3 imageToWorld = glm::mat3x3();
		imageToWorld /= pixelScale;
		
		mOCVPipelineTrace.setImageToWorldTransform( imageToWorld );
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
		
		cv::RotatedRect rotatedRect = minAreaRect(c) ;
		
		if (	radius > mContourMinRadius &&
				area > mContourMinArea &&
				min( rotatedRect.size.width, rotatedRect.size.height ) > mContourMinWidth )
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
			
			if ( mContourDPEpsilon > 0 )
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

