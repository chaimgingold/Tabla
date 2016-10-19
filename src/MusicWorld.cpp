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

vec2 MusicWorld::Score::fracToQuad( vec2 frac ) const
{
	vec2 top = lerp(mQuad[0],mQuad[1],frac.x);
	vec2 bot = lerp(mQuad[3],mQuad[2],frac.x);
	
	return lerp(bot,top,frac.y);
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

pair<float,float> getShapeRange( const vec2* pts, int npts, vec2 lookVec )
{
	float worldy1=MAXFLOAT, worldy2=-MAXFLOAT;

	for( int i=0; i<npts; ++i )
	{
		vec2 p = pts[i];
		
		float y = dot( p, lookVec );
		
		worldy1 = min( worldy1, y );
		worldy2 = max( worldy2, y );
	}
	
	return pair<float,float>(worldy1,worldy2);
}

void MusicWorld::updateContours( const ContourVector &contours )
{
	// board shape
	const vec2 lookVec = perp(mTimeVec);

	pair<float,float> worldYs(0,0);
	pair<float,float> worldXs(0,0);
	
	if ( getWorldBoundsPoly().size() > 0 )
	{
		worldYs = getShapeRange( &getWorldBoundsPoly().getPoints()[0], getWorldBoundsPoly().size(), lookVec );
		worldXs = getShapeRange( &getWorldBoundsPoly().getPoints()[0], getWorldBoundsPoly().size(), mTimeVec );
	}
	
	// erase old scores
	mScores.clear();
	
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
			
			// use place in world to determine some factors...
			pair<float,float> ys = getShapeRange( score.mQuad, 4, lookVec );
//			pair<float,float> xs = getShapeRange( score.mQuad, 4, mTimeVec );
			const float yf = 1.f - (ys.first - worldYs.first) / (worldYs.second-worldYs.first);

			// FIXME: fix this scaling to get an even distribution of instruments across the X axis
			const float xf = score.getPolyLine().calcCentroid().y / (worldXs.second-worldXs.first);

			// synth type
//			score.mSynthType = Score::SynthType::MIDI ;
//			score.mSynthType = Score::SynthType::Additive ;

			const float xAdditiveZone = 0.9;

			if ( xf > xAdditiveZone ) score.mSynthType = Score::SynthType::Additive;
			else score.mSynthType = Score::SynthType::MIDI;

			// Choose octave based on up<>down
			int octaveShift = (yf - .5f) * 10.f;
			int noteRoot = 60 + octaveShift*12; // middle C
			score.mNoteRoot = noteRoot;

			// MIDI synth params
			if ( score.mSynthType==Score::SynthType::MIDI )
			{
				// Choose instrument based on left<>right
				int instrumentNum = mMidiOuts.size() * (xf / xAdditiveZone);
				score.mNoteInstrument = instrumentNum;
				score.mNoteCount = mNoteCount;

			} else if ( score.mSynthType==Score::SynthType::Additive ) {

			}

			score.mPan		= .5f ;
			
			mScores.push_back(score);
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
	
	for( Score& s : mScores )
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
				pipeline.getStages().back()->mImageToWorld
					* glm::scale(vec3(1, outsize.y / (float)s.mNoteCount, 1))
				);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;
			
			// threshold
			cv::Mat thresholded;


			cv::threshold( quantized, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
			// NOTE(lxi): below broke note detection, restored to above
//			cv::threshold( quantized, thresholded, 220, 255, cv::THRESH_BINARY );
			pipeline.then( scoreName + "thresholded", thresholded);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;

			// output
			s.mQuantizedImage = thresholded;
		}
		
		//
		scoreNum++;
	}

	// update additive synths based on new image data
	updateAdditiveScoreSynthesis();
}

bool MusicWorld::isScoreValueHigh( uchar value ) const
{
	const int kValueThresh = 100;
	
	return value < kValueThresh ;
	// if dark, then high
	// else low
}

int MusicWorld::getNoteLengthAsImageCols( cv::Mat image, int x, int y ) const
{
	int x2;
	
	for( x2=x;
		 x2<image.cols
			&& isScoreValueHigh(image.at<unsigned char>(y,x2)) ;
		 ++x2 )
	{}

	int len = x2-x;
	
	if (len==1) len=0; // filter out 1 pixel wide notes.
	
	return len;
}

