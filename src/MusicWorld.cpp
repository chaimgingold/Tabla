//
//  MusicWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "PaperBounce3App.h" // for hotloadable file paths
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



MusicWorld::MusicWorld()
{
	mFileWatch.loadShader(
		PaperBounce3App::get()->hotloadableAssetPath("additive.vert"),
		PaperBounce3App::get()->hotloadableAssetPath("additive.frag"),
		[this](gl::GlslProgRef prog)
	{
		mAdditiveShader = prog; // allows null, so we can easily see if we broke it
	});

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

	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount);
	getXml(xml,"BeatCount",mBeatCount);
	getXml(xml,"NumOctaves",mNumOctaves);
	getXml(xml,"RootNote",mRootNote);

	if ( xml.hasChild("MusicVision") )
	{
		mVision.setParams(xml.getChild("MusicVision"));
	}
	
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
	const bool kVerbose = true;
	
	if (kVerbose) cout << "Instruments:" << endl;

	map<string,InstrumentRef> newInstr;

	for( auto i = xml.begin( "Instruments/Instrument" ); i != xml.end(); ++i )
	{
		Instrument instr;
		instr.setParams(*i);

		if (instr.mSynthType==Instrument::SynthType::Meta)
		{
			instr.mMetaParamInfo = getMetaParamInfo(instr.mMetaParam);
		}

		newInstr[instr.mName] = std::make_shared<Instrument>(instr);

		if (kVerbose) cout << "\t" << instr.mName << endl;
	}

	// bind new instruments
	auto oldInstr = mInstruments;
	mInstruments.clear();

	mInstruments = newInstr;
	for( auto i : mInstruments ) i.second->setup();

	// rebind scores to instruments
	for( auto s : mScores )
	{
		auto i = mInstruments.find( s.mInstrumentName );
		if (i==mInstruments.end()) s.mInstrument=0; // nil it!
		else s.mInstrument = i->second;
	}

	// update vision
	mVision.mTimeVec = mTimeVec;
	mVision.mTempos  = mTempos;
//	mVision.mScales  = mScales;
	mVision.mInstruments = mInstruments;
	mVision.mNoteCount = mNoteCount;
	mVision.mBeatCount = mBeatCount;
	mVision.generateInstrumentRegions(mInstruments,getWorldBoundsPoly());
}

void MusicWorld::worldBoundsPolyDidChange()
{
	mVision.generateInstrumentRegions(mInstruments,getWorldBoundsPoly());
}

Score* MusicWorld::getScoreForMetaParam( MetaParam p )
{
	for( int i=0; i<mScores.size(); ++i )
	{
		InstrumentRef instr = mScores[i].mInstrument; // getInstrumentForScore(mScores[i]);

		if ( instr && instr->mSynthType==Instrument::SynthType::Meta && instr->mMetaParam==p )
		{
			return &mScores[i];
		}
	}
	return 0;
}

void MusicWorld::updateScoresWithMetaParams() {
	// Update the scale/beat configuration in case it's changed from the xml
	for (auto &s : mScores ) {
		s.mRootNote = mRootNote;
		s.mNoteCount = mNoteCount;
		s.mScale = mScale;
		s.mBeatCount = mBeatCount;
        s.mNumOctaves = mNumOctaves;
		s.mOctave = roundf( (s.mOctaveFrac-.5f) * (float)mNumOctaves ) ;
	}
}


void MusicWorld::update()
{
	const float dt = getDT();

	tickGlobalClock(dt);

	for( auto &score : mScores )
	{
		score.tickPhase(mPhaseInBeats);
	}


	int additiveSynthNum=0;

	for( const auto &score : mScores )
	{
		auto instr = score.mInstrument; // getInstrumentForScore(score);
		if (!instr) continue;

		switch( instr->mSynthType )
		{
			// Additive
			case Instrument::SynthType::Additive:
			{
				// Update time
				mPureDataNode->sendFloat(toString(additiveSynthNum)+string("phase"),
										 score.getPlayheadFrac() );
				additiveSynthNum++;
			}
			break;

			// Notes
			case Instrument::SynthType::MIDI:
			case Instrument::SynthType::Striker:
			{
				// send midi notes
				if (!score.mQuantizedImage.empty())
				{
					int x = score.getPlayheadFrac() * (float)(score.mQuantizedImage.cols);

					for ( int y=0; y<score.mNoteCount; ++y )
					{
						unsigned char value = score.mQuantizedImage.at<unsigned char>(score.mNoteCount-1-y,x);

						int note = score.noteForY(y);

						if ( score.isScoreValueHigh(value) )
						{
							float duration =
								getBeatDuration() *
								score.mDurationFrac *
								score.getNoteLengthAsScoreFrac(score.mQuantizedImage,x,y);

							if (duration>0)
							{
								instr->doNoteOn( note, duration );
							}
						}
						// See if the note was previously triggered but no longer exists, and turn it off if so
						else if (instr->isNoteInFlight( note ))
						{
							// TODO: this should work as long a there isn't >1 score per instrument. In that case,
							// this will start to behave weirdly. Proper solution is to scan all scores and
							// aggregate all the on notes, and then join the list of desired on notes to actual on notes
							// in a single pass, taking action to on/off them as needed.
							instr->doNoteOff( note );
						}
					}
				}

				instr->tickArpeggiator();
			}
			break;

			// Meta
			case Instrument::SynthType::Meta:
			{
				mVision.mLastSeenMetaParamLoc[instr->mMetaParam] = score.getCentroid();
			}
			break;
		}

        // retire notes
        instr->updateNoteOffs();
	}



	// file watch
	mFileWatch.scanFiles();
}

