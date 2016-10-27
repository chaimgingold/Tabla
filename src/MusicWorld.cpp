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
#include "cinder/gl/Context.h"


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
		mQuad[0] = in( bestSide+3 );
		mQuad[1] = in( bestSide+2 );
		mQuad[2] = in( bestSide+1 );
		mQuad[3] = in( bestSide+0 );
		// wtf i don't get the logic but it works
		
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
	
	line[0] = lerp(mQuad[3],mQuad[2],t); // bottom
	line[1] = lerp(mQuad[0],mQuad[1],t); // top
}

vec2 MusicWorld::Score::getSizeInWorldSpace() const
{
	vec2 size;
	
	size.x = distance( lerp(mQuad[0],mQuad[3],.5f), lerp(mQuad[1],mQuad[2],.5f) );
	size.y = distance( lerp(mQuad[0],mQuad[1],.5f), lerp(mQuad[3],mQuad[2],.5f) );
	
	return size;
}

vec2 MusicWorld::Score::fracToQuad( vec2 frac ) const
{
	vec2 top = lerp(mQuad[0],mQuad[1],frac.x);
	vec2 bot = lerp(mQuad[3],mQuad[2],frac.x);
	
	return lerp(bot,top,frac.y);
}

int MusicWorld::Score::noteForY( InstrumentRef instr, int y ) const {

	if (instr && instr->mMapNotesToChannels) {
		return y;
	}


	int numNotes = mScale.size();
	int extraOctaveShift = y / numNotes * 12;
	int degree = y % numNotes;
	int note = mScale[degree];

	return note + extraOctaveShift + mNoteRoot + mOctave*12;
}

float
MusicWorld::Score::getQuadMaxInteriorAngle() const
{
	float mang=0.f;
	
	for( int i=0; i<4; ++i )
	{
		vec2 a = mQuad[i];
		vec2 x = mQuad[(i+1)%4];
		vec2 b = mQuad[(i+2)%4];
		
		a -= x;
		b -= x;
		
		float ang = acos( dot(a,b) / (length(a)*length(b)) );
		
		if (ang>mang) mang=ang;
		
	}
	
	return mang;
}