float MusicWorld::getNoteLengthAsScoreFrac( cv::Mat image, int x, int y ) const
{
	// can return 0, which means we filtered out super short image noise-notes
	
	return (float)getNoteLengthAsImageCols(image,x,y) / (float)image.cols;
}

void MusicWorld::update()
{
	// send @fps values to Pd
	int scoreNum=0;
	
	for( const auto &score : mScores )
	{
		if ( score.mSynthType==Score::SynthType::Additive ) {
			// Update time
			mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
									 score.getPlayheadFrac()*100.0 );
		}
		// send midi notes
		else if ( score.mSynthType==Score::SynthType::MIDI )
		{
			
			// notes
			int x = score.getPlayheadFrac() * (float)(score.mQuantizedImage.cols-1) ;

			for ( int y=0; y<score.mNoteCount; ++y )
			{
				unsigned char value = score.mQuantizedImage.at<unsigned char>(y,x) ;
				
				if ( isScoreValueHigh(value) )
				{
					float duration = score.mDuration * getNoteLengthAsScoreFrac(score.mQuantizedImage,x,y);
					
					if (duration>0)
					{
						doNoteOn( score.mNoteInstrument, score.mNoteRoot+y, duration );
					}
				}
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

	// search for "expired" notes and send them their note-off MIDI message,
	// then remove them from the mOnNotes map
	for ( auto it = mOnNotes.begin(); it != mOnNotes.end(); /*manually...*/ )
	{
		bool off = now > it->second.mStartTime + it->second.mDuration;
		
		if (off)
		{
			int instr = it->first.mInstrument;
			RtMidiOutRef midiOut = mMidiOuts[instr % mMidiOuts.size()];
			uchar channel = 0;
			sendNoteOff( midiOut, channel, it->first.mNote);
		}
		
		if (off) mOnNotes.erase(it++);
		else ++it;
	}
}

void MusicWorld::killAllNotes() {
	uchar channel = 0;
	for ( const auto &midiOut : mMidiOuts ) {
		for (int note = 0; note < 128; note++) {
			sendNoteOff( midiOut, channel, note );
		}
	}
}

void MusicWorld::doNoteOn( int instr, int note, float duration )
{
	if ( !isNoteInFlight(instr,note) )
	{
		RtMidiOutRef midiOut = mMidiOuts[instr % mMidiOuts.size()];
		uchar channel = 0;
		uchar velocity = 100; // 0-127
		sendNoteOn( midiOut, channel, note, velocity );
		
		tOnNoteInfo i;
		i.mStartTime = ci::app::getElapsedSeconds();
		i.mDuration  = duration;
		
		mOnNotes[ tOnNoteKey(instr,note) ] = i;
	}
}

void MusicWorld::sendNoteOn ( RtMidiOutRef midiOut, uchar channel, uchar note, uchar velocity ) {
	const uchar noteOnBits = 9;

	uchar channelBits = channel & 0xF;

	sendMidi( midiOut, (noteOnBits<<4) | channelBits, note, velocity);
}

void MusicWorld::sendNoteOff ( RtMidiOutRef midiOut, uchar channel, uchar note ) {
	const uchar velocity = 0; // MIDI supports "note off velocity", but that's esoteric and we're not using it
	const uchar noteOffBits = 8;

	uchar channelBits = channel & 0xF;

	sendMidi( midiOut, (noteOffBits<<4) | channelBits, note, velocity);
}

void MusicWorld::sendMidi( RtMidiOutRef midiOut, uchar a, uchar b, uchar c )
{

	std::vector<uchar> message(3);

	message[0] = a;
	message[1] = b;
	message[2] = c;

	midiOut->sendMessage( &message );

}

void MusicWorld::updateAdditiveScoreSynthesis() {

	// send scores to Pd
	int scoreNum=0;
	
	for( const auto &score : mScores )
	{
		// send image for additive synthesis
		if ( score.mSynthType==Score::SynthType::Additive && !score.mImage.empty() )
		{
			// Update pan
			mPureDataNode->sendFloat(string("pan")+toString(scoreNum),
									 score.mPan);

			// Update per-score pitch
			mPureDataNode->sendFloat(string("note-root")+toString(scoreNum),
									 score.mNoteRoot);

			// Create a float version of the image
			cv::Mat imageFloatMat;
			
			// Copy the uchar version scaled 0-1
			score.mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
			
			// Convert to a vector to pass to Pd
			std::vector<float> imageVector;
			imageVector.assign( (float*)imageFloatMat.datastart, (float*)imageFloatMat.dataend );

			mPureDataNode->writeArray(string("image")+toString(scoreNum),
									  imageVector);
		}

		//
		scoreNum++;
	}

	// Clear remaining scores
	const int kMaxScores = 8; // This corresponds to [clone 8 music-voice] in music.pd
	std::vector<float> emptyVector(10000, 1.0);
	while( scoreNum < kMaxScores ) {

		mPureDataNode->writeArray(string("image")+toString(scoreNum),
								  emptyVector);

		mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
								 0);

		scoreNum++;
	}
}

void MusicWorld::draw( bool highQuality )
{
	for( const auto &score : mScores )
	{
		if ( score.mSynthType==Score::SynthType::Additive )
		{
			// additive
			gl::color(.5,0,0);
			gl::draw( score.getPolyLine() );
		}
		else if ( score.mSynthType==Score::SynthType::MIDI )
		{
			// midi
			
			// draw notes
			// (probably wise to extract this geometry/data when processing vision data,
			// then use it for both playback and drawing).
			const float invcols = 1.f / (float)(score.mQuantizedImage.cols-1);

			const float yheight = 1.f / (float)score.mNoteCount;
			
			for ( int y=0; y<score.mNoteCount; ++y )
			{
				const float fracy1 = 1.f - (y * yheight + yheight*.2f);
				const float fracy2 = 1.f - (y * yheight + yheight*.8f);
				
				for( int x=0; x<score.mQuantizedImage.cols; ++x )
				{
					if ( isScoreValueHigh(score.mQuantizedImage.at<unsigned char>(y,x)) )
					{
						// how wide?
						int length = getNoteLengthAsImageCols(score.mQuantizedImage,x,y);
						
						vec2 start1 = score.fracToQuad( vec2( (float)(x * invcols), fracy1 ) ) ;
						vec2 end1   = score.fracToQuad( vec2( (float)(x+length) * invcols, fracy1) ) ;

						vec2 start2 = score.fracToQuad( vec2( (float)(x * invcols), fracy2 ) ) ;
						vec2 end2   = score.fracToQuad( vec2( (float)(x+length) * invcols, fracy2) ) ;
						
						if ( isNoteInFlight(score.mNoteInstrument, score.mNoteRoot+y) )
						{
							gl::color(1,0,0);
						}
						else gl::color(0,1,0);

						gl::drawSolidTriangle(start1, end1, end2);
						gl::drawSolidTriangle(start1, end2, start2);
						// this is insanity, but i don't yet get how to easily draw raw triangles in glNext.
						// gl::begin(GL_QUAD)/end didn't quite work.
						
						// skip ahead
						x = x + length+1;
					}
				}
			}
			
			// lines
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
		if (score.mSynthType==Score::SynthType::Additive) gl::color(0,1,1);
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
	// We open a fixed number of MIDI ports.
	// If no real MIDI ports are available, we open virtual MIDI ports instead.
	const int numMIDIPortsToOpen = 4;

	// Create a temp midi out just to query the number of available ports
	RtMidiOutRef tempMidiOut = make_shared<RtMidiOut>();
	int numRealMIDIPorts = tempMidiOut->getPortCount();

	for (int portNum = 0; portNum < numMIDIPortsToOpen; portNum++) {
		RtMidiOutRef midiOut = make_shared<RtMidiOut>();
		if (portNum < numRealMIDIPorts) {
			midiOut->openPort(portNum);
		} else {
			midiOut->openVirtualPort();
		}
		mMidiOuts.push_back(midiOut);
	}
	killAllNotes();


	// Create the synth engine
	mPureDataNode = cipd::PureDataNode::Global();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/music.pd")) );
}

MusicWorld::~MusicWorld() {
	killAllNotes();
	for ( const auto &midiOut : mMidiOuts ) {
		midiOut->closePort();
	}
	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
