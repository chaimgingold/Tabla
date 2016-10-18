//
//  MusicWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "MusicWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "xml.h"
#include "Pipeline.h"
#include "ocv.h"
#include "RtMidi.h"

#define MAX_SCORES 8

using namespace std::chrono;

// ShapeTracker was started to do inter-frame coherency,
// but I realized I don't need this to start with.
// It's gravy.
/*
class ShapeTracker
{
public:
	
	enum class ShapeState
	{
		Found,
		Missing
	};
	
	class Shape
	{
	public:
	private:
		friend class ShapeTracker;
		
		int	mId=0; // unique id for inter-frame tracking
		
		PolyLine2 mCreatedPoly;
		PolyLine2 mLastSeenPoly;
		
		ShapeState	mState;
		float		mEnteredStateWhen;

		Shape()
		{
			setState(ShapeState::Found);
		}

		void setState( ShapeState s )
		{
			mState = ShapeState::Found;
			mEnteredStateWhen = ci::app::getElapsedSeconds();
		}
	};
	
	void update( const ContourVector& );
	const Shape* getShape( int id ) const;
	
private:
	int getNewShapeId() { return mNextId++; }

	void indexShapes();
	
	int mNextId=1;
	vector<Shape>	mShapes;
	map<int,int>	mShapeById; // maps shape ids to mShapes indices
};

void ShapeTracker::update( const ContourVector& contours )
{
	// we're just going to filter for 4 cornered things for now
	const int kNumCorners = 4;
	
	for( const auto &c : contours )
	{
		if ( c.mPolyLine.size()==kNumCorners &&
			 c.mIsHole==false )
		{
			
		}
	}
}

const ShapeTracker::Shape* ShapeTracker::getShape( int id ) const
{
	auto i = mShapeById.find(id);
	
	if ( i == mShapeById.end() ) return 0;
	else return &mShapes[i->second];
}

void ShapeTracker::indexShapes()
{
	mShapeById.clear();
	
	for( int i=0; i<mShapes.size(); ++i )
	{
		mShapeById[mShapes[i].mId] = i;
	}
}
*/

bool MusicWorld::Score::setQuadFromPolyLine( PolyLine2 poly, vec2 timeVec )
{
	// could have simplified a bit and just examined two bisecting lines. oh well. it works.
	// also calling this object 'Score' and the internal scores 'score' is a little confusing.
	if ( poly.size()==4 )
	{
		auto in = [&]( int i ) -> vec2
		{
			return poly.getPoints()[i%4];
		};
		
		auto scoreEdge = [&]( int from, int to )
		{
			return dot( timeVec, normalize( in(to) - in(from) ) );
		};
		
		auto scoreSide = [&]( int side )
		{
			/* input pts:
			   0--1
			   |  |
			   3--2
			   
			   side 0 : score of 0-->1, 3-->2
			*/
			
			return ( scoreEdge(side+0,side+1) + scoreEdge(side+3,side+2) ) / 2.f;
		};
		
		int   bestSide =0;
		float bestScore=0;
		
		for( int i=0; i<4; ++i )
		{
			float score = scoreSide(i);
			
			if ( score > bestScore )
			{
				bestScore = score ;
				bestSide  = i;
			}
		}
		
		// copy to mQuad
		for ( int i=0; i<4; ++i ) mQuad[i] = in(bestSide+i);
		
		return true ;
	}
	else return false;
}

PolyLine2 MusicWorld::Score::getPolyLine() const
{
	PolyLine2 p;
	p.setClosed();
	
	for( int i=0; i<4; ++i ) p.getPoints().push_back( mQuad[i] );
	
	return p;
}

float MusicWorld::Score::getPlayheadFrac() const
{
	// could modulate quadPhase by size/shape of quad
	
	float t = fmod( (ci::app::getElapsedSeconds() - mStartTime)/mDuration, 1.f ) ;
	
	return t;
}

void MusicWorld::Score::getPlayheadLine( vec2 line[2] ) const
{
	float t = getPlayheadFrac();
	
	line[0] = lerp(mQuad[0],mQuad[1],t);
	line[1] = lerp(mQuad[3],mQuad[2],t);
}



MusicWorld::MusicWorld()
{
	mStartTime = ci::app::getElapsedSeconds();

	mTimeVec = vec2(0,-1);
	mTempo   = 8.f;
	
	setupSynthesis();
}

void MusicWorld::setParams( XmlTree xml )
{
	getXml(xml,"Tempo",mTempo);
	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount); // ??? not working
	
	cout << "NoteCount " << mNoteCount << endl;
}

