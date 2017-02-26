//
//  ocv.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#include "ocv.h"

namespace cinder {
	
	void vec2toOCV_4( vec2 in[4], cv::Point2f o[4] )
	{
		for ( int i=0; i<4; ++i ) o[i] = toOcv( in[i] );
	}
	
	vector<vec2> fromOcv( vector<cv::Point> pts )
	{
		vector<vec2> v;
		for( auto p : pts ) v.push_back(vec2(p.x,p.y));
		return v;
	}

	vector<vec2> fromOcv( vector<cv::Point2f> pts )
	{
		vector<vec2> v;
		for( auto p : pts ) v.push_back(vec2(p.x,p.y));
		return v;
	}

	vector<cv::Point2f> toOcv( vector<vec2> pts )
	{
		vector<cv::Point2f> v;
		for( auto p : pts ) v.push_back(cv::Point2f(p.x,p.y));
		return v;
	}

	// stuff below doesn't really belong in cinder namespace, but whatever.
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

	mat4 mat3to4 ( mat3 i )
	{
		mat4 o;
		
		// copy upper left 2x2
		for( int x=0; x<2; ++x )
		for( int y=0; y<2; ++y )
		{
			o[x][y] = i[x][y];
		}
		
		o[0][3] = i[0][2];
		o[1][3] = i[1][2];

		o[3][0] = i[2][0];
		o[3][1] = i[2][1];
		
		o[3][3] = i[2][2];
		
		return o;
	}

	mat3 mat4to3 ( mat4 i )
	{
		mat3 o;

		// copy upper left 2x2
		for( int x=0; x<2; ++x )
		for( int y=0; y<2; ++y )
		{
			o[x][y] = i[x][y];
		}

		o[0][2] = i[0][3];
		o[1][2] = i[1][3];

		o[2][0] = i[3][0];
		o[2][1] = i[3][1];

		o[2][2] = i[3][3];

		return o;
	}


	mat4 getOcvPerspectiveTransform( const vec2 from[4], const vec2 to[4] )
	{
		cv::Point2f srcpt[4], dstpt[4];
		
		for( int i=0; i<4; ++i )
		{
			srcpt[i] = toOcv( from[i] );
			dstpt[i] = toOcv( to[i] );
		}

		cv::Mat xform = cv::getPerspectiveTransform( srcpt, dstpt ) ;
		
		return mat3to4( fromOcvMat3x3(xform) );
	}
	
	gl::TextureRef matToTexture( cv::Mat mat, bool loadTopDown )
	{
		return gl::Texture::create(
			ImageSourceRef( new ImageSourceCvMat(mat)),
			gl::Texture::Format().loadTopDown(loadTopDown)
			);
	}
	
	gl::TextureRef matToTexture( cv::UMat mat, bool loadTopDown )
	{
		/*if (0)
		{
			// http://stackoverflow.com/questions/18086519/is-it-possible-to-bind-a-opencv-gpumat-as-an-opengl-texture
			
			cv::ogl::Texture2D texture = cv::ogl::Texture2D(mat,false); // no auto-release, we will own it

			return gl::Texture2d::create(
				GL_TEXTURE_2D, // texture target (is this right?)
				texture.texId(), // id
				texture.cols(), texture.rows(), // size
				false ); // do dispose it. (cinder owns the texture)
		}
		else*/
		{
	//		mImageGL = gl::Texture::create( fromOcv(mImageCV), gl::Texture::Format().loadTopDown() );
			return gl::Texture::create(
				ImageSourceRef( new ImageSourceCvMat( mat.getMat(cv::ACCESS_READ) ) ),
				gl::Texture::Format().loadTopDown(loadTopDown) );
		}
	}

	void getSubMatWithQuad(
		cv::InputArray in,
		cv::OutputArray out,
		const vec2 quad[4],
		const mat4& quadWorldToInputImage, // maps quadWorld -> input matrix space
		mat4& outputImageToWorld // maps output matrix space -> world space
		)
	{
		vec2 srcpt[4];
		cv::Point2f srcpt_cv[4];

		for ( int i=0; i<4; ++i ) {
			srcpt[i] = vec2( quadWorldToInputImage * vec4(quad[i],0,1) );
		}
		
		vec2toOCV_4(srcpt, srcpt_cv);

		// get output points
		vec2 outsize(
			max( distance(srcpt[0],srcpt[1]), distance(srcpt[3],srcpt[2]) ),
			max( distance(srcpt[0],srcpt[3]), distance(srcpt[1],srcpt[2]) )
		);
		vec2 dstpt[4] = { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };

		cv::Point2f dstpt_cv[4];
		vec2toOCV_4(dstpt, dstpt_cv);

		// grab it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt_cv, dstpt_cv ) ;
		cv::warpPerspective( in, out, xform, cv::Size(outsize.x,outsize.y) );
		
		outputImageToWorld = getOcvPerspectiveTransform(dstpt,quad);				
	}

	PolyLine2 approxPolyDP( const PolyLine2& p, float eps )
	{
		vector<cv::Point2f> approx ;
		
		cv::approxPolyDP( toOcv(p.getPoints()), approx, eps, p.isClosed() );

		PolyLine2 out;
		
		out.getPoints() = fromOcv(approx);
		out.setClosed(p.isClosed());
		return out;
	}
	
}