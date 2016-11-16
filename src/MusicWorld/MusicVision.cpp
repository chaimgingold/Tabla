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
	cout << "MusicVision::setParams( XmlTree xml )" << endl;

	getXml(xml,"TempoWorldUnitsPerSecond",mTempoWorldUnitsPerSecond);
	
	getXml(xml,"ScoreTrackRejectNumSamples",mScoreTrackRejectNumSamples);
	getXml(xml,"ScoreTrackRejectSuccessThresh",mScoreTrackRejectSuccessThresh);
	getXml(xml,"ScoreTrackMaxError",mScoreTrackMaxError);
	getXml(xml,"ScoreMaxInteriorAngleDeg",mScoreMaxInteriorAngleDeg);
	getXml(xml,"ScoreTrackTemporalBlendFrac",mScoreTrackTemporalBlendFrac);
	getXml(xml,"ScoreTrackTemporalBlendIfDiffFracLT",mScoreTrackTemporalBlendIfDiffFracLT);
	getXml(xml,"ScoreVisionTrimFrac",mScoreVisionTrimFrac);
}

Score*
MusicVision::findScoreMatch( const Score& needle, ScoreVector& hay, float maxErr, float* outError ) const
{
	Score* best   = 0;
	float  bestErr = MAXFLOAT;

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

bool
MusicVision::doesZombieScoreIntersectZombieScores( const Score& old, ScoreVector& scores ) const
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

			touches = ! PolyLine2::calcIntersection(opv,pv).empty() ;
			// this is CRAZY performance wise, but it should work for now
			// TODO: do poly-poly intersection correctly
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
MusicVision::shouldPersistOldScore ( const Score& old, ScoreVector& scores, const ContourVector &contours ) const
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

void MusicVision::assignUnassignedMetaParams( ScoreVector& scores ) const
{
	// what do we have?
	vector<int> sliders((int)MetaParam::kNumMetaParams);
	for( auto &s : sliders ) s=0;

	for ( const auto &s : scores )
	{
		if ( s.mInstrument && s.mInstrument->mSynthType==Instrument::SynthType::Meta )
		{
			sliders[(int)s.mInstrument->mMetaParam]++;
		}
	}

	// which ones do we want?
	set<MetaParam> unassignedParams;

	for( int i=0; i<sliders.size(); ++i )
	{
		if (sliders[i]==0) unassignedParams.insert( (MetaParam)i );
	}

	// assign
	for ( auto &s : scores )
	{
		if ( s.mInstrumentName == "_meta-slider" )
		{
			MetaParam param;

			if ( unassignedParams.empty() ) param = (MetaParam)(rand() % (int)MetaParam::kNumMetaParams);
			else
			{
				// choose param, closest distance from score to last place we saw unassigned param
				MetaParam bestParam = *unassignedParams.begin();
				float bestScore = MAXFLOAT;
				vec2 scoreLoc = s.getCentroid();

				for( auto i : unassignedParams )
				{
					auto j = mLastSeenMetaParamLoc.find(i);
					float score;

					if ( j == mLastSeenMetaParamLoc.end() ) {
						// never seen
						score = MAXFLOAT - 100.f; // slightly better than nothing
					} else {
						score = distance( scoreLoc, j->second );
					}

					// best?
					if ( score < bestScore )
					{
						bestScore = score;
						bestParam = j->first;
					}
				}

				// remove from list of options
				unassignedParams.erase( unassignedParams.find(bestParam) ); // should be in there!

				// pick it (bestParam redundant to param, but this naming makes algorithm clearer)
				param = bestParam;
			}

			// assign
			s.mInstrument = getInstrumentForMetaParam( param );
			if (s.mInstrument) s.mInstrumentName = s.mInstrument->mName;
		}
	}
}

float
MusicVision::getNearestTempo( float t ) const
{
	if (mTempos.empty())
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
	if (mTempos.size()==1) return mTempos[0]; // globally fixed
	if (mTempos[0]>=t) return mTempos[0]; // minimum

	// find nearest in between two tempos
	for( size_t i=0; i<mTempos.size()-1; ++i )
	{
		if ( t >= mTempos[i] && t <= mTempos[i+1] )
		{
			float mid = lerp(mTempos[i],mTempos[i+1],.5f);

			if ( t < mid ) return mTempos[i];
			else return mTempos[i+1];
		}
	}

	// maximum
	return mTempos.back();
}

float
MusicVision::decideDurationForScore ( const Score& score ) const
{
	vec2 size = score.getSizeInWorldSpace();

	float d = size.x / mTempoWorldUnitsPerSecond;

	float t = getNearestTempo(d);

	if (0) cout << size.x << "cm = " << d << "sec => " << t << "sec" << endl;

	return t;
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

void MusicVision::generateInstrumentRegions( map<string,InstrumentRef> &instruments, const PolyLine2& worldBounds )
{
	mInstrumentRegions.clear();

	Rectf worldRect( worldBounds.getPoints() );

	int ninstr=0;
	for( auto i : instruments )
	{
		if ( i.second->mSynthType!=Instrument::SynthType::Meta ) ninstr++;
	}

//	instruments.size();

	int dim = ceil( sqrt(ninstr) );

	vec2 scale = worldRect.getSize() / vec2(dim,dim);

	int x=0, y=0;

	for( auto i : instruments )
	{
		if ( i.second->mSynthType==Instrument::SynthType::Meta ) continue;

		Rectf r(0,0,1,1);
		r.offset( vec2(x,y) );
		r.scale(scale);

		PolyLine2 p;
		p.push_back( r.getUpperLeft() );
		p.push_back( r.getUpperRight() );
		p.push_back( r.getLowerRight() );
		p.push_back( r.getLowerLeft() );
		p.setClosed();

		mInstrumentRegions.push_back( pair<PolyLine2,InstrumentRef>(p,i.second) );

		//
		x++;
		if (x>=dim)
		{
			x=0;
			y++;
		}
	}
}

int MusicVision::getScoreOctaveShift( const Score& score, const PolyLine2& wrtRegion ) const
{
	const vec2 lookVec = perp(mTimeVec);

	pair<float,float> worldYs(0,0), scoreYs(0,0);

	if ( wrtRegion.size() > 0 )
	{
		worldYs = getShapeRange( &wrtRegion.getPoints()[0], wrtRegion.size(), lookVec );
	}

	scoreYs = getShapeRange( score.mQuad, 4, lookVec );

	float scorey = lerp(scoreYs.first,scoreYs.second,.5f);

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

InstrumentRef
MusicVision::decideInstrumentForScore( const Score& s, int* octaveShift ) const
{
	// do instrument by world region
	vec2 c = s.getPolyLine().calcCentroid();

	// find
	for( auto i : mInstrumentRegions )
	{
		if ( i.first.contains(c) )
		{
			if (octaveShift) *octaveShift = getScoreOctaveShift(s,i.first);
			return i.second;
		}
	}

	// default to first
	auto i = mInstrumentRegions.begin();
	if (i!=mInstrumentRegions.end())
	{
//		if (octaveShift) *octaveShift = getScoreOctaveShift(s,getWorldBoundsPoly());
			// and do octave wrt world bounds
//		return i->second;
		return 0; // just whatever, none for now.
	}

	// none
	return 0;
}

InstrumentRef
MusicVision::getInstrumentForMetaParam( MetaParam p ) const
{
	for ( auto i : mInstruments )
	{
		if ( i.second->mSynthType == Instrument::SynthType::Meta && i.second->mMetaParam==p )
		{
			return i.second;
		}
	}
	return 0;
}

MusicVision::ScoreVector
MusicVision::updateVision( const ContourVector &contours, Pipeline& pipeline, const ScoreVector& oldScores ) const
{
	ScoreVector v = getScores(contours,oldScores);
	
	updateScoresWithImageData(pipeline,v);
	
	return v;
}

MusicVision::ScoreVector
MusicVision::getScoresFromContours( const ContourVector& contours ) const
{
	ScoreVector scores;
	
	for( const auto &c : contours )
	{
		if ( !c.mIsHole && c.mTreeDepth==0 && c.mPolyLine.size()==4 )
		{
			Score score;

			// shape
			score.setQuadFromPolyLine(c.mPolyLine,mTimeVec);
			if ( score.getQuadMaxInteriorAngle() > toRadians(mScoreMaxInteriorAngleDeg) ) continue;

			// timing
			score.mDurationFrac = decideDurationForScore(score); // inherit, but it could be custom based on shape or something


			// meta param?
			if ( shouldBeMetaParamScore(score) )
			{
				score.mInstrumentName = "_meta-slider";
			}
			else
			{
				// instrument
				int octaveShift = 0;
				InstrumentRef instr = decideInstrumentForScore(score,&octaveShift);
				if (instr)
				{
					score.mInstrumentName = instr->mName ;
					score.mInstrument = instr;
				}

				// Choose octave based on up<>down
				score.mOctave	= octaveShift;
				score.mPan		= .5f ;
				score.mBeatCount = mBeatCount;
				score.mNoteCount = mNoteCount;
			}

			scores.push_back(score);
		}
	}
	
	return scores;
}

MusicVision::ScoreVector
MusicVision::mergeOldAndNewScores(
	const ScoreVector& oldScores,
	const ScoreVector& newScores,
	const ContourVector& contours ) const
{
	const bool kVerbose = false;
	
	// output is new scores
	ScoreVector output = newScores;
	
	// update output with old scores
	for ( const auto &oldScore : oldScores )
	{
		float matchError;
		Score* newScore = findScoreMatch(oldScore,output,mScoreTrackMaxError,&matchError);

		if ( newScore )
		{
			if (kVerbose) cout << "match" << endl;

			// new <= old
			*newScore = oldScore;
			
			// any exceptions introduce here
//			if (newScore->mMetaParamSliderValue==-1.f)
//			{
				// maybe we lost a slider value
//				newScore->mMetaParamSliderValue = oldScore.mMetaParamSliderValue;
//			}
			
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
	auto filterVector = []( ScoreVector& vec, function<bool(const Score&)> iffunc ) -> ScoreVector
	{
		ScoreVector out;
		
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

MusicVision::ScoreVector
MusicVision::getScores( const ContourVector& contours, const ScoreVector& oldScores ) const
{
	// get new ones
	ScoreVector newScores = getScoresFromContours(contours);

	// merge with old
	newScores = mergeOldAndNewScores(oldScores,newScores,contours);
	
	// finally, assign any unassigned meta-params
	assignUnassignedMetaParams(newScores);

	// return
	return newScores;
}


static float getImgDiffFrac( cv::Mat a, cv::Mat b )
{
	if ( !b.empty() && !a.empty() && a.size == b.size )
	{
		cv::Mat diffm;
		cv::subtract( a, b, diffm);
		cv::Scalar diff = cv::sum(diffm);
		float d = (diff[0]/255.f) / (a.size[0] * a.size[1]);
		//cout << "diff: " << d << endl;
		return d;
	}

	return 1.f;
}

static void doTemporalMatBlend( Pipeline& pipeline, string scoreName, cv::Mat oldimg, cv::Mat newimg, float oldWeight, float diffthresh )
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
			pipeline.getStages().back()->mLayoutHintScale = .5f;
			pipeline.getStages().back()->mLayoutHintOrtho = true;
		}
	}
	else if (verbose) cout << "no-blend " << diff << endl;
}

void MusicVision::quantizeImage( Pipeline& pipeline,
	Score& s,
	string scoreName,
	bool doTemporalBlend,
	cv::Mat oldTemporalBlendImage,
	int quantizeNumCols, int quantizeNumRows ) const
{
	cv::Mat &inquant = s.mImage;

	if (quantizeNumCols<=0) quantizeNumCols = inquant.cols;
	if (quantizeNumRows<=0) quantizeNumRows = inquant.rows;

	cv::Mat quantized;
	cv::resize( inquant, quantized, cv::Size(quantizeNumCols,quantizeNumRows), 0, 0, cv::INTER_AREA ); // best?
		// cv::INTER_AREA is the best.
		// cv::INTER_LANCZOS4 is good
		// these are not so great: cv::INTER_CUBIC, cv::INTER_NEAREST, cv::INTER_LINEAR

	vec2 outsize = vec2( s.mImage.cols, s.mImage.rows );

	pipeline.then( scoreName + " quantized", quantized);
	pipeline.setImageToWorldTransform(
		pipeline.getStages().back()->mImageToWorld
			* glm::scale(vec3(outsize.x / (float)quantizeNumCols, outsize.y / (float)quantizeNumRows, 1))
		);
	pipeline.getStages().back()->mLayoutHintScale = .5f;
	pipeline.getStages().back()->mLayoutHintOrtho = true;

	// blend
	if ( doTemporalBlend )
	{
		doTemporalMatBlend( pipeline, scoreName, oldTemporalBlendImage, quantized,
			mScoreTrackTemporalBlendFrac, mScoreTrackTemporalBlendIfDiffFracLT );
	}

	// threshold
	cv::Mat &inthresh = quantized;
	cv::Mat thresholded;

	if ( mScoreNoteVisionThresh < 0 ) {
		cv::threshold( inthresh, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
	} else {
		cv::threshold( inthresh, thresholded, mScoreNoteVisionThresh, 255, cv::THRESH_BINARY );
	}

	pipeline.then( scoreName + " thresholded", thresholded);
	pipeline.getStages().back()->mLayoutHintScale = .5f;
	pipeline.getStages().back()->mLayoutHintOrtho = true;


	// output
	s.mQuantizedImagePreThreshold = quantized;
	s.mQuantizedImage = thresholded;
}

void MusicVision::updateScoresWithImageData( Pipeline& pipeline, ScoreVector& scores ) const
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

		cv::Mat oldTemporalBlendImage;
		const bool doTemporalBlend = instr && instr->mSynthType==Instrument::SynthType::MIDI && mScoreTrackTemporalBlendFrac>0.f;
		if ( doTemporalBlend ) oldTemporalBlendImage = s.mQuantizedImagePreThreshold.clone(); // must clone to work!

		// get src points
		vec2		srcpt[4];
		cv::Point2f srcpt_cv[4];
		{
			vec2  trimto; // average of corners
			for ( int i=0; i<4; ++i )
			{
				srcpt[i] = vec2( world->mWorldToImage * vec4(s.mQuad[i],0,1) );
				trimto += srcpt[i]*.25f;
			}

			// trim
			for ( int i=0; i<4; ++i ) srcpt[i] = lerp( srcpt[i], trimto, mScoreVisionTrimFrac );

			// to ocv
			vec2toOCV_4(srcpt, srcpt_cv);
		}

		// get output points
		vec2 outsize(
			max( distance(srcpt[0],srcpt[1]), distance(srcpt[3],srcpt[2]) ),
			max( distance(srcpt[0],srcpt[3]), distance(srcpt[1],srcpt[2]) )
		);
		vec2 dstpt[4] = { {0,0}, {outsize.x,0}, {outsize.x,outsize.y}, {0,outsize.y} };

		cv::Point2f dstpt_cv[4];
		vec2toOCV_4(dstpt, dstpt_cv);

		// grab it
		cv::Mat xform = cv::getPerspectiveTransform( srcpt_cv, dstpt_cv ) ;
		cv::warpPerspective( world->mImageCV, s.mImage, xform, cv::Size(outsize.x,outsize.y) );

		pipeline.then( scoreName, s.mImage);
		pipeline.setImageToWorldTransform( getOcvPerspectiveTransform(dstpt,s.mQuad) );
		pipeline.getStages().back()->mLayoutHintScale = .5f;
		pipeline.getStages().back()->mLayoutHintOrtho = true;
		
		// quantize
		if ( instr && instr->mSynthType==Instrument::SynthType::MIDI )
		{
			// midi
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, s.mBeatCount, s.mNoteCount );
		}
		else if ( instr && instr->mSynthType==Instrument::SynthType::Meta )
		{
			float oldSliderValue = s.mMetaParamSliderValue;

			// meta-param
			quantizeImage(pipeline,s,scoreName,doTemporalBlend,oldTemporalBlendImage, 1, -1 );
				// we don't quantize to num states because it behaves weird.
				// maybe 2x or 4x that might help, but the code below handles continuous values best.

			// parse value...
			s.mMetaParamSliderValue = 0.f;
			float sumw = 0.f;

			for( int i=0; i<s.mQuantizedImage.rows; ++i )
			{
				float v = 1.f - (float)i / (float)(s.mQuantizedImage.rows-1);
				float w = 1.f - (float)s.mQuantizedImage.at<unsigned char>(i,0) / 255.f;

				sumw += w;
				s.mMetaParamSliderValue += v * w;
			}

			s.mMetaParamSliderValue /= sumw;

			if ( sumw==0.f )
			{
				// uh-oh! fall back to last frame value
				s.mMetaParamSliderValue = oldSliderValue;
			}
			
//			cout << "slider: " << s.mMetaParamSliderValue << endl;
		}
	}
}