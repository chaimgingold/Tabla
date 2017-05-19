//
//  AnimWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/19/16.
//
//

#include "AnimWorld.h"
#include "ocv.h"
#include "geom.h"
#include "xml.h"

using namespace std;
using namespace Anim;

static GameCartridgeSimple sCartridge("AnimWorld", [](){
	return std::make_shared<Anim::AnimWorld>();
});

PolyLine2 Frame::getQuadAsPoly() const
{
	PolyLine2 p;
	for( int i=0; i<4; ++i ) p.push_back(mQuad[i]);
	p.setClosed();
	return p;
}

vec2 Frame::calcQuadCenter() const
{
	return (mQuad[0]+mQuad[1]+mQuad[2]+mQuad[3])*.25f;
}

gl::TextureRef Frame::getAsTexture()
{
	if ( !mTexture && !mImageCV.empty() ) {
		mTexture = matToTexture(mImageCV,true);
	}
	return mTexture;
}

AnimWorld::AnimWorld()
{
}

void AnimWorld::setParams( XmlTree xml )
{
	getXml(xml,"DefaultTimeVec",mDefaultTimeVec);
	getXml(xml,"WorldUnitsToSeconds",mWorldUnitsToSeconds);
	getXml(xml,"MaxFrameDist",mMaxFrameDist);
	getXml(xml,"MaxScreenToFrameDist",mMaxScreenToFrameDist);
	getXml(xml,"DebugDrawTopology",mDebugDrawTopology);
	getXml(xml,"AnimLengthQuantizeToSec",mAnimLengthQuantizeToSec);
	getXml(xml,"EqualizeImages",mEqualizeImages);
	getXml(xml,"BlankEdgePixels",mBlankEdgePixels);
	getXml(xml,"FillCurrentFrame",mFillCurrentFrame);
	
	mLocalToGlobal = mat2( getTimeVec(), getUpVec() );
	mGlobalToLocal = inverse( mLocalToGlobal );
	
	if ( xml.hasChild("RectFinder") ) {
		mRectFinder.mParams.set( xml.getChild("RectFinder") );
	}	
}

void AnimWorld::initSettings()
{
	mTimeVec = mDefaultTimeVec;
}

XmlTree AnimWorld::getUserSettings() const
{
	XmlTree xml("settings","");
	
	xml.push_back( XmlTree("TimeVec", vecToString(mTimeVec)) );
	
	return xml;
}

void AnimWorld::setUserSettings( XmlTree settingsXml )
{
	if ( settingsXml.hasChild("settings") )
	{
		XmlTree xml = settingsXml.getChild("settings");
		
		getXml(xml, "TimeVec", mTimeVec );
	}
}

map<string,vec2> AnimWorld::getOrientationVecs() const
{
	map<string,vec2> m;
	
	m["Time"] = mTimeVec;
	
	return m;
}

void AnimWorld::setOrientationVec ( string name, vec2 value )
{
	if ( name=="Time" )
	{
		mTimeVec = value;
	}
	
	setAreUserSettingsDirty();
}

void AnimWorld::printAnims( const AnimSeqMap& as )
{
	cout << as.size() << " anims:" << endl;
	
	for( auto i : as )
	{
		cout << "\t" << i.first << " {";
		
		for( auto j : i.second )
		{
			cout << j << " ";
		}
		
		cout << "}" << endl;
	}
}

AnimSeqMap
AnimWorld::getAnimSeqs( const FrameVec& frames ) const
{
	AnimSeqMap as;
	
	for ( auto i : frames )
	{
		if ( i.isFirstAnimFrame() )
		{
			AnimSeq seq = getFrameIndicesOfSeq(frames,i);
			
			as[i.mIndex] = seq;
		}
	}
	
	return as;
}

int AnimWorld::getCurrentFrameIndexOfSeq( const FrameVec& frames, const AnimSeq& seq ) const
{
	vec2 worldLength = frames[seq.back()].mContour.mCenter - frames[seq.front()].mContour.mCenter;
//	float length = dot( worldLength, getTimeVec() ) / mWorldUnitsToSeconds ;
	float length = glm::length(worldLength) / mWorldUnitsToSeconds ;
//	float length = mWorldUnitsToSeconds; // hard coded for now..., for stability.
	
//	cout << glm::length(worldLength) << endl;
	
	// quantize length
	{
		float quantizeUnit = .5f;
		length = length - fmodf( length, quantizeUnit );
		length = max( length, quantizeUnit );
	}
	
	float t = fmod( mAnimTime, length ) / length;
	int index = constrain( (int)floorf(t * (float)(seq.size())), 0, (int)seq.size()-1 );
	int frame = seq[index];
	
//	cout << t << " -> " << index << " of " << seq.size() << " -> " << frame << endl;
	
	return frame;
}

