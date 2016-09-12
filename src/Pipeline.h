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
	
	void	setQuery( string q ) { mQuery=q; } ;
	string	getQuery() const { return mQuery; }
	
	string  getFirstStageName() const;
	string  getLastStageName () const;
	
	string	getNextStageName( string s ) const { return getAdjStageName(s, 1); }
	string	getPrevStageName( string s ) const { return getAdjStageName(s,-1); }
	
	void start();
	
	void then( Surface &img, string name );
	void then( cv::Mat &img, string name );
	void then( gl::Texture2dRef ref, string name );
	void then( vec2 frameSize, string name );
	
	void setImageToWorldTransform( const glm::mat4& );
	
	// add types:
	// - contour
	// (maybe a class that slots into here and does drawing and eventually interaction)
	
	class Stage
	{
	public:
		string			mName;
		mat4			mImageToWorld;
		mat4			mWorldToImage;
		vec2			mImageSize;
		gl::TextureRef	mImage;
	};
	
	const Stage* getQueryStage() const { return (mQueryIndex==-1) ? 0 : &mStages[mQueryIndex] ; }
	const vector<Stage>& getStages() const { return mStages ; }
	const Stage* getStage( string name ) const;
	
	void setCaptureAllStageImages( bool v ) { mCaptureAllStageImages=v; }
	
	mat4 getCoordSpaceTransform( string from, string to ) const;
		// from/to is name of coordinate space.
		// these can refer to a Stage::mName, or "world"
	
private:

	bool mCaptureAllStageImages = false; // if false, then only extract query stage. true: capture all.
	
	void then( function<gl::Texture2dRef()>, string name, vec2 size );
		// so that we only fabricate the texture if it matches the query.
		// this might be a silly optimization, and we should just cache them all.
	
	string getAdjStageName( string, int adj ) const;

	vector<Stage> mStages;
	
	string		   mQuery ;
	int			   mQueryIndex=-1 ;
	
} ;

#endif /* Pipeline_hpp */