void MusicWorld::updateContours( const ContourVector &contours )
{
	// board shape
	Rectf worldBoundsRect( getWorldBoundsPoly().getPoints() );
	
	// erase old scores
	mScore.clear();
	
	// get new ones
	for( const auto &c : contours )
	{
		if ( !c.mIsHole && c.mPolyLine.size()==4 )
		{
			Score score;
			
			// shape
			score.setQuadFromPolyLine(c.mPolyLine,mTimeVec);
			
			// timing
			score.mStartTime = mStartTime;
			score.mDuration = mTempo; // inherit, but it could be custom based on shape or something
			
			// synth type
//			score.mSynthType = c.mArea > 15 ? Score::SynthType::MIDI : Score::SynthType::Additive ;
			score.mSynthType = Score::SynthType::MIDI ;
//			score.mSynthType = Score::SynthType::Additive ;
			
			// synth params
			if ( score.mSynthType==Score::SynthType::MIDI )
			{
				score.mNoteCount = mNoteCount;
			}
			
			// spatialization
			vec2 centroid = score.getPolyLine().calcCentroid();
			vec2 centroidNorm = (centroid - worldBoundsRect.getLowerLeft()) / (worldBoundsRect.getSize()) ;

			score.mPan		= centroidNorm.y;
//			score.mNoteRoot = centroidNorm.x;
			score.mNoteRoot = 60; // middle C
			score.mNoteInstrument = floorf( centroidNorm.x / 8.f );
			// TODO: this needs to be oriented relative to mTimeVec; would a simple rotation do?
			
			mScore.push_back(score);
		}
	}
}

void MusicWorld::updateCustomVision( Pipeline& pipeline )
{
	// this function grabs the bitmaps for each score
	
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("clipped");
	if ( !world || world->mImageCV.empty() ) return;
	
	// for each score...
	vec2 outsize(100,100);

	vec2		srcpt[4];
	cv::Point2f srcpt_cv[4];
	vec2		dstpt[4]    = { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };
	cv::Point2f dstpt_cv[4]	= { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };

	int scoreNum=1;
	
	for( Score& s : mScore )
	{
		string scoreName = string("score")+toString(scoreNum);
		
		// get src points
		for ( int i=0; i<4; ++i )
		{
			srcpt[i] = vec2( world->mWorldToImage * vec4(s.mQuad[i],0,1) );
			srcpt_cv[i] = toOcv( srcpt[i] );
		}
		
		// use default dstpts
		
		// grab it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt_cv, dstpt_cv ) ;
		cv::warpPerspective( world->mImageCV, s.mImage, xform, cv::Size(outsize.x,outsize.y) );

		pipeline.then( scoreName, s.mImage);
		pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(dstpt,s.mQuad) );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;

		// midi quantize
		if ( s.mSynthType==Score::SynthType::MIDI )
		{
			// resample
			cv::Mat quantized;
			cv::resize( s.mImage, quantized, cv::Size(s.mImage.cols,s.mNoteCount) );
			pipeline.then( scoreName + "quantized", quantized);
			pipeline.setImageToWorldTransform(
				pipeline.getStages().back()->mImageToWorld * glm::scale(vec3(1,1.f/(float)s.mNoteCount,1))
				);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;
			
			// threshold
			cv::Mat thresholded;
			cv::threshold( quantized, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
			pipeline.then( scoreName + "thresholded", thresholded);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;

			// output
			s.mQuantizedImage = thresholded;
		}
		
		//
		scoreNum++;
	}

	// update synth
	updateScoreSynthesis();
}

void MusicWorld::update()
{
	// send @fps values to Pd
	int scoreNum=0;
	
	for( const auto &score : mScore )
	{
		// Update time
		mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
								 score.getPlayheadFrac()*score.mDuration );

		// midi
//		inst = int[] = { 3, 16, 4 }[scoreNum];
		
		if ( score.mSynthType==Score::SynthType::MIDI )
		{
			// instrument
			mPureDataNode->sendFloat(string("midi-instrument")+toString(scoreNum), scoreNum);
			
			// notes
			int x = score.getPlayheadFrac() * (float)(score.mQuantizedImage.cols-1) ;

			for ( int y=0; y<score.mNoteCount; ++y )
			{
				unsigned char value = score.mQuantizedImage.at<unsigned char>(y,x) ;
				
				if ( value < 100 ) value=1; // if dark, then high
				else value=0; // else low
				
				float duration = .3f;
				// TODO: look in image, and make duration length of on pixels in that row!
				
				if (value) doNoteOn( score.mNoteInstrument, score.mNoteRoot+y, duration );
			}
		}
		
		//
		scoreNum++;
	}
	
	// retire notes
	updateNoteOffs();
}

