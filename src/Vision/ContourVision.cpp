//
//  ContourVision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/13/16.
//
//

#include "ContourVision.h"
#include "ocv.h"

static map<string,ContourVision::ThresholdStyle> sStrToTresholdStyle;

ContourVision::Params::Params()
{
	if (sStrToTresholdStyle.empty())
	{
		sStrToTresholdStyle["Fixed"] = ContourVision::ThresholdStyle::Fixed;
		sStrToTresholdStyle["OtsuClipped"] = ContourVision::ThresholdStyle::OtsuClipped;
		sStrToTresholdStyle["OtsuInput"] = ContourVision::ThresholdStyle::OtsuInput;
		sStrToTresholdStyle["AdaptiveGaussian"] = ContourVision::ThresholdStyle::AdaptiveGaussian;
		sStrToTresholdStyle["AdaptiveMean"] = ContourVision::ThresholdStyle::AdaptiveMean;
	}
}

void ContourVision::Params::set( XmlTree xml )
{
	*this = Params(); // defaults
	
	getXml(xml,"ContourMinRadius",mContourMinRadius);
	getXml(xml,"ContourMinArea",mContourMinArea);
	getXml(xml,"ContourDPEpsilon",mContourDPEpsilon);
	getXml(xml,"ContourMinWidth",mContourMinWidth);
	getXml(xml,"ContourGetExactMinCircle",mContourGetExactMinCircle);
	getXml(xml,"FixedThreshold",mFixedThreshold);
	
	string threshStyle;
	if ( getXml(xml,"ThresholdStyle",threshStyle) )
	{
		auto i = sStrToTresholdStyle.find(threshStyle);
		if ( i != sStrToTresholdStyle.end() ) mThresholdStyle = i->second;
		else {
			cout << "ContourVision::Params::set unknown <ThresholdStyle> << '" << threshStyle << "'"
				 << "; valid values:" << endl;
			for ( auto s : sStrToTresholdStyle ) cout << "\t'" << s.first << "'" << endl;
		}
	}
}

