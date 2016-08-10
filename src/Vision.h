//
//  Vision.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef Vision_hpp
#define Vision_hpp

#include <string>
#include <vector>

#include "cinder/Surface.h"
#include "cinder/Xml.h"
#include "CinderOpenCV.h"
#include "LightLink.h"

#include "Vision.h"
#include "Contour.h"

using namespace ci;
using namespace ci::app;
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
	
	void setImageToWorldTransform( const cv::Mat& ); // 3x3
	void setImageToWorldTransform( const glm::mat3x3& ); // 3x3
	
	// add types:
	// - contour
	
//	gl::TextureRef getQueryFrame() const { return mQueryFrame ; }

	class Stage
	{
	public:
		string			mName;
		glm::mat3x3		mImageToWorld;
		glm::mat3x3		mWorldToImage;
		gl::TextureRef	mFrame ;
		
//		Rectf getBoundsInWorld() const;
	};
	
	const Stage* getQueryStage() const { return (mQueryIndex==-1) ? 0 : &mStages[mQueryIndex] ; }
	const vector<Stage>& getStages() const { return mStages ; }
	
private:

	void then( function<gl::Texture2dRef()>, string name );
		// so that we only fabricate the texture if it matches the query.
		// this might be a silly optimization, and we should just cache them all.
	
	string getAdjStageName( string, int adj ) const;

	vector<Stage> mStages;
	
	string		   mQuery ;
	int			   mQueryIndex=-1 ;
	
} ;


class Vision
{
public:

	// settings
	void setParams( XmlTree );

	float mResScale			=   .2;
	float mContourMinRadius	=	3;
	float mContourMinArea	=	100;
	float mContourDPEpsilon	=	5;
	float mContourMinWidth	=	5;

	void setLightLink( const LightLink &ll ) { mLightLink=ll; }
	
	// push input through
	void processFrame( const Surface &surface );
	
	// output
	ContourVector mContourOutput;
	
	// tracing output
	Pipeline	  mOCVPipelineTrace ;
	
private:
	LightLink mLightLink;

};

#endif /* Vision_hpp */
