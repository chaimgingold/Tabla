//
//  MusicWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "TablaApp.h" // for hotloadable file paths
#include "MusicWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/CinderMath.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "xml.h"
#include "Pipeline.h"
#include "ocv.h"
#include "RtMidi.h"

using namespace std::chrono;
using namespace ci::gl;

static GameCartridgeSimple sCartridge("MusicWorld", [](){
	return std::make_shared<MusicWorld>();
});

MusicWorld::MusicWorld()
{
	// TODO: refactor so easy to load more
	mFileWatch.loadShader(
		TablaApp::get()->hotloadableAssetPath( fs::path("shaders") / "additive.vert" ),
		TablaApp::get()->hotloadableAssetPath( fs::path("shaders") / "additive.frag" ),
		[this](gl::GlslProgRef prog)
	{
		mAdditiveShader = prog; // allows null, so we can easily see if we broke it
	});

	mFileWatch.loadShader(
		TablaApp::get()->hotloadableAssetPath( fs::path("shaders") / "rainbow.vert" ),
		TablaApp::get()->hotloadableAssetPath( fs::path("shaders") / "rainbow.frag" ),
		[this](gl::GlslProgRef prog)
	{
		mRainbowShader = prog; // allows null, so we can easily see if we broke it
		
		// update stamps
		for( auto &s : mStamps ) s.mRainbowShader = mRainbowShader;
	});

	//
	mLastFrameTime = ci::app::getElapsedSeconds();

	mTimeVec = vec2(0,-1);

	setupSynthesis();	
}

void MusicWorld::setParams( XmlTree xml )
{
	killAllNotes();

	mMeasureCounts.clear();
	mScale.clear();

	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount);
	getXml(xml,"RootNote",mRootNote);
	getXml(xml,"NumOctaves",mNumOctaves);
	getXml(xml,"PokieRobitPulseTime",mPokieRobitPulseTime);
	
	getXml(xml,"MaxTempo",mMaxTempo);
	getXml(xml,"DefaultTempo",mDefaultTempo);
	mTempo=mDefaultTempo;
	
	getXml(xml,"BeatsPerMeasure",mBeatsPerMeasure);
	getXml(xml,"BeatQuantization",mBeatQuantization);
	
	getXml(xml,"MeasureCounts",mMeasureCounts);

	if ( xml.hasChild("MusicVision") )
	{
		mVision.setParams(xml.getChild("MusicVision"));
	}
	
	if ( xml.hasChild("Stamps") )
	{
		mStamps.setParams(xml.getChild("Stamps"));
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
	mInstruments.clear();
	
	const bool kVerbose = true;
	if (kVerbose) cout << "Instruments:" << endl;
	
	int nextAdditiveSynthID = 0;
	for( auto i = xml.begin( "Instruments/Instrument" ); i != xml.end(); ++i )
	{
		Instrument instr;
		instr.setParams(*i);

		if (instr.mSynthType==Instrument::SynthType::Meta)
		{
			instr.mMetaParamInfo = getMetaParamInfo(instr.mMetaParam);
		}
		else if (instr.mSynthType == Instrument::SynthType::Additive)
		{
			instr.mAdditiveSynthID = nextAdditiveSynthID;
			nextAdditiveSynthID++;
		}
		instr.mPd = mPd;
		instr.mOpenSFZ = mOpenSFZ;
		instr.mPokieRobitPulseTime = mPokieRobitPulseTime;
		
		// store it
		mInstruments[instr.mName] = std::make_shared<Instrument>(instr);

		// log it
		if (kVerbose) cout << "\t" << instr.mName << endl;
	}

	// set them up
	for( auto i : mInstruments ) i.second->setup();
	loadInstrumentIcons();

	// rebind scores to instruments
	for( auto &s : mScores )
	{
		InstrumentRefs rebound; 
		
		for ( auto instrument : s.mInstruments )
		{
			auto i = mInstruments.find( instrument->mName );
			if (i==mInstruments.end()) /*instrument=0*/; // don't copy it over; forget it
			else rebound.push_back(i->second); // rebind
		}
		
		s.mInstruments = rebound;
	}
	
	// update vision
	mVision.mTimeVec = mTimeVec;
	mVision.mMeasureCounts  = mMeasureCounts;
	mVision.mInstruments = mInstruments;
	mVision.mNoteCount = mNoteCount;
	mVision.mBeatsPerMeasure = mBeatsPerMeasure;
	mVision.mBeatQuantization = mBeatQuantization;
	
	// update stamps
	mStamps.setup( mInstruments, getWorldBoundsPoly(), mTimeVec, mRainbowShader );

	// kill notes
	killAllNotes();
}

