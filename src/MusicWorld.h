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
#include "PureDataNode.h"
#include "RtMidi.h"
#include "Instrument.h"
#include "Score.h"


class MusicWorld : public GameWorld
{
public:

	MusicWorld();
	~MusicWorld();

	void setParams( XmlTree ) override;

	string getSystemName() const override { return "MusicWorld"; }

	void update() override;
	void updateContours( const ContourVector &c ) override;
	void updateCustomVision( Pipeline& ) override; // extract bitmaps we need

	void worldBoundsPolyDidChange() override;

	void draw( GameWorld::DrawType ) override;

private:

	map<MetaParam,vec2> mLastSeenMetaParamLoc; // for better inter-frame coherence
	MetaParamInfo getMetaParamInfo( MetaParam p ) const;

	//
	vec2  mTimeVec;		// in world space, which way does time flow forward?
	int	  mNoteCount=8;
	int	  mBeatCount=32;
	Scale mScale;
	float mRootNote=60;
	float mNumOctaves=5;

	int	  mScoreNoteVisionThresh=-1; // 0..255, or -1 for OTSU
	float mScoreVisionTrimFrac=0.f;

	int   mScoreTrackRejectNumSamples=10;
	float mScoreTrackRejectSuccessThresh=.2f;
	float mScoreTrackMaxError=1.f;
	float mScoreMaxInteriorAngleDeg=120.f;
	float mScoreTrackTemporalBlendFrac=.5f; // 0 means off, so all new
	float mScoreTrackTemporalBlendIfDiffFracLT=.1f; // only do blending if frames are similar enough; otherwise: fast no blend mode.

	// new tempo system
	vector<float> mTempos; // what tempos do we support? 0 entry means free form, 1 means all are fixed.
	float mTempoWorldUnitsPerSecond = 5.f;
	float getNearestTempo( float ) const; // input -> closest mTempos[]


	map<string,InstrumentRef> mInstruments;
	vector<Scale> mScales;

	vector< pair<PolyLine2,InstrumentRef> > mInstrumentRegions;
	void generateInstrumentRegions();
	int  getScoreOctaveShift( const Score& s, const PolyLine2& wrtRegion ) const;
	InstrumentRef decideInstrumentForScore( const Score&, int* octaveShift=0 );
	float		  decideDurationForScore  ( const Score& ) const;
	InstrumentRef getInstrumentForMetaParam( MetaParam ) const;

	bool		  shouldBeMetaParamScore( const Score& ) const;
	void		  assignUnassignedMetaParams(); // we do them all at once to get uniqueness.

	void updateMetaParameter(MetaParam metaParam, float value);
	void updateScoresWithMetaParams();


	vector<Score> mScores;

	InstrumentRef	getInstrumentForScore( const Score& ) const;

	Score* matchOldScoreToNewScore( const Score& old, float maxErr, float* matchError=0 ); // can return 0 if no match; returns new score (in mScores)
	float  scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const;
	bool   doesZombieScoreIntersectZombieScores( const Score& old ); // marks other zombies if so
	bool   shouldPersistOldScore  ( const Score& old, const ContourVector &contours );
		// match failed; do we want to keep it?
		// any intersecting zombie scores are marked for removal (mDoesZombieTouchOtherZombies)

	Score* getScoreForMetaParam( MetaParam );

	// image processing helper code
	void quantizeImage( Pipeline& pipeline,
		Score& score,
		string scoreName,
		bool doTemporalBlend,
		cv::Mat oldTemporalBlendImage,
		int quantizeNumCols, int quantizeNumRows ) const;
		// logs to pipeline
		// resizes score.mImage => mQuantizedImage + mQuantizedImagePreThreshold,
		// and does temporal smoothing (optionally)
		// if you pass -1 for cols or rows then it isn't resized in that dimension



	// synthesis
	cipd::PureDataNodeRef	mPureDataNode;	// synth engine
	cipd::PatchRef			mPatch;			// music patch

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
