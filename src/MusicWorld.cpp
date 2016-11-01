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
using namespace ci::gl;

void MusicWorld::Instrument::setParams( XmlTree xml )
{
	getXml(xml,"PlayheadColor",mPlayheadColor);
	getXml(xml,"ScoreColor",mScoreColor);
	getXml(xml,"NoteOffColor",mNoteOffColor);
	getXml(xml,"NoteOnColor",mNoteOnColor);
	
	getXml(xml,"Name",mName);
	
	if ( xml.hasChild("SynthType") )
	{
		string t = xml.getChild("SynthType").getValue();
		
		map<string,SynthType> synths;
		synths["Additive"] = SynthType::Additive;
		synths["MIDI"] = SynthType::MIDI;
		synths["Meta"] = SynthType::Meta;
		auto i = synths.find(t);
		mSynthType = (i!=synths.end()) ? i->second : SynthType::MIDI ; // default to MIDI
		
		if ( mSynthType==SynthType::Meta )
		{
			string p = xml.getChild("MetaParam").getValue();
			
			map<string,MetaParam> params;
			params["Scale"] = MetaParam::Scale;
			params["RootNote"] = MetaParam::RootNote;
			params["Tempo"] = MetaParam::Tempo;
			auto i = params.find(p);
			mMetaParam = (i!=params.end()) ? i->second : MetaParam::RootNote ; // default to RootNote
		}
	}
	
	getXml(xml,"Port",mPort);
	getXml(xml,"Channel",mChannel);
	getXml(xml,"MapNotesToChannels",mMapNotesToChannels);
}

MusicWorld::Instrument::~Instrument()
{
	if (mMidiOut) mMidiOut->closePort();
}

// Setting this keeps RtMidi from throwing exceptions when something goes wrong talking to the MIDI system.
void midiErrorCallback(RtMidiError::Type type, const std::string &errorText, void *userData) {
	cout << "MIDI out error: " << type << errorText << endl;
}

