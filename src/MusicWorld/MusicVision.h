//
//  MusicVision.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/15/16.
//
//

#ifndef MusicVision_h
#define MusicVision_h

#include "cinder/Xml.h"
#include "Score.h"
#include "Contour.h"

class MusicVision
{
public:

	typedef vector<Score> ScoreVector;
	
	// tuning
	void setParams( XmlTree );

	vec2 mTimeVec;
	vector<float> mTempos; // what tempos do we support? 0 entry means free form, 1 means all are fixed.
//	vector<Scale> mScales;
	map<string,InstrumentRef> mInstruments;
	int	  mNoteCount;
	int	  mBeatCount;

	// do it
	ScoreVector updateVision( const ContourVector &c, Pipeline&, const ScoreVector& oldScores ) const;
	
	// mutable state information ... to be redesigned with stamp/icon/token system
	typedef vector< pair<PolyLine2,InstrumentRef> > tInstrRegions;
	
	void generateInstrumentRegions(
		map<string,InstrumentRef> &instruments,
		const PolyLine2& worldBounds ); // TODO: kill/replace with stamp system

	tInstrRegions getInstrRegions() const { return mInstrumentRegions; }
	
	map<MetaParam,vec2> mLastSeenMetaParamLoc;
	// for better inter-frame coherence
	// set by MusicWorld, to keep updateVision purely functional
	
private:

	MetaParamInfo getMetaParamInfo( MetaParam p ) const;

	// params
	int	  mScoreNoteVisionThresh=-1; // 0..255, or -1 for OTSU
	float mScoreVisionTrimFrac=0.f;

	int   mScoreTrackRejectNumSamples=10;
	float mScoreTrackRejectSuccessThresh=.2f;
	float mScoreTrackMaxError=1.f;
	float mScoreMaxInteriorAngleDeg=120.f;
	float mScoreTrackTemporalBlendFrac=.5f; // 0 means off, so all new
	float mScoreTrackTemporalBlendIfDiffFracLT=.1f; // only do blending if frames are similar enough; otherwise: fast no blend mode.

	float mTempoWorldUnitsPerSecond = 5.f;

	// helpers
	ScoreVector getScores( const ContourVector&, const ScoreVector& oldScores ) const;
	ScoreVector getScoresFromContours( const ContourVector& ) const;
	ScoreVector mergeOldAndNewScores(
		const ScoreVector& oldScores,
		const ScoreVector& newScores,
		const ContourVector& contours ) const;
	void updateScoresWithImageData( Pipeline& pipeline, ScoreVector& scores ) const;
	
	float getNearestTempo( float ) const; // input -> closest mTempos[]
	
	//
	float		  decideDurationForScore  ( const Score& ) const;

	tInstrRegions mInstrumentRegions;
	int  getScoreOctaveShift( const Score& s, const PolyLine2& wrtRegion ) const;
	InstrumentRef decideInstrumentForScore( const Score&, int* octaveShift=0 ) const;
	InstrumentRef getInstrumentForMetaParam( MetaParam ) const;

	void		  assignUnassignedMetaParams( ScoreVector& ) const;
	// we do them all at once to get uniqueness.
	// TODO: kill/replace with stamp system

	//
	Score* findScoreMatch(
		const Score& needle,
		ScoreVector& hay,
		float maxErr,
		float* matchError=0 ) const; // can return 0 if no match; returns new score (in mScores)
	
	float  scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const;
	bool   doesZombieScoreIntersectZombieScores( const Score& old, ScoreVector& scores ) const; // marks other zombies if so
	bool   shouldPersistOldScore  ( const Score& old, ScoreVector& scores, const ContourVector &contours ) const;
		// match failed; do we want to keep it?
		// any intersecting zombie scores are marked for removal (mDoesZombieTouchOtherZombies)
	bool	shouldBeMetaParamScore( const Score& ) const;

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
	
};

#endif /* MusicVision_hpp */
