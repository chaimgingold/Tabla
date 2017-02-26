//
//  MusicVision.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 11/15/16.
//
//

#include "cinder/Rand.h"
#include "MusicVision.h"
#include "xml.h"
#include "ocv.h"
#include "geom.h"

void MusicVision::setParams( XmlTree xml )
{
	getXml(xml,"ScoreNoteVisionThresh",mScoreNoteVisionThresh);
	
	getXml(xml,"ScoreTrackRejectNumSamples",mScoreTrackRejectNumSamples);
	getXml(xml,"ScoreTrackRejectSuccessThresh",mScoreTrackRejectSuccessThresh);
	getXml(xml,"ScoreTrackMaxError",mScoreTrackMaxError);
//	getXml(xml,"ScoreMaxInteriorAngleDeg",mScoreMaxInteriorAngleDeg);
	getXml(xml,"ScoreTrackTemporalBlendFrac",mScoreTrackTemporalBlendFrac);
	getXml(xml,"ScoreTrackTemporalBlendIfDiffFracLT",mScoreTrackTemporalBlendIfDiffFracLT);
	
	getXml(xml,"MaxTokenScoreDistance",mMaxTokenScoreDistance);
	getXml(xml,"WorldUnitsPerMeasure",mWorldUnitsPerMeasure);
	getXml(xml,"BlankEdgePixels",mBlankEdgePixels);

	if ( xml.hasChild("RectFinder") ) {
		mRectFinder.mParams.set( xml.getChild("RectFinder") );
	}
}

Score*
MusicVision::findScoreMatch( const Score& needle, ScoreVec& hay, float maxErr, float* outError ) const
{
	Score* best   = 0;
	float  bestErr = maxErr;

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

	for( size_t i=0; i<hay.size(); ++i )
	{
		float err = scoreError( needle.mQuad, hay[i].mQuad, maxErr ) ;
//		cout << "err " << err << endl;
		if (err<bestErr)
		{
			bestErr=err;
			best=&hay[i];
		}
	}

	if (outError) *outError=bestErr;
	return best;
}

Score*
MusicVision::findExactScoreMatch( const Score& needle, ScoreVec& hay, float maxErr, float* outError ) const
{
	Score* best   = 0;
	float  bestErr = maxErr;

	auto getError = []( const vec2 a[4], const vec2 b[4] ) -> float
	{
		float sumerr=0.f;
		for( int i=0; i<4; ++i )
		{
			float d = distance( a[i], b[i] );
			sumerr += d ;
		}
		return sumerr;
	};

	for( size_t i=0; i<hay.size(); ++i )
	{
		float err = getError( needle.mQuad, hay[i].mQuad ) ;
//		cout << "err " << err << endl;
		if ( err<bestErr )
		{
			bestErr=err;
			best=&hay[i];
		}
	}

	if (outError) *outError=bestErr;
	return best;
}

