//
//  MusicWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#ifndef MusicWorld_hpp
#define MusicWorld_hpp

#include "GameWorld.h"
#include "FileWatch.h"

#include "RtMidi.h"
#include "Instrument.h"
#include "Score.h"
#include "MusicVision.h"
#include "MusicStamp.h"

class MusicWorld : public GameWorld
{
public:

	MusicWorld();
	~MusicWorld();

	void setParams( XmlTree ) override;

	string getSystemName() const override { return "MusicWorld"; }

	void update() override;
	virtual void updateVision( const Vision::Output&, Pipeline& ) override;

	void worldBoundsPolyDidChange() override;

	void draw( GameWorld::DrawType ) override;

	void cleanup() override;

private:

	//
	vec2  mTimeVec;		// in world space, which way does time flow forward?
	int	  mNoteCount=8;
	Scale mScale;
	float mRootNote=60;
	float mNumOctaves=5;
	float mPokieRobitPulseTime;
	float mMaxTempo=160;

	int	  mBeatsPerMeasure=4;
	int   mBeatQuantization=4;

	vector<float> mMeasureCounts; // what tempos do we support? 0 entries means free form, 1 entry means all are fixed.
	
	map<string,InstrumentRef> mInstruments;
	vector<Scale> mScales;
	ScoreVec	  mScores;
	MusicStampVec mStamps;
	
	// meta params
	MetaParamInfo getMetaParamInfo( MetaParam ) const;
	void updateMetaParamsWithDefaultsMaybe();
	void updateMetaParameter(MetaParam metaParam, float value);
	void updateScoresWithMetaParams();
	Score* getScoreForMetaParam( MetaParam ); // returns 1st

	// vision
	MusicVision mVision;
	ContourVector mContours;
	TokenMatchVec mTokens;
	
	// synthesis
	PureDataNodeRef	mPd;	// synth engine
	PatchRef		mPatch;			// music patch

	void setupSynthesis();

	void killAllNotes(); // sends a note off for all MIDI notes (0-127)

	void updateSynthesis(); // every tick
	void updateSynthesisWithVision(); // when vision changed
	void updateMetaParamsWithVision();
	
	typedef map<InstrumentRef,vector<Score const*>> InstrumentsToScores;
	InstrumentsToScores getInstrumentsToScores() const;
	
	// DT calculation
	float mLastFrameTime;
	float getDT();

	// Global clock
	float mPhase=0;
	float mDefaultTempo=120;
	float mTempo=120;
	void  tickGlobalClock(float dt);
	float getBeatDuration() const;

	//
	FileWatch mFileWatch; // hotload our shaders; in this class so when class dies all the callbacks expire.
	void loadInstrumentIcons();

	// Shaders
	gl::GlslProgRef mAdditiveShader;
	gl::GlslProgRef mRainbowShader;
};

#endif /* MusicWorld_hpp */
