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
#include "MusicStamp.h"
#include "RectFinder.h"

class MusicVision
{
public:

	// tuning
	void setParams( XmlTree );

	vec2 mTimeVec;
	vector<float> mMeasureCounts; // what tempos do we support? 0 entry means free form, 1 means all are fixed.
	map<string,InstrumentRef> mInstruments;
	int	  mNoteCount;
	
	int	  mBeatsPerMeasure;
	int	  mBeatQuantization;
	
	PolyLine2 mWorldBoundsPoly;

	// do it
	ScoreVec updateVision( const Vision::Output&, Pipeline&, const ScoreVec& oldScores, MusicStampVec& ) const;
	
private:

	MetaParamInfo getMetaParamInfo( MetaParam p ) const;

	// params
	int	  mScoreNoteVisionThresh=-1; // 0..255, or -1 for OTSU

	int   mScoreTrackRejectNumSamples=10;
	float mScoreTrackRejectSuccessThresh=.2f;
	float mScoreTrackMaxError=1.f;
	float mScoreTrackTemporalBlendFrac=.5f; // 0 means off, so all new
	float mScoreTrackTemporalBlendIfDiffFracLT=.1f; // only do blending if frames are similar enough; otherwise: fast no blend mode.
	
	float mMaxTokenScoreDistance=5.f;
	
	float mWorldUnitsPerMeasure = 5.f;
	int  mBlankEdgePixels=0;


	// helpers
	ScoreVec getScores(
		const ContourVector&,
		const ScoreVec& oldScores,
		MusicStampVec& stamps,
		const TokenMatchVec& ) const;
	
	ScoreVec getScoresFromContours( const ContourVector&, MusicStampVec& stamps, const TokenMatchVec& ) const;
	
	ScoreVec mergeOldAndNewScores(
		const ScoreVec& oldScores,
		const ScoreVec& newScores,
		const ContourVector& contours,
		bool isUsingTokens ) const;
	
	void updateScoresWithImageData( Pipeline& pipeline, ScoreVec& scores ) const;
	
	float getNearestTempo( float ) const; // input -> closest mMeasureCounts[]
	
	RectFinder mRectFinder;
	
	//
	float		  decideMeasureCountForScore( const Score& ) const;
	InstrumentRef decideInstrumentForScore( const Score&, MusicStampVec&, const TokenMatchVec& tokens ) const;

	float getScoreOctaveShift( const Score& s, const PolyLine2& wrtRegion ) const;

	// find matching scores
	// can return 0 if no match; returns new score (in hay)
	// - findScoreMatch: (original) looks for permuted matches (a rotated quad with same verts will match)
	// - findExactScoreMatch: (new) no permuted matches allowed (must have same orientation)
	Score* findScoreMatch(
		const Score& needle,
		ScoreVec& hay,
		float maxErr,
		float* matchError=0 ) const;

	Score* findExactScoreMatch(
		const Score& needle,
		ScoreVec& hay,
		float maxErr,
		float* matchError=0 ) const;
	
	float  scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const;
	bool   doesZombieScoreIntersectZombieScores( const Score& old, ScoreVec& scores ) const; // marks other zombies if so
	bool   shouldPersistOldScore  ( const Score& old, ScoreVec& scores, const ContourVector &contours ) const;
		// match failed; do we want to keep it?
		// any intersecting zombie scores are marked for removal (mDoesZombieTouchOtherZombies)
	bool	shouldBeMetaParamScore( const Score& ) const;

	// image processing helper code
	void quantizeImage( Pipeline& pipeline,
		Score& score,
		string scoreName,
		bool doTemporalBlend,
		cv::UMat oldTemporalBlendImage,
		int quantizeNumCols, int quantizeNumRows ) const;
		// logs to pipeline
		// resizes score.mImage => mQuantizedImage + mQuantizedImagePreThreshold,
		// and does temporal smoothing (optionally)
		// if you pass -1 for cols or rows then it isn't resized in that dimension
	
	static float getSliderValueFromQuantizedImageData( const Score& );
};

#endif /* MusicVision_hpp */