void
MusicWorld::loadInstrumentIcons()
{
	// Note: This code is a little weird to properly support hot-loading. Bear with me.
	// 1. We need mInstruments to be fully in place
	// 2. All file reload callbacks search mInstruments for the place to put the icon (needs 1).
	//	This is to ensure we don't force any deleted instruments to stick around by holding a shared pointer to them.

	// load icons
	for( auto &i : mInstruments )
	{
		if (!i.second) continue; // !!
		Instrument& instr = *i.second;
		
		// load icon
		if ( !instr.mIconFileName.empty() )
		{
			string fileName = instr.mIconFileName;
			
			fs::path path = TablaApp::get()->hotloadableAssetPath( fs::path("music-icons") / fileName );
			
			mFileWatch.load( path, [this,fileName]( fs::path path )
			{
				// load the texture
				gl::Texture::Format format;
				format.loadTopDown(false);
				format.mipmap(true);
				gl::TextureRef icon = Texture2d::create( loadImage(path), format );

				// Find the instrument...
				for( auto &j : mInstruments )
				{
					if ( j.second->mIconFileName == fileName )
					{
						if (icon) j.second->mIcon = icon;
						else cout << "Failed to load icon '" << fileName << "'" << endl;
					}
				}
			});
		}
	}

}

void MusicWorld::worldBoundsPolyDidChange()
{
	mVision.mWorldBoundsPoly = getWorldBoundsPoly();
	mStamps.setup( mInstruments, getWorldBoundsPoly(), mTimeVec, mRainbowShader );
}

Score* MusicWorld::getScoreForMetaParam( MetaParam p )
{
	for( int i=0; i<mScores.size(); ++i )
	{
		for ( auto instr : mScores[i].mInstruments )
		{
			if ( instr->mSynthType==Instrument::SynthType::Meta && instr->mMetaParam==p )
			{
				return &mScores[i];
			}
		}
	}
	return 0;
}

void MusicWorld::updateMetaParamsWithDefaultsMaybe()
{
	// if we no longer have a score for a meta param,
	// make it default value again.
	
	for( auto i : mInstruments )
	{
		if ( i.second->mSynthType==Instrument::SynthType::Meta )
		{
			Score* s = mScores.getScoreForInstrument(i.second);
			
			if (!s)
			{
				auto info = i.second->mMetaParamInfo;
				updateMetaParameter( i.second->mMetaParam, info.mDefaultValue );
			}
		}
	}
}

void MusicWorld::updateScoresWithMetaParams() {
	// Update the scale/beat configuration in case it's changed from the xml
	for (auto &s : mScores ) {
		s.mRootNote = mRootNote;
		s.mNoteCount = mNoteCount;
		s.mScale = mScale;
		s.mBeatsPerMeasure = mBeatsPerMeasure;
		s.mBeatQuantization = mBeatQuantization;
        s.mNumOctaves = mNumOctaves;
		s.mOctave = roundf( (s.mOctaveFrac-.5f) * (float)mNumOctaves ) ;
	}
}

void MusicWorld::update()
{
	// Advance time
	const float dt = getDT();
	tickGlobalClock(dt);

	// Advance each score
	for( auto &score : mScores ) score.tick(mPhase, getBeatDuration());

	// Update Instruments (with their scores)
	updateSynthesis();

	//
	mStamps.tick(mScores,mContours,mPhase,getBeatDuration());
	
	// file watch
	mFileWatch.scanFiles();
}

void MusicWorld::updateVision( const Vision::Output& visionOut, Pipeline &p )
{
	mContours = visionOut.mContours;
	mTokens = visionOut.mTokens;
	mStamps.updateWithTokens(mTokens);
	mScores = mVision.updateVision(visionOut,p,mScores,mStamps);
	
	updateMetaParamsWithDefaultsMaybe();
	updateScoresWithMetaParams();
	updateSynthesisWithVision();
	updateMetaParamsWithVision();
	
	// push data to scores
	for( Score &s : mScores )
	{
		if ( s.mInstruments.hasSynthType(Instrument::SynthType::Additive) )
		{
			s.mAdditiveShader = mAdditiveShader;
			break;
		}
	}
}

float MusicWorld::getBeatDuration() const {
	static const float oneMinute = 60;

	const float oneBeat = oneMinute / mTempo;

	return oneBeat;
}

void MusicWorld::tickGlobalClock(float dt) {

	const float beatsPerSec = 1 / getBeatDuration();

	const float elapsedBeats = beatsPerSec * dt;

	mPhase += elapsedBeats;
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

	switch(p)
	{
		case MetaParam::Scale:
			info.mNumDiscreteStates = mScales.size();
			info.mDefaultValue = 0.f; // this just works for now, not tuned. assuming 0 is it.
			break;
			
		case MetaParam::RootNote:
			info.mNumDiscreteStates = 12;
			info.mDefaultValue = 0.f; // this just works for now, not tuned. assuming 0 is it.
			break;

		case MetaParam::Tempo:
			info.mNumDiscreteStates = -1;
			info.mDefaultValue = (float)mDefaultTempo / (float)mMaxTempo;
			break;
			
		default:
			break;
	}

	return info;
}

