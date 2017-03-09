//
//  Pipeline.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#include "Pipeline.h"
#include "ocv.h"
//#include "CinderOpenCV.h"
//#include "opencv2/
#include "opencv2/core/opengl.hpp"

gl::TextureRef Pipeline::Stage::getGLImage() const
{
	if ( !mImageGL && !mImageCV.empty() )
	{
		mImageGL = matToTexture(mImageCV,true);
	}
	
	return mImageGL;
}

void Pipeline::Stage::setImageToWorldTransform( const glm::mat4& m )
{
	mImageToWorld = m;
	mWorldToImage = inverse(m);
}

void Pipeline::start()
{
	mStages.clear();
}

Pipeline::StageRef Pipeline::then( string name, Surface &img )
{
	StageRef s = then( name, img.getSize() );

	if ( getShouldCacheImage(s) ) s->mImageGL = gl::Texture::create( img );
	
	return s;
}

Pipeline::StageRef Pipeline::then( string name, cv::Mat &img )
{
	StageRef s = then( name, vec2(img.cols,img.rows) );

	if ( getShouldCacheImage(s) )
	{
		s->mImageCV = img.getUMat(cv::ACCESS_READ); // copy by value
		//s->mImageCV = img; // copy by ref (problematic)
	}

	return s;
}

Pipeline::StageRef Pipeline::then( string name, cv::UMat &img )
{
	StageRef s = then( name, vec2(img.cols,img.rows) );

	if ( getShouldCacheImage(s) )
	{
		s->mImageCV = img.clone();
//		img.copyTo(s->mImageCV); // copy by value
	}

	return s;
}

Pipeline::StageRef Pipeline::then( string name, gl::Texture2dRef ref )
{
	StageRef s = then( name, ref->getSize() );

	if ( getShouldCacheImage(s) ) s->mImageGL = ref;

	return s;
}

Pipeline::StageRef Pipeline::then( string name, gl::TextureCubeMapRef ref )
{
	StageRef s = then( name, ivec2( ref->getWidth(), ref->getHeight() ) );

	if ( getShouldCacheImage(s) ) s->mImageCubeMapGL = ref;

	return s;
}

Pipeline::StageRef Pipeline::then( string name, vec2 size )
{
	StageRef s = then(name);
	s->mImageSize = size;
	return s;
}

Pipeline::StageRef Pipeline::then( string name )
{
	StageRef s = make_shared<Stage>();
	s->mName = name ;
	
	if ( !mStages.empty() )
	{
		// inherit from last
		s->mImageToWorld = mStages.back()->mImageToWorld;
		s->mWorldToImage = mStages.back()->mWorldToImage;
		
		// (by default it will be the identity matrix, i think)
	}
	
	s->mStyle.mOrthoGroup = mOrthoGroup;
	
	mStages.push_back(s);

	return mStages.back();
}

bool Pipeline::getShouldCacheImage( const StageRef s ) const
{
	return mCaptureAllStageImages
	    || mCaptureStages.find(s->mName) != mCaptureStages.end();
}

string  Pipeline::getFirstStageName() const
{
	if ( empty() ) return "" ;
	else return mStages.front()->mName;
}

string  Pipeline::getLastStageName () const
{
	if ( empty() ) return "" ;
	else return mStages.back()->mName;
}

string Pipeline::getAdjStageName( string name, int adj ) const
{
	for ( size_t i=0; i<mStages.size(); ++i )
	{
		if ( mStages[i]->mName==name )
		{
			int k = (int)i + adj ;
			
			k = min( k, (int)mStages.size()-1 );
			k = max( k, 0 );
			
			return mStages[k]->mName;
		}
	}
	
	return getFirstStageName();
}

const Pipeline::StageRef Pipeline::getStage( string name ) const
{
	for ( size_t i=0; i<mStages.size(); ++i )
	{
		if ( mStages[i]->mName==name ) return mStages[i];
	}
	
	return 0;
}

mat4 Pipeline::getCoordSpaceTransform( string from, string to ) const
{
	if (0) cout << from << " => " << to << endl;
	
	mat4 t;

	// identity?
	if (from!=to)
	{
		auto warn = []( string msg )
		{
			cout << "getCoordSpaceTransform warning: " << msg << endl;
		};
		
		const StageRef fromStage = getStage(from);
		const StageRef toStage   = getStage(to);
		
		// warnings
		{
			if (( from=="world" && fromStage ) ||
				( to  =="world" && toStage   )) warn("There is a stage with reserved named 'world'.") ;
		}
		
		// world -> to
		if ( to!="world" )
		{
			if ( toStage ) t *= toStage->mWorldToImage;
			else warn( string("No '") + to + "' stage." );
		}

		// from -> world
		if ( from!="world" )
		{
			if ( fromStage ) t *= fromStage->mImageToWorld;
			else warn( string("No '") + from + "' stage." );
		}
	}
	
	return t;
}

void Pipeline::beginOrthoGroup()
{
	assert(mOrthoGroup==-1);
	mOrthoGroup = mNextOrthoGroup++;
}

void Pipeline::endOrthoGroup()
{
	assert(mOrthoGroup>=0);
	mOrthoGroup=-1;
}