void AnimWorld::updateCurrentFrames( AnimSeqMap& seqs, const FrameVec& frames, float currentTime )
{
	for( auto &i : seqs )
	{
		i.second.mCurrentFrameIndex = getCurrentFrameIndexOfSeq( frames, i.second );
	}
}

vector<int>
AnimWorld::getFrameIndicesOfSeq( const FrameVec& frames, const Frame& firstFrame ) const
{
	vector<int> seq;
	
	for( int n = firstFrame.mIndex; n!=-1; n = frames[n].mNextFrameIndex )
	{
		seq.push_back(n);
	}
	
	return seq;
}

FrameVec AnimWorld::getFrames(
	const Pipeline::StageRef world,
	const ContourVector &contours,
	Pipeline& pipeline ) const
{
	FrameVec frames;
	if ( !world || world->mImageCV.empty() ) return frames;

	pipeline.beginOrthoGroup();
	
	for ( auto c : contours )
	{
		if ( c.mIsHole || c.mTreeDepth>0 ) { //|| c.mPolyLine.size()!=4 ) {
			continue;
		}
			
		Frame frame;
		string frameName = string("[") + toString(frame.mIndex) + "]"; 
		
		if ( !mRectFinder.getRectFromPoly(c.mPolyLine,frame.mRectPoly) ) continue;

		frame.mIndex = frames.size();
		frame.mContour = c;

		getOrientedQuadFromPolyLine(frame.mRectPoly, mTimeVec, frame.mQuad);
		
		getSubMatWithQuad( world->mImageCV, frame.mImageCV, frame.mQuad, world->mWorldToImage, frame.mFrameImageToWorld );
		
		pipeline.then( "Frame"+frameName, frame.mImageCV);
		pipeline.back()->setImageToWorldTransform( frame.mFrameImageToWorld );
		pipeline.back()->mStyle.mScale = .5f;
		
		// blank edges
		if (mBlankEdgePixels>0)
		{
			cv::rectangle(frame.mImageCV, cv::Point(0,0), cv::Point(frame.mImageCV.cols-1,frame.mImageCV.rows-1), 255, mBlankEdgePixels );
			
			pipeline.then(string("Frame edge blanked"+frameName) + frame.mIndex, frame.mImageCV);
			pipeline.back()->mStyle.mScale = .5f;
		}
		
		// equalize?
		if (mEqualizeImages)
		{
			UMat frameContourImageEqualized;
			equalizeHist(frame.mImageCV, frameContourImageEqualized);
			pipeline.then(string("Frame equalized ") + frame.mIndex, frameContourImageEqualized);
			pipeline.back()->mStyle.mScale = .5f;
			frame.mImageCV = frameContourImageEqualized; // use that!
		}

		frames.push_back(frame);
	}
	
	pipeline.endOrthoGroup();
	
	return frames;
}

FrameVec
AnimWorld::getFrameTopology( const FrameVec& in ) const
{
	// we could also assign frames to be screens if they are blank.
	// but that could also muck people's ability to create and fill in a sequence.
	
	
	FrameVec out=in;
	
	// next/prev
	for( int i=0; i<mFrames.size(); ++i )
	{
		Frame&f = out[i];
		
		f.mNextFrameIndex = getSuccessorFrameIndex(f,out);
		
		// prev index
		if (f.mNextFrameIndex!=-1)
		{
			out[f.mNextFrameIndex].mPrevFrameIndex = i;
		}
	}
	
	// first and sequence index, length
	for ( auto &f : out )
	{
		if ( f.mPrevFrameIndex==-1 ) // is first
		{
			// first (and get length)
			int count=1; // 1 indexed
			
			int lastn=-1;
			
			for( int n = f.mIndex; n!=-1; n = out[n].mNextFrameIndex )
			{
				// sanity check to make sure no cyclical next/prev loops (it happens for some reason sometime!)
				if ( out[n].mFirstFrameIndex == f.mIndex )
				{
					cout << "Cyclical next/prev loop!!!" << endl;
					
					// break the cycle
					if (lastn!=-1) out[lastn].mNextFrameIndex = -1;
					f.mPrevFrameIndex = -1;
					break;
				}
				
				// set
				out[n].mFirstFrameIndex = f.mIndex;
				out[n].mSeqFrameNum = count++;
				
				//
				lastn=n;
			}
			
			// set length
			int length = count-1; // count is 1 indexed, so -1
			
			for( int n = f.mIndex; n!=-1; n = out[n].mNextFrameIndex )
			{
				out[n].mSeqFrameCount = length;
			}
		}
	}
	
	// screens
	for( auto &f : out )
	{
		if ( !f.isAnimFrame() )
		{
			f.mScreenFirstFrameIndex = getFirstFrameIndexOfAnimForScreen_Closest(f,out);
		}
	}
	
	return out;
}

