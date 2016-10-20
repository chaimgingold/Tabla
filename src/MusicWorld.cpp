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

void MusicWorld::Instrument::setParams( XmlTree xml )
{
	getXml(xml,"PlayheadColor",mPlayheadColor);
	getXml(xml,"ScoreColor",mScoreColor);
	getXml(xml,"NoteOffColor",mNoteOffColor);
	getXml(xml,"NoteOnColor",mNoteOnColor);
	
	getXml(xml,"Name",mName);
	getXml(xml,"Scale",mScale);
//	getXml(xml,"IsAdditiveSynth",mIsAdditiveSynth);
	
	if ( xml.hasChild("SynthType") )
	{
		string t = xml.getChild("SynthType").getValue();
		
		if (t=="Additive") mSynthType=SynthType::Additive;
		else mSynthType=SynthType::MIDI;
	}
	
	getXml(xml,"Port",mPort);
	getXml(xml,"Channel",mChannel);
	getXml(xml,"MapNotesToChannels",mMapNotesToChannels);
}

MusicWorld::Instrument::~Instrument()
{
	if (mMidiOut) mMidiOut->closePort();
}

void MusicWorld::Instrument::setup()
{
	assert(!mMidiOut);
	
	cout << "Opening port " << mPort << " for '" << mName << "'" << endl;

	mMidiOut = make_shared<RtMidiOut>();

	if (mPort < mMidiOut->getPortCount()) {
		mMidiOut->openPort( mPort );
	} else {
		cout << "...Opening virtual port for '" << mName << "'" << endl;
		mMidiOut->openVirtualPort(mName);
	}
}

// For most synths this is just the assigned channel,
// but in the case of the Volca Sample we want to
// send each note to a different channel as per its MIDI implementation.
int MusicWorld::Instrument::channelForNote(int note) {
	if (mMapNotesToChannels > 0) {
		return note % mMapNotesToChannels;
	}
	return mChannel;
}

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
	killAllNotes();

	getXml(xml,"Tempo",mTempo);
	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount);
	getXml(xml,"BeatCount",mBeatCount);
	getXml(xml,"ScoreNoteVisionThresh", mScoreNoteVisionThresh);
	
	cout << "Notes " << mNoteCount << endl;
	
	// instruments
	cout << "Instruments:" << endl;
	
	map<string,InstrumentRef> newInstr;
	
	for( auto i = xml.begin( "Instruments/Instrument" ); i != xml.end(); ++i )
	{
		Instrument instr;
		instr.setParams(*i);
		
		cout << "\t" << instr.mName << endl;
		
		newInstr[instr.mName] = std::make_shared<Instrument>(instr);
	}
	
	// bind new instruments
	auto oldInstr = mInstruments;
	mInstruments.clear();

	mInstruments = newInstr;
	for( auto i : mInstruments ) i.second->setup();
	
	// TODO: diff them smartly.
//	for( auto i : newInstr )
//	{
//		
//	}

	// instr regions
	generateInstrumentRegions();
}