MusicWorld::MusicWorld()
{
	mStartTime = ci::app::getElapsedSeconds();

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
	getXml(xml,"Scale",mScale);
	getXml(xml,"NumOctaves",mNumOctaves);

	getXml(xml,"ScoreNoteVisionThresh", mScoreNoteVisionThresh);
	getXml(xml,"ScoreVisionTrimFrac", mScoreVisionTrimFrac);

	getXml(xml,"ScoreRejectNumSamples",mScoreRejectNumSamples);
	getXml(xml,"ScoreRejectSuccessThresh",mScoreRejectSuccessThresh);
	getXml(xml,"ScoreTrackMaxError",mScoreTrackMaxError);
	getXml(xml,"ScoreMaxInteriorAngleDeg",mScoreMaxInteriorAngleDeg);

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
	
	int x=0, y=0;
	
	for( auto i : mInstruments )
	{
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
MusicWorld::decideInstrumentForScore( const Score& s, int* octaveShift ) const
{
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
	if ( scoreFractionInContours(old, contours, mScoreRejectNumSamples) < mScoreRejectSuccessThresh ) return false;
	
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
			score.mStartTime = mStartTime;
			score.mDuration  = decideDurationForScore(score); // inherit, but it could be custom based on shape or something

			// instrument
			int octaveShift = 0;
			InstrumentRef instr = decideInstrumentForScore(score,&octaveShift);
			if (instr) score.mInstrumentName = instr->mName ;
			
			// Choose octave based on up<>down
			int noteRoot = 60 + octaveShift*12; // middle C
			score.mOctave = octaveShift;
			score.mNoteRoot = noteRoot;

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
			
			// de-jitter by copying old vertices forward
			for( int i=0; i<4; ++i ) match->mQuad[i] = c.mQuad[i];
		}
		else
		{
			// zombie! c has no match...
			
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
	
	// 2nd eliminate intersecting zombie pass (anything flagged)
	oldScores = mScores;
	mScores.clear();
	
	for( auto &s : oldScores )
	{
		if ( !s.mDoesZombieTouchOtherZombies ) mScores.push_back(s);
	}
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

		// quantization params
		int quantizeNumRows = s.mNoteCount;
		int quantizeNumCols = mBeatCount;
		
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
		float outdim = 0.f;
		for( int i=0; i<4; ++i ) outdim = max( outdim, distance(srcpt[i],srcpt[(i+1)%4]) );

		vec2		outsize		= vec2(1,1) * outdim;
		vec2		dstpt[4]    = { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };

		cv::Point2f dstpt_cv[4];
		vec2toOCV_4(dstpt, dstpt_cv);
		
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
			
			if (quantizeNumCols<=0) quantizeNumCols = inquant.cols;
			if (quantizeNumRows<=0) quantizeNumRows = inquant.rows;
			
			cv::Mat quantized;
//			cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_NEAREST );
//			cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_LINEAR );
//			cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_CUBIC );
//			cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_LANCZOS4 ); // good
			cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_AREA ); // best?
			pipeline.then( scoreName + "quantized", quantized);
			pipeline.setImageToWorldTransform(
				pipeline.getStages().back()->mImageToWorld
					* glm::scale(vec3(outsize.x / (float)quantizeNumCols, outsize.y / (float)quantizeNumRows, 1))
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
			&& isScoreValueHigh(image.at<unsigned char>(image.rows-1-y,x2)) ;
		 ++x2 )
	{}

	int len = x2-x;
	
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
			int x = score.getPlayheadFrac() * (float)(score.mQuantizedImage.cols-1) ;

			for ( int y=0; y<score.mNoteCount; ++y )
			{
				unsigned char value = score.mQuantizedImage.at<unsigned char>(score.mNoteCount-1-y,x) ;
				
				if ( isScoreValueHigh(value) )
				{
					float duration = score.mDuration * getNoteLengthAsScoreFrac(score.mQuantizedImage,x,y);
					
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

void MusicWorld::updateAdditiveScoreSynthesis() {


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
		if (!instr) {
			continue;
		}
		
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
			std::vector<float> imageVector;
			imageVector.assign( (float*)imageFloatMat.datastart, (float*)imageFloatMat.dataend );

			mPureDataNode->writeArray(toString(synthNum)+string("image"), imageVector);

			mPureDataNode->sendFloat(toString(synthNum)+string("volume"), 1);
			
			synthNum++;
		}
	}
}

void drawLines( vector<vec2> points )
{
	const int dims = 2;
	const int size = sizeof( vec2 ) * points.size();
	auto ctx = context();
	const GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg ) {
//		CI_LOG_E( "No GLSL program bound" );
		return;
	}

	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();

	VboRef arrayVbo = ctx->getDefaultArrayVbo( size );
	ScopedBuffer bufferBindScp( arrayVbo );

	arrayVbo->bufferSubData( 0, size, points.data() );
	int posLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, dims, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)nullptr );
	}
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_LINES, 0, points.size() );
	ctx->popVao();
}

void drawSolidTriangles( vector<vec2> pts )
{
	auto ctx = context();
	const GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg ) {
//		CI_LOG_E( "No GLSL program bound" );
		return;
	}

//	GLfloat data[3*2+3*2]; // both verts and texCoords
//	memcpy( data, pts, sizeof(float) * pts.size() * 2 );
//	if( texCoord )
//		memcpy( data + 3 * 2, texCoord, sizeof(float) * 3 * 2 );

	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();
	VboRef defaultVbo = ctx->getDefaultArrayVbo( sizeof(float)*pts.size()*2 );
	ScopedBuffer bufferBindScp( defaultVbo );
	defaultVbo->bufferSubData( 0, sizeof(float) * pts.size() * 2, &pts[0] );

	int posLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::POSITION );
	if( posLoc >= 0 ) {
		enableVertexAttribArray( posLoc );
		vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}