PolyLine2 AnimWorld::globalToLocal( PolyLine2 p ) const
{
	PolyLine2 pp=p;
	
	for ( int i=0; i<p.size(); ++i )
	{
		vec2 &v = p.getPoints()[i];
		v = mGlobalToLocal * v;
	}
	
	return p;
}

Rectf
AnimWorld::getFrameBBoxInLocalSpace( const Frame& f ) const
{
	Rectf r( globalToLocal(f.mRectPoly).getPoints() );
	
	return r;
}

int
AnimWorld::getFirstFrameIndexOfAnimForScreen_Above( const Frame& screen, const FrameVec& frames ) const
{
	Rectf screenr = getFrameBBoxInLocalSpace(screen);
	// cout << screenr << endl;
	
	int besti=-1;
	float bestd=mMaxScreenToFrameDist;
	
	for( const auto &f : frames )
	{
		Rectf fr = getFrameBBoxInLocalSpace(f);
		
		if ( f.isAnimFrame() && f.mIndex != screen.mIndex
		  && !(screenr.x1 > fr.x2 || screenr.x2 < fr.x1 )
		  )
		{
			float d = screenr.y1 - fr.y2;
			
			if (d>0 && d<bestd)
			{
				bestd=d;
				besti=f.mFirstFrameIndex;
			}
		}
	}
	
	return besti;
}

int
AnimWorld::getFirstFrameIndexOfAnimForScreen_Closest( const Frame& screen, const FrameVec& frames ) const
{
	int besti=-1;
	float bestd=mMaxScreenToFrameDist;
	
	for( const auto &f : frames )
	{
		if ( f.isAnimFrame() && f.mIndex != screen.mIndex )
		{
			float d = distance(f.mContour.mCenter, screen.mContour.mCenter);
			// ideally d would be edge to edge, not center to center, but this is good enough  
			
			if (d<bestd)
			{
				bestd=d;
				besti=f.mFirstFrameIndex;
			}
		}
	}
	
	return besti;
}

int
AnimWorld::getSuccessorFrameIndex( const Frame& f, const FrameVec& fs ) const
{
	float dist;
	int i;
	
	i = getAdjacentFrameIndex(f, fs, getTimeVec(), &dist );
	
	if ( i != -1 && dist > mMaxFrameDist ) i=-1; // too far
	
	return i;
}

int AnimWorld::getAdjacentFrameIndex( const Frame& f, const FrameVec& fs, vec2 direction, float* distance ) const
{
	int best=-1;
	float bestdist=MAXFLOAT;
	
	vec2 origin = f.calcQuadCenter();
	
	for( int i=0; i<fs.size(); ++i )
	{
		if ( fs[i].mIndex != f.mIndex )
		{
			float t;
			
			if ( rayIntersectPoly( fs[i].getQuadAsPoly(), origin, direction, &t) && t < bestdist )
			{
				bestdist = t;
				best=i;
			}
		}
	}
	
	if (distance) *distance = bestdist;
	return best;
}

void AnimWorld::updateVision( const Vision::Output& visionOut, Pipeline&pipeline )
{
	const Pipeline::StageRef source = pipeline.getStage("clipped");
	if ( !source || source->mImageCV.empty() ) return;

	mFrames = getFrames( source, visionOut.mContours, pipeline );
	
	mFrames = getFrameTopology( mFrames );
	
	mAnims = getAnimSeqs(mFrames);
	updateCurrentFrames(mAnims,mFrames,mAnimTime);
	
	if (0) printAnims(mAnims);
}

void AnimWorld::update()
{
	mAnimTime = ci::app::getElapsedSeconds();
	
	updateCurrentFrames(mAnims,mFrames,mAnimTime);
}

