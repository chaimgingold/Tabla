//
//  Pipeline.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#include "Pipeline.h"
#include "ocv.h"

void Pipeline::start()
{
	mStages.clear();
	mQueryIndex = -1;
}

void Pipeline::then( Surface &img, string name )
{
	then( [&](){
			return gl::Texture::create( img );
		}, name, img.getSize() ) ;
}

void Pipeline::then( cv::Mat &img, string name )
{
	then( [&](){
			return gl::Texture::create( fromOcv(img), gl::Texture::Format().loadTopDown() );
		}, name, vec2(img.cols,img.rows) ) ;
}

void Pipeline::then( gl::Texture2dRef ref, string name )
{
	then( [&](){
			return ref;
		}, name, ref->getSize() ) ;
}

void Pipeline::then( vec2 frameSize, string name )
{
	auto x = []() -> gl::Texture2dRef { return gl::Texture2dRef() ; } ;
	
	then( x, name, frameSize ) ;
}

void Pipeline::then( function<gl::Texture2dRef()> func, string name, vec2 size )
{
	Stage s;
	s.mName = name ;
	s.mImageSize = size;
	
	if ( !mStages.empty() )
	{
		// inherit from last
		s.mImageToWorld = mStages.back().mImageToWorld;
		s.mWorldToImage = mStages.back().mWorldToImage;
		
		// (by default it will be the identity matrix, i think)
	}
	
	mStages.push_back(s);
	
	bool isQuery = ( name==mQuery );
	
	if (isQuery) mQueryIndex = mStages.size()-1;
	
	if (isQuery || mCaptureAllStageImages) mStages.back().mImage = func();
}

void Pipeline::setImageToWorldTransform( const glm::mat4& m )
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

const Pipeline::Stage* Pipeline::getStage( string name ) const
{
	for ( size_t i=0; i<mStages.size(); ++i )
	{
		if ( mStages[i].mName==name ) return &mStages[i];
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
		
		const Stage* fromStage = getStage(from);
		const Stage* toStage   = getStage(to);
		
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