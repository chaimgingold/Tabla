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

class MusicWorld : public GameWorld
{
public:

	MusicWorld();
	~MusicWorld();

	void setParams( XmlTree ) override;

	string getSystemName() const override { return "MusicWorld"; }

	void update() override;
	void updateVision( const ContourVector &c, Pipeline& ) override;

	void worldBoundsPolyDidChange() override;

	void draw( GameWorld::DrawType ) override;

	void cleanup() override;

private:

	//
	vec2  mTimeVec;		// in world space, which way does time flow forward?
	int	  mNoteCount=8;
	int	  mBeatCount=32;
	Scale mScale;
	float mRootNote=60;
	float mNumOctaves=5;


	// new tempo system
	vector<float> mTempos; // what tempos do we support? 0 entry means free form, 1 means all are fixed.

	map<string,InstrumentRef> mInstruments;
	vector<Scale> mScales;

	MetaParamInfo getMetaParamInfo( MetaParam ) const;
	void updateMetaParameter(MetaParam metaParam, float value);
	void updateScoresWithMetaParams();


	vector<Score> mScores;

//	InstrumentRef	getInstrumentForScore( const Score& ) const;
	Score*			getScoreForMetaParam( MetaParam );

	// vision
	MusicVision mVision;

	// synthesis
	PureDataNodeRef	mPureDataNode;	// synth engine
	PatchRef			mPatch;			// music patch

	void setupSynthesis();
	void updateAdditiveScoreSynthesis();

	void killAllNotes(); // sends a note off for all MIDI notes (0-127)

	// DT calculation
	float mLastFrameTime;
	float getDT();

	// Global clock
	float mPhaseInBeats=0;
	float mDurationInBeats=16;
	float mTempo=120;
	void tickGlobalClock(float dt);
	float getBeatDuration() const;

	// Shaders
	FileWatch mFileWatch; // hotload our shaders; in this class so when class dies all the callbacks expire.

	gl::GlslProgRef mAdditiveShader;
};

class MusicWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "MusicWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<MusicWorld>();
	}
};

#endif /* MusicWorld_hpp */