void MusicWorld::Instrument::setup()
{
	assert(!mMidiOut);

	if (mSynthType == SynthType::MIDI) {
		cout << "Opening port " << mPort << " for '" << mName << "'" << endl;

		// RtMidiOut can throw an OSError if it has trouble initializing CoreMIDI.
		try {
			mMidiOut = make_shared<RtMidiOut>();
		} catch (std::exception) {
			cout << "Error creating RtMidiOut " << mPort << " for '" << mName << "'" << endl;
		}

		if (mMidiOut) {

			mMidiOut->setErrorCallback(midiErrorCallback, NULL);
			if (mPort < mMidiOut->getPortCount()) {
				mMidiOut->openPort( mPort );
			} else {
				cout << "...Opening virtual port for '" << mName << "'" << endl;
				mMidiOut->openVirtualPort(mName);
			}
		}

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

MusicWorld::MusicWorld()
{
	mLastFrameTime = ci::app::getElapsedSeconds();

	mTimeVec = vec2(0,-1);
	
	setupSynthesis();
}

void MusicWorld::setParams( XmlTree xml )
{
	killAllNotes();

	mTempos.clear();
	mScale.clear();
	
	getXml(xml,"Tempos",mTempos);
	getXml(xml,"TempoWorldUnitsPerSecond",mTempoWorldUnitsPerSecond);
	
	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount);
	getXml(xml,"BeatCount",mBeatCount);
	getXml(xml,"NumOctaves",mNumOctaves);
	getXml(xml,"RootNote",mRootNote);

	getXml(xml,"ScoreNoteVisionThresh", mScoreNoteVisionThresh);
	getXml(xml,"ScoreVisionTrimFrac", mScoreVisionTrimFrac);

	getXml(xml,"ScoreTrackRejectNumSamples",mScoreTrackRejectNumSamples);
	getXml(xml,"ScoreTrackRejectSuccessThresh",mScoreTrackRejectSuccessThresh);
	getXml(xml,"ScoreTrackMaxError",mScoreTrackMaxError);
	getXml(xml,"ScoreMaxInteriorAngleDeg",mScoreMaxInteriorAngleDeg);
	getXml(xml,"ScoreTrackTemporalBlendFrac",mScoreTrackTemporalBlendFrac);
	getXml(xml,"ScoreTrackTemporalBlendIfDiffFracLT",mScoreTrackTemporalBlendIfDiffFracLT);

	// scales
	mScales.clear();
	for( auto i = xml.begin( "Scales/Scale" ); i != xml.end(); ++i )
	{
		Scale notes;
		string name; // not used yet

		getXml(*i,"Name",name);
		getXml(*i,"Notes",notes);

		mScales.push_back(notes);
	}
	mScale = mScales[0];

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
	
	int ninstr=0;
	for( auto i : mInstruments )
	{
		if ( i.second->mSynthType!=Instrument::SynthType::Meta ) ninstr++;
	}
	
	mInstruments.size();
	
	int dim = ceil( sqrt(ninstr) );
	
	vec2 scale = worldRect.getSize() / vec2(dim,dim);
	
	int x=0, y=0;
	
	for( auto i : mInstruments )
	{
		if ( i.second->mSynthType==Instrument::SynthType::Meta ) continue;
		
		Rectf r(0,0,1,1);
		r.offset( vec2(x,y) );
		r.scale(scale);

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

int MusicWorld::getScoreOctaveShift( const Score& score, const PolyLine2& wrtRegion ) const
{
	const vec2 lookVec = perp(mTimeVec);

	pair<float,float> worldYs(0,0), scoreYs(0,0);
	
	if ( wrtRegion.size() > 0 )
	{
		worldYs = getShapeRange( &wrtRegion.getPoints()[0], wrtRegion.size(), lookVec );
	}
	
	scoreYs = getShapeRange( score.mQuad, 4, lookVec );
	
	float scorey = lerp(scoreYs.first,scoreYs.second,.5f);

	float f = 1.f - (scorey - worldYs.first) / (worldYs.second - worldYs.first);
	// don't understand why i need 1-, but it works.
	
	int o = roundf( (f-.5f) * (float)mNumOctaves ) ;
	
	if (0) cout << o << endl;
	
	
	//
	
	if (0)
	{
		auto c = []( string n, pair<float,float> p )
		{
			cout << n << ": [ " << p.first << ", " << p.second << " ]" ;
		};
		
		cout << "s " << " { " ;
		c("worldYs",worldYs);
		cout << "  ";
		c("scoreYs",scoreYs);
		cout <<" }"<<endl;
	}

	return o;
}

MusicWorld::InstrumentRef
MusicWorld::decideInstrumentForScore( const Score& s, int* octaveShift )
{
	// meta-parameter?
	vec2 scoreSize = s.getSizeInWorldSpace();
	
	if ( min(scoreSize.x,scoreSize.y) < 10.f )
	{
		InstrumentRef meta = getInstrumentForMetaParam( chooseNextMetaParam() );
		if (meta) return meta;
	}
	
	// ...else: do instrument by world region
	vec2 c = s.getPolyLine().calcCentroid();
	
	// find
	for( auto i : mInstrumentRegions )
	{
		if ( i.first.contains(c) )
		{
			if (octaveShift) *octaveShift = getScoreOctaveShift(s,i.first);
			return i.second;
		}
	}
	
	// default to first
	auto i = mInstrumentRegions.begin();
	if (i!=mInstrumentRegions.end())
	{
		if (octaveShift) *octaveShift = getScoreOctaveShift(s,getWorldBoundsPoly());
			// and do octave wrt world bounds
		return i->second;
	}
	
	// none
	return 0;
}

float
MusicWorld::getNearestTempo( float t ) const
{
	if (mTempos.empty())
	{
		// free-form
		if (1)
		{
			// quantize (de-jitter, which amplifies over time)
			const float k = 4.f;
			t *= k;
			t = roundf(t);
			t /= k;
		}
		
		return t;
	}
	if (mTempos.size()==1) return mTempos[0]; // globally fixed
	if (mTempos[0]>=t) return mTempos[0]; // minimum
	
	// find nearest in between two tempos
	for( size_t i=0; i<mTempos.size()-1; ++i )
	{
		if ( t >= mTempos[i] && t <= mTempos[i+1] )
		{
			float mid = lerp(mTempos[i],mTempos[i+1],.5f);
			
			if ( t < mid ) return mTempos[i];
			else return mTempos[i+1];
		}
	}
	
	// maximum
	return mTempos.back();
}

float
MusicWorld::decideDurationForScore ( const Score& score ) const
{
	vec2 size = score.getSizeInWorldSpace();
	
	float d = size.x / mTempoWorldUnitsPerSecond;

	float t = getNearestTempo(d);
	
	if (0) cout << size.x << "cm = " << d << "sec => " << t << "sec" << endl;
	
	return t;
}

MusicWorld::InstrumentRef
MusicWorld::getInstrumentForMetaParam( MetaParam p ) const
{
	for ( auto i : mInstruments )
	{
		if ( i.second->mSynthType == Instrument::SynthType::Meta && i.second->mMetaParam==p )
		{
			return i.second;
		}
	}
	return 0;
}

MusicWorld::Score* MusicWorld::matchOldScoreToNewScore( const Score& old, float maxErr, float* outError )
{
	Score* best   = 0;
	float  bestErr = MAXFLOAT;
	
	auto closestCorner = []( vec2 pt, const vec2 quad[4], float& err, float maxerr=MAXFLOAT ) -> int
	{
		float cornerErr=maxerr;
		int c=-1;
		
		for( int i=0; i<4; ++i )
		{
			float d = distance(quad[i],pt);
			
			if (d<cornerErr)
			{
				c=i;
				cornerErr=d;
			}
		}
		
		err=cornerErr;
		return c;
	};
	
	auto scoreError = [closestCorner]( const vec2 a[4], const vec2 b[4], float maxerr=MAXFLOAT ) -> float
	{
		float c[4] = {MAXFLOAT,MAXFLOAT,MAXFLOAT,MAXFLOAT};
		
		for( int i=0; i<4; ++i )
		{
			float err;
			
			int cc = closestCorner( a[i], b, err, maxerr );
			if (cc==-1) return MAXFLOAT;
			c[cc]=err;
		}

//		cout << "\t" << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << endl;
		
		float sumerr=0.f;
		for( int i=0; i<4; ++i )
		{
			if ( c[i] == MAXFLOAT ) return MAXFLOAT;
			else sumerr += c[i];
		}
		return sumerr;
	};
	
	for( size_t i=0; i<mScores.size(); ++i )
	{
		float err = scoreError( old.mQuad, mScores[i].mQuad, maxErr ) ;
//		cout << "err " << err << endl;
		if (err<bestErr)
		{
			bestErr=err;
			best=&mScores[i];
		}
	}
	
	if (outError) *outError=bestErr;
	return best;
}

bool
MusicWorld::doesZombieScoreIntersectZombieScores( const Score& old )
{
	bool r=false;
	
	PolyLine2 op = old.getPolyLine();
	vector<PolyLine2> opv;
	opv.push_back(op);
	
	for( auto &s : mScores )
	{
		bool touches=false;
		
		{
			PolyLine2 p = s.getPolyLine();
			vector<PolyLine2> pv;
			pv.push_back(p);

			touches = ! PolyLine2::calcIntersection(opv,pv).empty() ;
			// this is CRAZY performance wise, but it should work for now
			// TODO: do poly-poly intersection correctly
		}
		
		if ( touches )
		{
			if (s.mIsZombie) s.mDoesZombieTouchOtherZombies = true;
			r=true;
		}
	}
	
	return r;
}

bool
MusicWorld::shouldPersistOldScore ( const Score& old, const ContourVector &contours )
{
	// did we go dark?
	if ( scoreFractionInContours(old, contours, mScoreTrackRejectNumSamples) < mScoreTrackRejectSuccessThresh ) return false;
	
	// do we touch other zombies?
	if ( doesZombieScoreIntersectZombieScores(old) ) return false;
	
	return true;
}

float
MusicWorld::scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const
{
	int hit=0;
	
	for( int i=0; i<numSamples; ++i )
	{
		vec2 pt( randFloat(), randFloat() ); // [0..1,0..1] random
		
		pt = old.fracToQuad(pt); // map into score quad
		
		// see if it lands in top level contours
		for( auto &c : contours )
		{
			if ( c.mTreeDepth==0 && c.mPolyLine.contains(pt) ) hit++;
		}
	}
	
	return (float)hit / (float)numSamples;
}

MusicWorld::Score* MusicWorld::getScoreForMetaParam( MetaParam p )
{
	for( int i=0; i<mScores.size(); ++i )
	{
		InstrumentRef instr = getInstrumentForScore(mScores[i]);
		
		if ( instr && instr->mSynthType==Instrument::SynthType::Meta && instr->mMetaParam==p )
		{
			return &mScores[i];
		}
	}
	return 0;
}

MusicWorld::MetaParam MusicWorld::chooseNextMetaParam()
{
	MetaParam start = mNextMetaParam;
	
	while ( getScoreForMetaParam(mNextMetaParam) )
	{
		mNextMetaParam = (MetaParam)( ((int)mNextMetaParam + 1 ) % (int)MetaParam::kNumMetaParams );
		
		if (mNextMetaParam==start) break;
	}
	
	MetaParam result = mNextMetaParam;
	
	mNextMetaParam = (MetaParam)( ((int)mNextMetaParam + 1) % (int)MetaParam::kNumMetaParams );
	
	return result;
}

void MusicWorld::updateContours( const ContourVector &contours )
{
	// erase old scores
	vector<Score> oldScores = mScores;
	mScores.clear();
	
	// get new ones
	for( const auto &c : contours )
	{
		if ( !c.mIsHole && c.mTreeDepth==0 && c.mPolyLine.size()==4 )
		{
			Score score;
			
			// shape
			score.setQuadFromPolyLine(c.mPolyLine,mTimeVec);
			if ( score.getQuadMaxInteriorAngle() > toRadians(mScoreMaxInteriorAngleDeg) ) continue;
			
			// timing
			score.mDurationFrac = decideDurationForScore(score); // inherit, but it could be custom based on shape or something

			// instrument
			int octaveShift = 0;
			InstrumentRef instr = decideInstrumentForScore(score,&octaveShift);
			if (instr) score.mInstrumentName = instr->mName ;
			
			// Choose octave based on up<>down
			int noteRoot = mRootNote + octaveShift*12; // middle C
			score.mOctave = octaveShift;
			score.mRootNote = noteRoot;

			// Inherit scale from global scale
			score.mScale = mScale;

			// MIDI synth params
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
		float matchError;
		Score* match = matchOldScoreToNewScore(c,mScoreTrackMaxError,&matchError);
		
		if ( match )
		{
//			cout << "match" << endl;
			
			if (1) // just copy everything
			{
				*match = c;
				// ...any exceptions?
			}
			else // do it piecemeal
			{
				for( int i=0; i<4; ++i ) match->mQuad[i] = c.mQuad[i]; // de-jitter vertices
	
				// copy images forward so we can do temporal anti-aliasing and other effects
				match->mImage = c.mImage;
				match->mQuantizedImage = c.mImage;
			}
		}
		else // zombie! (c has no match)
		{
			// persist it?
			if ( shouldPersistOldScore(c,contours) ) // this flags other zombies that might need to be culled
			{
//				cout << "zombie" << endl;

				mScores.push_back(c);
				mScores.back().mIsZombie = true;
			}
//			else cout << "die" << endl;
		}
	}
	
	// 2nd pass eliminate intersecting zombie (anything flagged)
	oldScores = mScores;
	mScores.clear();
	
	for( auto &s : oldScores )
	{
		if ( !s.mDoesZombieTouchOtherZombies ) mScores.push_back(s);
	}

	updateScoresWithMetaParams();
}

void MusicWorld::updateScoresWithMetaParams() {
	// Update the scale/beat configuration in case it's changed from the xml
	// FIXME: is this the right place to do this?
	for (auto &s : mScores ) {
		s.mRootNote = mRootNote;
		s.mNoteCount = mNoteCount;
		s.mScale = mScale;
		s.mBeatCount = mBeatCount;
	}
}

static float getImgDiffFrac( cv::Mat a, cv::Mat b )
{
	if ( !b.empty() && !a.empty() && a.size == b.size )
	{
		cv::Mat diffm;
		cv::subtract( a, b, diffm);
		cv::Scalar diff = cv::sum(diffm);
		float d = (diff[0]/255.f) / (a.size[0] * a.size[1]);
		//cout << "diff: " << d << endl;
		return d;
	}
	
	return 1.f;
}

static void doTemporalMatBlend( Pipeline& pipeline, string scoreName, cv::Mat oldimg, cv::Mat newimg, float oldWeight, float diffthresh )
{
	// newimg = oldimg * oldWeight + newimg * (1 - oldWeight)

	bool verbose=false;
	
	float diff = getImgDiffFrac(oldimg,newimg);
	
	if ( diff < diffthresh )
	{
		if (verbose) cout << "blend" << endl ;
		
		if ( !oldimg.empty() && oldimg.size == newimg.size )
		{
			float newWeight = 1.f - oldWeight;
			
			cv::addWeighted( newimg, newWeight, oldimg, oldWeight, 0.f, newimg );
			pipeline.then( scoreName + " temporally blended", newimg);
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;
		}
	}
	else if (verbose) cout << "no-blend " << diff << endl;
}

void MusicWorld::quantizeImage( Pipeline& pipeline,
	MusicWorld::Score& s,
	string scoreName,
	bool doTemporalBlend,
	cv::Mat oldTemporalBlendImage,
	int quantizeNumCols, int quantizeNumRows ) const
{
	cv::Mat &inquant = s.mImage;
	
	if (quantizeNumCols<=0) quantizeNumCols = inquant.cols;
	if (quantizeNumRows<=0) quantizeNumRows = inquant.rows;
	
	cv::Mat quantized;
	cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_AREA ); // best?
		// cv::INTER_AREA is the best.
		// cv::INTER_LANCZOS4 is good
		// these are not so great: cv::INTER_CUBIC, cv::INTER_NEAREST, cv::INTER_LINEAR
	
	vec2 outsize = vec2( s.mImage.cols, s.mImage.rows );
	
	pipeline.then( scoreName + " quantized", quantized);
	pipeline.setImageToWorldTransform(
		pipeline.getStages().back()->mImageToWorld
			* glm::scale(vec3(outsize.x / (float)quantizeNumCols, outsize.y / (float)quantizeNumRows, 1))
		);
	pipeline.getStages().back()->mLayoutHintScale = .5f;
	pipeline.getStages().back()->mLayoutHintOrtho = true;

	// blend
	if ( doTemporalBlend )
	{
		doTemporalMatBlend( pipeline, scoreName, oldTemporalBlendImage, quantized,
			mScoreTrackTemporalBlendFrac, mScoreTrackTemporalBlendIfDiffFracLT );
	}
	
	// threshold
	cv::Mat &inthresh = quantized;
	cv::Mat thresholded;

	if ( mScoreNoteVisionThresh < 0 ) {
		cv::threshold( inthresh, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	} else {
		cv::threshold( inthresh, thresholded, mScoreNoteVisionThresh, 255, cv::THRESH_BINARY );
	}
	
	pipeline.then( scoreName + " thresholded", thresholded);
	pipeline.getStages().back()->mLayoutHintScale = .5f;
	pipeline.getStages().back()->mLayoutHintOrtho = true;


	// output
	s.mQuantizedImagePreThreshold = quantized;
	s.mQuantizedImage = thresholded;
}

void MusicWorld::updateCustomVision( Pipeline& pipeline )
{
	// this function grabs the bitmaps for each score
	
	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("thresholded");
	if ( !world || world->mImageCV.empty() ) return;
	
	// for each score...
	int scoreNum=1;
	
	for( Score& s : mScores )
	{
		string scoreName = string("score")+toString(scoreNum);
		const auto instr = getInstrumentForScore(s);
		
		cv::Mat oldTemporalBlendImage;
		const bool doTemporalBlend = instr && instr->mSynthType==Instrument::SynthType::MIDI && mScoreTrackTemporalBlendFrac>0.f;
		if ( doTemporalBlend ) oldTemporalBlendImage = s.mQuantizedImagePreThreshold.clone(); // must clone to work!
		
		// get src points
		vec2		srcpt[4];
		cv::Point2f srcpt_cv[4];
		{
			vec2  trimto; // average of corners
			for ( int i=0; i<4; ++i )
			{
				srcpt[i] = vec2( world->mWorldToImage * vec4(s.mQuad[i],0,1) );
				trimto += srcpt[i]*.25f;
			}
			
			// trim
			for ( int i=0; i<4; ++i ) srcpt[i] = lerp( srcpt[i], trimto, mScoreVisionTrimFrac );
			
			// to ocv
			vec2toOCV_4(srcpt, srcpt_cv);
		}
		
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
		cv::warpPerspective( world->mImageCV, s.mImage, xform, cv::Size(outsize.x,outsize.y) );

		pipeline.then( scoreName, s.mImage);
		pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(dstpt,s.mQuad) );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;
		
		// quantize
		if ( instr && instr->mSynthType==Instrument::SynthType::MIDI )
		{
			// midi
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, mBeatCount, s.mNoteCount );
		}
		else if ( instr && instr->mSynthType==Instrument::SynthType::Meta )
		{
			// meta-param
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, 1, -1 );
			
			// parse value...
			s.mMetaParamSliderValue = 0.f;
			float sumw = 0.f;
			
			for( int i=0; i<s.mQuantizedImage.rows; ++i )
			{
				float v = 1.f - (float)i / (float)(s.mQuantizedImage.rows-1);
				float w = 1.f - (float)s.mQuantizedImage.at<unsigned char>(i,0) / 255.f;
				
				sumw += w;
				s.mMetaParamSliderValue += v * w;
			}
			
			s.mMetaParamSliderValue /= sumw;
			
			if ( sumw==0.f )
			{
				// uh-oh!
				s.mMetaParamSliderValue=0.f;
			}
			else
			{
				// update
				updateMetaParameter( instr->mMetaParam, s.mMetaParamSliderValue );
			}
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

void MusicWorld::update()
{
	const float dt = getDT();

	tickGlobalClock(dt);

	for( auto &score : mScores )
	{
		score.tickPhase(mPhaseInBeats);
	}


	int synthNum=0;
	
	for( const auto &score : mScores )
	{
		auto instr = getInstrumentForScore(score);
		if (!instr) continue;

		if ( instr->mSynthType==Instrument::SynthType::Additive ) {
			// Update time
			mPureDataNode->sendFloat(toString(synthNum)+string("phase"),
									 score.getPlayheadFrac() );
			synthNum++;
		}
		// send midi notes
		else if ( instr->mSynthType==Instrument::SynthType::MIDI )
		{
			// notes
			int x = score.getPlayheadFrac() * (float)(score.mQuantizedImage.cols);

			for ( int y=0; y<score.mNoteCount; ++y )
			{
				unsigned char value = score.mQuantizedImage.at<unsigned char>(score.mNoteCount-1-y,x);
				
				if ( score.isScoreValueHigh(value) )
				{
					float duration =
					    getBeatDuration() *
					    score.mDurationFrac *
					    score.getNoteLengthAsScoreFrac(score.mQuantizedImage,x,y);
					
					if (duration>0)
					{
						int note = score.noteForY(instr, y);
						doNoteOn( instr, note, duration );
					}
				}
			}
		}
	}
	
	// retire notes
	updateNoteOffs();
}

float MusicWorld::getBeatDuration() const {
	static const float oneMinute = 60;

	const float oneBeat = oneMinute / mTempo;

	return oneBeat;
}

void MusicWorld::tickGlobalClock(float dt) {

	const float beatsPerSec = 1 / getBeatDuration();

	const float elapsedBeats = beatsPerSec * dt;

	const float newPhase = mPhaseInBeats + elapsedBeats;

	mPhaseInBeats = fmod(newPhase, mDurationInBeats);
}


float MusicWorld::getDT() {
	const float now = ci::app::getElapsedSeconds();
	const float dt = now - mLastFrameTime;
	mLastFrameTime = now;

	return dt;
}

bool MusicWorld::isNoteInFlight( InstrumentRef instr, int note ) const
{
	auto i = mOnNotes.find( tOnNoteKey(instr,note) );
	return i != mOnNotes.end();
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

void MusicWorld::killAllNotes()
{
	for ( const auto i : mInstruments )
	{
		if (i.second->mMidiOut)
		{
			for (int channel = 0; channel < 16; channel++)
			{
				for (int note = 0; note < 128; note++)
				{
					sendNoteOff( i.second->mMidiOut, channel, note );
				}
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

void MusicWorld::sendNoteOn ( RtMidiOutRef midiOut, uchar channel, uchar note, uchar velocity )
{
	const uchar noteOnBits = 9;

	uchar channelBits = channel & 0xF;

	sendMidi( midiOut, (noteOnBits<<4) | channelBits, note, velocity);
}

void MusicWorld::sendNoteOff ( RtMidiOutRef midiOut, uchar channel, uchar note )
{
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

void MusicWorld::updateMetaParameter(MetaParam metaParam, float value)
{
	switch (metaParam) {
		case MetaParam::Scale:
			mScale = mScales[ min( (int)(value * mScales.size()), (int)mScales.size()-1 ) ];
			break;
		case MetaParam::RootNote:
			// this could also be "root degree", and stay locked to the scale (or that could be separate slider)
			mRootNote = value * 12 + 48;
			break;
		case MetaParam::Tempo:
			mTempo = value * 120;
			break;
		default:
			break;
	}

	updateScoresWithMetaParams();
}

void MusicWorld::updateAdditiveScoreSynthesis()
{
	const int kMaxSynths = 8; // This corresponds to [clone 8 music-voice] in music.pd

	// Mute all synths
	for( int synthNum=0; synthNum<kMaxSynths; ++synthNum )
	{
		mPureDataNode->sendFloat(toString(synthNum)+string("volume"), 0);
	}

	// send scores to Pd
	int synthNum=0;
	
	for( const auto &score : mScores )
	{
		InstrumentRef instr = getInstrumentForScore(score);
		if (!instr) continue;
		
		// send image for additive synthesis
		if ( instr->mSynthType==Instrument::SynthType::Additive && !score.mImage.empty() )
		{

			int rows = score.mImage.rows;
			int cols = score.mImage.cols;
			
			// Update pan
//			mPureDataNode->sendFloat(toString(scoreNum)+string("pan"),
//									 score.mPan);
//
//			// Update per-score pitch
//			mPureDataNode->sendFloat(toString(scoreNum)+string("note-root"),
//									 20 );
//
//			// Update range of notes covered by additive synthesis
//			mPureDataNode->sendFloat(toString(scoreNum)+string("note-range"),
//									 100 );

			// Update resolution
			mPureDataNode->sendFloat(toString(synthNum)+string("resolution-x"),
									 cols);

			mPureDataNode->sendFloat(toString(synthNum)+string("resolution-y"),
									 rows);

			// Create a float version of the image
			cv::Mat imageFloatMat;
			
			// Copy the uchar version scaled 0-1
			score.mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
			
			// Convert to a vector to pass to Pd

			// Grab the current column at the playhead. We send updates even if the
			// phase doesn't change enough to change the colIndex,
			// as the pixels might have changed instead.
			float phase = score.getPlayheadFrac();
			int colIndex = imageFloatMat.cols * phase;

			cv::Mat columnMat = imageFloatMat.col(colIndex);

			// We use a list message rather than writeArray as it causes less contention with Pd's execution thread
			auto list = pd::List();
			for (int i = 0; i < columnMat.rows; i++) {
				list.addFloat(columnMat.at<float>(i, 0));
			}

			mPureDataNode->sendList(toString(synthNum)+string("vals"), list);

			// Turn the synth on
			mPureDataNode->sendFloat(toString(synthNum)+string("volume"), 1);

			synthNum++;
		}
	}
}

void MusicWorld::draw( GameWorld::DrawType drawType )
{
//	return;
	bool drawSolidColorBlocks = drawType == GameWorld::DrawType::Projector;
	// otherwise we can't see score contents in the UI view... 
	
	// instrument regions
	if (drawSolidColorBlocks)
	{
		for( auto &r : mInstrumentRegions )
		{
			gl::color( r.second->mScoreColor );
			gl::drawSolid( r.first ) ;
		}
	}
	
	// scores
	for( const auto &score : mScores )
	{
		score.draw(*this,drawType);
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

	// Lets us use lists to set arrays, which seems to cause less thread contention
	mPureDataNode->setMaxMessageLength(1024);

	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/music.pd")) );
}

MusicWorld::~MusicWorld() {
	cout << "~MusicWorld" << endl;

	killAllNotes();

	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