bool
MusicVision::doesZombieScoreIntersectZombieScores( const Score& old, ScoreVec& scores ) const
{
	bool r=false;

	PolyLine2 op = old.getPolyLine();
	vector<PolyLine2> opv;
	opv.push_back(op);

	for( auto &s : scores )
	{
		bool touches=false;

		{
			PolyLine2 p = s.getPolyLine();
			vector<PolyLine2> pv;
			pv.push_back(p);

			try {
				touches = ! PolyLine2::calcIntersection(opv,pv).empty() ;
				// this is CRAZY performance wise, but it should work for now
				// TODO: do poly-poly intersection correctly
			} catch(...) {} // just in case boost excepts us
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
MusicVision::shouldPersistOldScore ( const Score& old, ScoreVec& scores, const ContourVector &contours ) const
{
	// did we go dark?
	if ( scoreFractionInContours(old, contours, mScoreTrackRejectNumSamples) < mScoreTrackRejectSuccessThresh ) return false;

	// do we touch other zombies?
	if ( doesZombieScoreIntersectZombieScores(old,scores) ) return false;

	return true;
}

float
MusicVision::scoreFractionInContours( const Score& old, const ContourVector &contours, int numSamples ) const
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

bool
MusicVision::shouldBeMetaParamScore( const Score& s ) const
{
	vec2 scoreSize = s.getSizeInWorldSpace();

	return min(scoreSize.x,scoreSize.y) < 10.f ;
}

float
MusicVision::getNearestTempo( float t ) const
{
	if (mMeasureCounts.empty())
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
	if (mMeasureCounts.size()==1) return mMeasureCounts[0]; // globally fixed
	if (mMeasureCounts[0]>=t) return mMeasureCounts[0]; // minimum

	// find nearest in between two tempos
	for( size_t i=0; i<mMeasureCounts.size()-1; ++i )
	{
		if ( t >= mMeasureCounts[i] && t <= mMeasureCounts[i+1] )
		{
			float mid = lerp(mMeasureCounts[i],mMeasureCounts[i+1],.5f);

			if ( t < mid ) return mMeasureCounts[i];
			else return mMeasureCounts[i+1];
		}
	}

	// maximum
	return mMeasureCounts.back();
}

float
MusicVision::decideMeasureCountForScore ( const Score& score ) const
{
	if ( score.mInstrument && score.mInstrument->mSynthType == Instrument::SynthType::Meta )
	{
		return 1; // 1 measure
	}
	else
	{
		vec2 size = score.getSizeInWorldSpace();

		float d = size.x / mWorldUnitsPerMeasure;

		float t = getNearestTempo(d);

		if (0) cout << size.x << "cm = " << d << "measures => " << t << "measures" << endl;

		return t;
	}
}

static float distanceBetweenPolys( const PolyLine2& a, const PolyLine2& b )
{
	vec2 bc = b.calcCentroid();
	
	vec2 p1 = closestPointOnPoly(bc, a);
	vec2 p2 = closestPointOnPoly(p1, b);
	
	return distance(p1,p2);
}

InstrumentRef
MusicVision::decideInstrumentForScore( const Score& score, MusicStampVec& stamps, const TokenMatchVec& tokens ) const
{
	InstrumentRef best;
	
	if (stamps.areTokensEnabled())
	{
		float bestScore = mMaxTokenScoreDistance;
		
		for( const auto &t : tokens )
		{
			MusicStamp* s = stamps.getStampByInstrumentName(t.getName());
			
			if (s)
			{
				float sscore = distanceBetweenPolys(score.getPolyLine(),t.getPoly());
				
				if (sscore<bestScore)
				{
					bestScore = sscore;
					best = s->mInstrument;
				}
			}
			else cout << "Couldn't find a stamp for token '" << t.getName() << "'" << endl;
		}
	}
	else
	{
		float bestScore=MAXFLOAT;
		
		for( const auto &s : stamps )
		{
			if ( s.isInstrumentAvailable() && score.getPolyLine().contains(s.mLoc) )
			{
				float sscore = distance( score.getCentroid(), s.mLoc );
				if (sscore<bestScore)
				{
					bestScore = sscore;
					best = s.mInstrument;
				}
			}
		}
	}
	
	return best;
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

float MusicVision::getScoreOctaveShift( const Score& score, const PolyLine2& wrtRegion ) const
{
	const vec2 lookVec = perp(mTimeVec);

	pair<float,float> worldYs(0,0), scoreYs(0,0);

	if ( wrtRegion.size() > 0 )
	{
		worldYs = getShapeRange( &wrtRegion.getPoints()[0], wrtRegion.size(), lookVec );
	}

	scoreYs = getShapeRange( score.mQuad, 4, lookVec );

	float scoreHeight = scoreYs.second - scoreYs.first;
	float scorey = lerp(scoreYs.first,scoreYs.second,.5f);
	
	// inset world
	worldYs.second -= scoreHeight/2.f;
	worldYs.first  += scoreHeight/2.f;
	
	float f = 1.f - (scorey - worldYs.first) / (worldYs.second - worldYs.first);
	// don't understand why i need 1-, but it works.

//	int o = roundf( (f-.5f) * (float)mNumOctaves ) ;

//	if (0) cout << o << endl;


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

	return f;
}

ScoreVec
MusicVision::updateVision(
	const Vision::Output&	visionOut,
	Pipeline&				pipeline,
	const ScoreVec&			oldScores,
	MusicStampVec& stamps ) const
{
	ScoreVec v = getScores( visionOut.mContours, oldScores, stamps, visionOut.mTokens );
	
	updateScoresWithImageData(pipeline,v);
	
	return v;
}

ScoreVec
MusicVision::getScoresFromContours(
	const ContourVector& contours,
	MusicStampVec& stamps,
	const TokenMatchVec& tokens ) const
{
	ScoreVec scores;
	
	for( const auto &c : contours )
	{
		PolyLine2 rectPoly;

		if ( !c.mIsHole && c.mTreeDepth==0
		   && mRectFinder.getRectFromPoly(c.mPolyLine,rectPoly)
		   && !tokens.doesOverlapToken(rectPoly) )
		{
			Score score;

			// shape
			score.setQuadFromPolyLine(rectPoly,mTimeVec);
//			if ( score.getQuadMaxInteriorAngle() > toRadians(mScoreMaxInteriorAngleDeg) ) continue;
				// this final test is now redundant to RectFinder

			// instrument
			InstrumentRef instr = decideInstrumentForScore(score,stamps,tokens);
			if (instr)
			{
				score.mInstrumentName = instr->mName ;
				score.mInstrument = instr;
			}

			// timing
			// Choose octave based on up<>down
			score.mOctaveFrac = (mWorldBoundsPoly.size()>0) ? getScoreOctaveShift(score,mWorldBoundsPoly) : .5f;
			score.mOctave	= 0; // will be set wrt mOctaveFrac by MusicWorld
			
			score.mPan		= .5f ;
			
			score.mMeasureCount = decideMeasureCountForScore(score);
			score.mBeatQuantization = mBeatQuantization;
			score.mBeatsPerMeasure = mBeatsPerMeasure;
			
			score.mNoteCount = mNoteCount;
			
			// save
			scores.push_back(score);
		}
	}
	
	return scores;
}

ScoreVec MusicVision::mergeOldAndNewScores(
	const ScoreVec& oldScores,
	const ScoreVec& newScores,
	const ContourVector& contours,
	bool isUsingTokens ) const
{
	const bool kVerbose = false;
	
	// output is new scores
	ScoreVec output = newScores;
	
	// update output with old scores
	for ( const auto &oldScore : oldScores )
	{
		float matchError;
		Score* newScore = findExactScoreMatch(oldScore,output,mScoreTrackMaxError,&matchError);

		if ( newScore )
		{
			if (kVerbose) cout << "match" << endl;

			// new <= old
			InstrumentRef newInstrument = newScore->mInstrument;
			
			if ( oldScore.mInstrument )
			{
				// overwrite new score with old, but not...
				// ...if old score had no instrument

				// otherwise
				*newScore = oldScore;
				
				// but...
				if (isUsingTokens) newScore->mInstrument = newInstrument;
			}
		}
		else // zombie! (c has no match)
		{
			// persist it?
			if ( shouldPersistOldScore(oldScore,output,contours) ) // this flags other zombies that might need to be culled
			{
				if (kVerbose) cout << "zombie" << endl;
				
				Score out = oldScore;
				out.mIsZombie = true;
				
				output.push_back(out);
			}
			else if (kVerbose) cout << "die" << endl;
		}
	}

	// 2nd pass eliminate intersecting zombie (anything flagged)
	auto filterVector = []( ScoreVec& vec, function<bool(const Score&)> iffunc ) -> ScoreVec
	{
		ScoreVec out;
		
		for( auto &s : vec )
		{
			if ( iffunc(s) ) out.push_back(s);
		}
		
		return out;
	};
	
	output = filterVector( output, []( const Score& score ) -> bool
	{
		return !score.mDoesZombieTouchOtherZombies;
	});
	
	// return
	return output;
}

ScoreVec MusicVision::getScores(
	const ContourVector& contours,
	const ScoreVec& oldScores,
	MusicStampVec& stamps,
	const TokenMatchVec& tokens ) const
{
	// get new ones
	ScoreVec newScores = getScoresFromContours(contours,stamps,tokens);

	// merge with old
	newScores = mergeOldAndNewScores(oldScores,newScores,contours,stamps.areTokensEnabled());
	
	// return
	return newScores;
}


static float getImgDiffFrac( cv::UMat a, cv::UMat b )
{
	if ( !b.empty() && !a.empty() && a.size == b.size )
	{
		cv::UMat diffm;
		cv::subtract( a, b, diffm);
		cv::Scalar diff = cv::sum(diffm);
		float d = (diff[0]/255.f) / (a.size[0] * a.size[1]);
		//cout << "diff: " << d << endl;
		return d;
	}

	return 1.f;
}

static void doTemporalMatBlend(
	Pipeline& pipeline,
	string scoreName,
	cv::UMat oldimg,
	cv::UMat newimg,
	float oldWeight,
	float diffthresh )
{
	// newimg = oldimg * oldWeight + newimg * (1 - oldWeight)

	bool verbose=false;

	float diff = getImgDiffFrac(oldimg,newimg);

	if ( diff < diffthresh )
	{
		if (verbose) cout << "blend" << endl ;

		if ( !oldimg.empty() && oldimg.size == newimg.size )
		{
			float newWeight = 1.f - oldWeight;

			cv::addWeighted( newimg, newWeight, oldimg, oldWeight, 0.f, newimg );
			pipeline.then( scoreName + " temporally blended", newimg);
			pipeline.back()->mLayoutHintScale = .5f;
			pipeline.back()->mLayoutHintOrtho = true;
		}
	}
	else if (verbose) cout << "no-blend " << diff << endl;
}

void MusicVision::quantizeImage( Pipeline& pipeline,
	Score& s,
	string scoreName,
	bool doTemporalBlend,
	cv::UMat oldTemporalBlendImage,
	int quantizeNumCols, int quantizeNumRows ) const
{
	cv::UMat &inquant = s.mImage;

	if (quantizeNumCols<=0) quantizeNumCols = inquant.cols;
	if (quantizeNumRows<=0) quantizeNumRows = inquant.rows;

	cv::UMat quantized;
	cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_AREA ); // best?
		// cv::INTER_AREA is the best.
		// cv::INTER_LANCZOS4 is good
		// these are not so great: cv::INTER_CUBIC, cv::INTER_NEAREST, cv::INTER_LINEAR

	vec2 outsize = vec2( s.mImage.cols, s.mImage.rows );

	pipeline.then( scoreName + " quantized", quantized);
	pipeline.back()->setImageToWorldTransform(
		pipeline.back()->mImageToWorld
			* glm::scale(vec3(outsize.x / (float)quantizeNumCols, outsize.y / (float)quantizeNumRows, 1))
		);
	pipeline.back()->mLayoutHintScale = .5f;
	pipeline.back()->mLayoutHintOrtho = true;

	// blend
	if ( doTemporalBlend )
	{
		doTemporalMatBlend( pipeline, scoreName, oldTemporalBlendImage, quantized,
			mScoreTrackTemporalBlendFrac, mScoreTrackTemporalBlendIfDiffFracLT );
	}

	// threshold
	cv::UMat &inthresh = quantized;
	cv::Mat thresholded;

	if ( mScoreNoteVisionThresh < 0 ) {
		cv::threshold( inthresh, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	} else {
		cv::threshold( inthresh, thresholded, mScoreNoteVisionThresh, 255, cv::THRESH_BINARY );
	}

	pipeline.then( scoreName + " thresholded", thresholded);
	pipeline.back()->mLayoutHintScale = .5f;
	pipeline.back()->mLayoutHintOrtho = true;


	// output
	s.mQuantizedImagePreThreshold = quantized;
	s.mQuantizedImage = thresholded;
}

float MusicVision::getSliderValueFromQuantizedImageData( const Score& s )
{
	// alternative idea would be to take max, not avg
	
//	float oldSliderValue = s.mMetaParamSliderValue;
	float value = 0.f;

	float sumw = 0.f;

	for( int i=0; i<s.mQuantizedImage.rows; ++i )
	{
		float v = 1.f - (float)i / (float)(s.mQuantizedImage.rows-1);
		float w = 1.f - (float)s.mQuantizedImage.at<unsigned char>(i,0) / 255.f;

		sumw += w;
		value += v * w;
	}

	value /= sumw;

	if ( sumw==0.f )
	{
//		value = oldSliderValue; // uh-oh! fall back to last frame value
		value = s.mInstrument->mMetaParamInfo.mDefaultValue;
	}
	
	return value;
//			cout << "slider: " << s.mMetaParamSliderValue << endl;
}

void MusicVision::updateScoresWithImageData( Pipeline& pipeline, ScoreVec& scores ) const
{
	// this function grabs the bitmaps for each score

	// get the image we are slicing from
	Pipeline::StageRef world = pipeline.getStage("thresholded");
	if ( !world || world->mImageCV.empty() ) return;

	// for each score...
	for( int si=0; si<scores.size(); ++si )
	{
		Score& s = scores[si];
		string scoreName = string("score")+toString(si+1);
		const auto instr = s.mInstrument;

		cv::UMat oldTemporalBlendImage;
		const bool doTemporalBlend = instr && instr->isNoteType() && mScoreTrackTemporalBlendFrac>0.f;
		if ( doTemporalBlend ) oldTemporalBlendImage = s.mQuantizedImagePreThreshold.clone(); // must clone to work!

		// extract image
		mat4 scoreImageToWorld;
		getSubMatWithQuad( world->mImageCV, s.mImage, s.mQuad, world->mWorldToImage, scoreImageToWorld );
		
		pipeline.then( scoreName, s.mImage);
		pipeline.back()->setImageToWorldTransform( scoreImageToWorld );
		pipeline.back()->mLayoutHintScale = .5f;
		pipeline.back()->mLayoutHintOrtho = true;

		// blank out edges
		if (mBlankEdgePixels>0)
		{
			cv::rectangle(s.mImage, cv::Point(0,0), cv::Point(s.mImage.cols-1,s.mImage.rows-1), 255, mBlankEdgePixels );

			pipeline.then( scoreName + " blanked edges", s.mImage );
			pipeline.back()->mLayoutHintScale = .5f;
			pipeline.back()->mLayoutHintOrtho = true;
		}
		
		// quantize
		if ( instr && instr->isNoteType() )
		{
			// notes
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, s.getQuantizedBeatCount(), s.mNoteCount );
		}
		else if ( instr && instr->mSynthType==Instrument::SynthType::Meta )
		{
			// meta-param
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, 1, -1 );
				// we don't quantize to num states because it behaves weird.
				// maybe 2x or 4x that might help, but the code below handles continuous values best.

			s.mMetaParamSliderValue = getSliderValueFromQuantizedImageData(s);
		}
		
		// additive texture grab
		if (instr && instr->mSynthType==Instrument::SynthType::Additive) {
			s.mTexture = matToTexture(s.mImage,false);
		}
	}
}
