//
//  Pipeline.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/10/16.
//
//

#ifndef Pipeline_hpp
#define Pipeline_hpp

#include "cinder/Surface.h"
#include "CinderOpenCV.h"

#include <string>
#include <vector>
#include <memory>

using namespace ci;
using namespace std;

class Pipeline
{
	/*	Pipeline isn't the pipeline so much as helping us trace the pipeline's process.
		However, I'm sticking with this name as I hope that it can one day develop into
		both debugging insight into the pipeline, controls for it, and eventually a program
		that is the image munging pipeline itself.
	*/
	
public:
	
	bool empty() const { return mStages.empty(); }
	
	string  getFirstStageName() const;
	string  getLastStageName () const;
	
	string	getNextStageName( string s ) const { return getAdjStageName(s, 1); }
	string	getPrevStageName( string s ) const { return getAdjStageName(s,-1); }
	
	class Stage
	{
	public:
		void setImageToWorldTransform( const glm::mat4& );
		gl::TextureRef getGLImage() const;

		string			mName;
		mat4			mImageToWorld;
		mat4			mWorldToImage;
		vec2			mImageSize;
		
		cv::UMat			mImageCV;
		mutable gl::TextureRef	mImageGL;
		gl::TextureCubeMapRef	mImageCubeMapGL;
		
		// layout hints
		struct Style
		{
			float	mScale=1.f;
			int		mOrthoGroup=-1;
		};
		Style mStyle;
	};

	typedef std::shared_ptr<Stage> StageRef;


	void start();
	
	StageRef then( string name );
	StageRef then( string name, vec2 frameSize );

	StageRef then( string name, Surface &img );
	StageRef then( string name, cv::Mat &img );
	StageRef then( string name, cv::UMat &img );
	
	// copy by ref, not value; you must do it yourself
	StageRef then( string name, gl::Texture2dRef ref );
	StageRef then( string name, gl::TextureCubeMapRef ref );
	
	const vector<StageRef>& getStages() const { return mStages ; } // result needs to be non-writable!
	const StageRef getStage( string name ) const;
	StageRef back() { return mStages.empty() ? StageRef() : mStages.back(); } // eneds proper const version
	
	void setCaptureAllStageImages( bool v ) { mCaptureAllStageImages=v; }
	set<string> getCaptureStages() const { return mCaptureStages; }
	void setCaptureStages( set<string> s ) { mCaptureStages=s; }
	
	mat4 getCoordSpaceTransform( string from, string to ) const;
		// from/to is name of coordinate space.
		// these can refer to a Stage::mName, or "world"
	
	void beginOrthoGroup();
	void endOrthoGroup();
	
private:

	// sticky style state
	int mOrthoGroup=-1;
	int mNextOrthoGroup=0;
	
	//
	bool mCaptureAllStageImages = false; // if false, then only extract query stage. true: capture all.
	set<string> mCaptureStages;
	
	void then( function<gl::Texture2dRef()>, string name, vec2 size );
		// so that we only fabricate the texture if it matches the query.
		// this might be a silly optimization, and we should just cache them all.

	bool getShouldCacheImage( const StageRef ) const;
	
	string getAdjStageName( string, int adj ) const;

	vector<StageRef> mStages;
	
} ;

#endif /* Pipeline_hpp */