//	if( texCoord ) {
//		int texLoc = curGlslProg->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
//		if( texLoc >= 0 ) {
//			enableVertexAttribArray( texLoc );
//			vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*6) );
//		}
//	}
	ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->drawArrays( GL_TRIANGLES, 0, pts.size() );
	ctx->popVao();
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
		InstrumentRef instr = getInstrumentForScore(score);
		if (!instr) continue;

		if (drawSolidColorBlocks)
		{
			gl::color(0,0,0);
			gl::drawSolid(score.getPolyLine());
		}
		
		if ( instr->mSynthType==Instrument::SynthType::Additive )
		{
			// additive
			gl::color(instr->mScoreColor);
			gl::draw( score.getPolyLine() );
		}
		else if ( instr->mSynthType==Instrument::SynthType::MIDI )
		{
			// midi
			
			// collect notes into a batch
			// (probably wise to extract this geometry/data when processing vision data,
			// then use it for both playback and drawing).
			const float invcols = 1.f / (float)(score.mQuantizedImage.cols-1);

			const float yheight = 1.f / (float)score.mNoteCount;
			
			vector<vec2> onNoteTris;
			vector<vec2> offNoteTris;
			
			const float playheadFrac = score.getPlayheadFrac();
			
			for ( int y=0; y<score.mNoteCount; ++y )
			{
				const float y1frac = y * yheight + yheight*.2f;
				const float y2frac = y * yheight + yheight*.8f;
				
				for( int x=0; x<score.mQuantizedImage.cols; ++x )
				{
					if ( isScoreValueHigh(score.mQuantizedImage.at<unsigned char>(score.mNoteCount-1-y,x)) )
					{
						// how wide?
						int length = getNoteLengthAsImageCols(score.mQuantizedImage,x,y);
						
						const float x1frac = x * invcols;
						const float x2frac = min( 1.f, (x+length) * invcols );
						
						vec2 start1 = score.fracToQuad( vec2( x1frac, y1frac) ) ;
						vec2 end1   = score.fracToQuad( vec2( x2frac, y1frac) ) ;

						vec2 start2 = score.fracToQuad( vec2( x1frac, y2frac) ) ;
						vec2 end2   = score.fracToQuad( vec2( x2frac, y2frac) ) ;
						
						const bool isInFlight = playheadFrac > x1frac && playheadFrac < x2frac;
						
						vector<vec2>& tris = isInFlight ? onNoteTris : offNoteTris ;
						
						tris.push_back( start1 );
						tris.push_back( start2 );
						tris.push_back( end2 );
						tris.push_back( start1 );
						tris.push_back( end2 );
						tris.push_back( end1 );
						
						// skip ahead
						x = x + length+1;
					}
				}
			}
			
			// draw batched notes
			if (!onNoteTris.empty())
			{
				gl::color(instr->mNoteOnColor);
				drawSolidTriangles(onNoteTris);
			}
			if (!offNoteTris.empty())
			{
				gl::color(instr->mNoteOffColor);
				drawSolidTriangles(offNoteTris);
			}
			
			// lines
			{
				// TODO: Make this configurable for 5/4 time, etc.
				const int drawBeatDivision = 4;

				vector<vec2> pts;
				
				gl::color(instr->mScoreColor);
				gl::draw( score.getPolyLine() );

				// Scale lines
				for( int i=0; i<score.mNoteCount; ++i )
				{
					float f = (float)(i+1) / (float)score.mNoteCount;
					
					pts.push_back( lerp(score.mQuad[3], score.mQuad[0],f) );
					pts.push_back( lerp(score.mQuad[2], score.mQuad[1],f) );
				}

				// Off-beat lines
				for( int i=0; i<score.mBeatCount; ++i )
				{
					if (i % drawBeatDivision == 3) continue;

					float f = (float)(i+1) / (float)score.mBeatCount;

					pts.push_back( lerp(score.mQuad[1], score.mQuad[0],f) );
					pts.push_back( lerp(score.mQuad[2], score.mQuad[3],f) );
				}
				
				drawLines(pts);

				// New points for new colors
				pts.clear();
				// Down-beat lines
				for( int i=0; i<score.mBeatCount; ++i )
				{
					// FIXME: why do we need to compare i % drawBeatDivision != 3 with 3 to hit the downbeat? (ditto above)
					if (i % drawBeatDivision != 3) continue;

					float f = (float)(i+1) / (float)score.mBeatCount;

					pts.push_back( lerp(score.mQuad[1], score.mQuad[0],f) );
					pts.push_back( lerp(score.mQuad[2], score.mQuad[3],f) );
				}
				vec3 scoreColorHSV = rgbToHsv(instr->mScoreColor);
				scoreColorHSV.x = fmod(scoreColorHSV.x + 0.5, 1.0);
				gl::color( hsvToRgb(scoreColorHSV)  );
				drawLines(pts);
			}


		}
		
		//
		if (1)
		{
			gl::color( instr->mPlayheadColor );
			
			vec2 playhead[2];
			score.getPlayheadLine(playhead);
			gl::drawLine( playhead[0], playhead[1] );
		
			// octave indicator
			float octaveFrac = (float)score.mOctave / (float)mNumOctaves + .5f ;
			
			gl::drawSolidCircle( lerp(playhead[0],playhead[1],octaveFrac), .5f, 6 );
		}

		// quad debug
		if (0)
		{
			gl::color( 1,0,0 );
			gl::drawSolidCircle(score.mQuad[0], 1);
			gl::color( 0,1,0 );
			gl::drawSolidCircle(score.mQuad[1], 1);
		}
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