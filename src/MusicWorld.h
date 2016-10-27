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
#include "PureDataNode.h"
#include "RtMidi.h"

typedef std::shared_ptr<RtMidiOut> RtMidiOutRef;

typedef vector<int> Scale;

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
	
	class Instrument;
	class Score;
	
	// params
	float mStartTime;	// when MusicWorld created
	vec2  mTimeVec;		// in world space, which way does time flow forward?
	int	  mNoteCount=8;
	int	  mBeatCount=32;
	Scale mScale;
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
	
	// instrument info
	class Instrument
	{
	public:

		~Instrument();
		
		void setParams( XmlTree );
		void setup();

		int channelForNote(int note);
		
		// colors!
		ColorA mPlayheadColor;
		ColorA mScoreColor;
		ColorA mNoteOffColor, mNoteOnColor;
		
		//
		string mName;

		//
		enum class SynthType
		{
			Additive = 1,
			MIDI	 = 2
		};
		
		SynthType mSynthType;
		
		// midi
		int  mPort=0;
		int  mChannel=0;
		int  mMapNotesToChannels=0; // for KORG Volca Sample
		RtMidiOutRef mMidiOut;
		
	};
	typedef std::shared_ptr<Instrument> InstrumentRef;
	
	map<string,InstrumentRef> mInstruments;
	
	vector< pair<PolyLine2,InstrumentRef> > mInstrumentRegions;
	void generateInstrumentRegions();
	int  getScoreOctaveShift( const Score& s, const PolyLine2& wrtRegion ) const;
	InstrumentRef decideInstrumentForScore( const Score&, int* octaveShift=0 ) const;
	float		  decideDurationForScore  ( const Score& ) const;
	
	// scores
	class Score
	{
	public:
		
		// shape
		vec2  mQuad[4];
		/*  Vertices are played back like so:
		
			0---1
			|   |
			3---2
			
			 --> time		*/

		// detection meta-data
		bool mIsZombie=false;
		bool mDoesZombieTouchOtherZombies=false;
		
		// image data
		cv::Mat		mImage;				// thresholded image
		cv::Mat		mQuantizedImagePreThreshold; // (for inter-frame smoothing)
		cv::Mat		mQuantizedImage;	// quantized image data for midi playback
		
		// synth parameters
		string		mInstrumentName;
		
		float		mStartTime;
		float		mDuration;
		
		int			mOctave;
		float		mNoteRoot;
		int			mNoteCount;
		int         mBeatCount;
		int			mNoteInstrument;
		
		float		mPan;
		//float		mVolume;
		
		// constructing shape
		bool		setQuadFromPolyLine( PolyLine2, vec2 timeVec );
		
		// getting stuff
		PolyLine2	getPolyLine() const;
		float		getPlayheadFrac() const;
		void		getPlayheadLine( vec2 line[2] ) const; // line goes from [3..2] >> [0..1]
		vec2		getSizeInWorldSpace() const;
			// if not a perfect rect, then it's approximate (measures two bisecting axes)
			// x is axis along which playhead moves; y is perp to it (parallel to playhead)
		vec2		fracToQuad( vec2 frac ) const; // frac.x = time[0,1], frac.y = note_space[0,1]
		float		getQuadMaxInteriorAngle() const; // looking for concave-ish shapes...
		
		Scale mScale;
		int noteForY( InstrumentRef instr, int y ) const;
	};
	vector<Score> mScores;
	
	InstrumentRef	getInstrumentForScore( const Score& ) const;
	
	Score* matchOldScoreToNewScore( const Score& old, float maxErr, float* matchError=0 ); // can return 0 if no match; returns new score (in mScores)
	float  scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const;
	bool   doesZombieScoreIntersectZombieScores( const Score& old ); // marks other zombies if so
	bool   shouldPersistOldScore  ( const Score& old, const ContourVector &contours );
		// match failed; do we want to keep it?
		// any intersecting zombie scores are marked for removal (mDoesZombieTouchOtherZombies)
	

	// midi note playing and management
	bool  isScoreValueHigh( uchar ) const;
	float getNoteLengthAsScoreFrac( cv::Mat image, int x, int y ) const;
	int   getNoteLengthAsImageCols( cv::Mat image, int x, int y ) const;
	
	struct tOnNoteKey
	{
		tOnNoteKey();
		tOnNoteKey( InstrumentRef i, int n ) : mInstrument(i), mNote(n){}
		
		InstrumentRef mInstrument;
		int mNote;
		
	};

	struct cmpOnNoteKey {
		bool operator()(const tOnNoteKey& a, const tOnNoteKey& b) const {
			return pair<InstrumentRef,int>(a.mInstrument,a.mNote) > pair<InstrumentRef,int>(b.mInstrument,b.mNote);
		}
	};

	struct tOnNoteInfo
	{
		float mStartTime;
		float mDuration;
	};
	
	map< tOnNoteKey, tOnNoteInfo, cmpOnNoteKey > mOnNotes ;
		// (instrument,note) -> (start time, duration)
	
	bool isNoteInFlight( InstrumentRef instr, int note ) const;
	void updateNoteOffs();
	void doNoteOn( InstrumentRef instr, int note, float duration ); // start time is now



	// midi convenience methods
	void sendMidi( RtMidiOutRef, uchar, uchar, uchar );
	void sendNoteOn ( RtMidiOutRef midiOut, uchar channel, uchar note, uchar velocity );
	void sendNoteOff ( RtMidiOutRef midiOut, uchar channel, uchar note );

	void killAllNotes(); // sends a note off for all MIDI notes (0-127)
	
	// synthesis
	cipd::PureDataNodeRef	mPureDataNode;	// synth engine
	cipd::PatchRef			mPatch;			// music patch
//	vector<RtMidiOutRef>	mMidiOuts;

	void setupSynthesis();
	void updateAdditiveScoreSynthesis();
};

class MusicWorldCartridge : public GameCartridge
{
public:
	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<MusicWorld>();
	}
};

#endif /* MusicWorld_hpp */
