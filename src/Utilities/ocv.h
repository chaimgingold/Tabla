//
//  ocv.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#ifndef ocv_h
#define ocv_h

#include "ocv.h"
#include "CinderOpenCV.h"

using namespace std;

namespace cinder {
	
	void vec2toOCV_4( vec2 in[4], cv::Point2f o[4] );
	
	vector<vec2> fromOcv( vector<cv::Point> pts );
	vector<vec2> fromOcv( vector<cv::Point2f> pts );
	vector<cv::Point2f> toOcv( vector<vec2> pts );

	// stuff below doesn't really belong in cinder namespace, but whatever.
	glm::mat3x3 fromOcvMat3x3( const cv::Mat& m );

	mat4 mat3to4 ( mat3 i );
	mat3 mat4to3 ( mat4 i );

	mat4 getOcvPerspectiveTransform( const vec2 from[4], const vec2 to[4] );

	template <class T>
	bool isMatEqual( cv::Mat a, cv::Mat b )
	{
		// http://stackoverflow.com/questions/9905093/how-to-check-whether-two-matrixes-are-identical-in-opencv

		if ( a.empty() != b.empty() ) return false;
		if ( a.cols != b.cols ) return false;
		if ( a.rows != b.rows ) return false;

		for( int x=0; x<b.cols; ++x )
		for( int y=0; y<b.cols; ++y )
		{
			if ( a.at<T>(y,x) != b.at<T>(y,x) ) return false;
		}

		return true;
	}
	
	gl::TextureRef matToTexture( cv::UMat mat, bool loadTopDown );
	gl::TextureRef matToTexture( cv::Mat  mat, bool loadTopDown );

	PolyLine2 approxPolyDP( const PolyLine2&, float eps );
	
	void getSubMatWithQuad(
		cv::InputArray,
		cv::OutputArray,
		const vec2 quadWorld[4],
		const mat4& quadWorldToInputImage, // maps quadWorld -> input matrix space
		mat4& outputImageToWorld // maps output matrix space -> world space
		);
	
}


#endif /* ocv_h */