void MusicWorld::updateVision( const ContourVector &c, Pipeline &p )
{
	mScores = mVision.updateVision(c,p,mScores);
	
	updateScoresWithMetaParams();
	updateAdditiveScoreSynthesis(); // update additive synths based on new image data
	
	// fill out some data...
	for( Score &s : mScores )
	{
		if (s.mInstrument)
		{
			switch(s.mInstrument->mSynthType)
			{
				case Instrument::SynthType::Additive:
				{
					s.mAdditiveShader = mAdditiveShader;
					break;
				}
				
				case Instrument::SynthType::Meta:
				{
					updateMetaParameter( s.mInstrument->mMetaParam, s.mMetaParamSliderValue );
					break;
				}
				
				default:break;
			} // switch
		} // if
	} // loop
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

void MusicWorld::killAllNotes()
{
	for ( const auto i : mInstruments )
	{
        i.second->killAllNotes();
	}
}

MetaParamInfo
MusicWorld::getMetaParamInfo( MetaParam p ) const
{
	MetaParamInfo info;

	if ( p==MetaParam::Scale )
	{
		info.mNumDiscreteStates = mScales.size();
	}
	else if ( p==MetaParam::RootNote )
	{
		info.mNumDiscreteStates = 12;
	}

	return info;
}

void MusicWorld::updateMetaParameter(MetaParam metaParam, float value)
{
	if ( value < 0 || value > 1.f )
	{
		cout << "updateMetaParameter: param is out of bounds (" << value << ")" << endl;
	}
	
	switch (metaParam) {
		case MetaParam::Scale:
			mScale = mScales[ max( 0, min( (int)(value * mScales.size()), (int)mScales.size()-1 ) ) ];
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
	int additiveSynthID=0;

	for( const auto &score : mScores )
	{
		InstrumentRef instr = score.mInstrument;
		if (!instr) continue;

		// send image for additive synthesis
		if ( instr->mSynthType==Instrument::SynthType::Additive && !score.mImage.empty() )
		{

			int rows = score.mImage.rows;
			int cols = score.mImage.cols;

			// Update pan
			mPureDataNode->sendFloat(toString(additiveSynthID)+string("pan"),
									 score.mPan);

			// Update per-score pitch
			mPureDataNode->sendFloat(toString(additiveSynthID)+string("note-root"),
									 20 );

			// Update range of notes covered by additive synthesis
			mPureDataNode->sendFloat(toString(additiveSynthID)+string("note-range"),
									 100 );

			// Update resolution
			mPureDataNode->sendFloat(toString(additiveSynthID)+string("resolution-x"),
									 cols);

			mPureDataNode->sendFloat(toString(additiveSynthID)+string("resolution-y"),
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

			mPureDataNode->sendList(toString(additiveSynthID)+string("vals"), list);

			// Turn the synth on
			mPureDataNode->sendFloat(toString(additiveSynthID)+string("volume"), 1);

			additiveSynthID++;
		}
	}
}

void MusicWorld::draw( GameWorld::DrawType drawType )
{
//	return;
	bool drawSolidColorBlocks = drawType == GameWorld::DrawType::Projector;
	// otherwise we can't see score contents in the UI view...

	// instrument regions
	if (0&&drawSolidColorBlocks)
	{
		for( auto &r : mVision.getInstrRegions() )
		{
			gl::color( r.second->mScoreColor );
			gl::drawSolid( r.first ) ;
		}
	}

	// scores
	for( const auto &score : mScores )
	{
		score.draw(drawType);
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

	auto reloadPdLambda = [this]( fs::path path ){
		// Ignore the passed-in path, we only want to reload the root patch
		auto rootPatch = PaperBounce3App::get()->hotloadableAssetPath("synths/music.pd");
		mPureDataNode->closePatch(mPatch);
		mPatch = mPureDataNode->loadPatch( DataSourcePath::create(rootPatch) );
	};

	// Register file-watchers for all the major pd patch components
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music.pd"), reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-image.pd"), reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-grain.pd"), reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-osc.pd"), reloadPdLambda);
}

MusicWorld::~MusicWorld() {
	cout << "~MusicWorld" << endl;

	killAllNotes();

	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}
