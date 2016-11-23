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
#include "cinder/CinderMath.h"
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

	mMeasureCounts.clear();
	mScale.clear();

	getXml(xml,"TimeVec",mTimeVec);
	getXml(xml,"NoteCount",mNoteCount);
	getXml(xml,"RootNote",mRootNote);
	getXml(xml,"NumOctaves",mNumOctaves);
	getXml(xml,"PokieRobitPulseTime",mPokieRobitPulseTime);
	getXml(xml,"MaxTempo",mMaxTempo);
	
	getXml(xml,"BeatsPerMeasure",mBeatsPerMeasure);
	getXml(xml,"BeatQuantization",mBeatQuantization);
	
	getXml(xml,"MeasureCounts",mMeasureCounts);

	if ( xml.hasChild("MusicVision") )
	{
		mVision.setParams(xml.getChild("MusicVision"));
	}
	
	getXml(xml,"StampIconWidth",mStampIconWidth);
	getXml(xml,"StampPaletteGutter",mStampPaletteGutter);
	
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
		instr.mPureDataNode = mPureDataNode;
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
		auto i = mInstruments.find( s.mInstrumentName );
		if (i==mInstruments.end()) s.mInstrument=0; // nil it!
		else s.mInstrument = i->second; // rebind
	}
	
	// update vision
	mVision.mTimeVec = mTimeVec;
	mVision.mMeasureCounts  = mMeasureCounts;
	mVision.mInstruments = mInstruments;
	mVision.mNoteCount = mNoteCount;
	mVision.mBeatsPerMeasure = mBeatsPerMeasure;
	mVision.mBeatQuantization = mBeatQuantization;
	
	// update stamps
	setupStamps();

	// kill notes
	killAllNotes();
}

void MusicWorld::setupStamps()
{
	mStamps.clear();
	
	// get info about shape of world
	vec2 c = getWorldBoundsPoly().calcCentroid();
	float minr = MAXFLOAT, maxr = 0.f;
	
	for( auto p : getWorldBoundsPoly() )
	{
		float d = distance(c,p);
		minr = min( minr, d );
		maxr = max( maxr, d );
	}
	
	// compute layout info
	float stampDist = mStampPaletteGutter + mStampIconWidth ;
	float circum = stampDist * (float) mInstruments.size();
	float r = circum / (2.f * M_PI); // 2πr
	r = min(r,minr-10.f); // what r to put them at?
	// this could be based on the number of stamps we want in the ring
	
	// make stamps
	float angle=0.f;
	float anglestep = M_PI*2.f / (float)mInstruments.size() ;
	
	for( auto i : mInstruments )
	{
		MusicStamp s;
		
		s.mXAxis = mTimeVec;
		s.mLoc = c + vec2(glm::rotate( vec3(r,0,0), angle, vec3(0,0,1) ));
		s.mIconWidth = mStampIconWidth;
		s.mInstrument = i.second;
		
		angle += anglestep;
		
		mStamps.push_back(s);
	}
}

MusicStamp*
MusicWorld::getStampByInstrument( InstrumentRef instr )
{
	for( auto &i : mStamps )
	{
		if (i.mInstrument==instr) return &i;
	}
	return 0;
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
			
			fs::path path = PaperBounce3App::get()->hotloadableAssetPath( fs::path("music-icons") / fileName );
			
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
	setupStamps();
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

	tickStamps();
	
	// file watch
	mFileWatch.scanFiles();
}

const Score* MusicWorld::pickScore( vec2 p ) const
{
	for( auto &s : mScores )
	{
		if (s.getPolyLine().contains(p)) return &s;
	}
	return 0;
}