void MusicWorld::updateMetaParameter(MetaParam metaParam, float value)
{
	if ( value < 0 || value > 1.f )
	{
		if (0) cout << "updateMetaParameter: param is out of bounds (" << value << ")" << endl;
		// ignore it, as it's probably just an empty, unitialized value/slider.
	}
	else
	{
		switch (metaParam) {
			case MetaParam::Scale:
			{
				mScale = mScales[ constrain( (int)(value * mScales.size()), 0, (int)mScales.size()-1 ) ];

				auto scaleDegrees = pd::List();
				for (int i = 0; i < mScale.size(); i++) {
					scaleDegrees.addFloat(mScale[i]);
				}
				mPd->sendList("global-scale", scaleDegrees);
				break;
			}
			case MetaParam::RootNote:
			{
				// this could also be "root degree", and stay locked to the scale (or that could be separate slider)
				mRootNote = (int)(value * 12) + 48;

				mPd->sendFloat("global-root-note", mRootNote);
				break;
			}
			case MetaParam::Tempo:
			{
				mTempo = value * mMaxTempo;
				assert(mTempo>=0.f);
				break;
			}
			default:
				break;
		}

		updateScoresWithMetaParams();
	}
}

void
MusicWorld::updateSynthesis()
{
	InstrumentsToScores i2s = getInstrumentsToScores();

	for( auto instrument : i2s )
	{
		instrument.first->updateSynthesis(instrument.second);
	}	
}

void
MusicWorld::updateSynthesisWithVision()
{
	InstrumentsToScores i2s = getInstrumentsToScores();
	
	for( auto instrument : i2s )
	{
		instrument.first->updateSynthesisWithVision(instrument.second);
	}
}

void MusicWorld::updateMetaParamsWithVision()
{
	for( const Score &s : mScores )
	{
		// This is a little weird... But if somehow multiple metaparams have gotten
		// on there, then let's just do them all. And draw will draw them all.
		// That way if we mess up somewhere and bind>1 to a score at least it will
		// carry on robustly.
		// If we switch to using getInstrumentsToScores() then we can detect >1 per param easily.
		for( auto i : s.mInstruments )
		{
			if ( i->mSynthType==Instrument::SynthType::Meta )
			{
				updateMetaParameter( i->mMetaParam, s.getMetaParamSliderValue(i) );
			}
		}
	}
}

MusicWorld::InstrumentsToScores
MusicWorld::getInstrumentsToScores() const
{
	InstrumentsToScores i2s;
	
	// make sure we have an entry for each instrument
	for( auto i : mInstruments )
	{
		i2s[i.second] = vector<Score const*>();
	}
	
	// index scores by instrument
	for( const auto &s : mScores )
	{
		for( const auto i : s.mInstruments )
		{
			i2s[i].push_back(&s);
		}
	}
	
	return i2s;
}

void MusicWorld::draw( GameWorld::DrawType drawType )
{
	if (1)
	{
		for( const auto &t : mTokens )
		{
			if (mStamps.areTokensEnabled()) gl::color(0,1,1,.6);
			else gl::color(1,1,0);
			gl::drawSolid(t.getPoly());

			gl::color(1,1,0);
			gl::draw(t.getPoly());
		}
	}

	// scores, stamps
	for( const auto &s : mScores ) s.draw(drawType);
	mStamps.draw();

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
	mPd = TablaApp::get()->getPd();
	mOpenSFZ = TablaApp::get()->getOpenSFZ();

	cout << "loading patch..." << endl;

	auto app = TablaApp::get();
	std::vector<fs::path> paths =
	{
		app->hotloadableAssetPath("synths/MusicWorld/music-world.pd"),
		app->hotloadableAssetPath("synths/MusicWorld/music-image.pd"),
		app->hotloadableAssetPath("synths/MusicWorld/music-grain.pd"),
		app->hotloadableAssetPath("synths/MusicWorld/music-osc.pd")
	};

	// Register file-watchers for all the major pd patch components
	mFileWatch.load( paths, [this,app]()
	{
		// Reload the root patch
		auto rootPatch = app->hotloadableAssetPath("synths/MusicWorld/music-world.pd");
		mPd->closePatch(mPatch);
		mPatch = mPd->loadPatch( DataSourcePath::create(rootPatch) ).get();
	});

	cout << "done loading patch." << endl;
}

void MusicWorld::cleanup() {
	killAllNotes();

	// Clean up synth engine
	mPd->closePatch(mPatch);
	mPatch.reset();
}

MusicWorld::~MusicWorld() {
	cout << "~MusicWorld" << endl;

	cleanup();
}