void AnimWorld::drawScreen( const Frame& f, float alpha )
{
	int frame = mAnims[f.mScreenFirstFrameIndex].mCurrentFrameIndex;
	assert(frame!=-1);
//	cout << ci::app::getElapsedFrames() << ": " << frame << endl;
	
	gl::TextureRef tex = mFrames[frame].getAsTexture();
	if (!tex)
	{
		gl::color( 1, .8, 0, alpha );
		gl::draw(f.mRectPoly);
	}
	else
	{
		// This is kind of a pile of poo, but i got it to work, and that's what counts right now.
		// Ideally we draw into mQuad properly, but this is good enough.
		// Not rectangular screens (with non-90 degree angles) will start to look weird,
		// but this is pretty good.
		
		gl::pushModelMatrix();

		const vec2 screenCenter = lerp(f.mQuad[0],f.mQuad[2],.5f);
		
		const vec2 screenSize = vec2(
			distance(f.mQuad[0], f.mQuad[1]),
			distance(f.mQuad[1], f.mQuad[2])
			); 
		
		const vec2 scale = screenSize / vec2(tex->getSize());

		mat3 rotate(
			vec3(normalize(f.mQuad[1]-f.mQuad[0]),0),
			vec3(normalize(f.mQuad[2]-f.mQuad[1]),0),
			vec3(0,0,1)
		);
		
		gl::translate(screenCenter);
		gl::scale(vec2(1,1)*min(scale.x,scale.y));
		gl::multModelMatrix( mat4(rotate) );
		gl::translate( -vec2(tex->getSize())/2.f );
		
		gl::color(1,1,1,alpha);
		gl::draw(tex);
		
		gl::popModelMatrix();
	}
}

void AnimWorld::draw( DrawType drawType )
{
	for( const auto &f : mFrames )
	{
		if ( f.isAnimFrame() )
		{
			bool isCurrentFrame = ( mAnims[f.mFirstFrameIndex].mCurrentFrameIndex == f.mIndex );
			
			Color color = lerp( Color(0,1,1),Color(1,.5,0), (float)(f.mSeqFrameNum-1) / (float)(f.mSeqFrameCount-1) );
			

			if ( isCurrentFrame ) {
				gl::color( ColorA(color, drawType==DrawType::Projector ? 1.f : .5f) );
				if (mFillCurrentFrame) gl::drawSolid(f.mRectPoly);
				else {
					// fast solution to thick rectangle
					PolyLine2 mask;
					vec2 c = f.mRectPoly.calcCentroid();
					float k = 2.f;
					for ( int i=0; i<f.mRectPoly.size(); ++i )
					{
						vec2 in = f.mRectPoly.getPoints()[i];
						vec2 q = in + normalize(c-in) * k;
						mask.push_back(q);
					}
					mask.setClosed();
					
					gl::drawSolid(f.mRectPoly);
					gl::color(0,0,0);
					gl::drawSolid(mask);
					
//					gl::draw(f.mRectPoly);
				}
			} else {
				gl::color(color);
				gl::draw(f.mRectPoly);
			}
		}
		else if ( f.isScreen() )
		{
			if (drawType == DrawType::Projector)
			{
//				gl::color(1,1,1); // bathe screen in light
//				gl::drawSolid(f.mRectPoly);
				drawScreen(f,1.f);
			}
			else
			{
				gl::color(0,0,0,.3f);
				gl::draw(f.mRectPoly);
				drawScreen(f,.5f);
			}
		}
		else
		{
			gl::color( Color(1,.8,0) );
			gl::draw(f.mRectPoly);
			
			if (mDebugDrawTopology)
			{
				// axes
				gl::color(1,0,0);
				gl::drawLine(f.mContour.mCenter, f.mContour.mCenter + getTimeVec() * 5.f );
				gl::color(0,1,0);
				gl::drawLine(f.mContour.mCenter, f.mContour.mCenter + getUpVec() * 5.f );
			}
		}
		
		if (mDebugDrawTopology)
		{
			Color c[4] = { Color(1,0,0), Color(0,1,0), Color(0,0,1), Color(1,1,1) };
			
			for( int i=0; i<4; ++i )
			{
				gl::color(c[i]);
				gl::drawSolidCircle(f.mQuad[i],1.f);
			}
		}
		
		// test bounding boxes in local space
		if (0)
		{
			Rectf r = getFrameBBoxInLocalSpace(f);
			
			r = Rectf( mLocalToGlobal * r.getUpperLeft(), mLocalToGlobal * r.getLowerRight() );
			
			gl::color(1,0,0);
			gl::drawStrokedRect(r,1.f);
		}
	}
	
	if (mDebugDrawTopology)
	{
		for( const auto &f : mFrames )
		{
			if ( f.mNextFrameIndex!=-1 )
			{
				gl::color(0,0,1);
				gl::drawLine(f.mContour.mCenter, mFrames[f.mNextFrameIndex].mContour.mCenter );
			}

			if ( f.isScreen() ) //&& f.mScreenFirstFrameIndex!=-1 )
			{
				gl::color(1,0,0);
				gl::drawLine(f.mContour.mCenter, mFrames[f.mScreenFirstFrameIndex].mContour.mCenter );
			}
		}
	}
}

void AnimWorld::drawMouseDebugInfo( vec2 v )
{
//	cout << v << " => " << mGlobalToLocal * v << endl;
}