void MusicWorld::tickStamps()
{
	// Forget stamps' scores
	for( auto &stamp : mStamps ) stamp.mHasScore = false;

	// Note scores => stamps
	for( const auto &score : mScores )
	{
		MusicStamp* stamp = getStampByInstrument( score.mInstrument );

		if (stamp)
		{
			stamp->mXAxis = score.getPlayheadVec();
			stamp->mHasScore = true;
			stamp->mLoc = lerp( stamp->mLoc, score.getCentroid(), .5f );
			stamp->mIconPoseTarget = score.getIconPoseFromScore(score.getPlayheadFrac());
			
			stamp->mIconPoseTarget += tIconAnimState::getSwayForScore(score.getPlayheadFrac()); // blend in sway
		}
	}

	set<int> contoursUsed;
	
	for( auto &stamp : mStamps )
	{
		// filter
		if (stamp.mHasScore) continue;

		// idle sway animation
		stamp.mXAxis = mTimeVec;
		stamp.mIconPoseTarget = tIconAnimState() + tIconAnimState::getIdleSway(mPhase, getBeatDuration());
		if (stamp.mInstrument) stamp.mIconPoseTarget.mColor = stamp.mInstrument->mNoteOffColor;
		
		// move towards an intersecting contour?
		const Contour* contains = mContours.findLeafContourContainingPoint(stamp.mLoc);
		
		if ( contains && !contains->mIsHole && contains->mPolyLine.contains(contains->mCenter) )
		{
			// ensure:
			// 1. the centroid is still in the polygon!, and
			// 2. it isn't in a score
			// 3. we aren't reusing an ocv contour >1x
			
			if ( !pickScore(contains->mCenter) && contoursUsed.find(contains->mOcvContourIndex) == contoursUsed.end() )
			{
				stamp.mLoc = lerp( stamp.mLoc, contains->mCenter, .5f );
				contoursUsed.insert(contains->mOcvContourIndex);
			}
		}
	}
	
	// Tick stamps
	for( auto &stamp : mStamps ) stamp.tick();
}

void MusicWorld::updateVision( const ContourVector &c, Pipeline &p )
{
	mContours = c;
	mScores = mVision.updateVision(c,p,mScores,mStamps);
	
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
				mPureDataNode->sendList("global-scale", scaleDegrees);
				break;
			}
			case MetaParam::RootNote:
			{
				// this could also be "root degree", and stay locked to the scale (or that could be separate slider)
				mRootNote = (int)(value * 12) + 48;

				mPureDataNode->sendFloat("global-root-note", mRootNote);
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

void MusicWorld::updateAdditiveScoreSynthesis()
{
	const int kMaxSynths = 8; // This corresponds to [clone 8 music-voice] in music.pd

	// Mute all additive synths, in case their score has disappeared (FIXME: do this in ~Score() ?)
	for( int additiveSynthID=0; additiveSynthID<kMaxSynths; ++additiveSynthID )
	{
		mPureDataNode->sendFloat(toString(additiveSynthID)+string("volume"), 0);
	}

	// send scores to Pd
	for( auto &score : mScores )
	{
		score.updateAdditiveSynthesis();
	}
}

void MusicWorld::draw( GameWorld::DrawType drawType )
{
	// scores, stamps
	for( const auto &s : mScores ) s.draw(drawType);
	for( const auto &s : mStamps ) s.draw();

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
	mPureDataNode = PureDataNode::Global();

	// Lets us use lists to set arrays, which seems to cause less thread contention
	mPureDataNode->setMaxMessageLength(1024);

	auto reloadPdLambda = [this]( fs::path path ){
		// Ignore the passed-in path, we only want to reload the root patch
		auto rootPatch = PaperBounce3App::get()->hotloadableAssetPath("synths/music.pd");
		mPureDataNode->closePatch(mPatch);
		mPatch = mPureDataNode->loadPatch( DataSourcePath::create(rootPatch) );
	};

	// Register file-watchers for all the major pd patch components
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music.pd"),       reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-image.pd"), reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-grain.pd"), reloadPdLambda);
	mFileWatch.load( PaperBounce3App::get()->hotloadableAssetPath("synths/music-osc.pd"),   reloadPdLambda);
}

void MusicWorld::cleanup() {
	killAllNotes();

	// Clean up synth engine
	mPureDataNode->closePatch(mPatch);
}

MusicWorld::~MusicWorld() {
	cout << "~MusicWorld" << endl;

	cleanup();
}