ContourVec ContourVision::findContours( const Pipeline::StageRef input, Pipeline& pipeline, float contourPixelToWorld )
{
	cv::Mat thresholded; // if I make this a UMat, then threshold sometimes crashes (!)
	ContourVec output;

	// fail hard
	if ( !input || input->mImageCV.empty() )
	{
		cout << "ContourVision::findContours input image missing!" << endl;
		return output;
	}
		
	// blur
//	cv::GaussianBlur( clipped, gray, cv::Size(5,5), 0 );
//	pipeline.then( gray, "gray" );

	// threshold
	{
		switch( mParams.mThresholdStyle )
		{
			case ThresholdStyle::OtsuClipped:
				cv::threshold(	input->mImageCV, thresholded,
								0, 255,
								cv::THRESH_BINARY + cv::THRESH_OTSU );
				break;
				
			case ThresholdStyle::Fixed:
				cv::threshold(	input->mImageCV, thresholded,
								mParams.mFixedThreshold,
								255, cv::THRESH_BINARY );
				break;
				
			case ThresholdStyle::OtsuInput:
				{
					const float kScaleFactor = .2;
					
					const auto cinput = pipeline.getStage("input");
					if ( cinput && !cinput->mImageCV.empty() )
					{
						// shrink + gray
						cv::UMat resized, gray;
						cv::resize( cinput->mImageCV, resized, cv::Size(),
							kScaleFactor, kScaleFactor, cv::INTER_LINEAR );
						cv::cvtColor( resized, gray, CV_BGR2GRAY);
						pipeline.then( "(otsu) input gray", gray );
						pipeline.back()->setImageToWorldTransform(
							cinput->mImageToWorld * glm::scale( 1.f / vec3(kScaleFactor, kScaleFactor, 1) ) );
						
						// compute threshold value for input
						double thresh = cv::threshold( gray, thresholded, 0, 255,
								cv::THRESH_BINARY + cv::THRESH_OTSU );
						pipeline.then( "(otsu) input thresholded", thresholded );
						
						// apply to clipped
						cv::threshold( input->mImageCV, thresholded,
								thresh,
								255, cv::THRESH_BINARY );
					}
				}
				break;

			case ThresholdStyle::AdaptiveGaussian:				
				cv::adaptiveThreshold( input->mImageCV, thresholded,
					255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY,
					10, 10);
				break;
				
			case ThresholdStyle::AdaptiveMean:
				cv::adaptiveThreshold( input->mImageCV, thresholded,
					255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
					32, -10);
				break;

		}
		pipeline.then( "thresholded", thresholded );
		pipeline.back()->setImageToWorldTransform( input->mImageToWorld );
	}
	
	// contour detect
	vector<vector<cv::Point> > contoursi;
	vector<cv::Vec4i> hierarchy;
	
	cv::findContours( thresholded, contoursi, hierarchy, cv::RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
	
	// transform contours to world space...
	// ideally we'd transform them in place, BUT since they are stored as integers
	// we can't do this without aliasing along world unit boundaries.
	// SO we need to keep the scaling ratio in mind as we go...
	// also IDEALLY we'd use the transform matrix above and not just a single scaling factor,
	// but this works.

	// convert to 2f and scale
	vector< vector<cv::Point2f> > contours;
	
	for( auto c : contoursi )
	{
		vector<cv::Point2f> nc(c.size());
		
		for ( size_t i=0; i<nc.size(); ++i )
		{
			nc[i] = c[i]; // convert
			nc[i] *= contourPixelToWorld; // scale
		}
		
		contours.push_back(nc);
	}
	
	// filter and process contours into our format
	map<int,int> ocvContourIdxToMyContourIdx ;
	
	for( int i=0; i<contours.size(); ++i )
	{
		const auto & c = contours[i] ;
		
		float		area = cv::contourArea(c);
		
		cv::RotatedRect ocvMinRotatedRect = minAreaRect(c) ;
		Contour::tRotatedRect minRotatedRect;
		minRotatedRect.mCenter = fromOcv(ocvMinRotatedRect.center);
		minRotatedRect.mSize   = vec2(ocvMinRotatedRect.size.width,ocvMinRotatedRect.size.height);
		minRotatedRect.mAngle  = ocvMinRotatedRect.angle;
		const float minWidth = (float)(min( minRotatedRect.mSize.x, minRotatedRect.mSize.y ));
		const float maxWidth = (float)(max( minRotatedRect.mSize.x, minRotatedRect.mSize.y ));

		vec2  center ;
		float radius ;
		{
			if (mParams.mContourGetExactMinCircle) {
				cv::Point2f ocvcenter;
				cv::minEnclosingCircle( c, ocvcenter, radius ) ;
				center  = fromOcv(ocvcenter);
			} else {
				center = minRotatedRect.mCenter;
				radius = maxWidth/2.f;
			}
		}
		
		if (	radius   > mParams.mContourMinRadius &&
				area     > mParams.mContourMinArea   &&
				minWidth > mParams.mContourMinWidth )
		{
			auto addContour = [&]( const vector<cv::Point2f>& c )
			{
				Contour myc ;
				
				myc.mPolyLine = PolyLine2( fromOcv(c) );
				myc.mPolyLine.setClosed();

				myc.mPerimeter = cv::arcLength(c,true);
				myc.mRadius = radius ;
				myc.mCenter = center ;
				myc.mArea   = area ;
				myc.mBoundingRect = Rectf( myc.mPolyLine.getPoints() ); // after scaling points!
				myc.mRotatedBoundingRect = minRotatedRect;
				myc.mIndex = output.size() ;
				myc.mOcvContourIndex = i ;

				
				myc.mTreeDepth = 0 ;
				{
					int n = i ;
					while ( (n = hierarchy[n][3]) >= 0 )
					{
						myc.mTreeDepth++ ;
					}
				}
				myc.mIsHole = ( myc.mTreeDepth % 2 ) ; // odd depth # children are holes
				myc.mIsLeaf = hierarchy[i][2] < 0 ;
				
				output.push_back( myc );
				
				// store my index mapping
				ocvContourIdxToMyContourIdx[i] = output.size()-1 ;
			} ;
			
			if ( mParams.mContourDPEpsilon > 0 )
			{
				// simplify
				vector<cv::Point2f> approx ;
				
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
	for ( size_t i=0; i<output.size(); ++i )
	{
		Contour &c = output[i] ;
		
		if ( hierarchy[c.mOcvContourIndex][3] >= 0 )
		{
			c.mParent = ocvContourIdxToMyContourIdx[ hierarchy[c.mOcvContourIndex][3] ] ;
			
//				assert( myc.mParent is valid ) ;
			
			assert( c.mParent >= 0 && c.mParent < output.size() ) ;
			
			output[c.mParent].mChild.push_back( i ) ;
		}
	}
	
	return output;
}