bool MusicWorld::isNoteInFlight( int instr, int note ) const
{
	return mOnNotes.find( tOnNoteKey(instr,note) ) != mOnNotes.end();
}

void MusicWorld::updateNoteOffs()
{
	const float now = ci::app::getElapsedSeconds();
	
	for ( auto it = mOnNotes.begin(); it != mOnNotes.end(); /*manually...*/ )
	{
		bool off = now > it->second.mStartTime + it->second.mDuration;
		
		if (off)
		{
			uchar channel = it->first.mInstrument & 0xF;
			sendMidi( (8<<4) | channel, it->first.mNote, 40 );
		}
		
		if (off) mOnNotes.erase(it++);
		else ++it;
	}
}

void MusicWorld::doNoteOn( int instr, int note, float duration )
{
	if ( !isNoteInFlight(instr,note) )
	{
		uchar channel = instr & 0xF;
		sendMidi( (9<<4) | channel, note, 90);
		
		tOnNoteInfo i;
		i.mStartTime = ci::app::getElapsedSeconds();
		i.mDuration  = duration;
		
		mOnNotes[ tOnNoteKey(instr,note) ] = i;
	}
}

void MusicWorld::sendMidi( int a, int b, int c )
{
	if (mMidiOut)
	{
		std::vector<unsigned char> message(3);

		message[0] = a;
		message[1] = b;
		message[2] = c;
		
		mMidiOut->sendMessage( &message );
	}
}

void MusicWorld::updateScoreSynthesis() {

	// send scores to Pd
	int scoreNum=0;
	
	for( const auto &score : mScore )
	{
		// Update pan
		mPureDataNode->sendFloat(string("pan")+toString(scoreNum),score.mPan);
		
		// Update per-score pitch
		mPureDataNode->sendFloat(string("note-root")+toString(scoreNum),score.mNoteRoot);

		// Update time
//		mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
//								 score.getPlayheadFrac()*score.mDuration );

		// send image
		if ( !score.mImage.empty() )
		{
			// Create a float version of the image
			cv::Mat imageFloatMat;
			
			// Copy the uchar version scaled 0-1
			score.mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
			
			// Convert to a vector to pass to Pd
			std::vector<float> imageVector;
			imageVector.assign( (float*)imageFloatMat.datastart, (float*)imageFloatMat.dataend );

			mPureDataNode->writeArray(string("image")+toString(scoreNum), imageVector);
		}

		//
		scoreNum++;
	}

	// Clear remaining scores
	std::vector<float> emptyVector(10000, 1.0);
	while( scoreNum < MAX_SCORES ) {
		mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
								 0);
		mPureDataNode->writeArray(string("image")+toString(scoreNum), emptyVector);
		scoreNum++;
	}
}

void MusicWorld::draw( bool highQuality )
{
	for( const auto &score : mScore )
	{
		if ( score.mSynthType==Score::SynthType::Additive )
		{
			gl::color(.5,0,0);
			gl::draw( score.getPolyLine() );
		}
		else
		{
			gl::color(1,0,0);
			gl::draw( score.getPolyLine() );

			for( int i=0; i<score.mNoteCount; ++i )
			{
				float f = (float)(i+1) / (float)score.mNoteCount;
				
				gl::drawLine(
					lerp(score.mQuad[3], score.mQuad[0],f),
					lerp(score.mQuad[2], score.mQuad[1],f)
					);
			}
		}
		
		//
		if (score.mSynthType==Score::SynthType::Additive) gl::color(0,1,0);
		else gl::color(0, 1, 0);
		
		vec2 playhead[2];
		score.getPlayheadLine(playhead);
		gl::drawLine( playhead[0], playhead[1] );
	}
	
	// draw time direction (for debugging score generation)
	if (0)
	{
		gl::color(0,1,0);
		gl::drawLine( vec2(0,0), vec2(0,0) + mTimeVec*10.f );
	}
}

// Synthesis
void MusicWorld::setupSynthesis()
{
	mMidiOut = make_shared<RtMidiOut>();
	
	mMidiOut->openVirtualPort();
	
	unsigned int nPorts = mMidiOut->getPortCount();
	if ( nPorts == 0 ) {
		std::cout << "No ports available!\n";
	}
	
	// Create the synth engine
	mPureDataNode = cipd::PureDataNode::Global();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/music.pd")) );
}

MusicWorld::~MusicWorld() {
	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