void MusicWorld::worldBoundsPolyDidChange()
{
	generateInstrumentRegions();
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

void MusicWorld::generateInstrumentRegions()
{
	mInstrumentRegions.clear();
	
	Rectf worldRect( getWorldBoundsPoly().getPoints() );
	
	int ninstr = mInstruments.size();
	
	int dim = ceil( sqrt(ninstr) );
	
	vec2 scale = worldRect.getSize() / vec2(dim,dim);
//	vec2 scale = worldRect.getSize() / vec2(dim,dim);
	
	int x=0, y=0;
	
	for( auto i : mInstruments )
	{
		Rectf r(0,0,1,1);
		r.offset( vec2(x,y) );
		r.scale(scale);
		
		cout << r << " " << i.second->mName << endl;
		
		PolyLine2 p;
		p.push_back( r.getUpperLeft() );
		p.push_back( r.getUpperRight() );
		p.push_back( r.getLowerRight() );
		p.push_back( r.getLowerLeft() );
		p.setClosed();
		
		mInstrumentRegions.push_back( pair<PolyLine2,InstrumentRef>(p,i.second) );
		
		//
		x++;
		if (x>=dim)
		{
			x=0;
			y++;
		}
	}
}

MusicWorld::InstrumentRef
MusicWorld::decideInstrumentForScore( const Score& s ) const
{
	vec2 c = s.getPolyLine().calcCentroid();
	
	// find
	for( auto i : mInstrumentRegions )
	{
		if ( i.first.contains(c) ) return i.second;
	}
	
	// default to first
	auto i = mInstrumentRegions.begin();
	if (i!=mInstrumentRegions.end()) return i->second;
	
	// none
	return 0;
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
	vector<Score> oldScores = mScores;
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

			// instrument
			InstrumentRef instr = decideInstrumentForScore(score);
			
			if (instr) score.mInstrumentName = instr->mName ;
			
			/*
			// use place in world to determine some factors...
			pair<float,float> ys = getShapeRange( score.mQuad, 4, lookVec );
//			pair<float,float> xs = getShapeRange( score.mQuad, 4, mTimeVec );
			const float yf = 1.f - (ys.first - worldYs.first) / (worldYs.second-worldYs.first);

			// FIXME: fix this scaling to get an even distribution of instruments across the X axis
			const float xf = score.getPolyLine().calcCentroid().y / (worldXs.second-worldXs.first);

			// synth type
			const float xAdditiveZone = 0.5;
			if ( yf > xAdditiveZone ) score.mInstrumentName = "Additive";
			else score.mInstrumentName = "Volca Sample";
				// TODO: make and use a spatial grid for each instrument.
			*/
			
			// Choose octave based on up<>down
//			int octaveShift = (yf - .5f) * 10.f;
			int octaveShift = 0;
			int noteRoot = 60 + octaveShift*12; // middle C
			score.mNoteRoot = noteRoot;

			// MIDI synth params
//			auto instr = getInstrumentForScore(score);
//			assert(instr);
			
			if ( instr && instr->mSynthType==Instrument::SynthType::MIDI )
			{
				score.mNoteCount = mNoteCount;
				score.mBeatCount = mBeatCount;
			}

			score.mPan		= .5f ;
			
			mScores.push_back(score);
		}
	}
	
	// map old <=> new scores
	for ( const auto &c : oldScores )
	{
		// do polygon similarity test...
		// Hard: test distance of old mQuad to all new mQuads.
		//		each corner to each corner..., to mitigate possible ordering issues
		//		cumulative error < thresh
		// Soft: test perimeter similarity of old mQuad to new contours
		//		perhaps each old corner to nearest point on new contours (same one)
		//		even more: sample a bunch of old perimeter points and test distance to new polygons.
		// Reject: we also need a hard rejection, a way to quickly eliminate something that has moved.
		//		something like: no overlapping scores allowed. and if they overlap, then kill non-zombies.
		//		do this at the very end? if any zombie quads intersect non-zombie quads, then kill zombie.
		
		// Checking zombie against not-zombie could actually be about mScoreDetectError
		// new scores have an error of zero
		// older scores are >0
		// this could be basis for zombie rejection
		
		// Q: When, if at all, do we update quads with updated shapes? (only if we have a 4 corner to 4 corner match, so error is quite small)
		
		// cases:
		// 1: [ Hard] found a match. optionally propagate forward any data.
		// 2: [!Hard] not found
		//
		//	2a: [!Soft] || [Reject] (hard reject) cull <probably a zombie timeout or some other factor>
		//			eg [Reject] zombie quad intersects non-zombie quad
		//			eg timeout (?)
		//			eg [Reject] no nearby soft second pass polygon test to match to
		//  2b: [Soft]  keep zombie around, propagate forward into mScore and mark as missing.
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
		auto instr = getInstrumentForScore(s);
		
		if ( instr && instr->mSynthType==Instrument::SynthType::MIDI )
		{
			// quantize
			cv::Mat &inquant = s.mImage;
			
			const int numRows = s.mNoteCount;
			int numCols = inquant.cols;
			if (mBeatCount > 0) numCols = mBeatCount;

			cv::Mat quantized;
			cv::resize( inquant, quantized, cv::Size(numCols,numRows) );
			pipeline.then( scoreName + "quantized", quantized);
			pipeline.setImageToWorldTransform(
				pipeline.getStages().back()->mImageToWorld
					* glm::scale(vec3(outsize.x / (float)numCols, outsize.y / (float)numRows, 1))
				);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;

			// threshold
			cv::Mat &inthresh = quantized;
			cv::Mat thresholded;

			if ( mScoreNoteVisionThresh < 0 )
			{
				cv::threshold( inthresh, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
			}
			else
			{
				cv::threshold( inthresh, thresholded, mScoreNoteVisionThresh, 255, cv::THRESH_BINARY );
			}
			
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

MusicWorld::InstrumentRef MusicWorld::getInstrumentForScore( const Score& score ) const
{
	auto i = mInstruments.find(score.mInstrumentName);
	
	if ( i == mInstruments.end() )
	{
		// fail!
		i = mInstruments.begin();
		if (i != mInstruments.end()) return i->second;
		else return 0;
	}
	else return i->second;
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
		auto instr = getInstrumentForScore(score);
		if (!instr) continue;

		if ( instr->mSynthType==Instrument::SynthType::Additive ) {
			// Update time
			mPureDataNode->sendFloat(string("phase")+toString(scoreNum),
									 score.getPlayheadFrac()*100.0 );
		}
		// send midi notes
		else if ( instr->mSynthType==Instrument::SynthType::MIDI )
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
						doNoteOn( instr, score.mNoteRoot+y, duration );
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

bool MusicWorld::isNoteInFlight( InstrumentRef instr, int note ) const
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
			InstrumentRef instr = it->first.mInstrument;

			uchar channel = instr->channelForNote(it->first.mNote);

			sendNoteOff( instr->mMidiOut, channel, it->first.mNote);
		}
		
		if (off) mOnNotes.erase(it++);
		else ++it;
	}
}

void MusicWorld::killAllNotes() {
	for ( const auto i : mInstruments ) {
		for (int channel = 0; channel < 16; channel++) {
			for (int note = 0; note < 128; note++) {
				sendNoteOff( i.second->mMidiOut, channel, note );
			}
		}
	}
}

void MusicWorld::doNoteOn( InstrumentRef instr, int note, float duration )
{
	if ( !isNoteInFlight(instr,note) && instr->mMidiOut )
	{
		uchar velocity = 100; // 0-127

		uchar channel = instr->channelForNote(note);

		sendNoteOn( instr->mMidiOut, channel, note, velocity );
		
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
		InstrumentRef instr = getInstrumentForScore(score);
		if (!instr) continue;
		
		// send image for additive synthesis
		if ( instr->mSynthType==Instrument::SynthType::Additive && !score.mImage.empty() )
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
	// instrument regions
	for( auto &r : mInstrumentRegions )
	{
		gl::color( r.second->mScoreColor );
		gl::drawSolid( r.first ) ;
	}
	
	// scores
	for( const auto &score : mScores )
	{
		InstrumentRef instr = getInstrumentForScore(score);
		if (!instr) continue;

		gl::color(0,0,0);
		gl::drawSolid(score.getPolyLine());
		
		if ( instr->mSynthType==Instrument::SynthType::Additive )
		{
			// additive
			gl::color(instr->mScoreColor);
			gl::draw( score.getPolyLine() );
		}
		else if ( instr->mSynthType==Instrument::SynthType::MIDI )
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
						
						if ( isNoteInFlight(instr, score.mNoteRoot+y) ) gl::color(instr->mNoteOnColor);
						else gl::color(instr->mNoteOffColor);

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
			gl::color(instr->mScoreColor);
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
		gl::color( instr->mPlayheadColor );
		
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
	killAllNotes();

	// Create the synth engine
	mPureDataNode = cipd::PureDataNode::Global();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/music.pd")) );
}

MusicWorld::~MusicWorld() {
	cout << "~MusicWorld" << endl;

	killAllNotes();

	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
