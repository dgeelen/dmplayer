#include "filter_normalize.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include "filter_sampleconverter.h"
#include "filter_libsamplerate.h"
#include "filter_split.h"
#include <algorithm>
#include <limits>

using namespace std;

#define REFERENCE_LEVEL (-20) // NOTE: 00497-Pink_min_20_dBFS_RMS_uncor_st_441.WAV averages at -31dB FS, but the replaygain site states 20dB FS?

/*
 * for each filter, [0] is 48000, [1] is 44100, [2] is 32000,
 *                  [3] is 24000, [4] is 22050, [5] is 16000,
 *                  [6] is 12000, [7] is 11025, [8] is  8000
 * Filters values obtained from MP3Gain sources
 */
#define YULE_TAPS 11
#define BUTTER_TAPS 3
#define N_SUPPORTED_SAMPLE_RATES 9
int SAMPLERATE[N_SUPPORTED_SAMPLE_RATES] = {48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025};
const float AYule[N_SUPPORTED_SAMPLE_RATES][YULE_TAPS] = {
{  1.00000000000000,  -3.84664617118067,   7.81501653005538, -11.34170355132042,
  13.05504219327545, -12.28759895145294,   9.48293806319790,  -5.87257861775999,
   2.75465861874613,  -0.86984376593551,   0.13919314567432                   },
{  1.00000000000000,  -3.47845948550071,   6.36317777566148,  -8.54751527471874,
   9.47693607801280,  -8.81498681370155,   6.85401540936998,  -4.39470996079559,
   2.19611684890774,  -0.75104302451432,  0.13149317958808                    },
{  1.00000000000000,  -2.37898834973084,   2.84868151156327,  -2.64577170229825,
   2.23697657451713,  -1.67148153367602,   1.00595954808547,  -0.45953458054983,
   0.16378164858596,  -0.05032077717131,   0.02347897407020                   },
{  1.00000000000000,  -1.61273165137247,   1.07977492259970,  -0.25656257754070,
  -0.16276719120440,  -0.22638893773906,   0.39120800788284,  -0.22138138954925,
   0.04500235387352,   0.02005851806501,   0.00302439095741                   },
{  1.00000000000000,  -1.49858979367799,   0.87350271418188,   0.12205022308084,
  -0.80774944671438,   0.47854794562326,  -0.12453458140019,  -0.04067510197014,
   0.08333755284107,  -0.04237348025746,   0.02977207319925                   },
{  1.00000000000000,  -0.62820619233671,   0.29661783706366,  -0.37256372942400,
   0.00213767857124,  -0.42029820170918,   0.22199650564824,   0.00613424350682,
   0.06747620744683,   0.05784820375801,   0.03222754072173                   },
{  1.00000000000000,  -1.04800335126349,   0.29156311971249,  -0.26806001042947,
   0.00819999645858,   0.45054734505008,  -0.33032403314006,   0.06739368333110,
  -0.04784254229033,   0.01639907836189,   0.01807364323573                   },
{  1.00000000000000,  -0.51035327095184,  -0.31863563325245,  -0.20256413484477,
   0.14728154134330,   0.38952639978999,  -0.23313271880868,  -0.05246019024463,
  -0.02505961724053,   0.02442357316099,   0.01818801111503                   },
{  1.00000000000000,  -0.25049871956020,  -0.43193942311114,  -0.03424681017675,
  -0.04678328784242,   0.26408300200955,   0.15113130533216,  -0.17556493366449,
  -0.18823009262115,   0.05477720428674,   0.04704409688120                  }};

const float BYule[N_SUPPORTED_SAMPLE_RATES][YULE_TAPS] = {{
   0.03857599435200,  -0.02160367184185,  -0.00123395316851,  -0.00009291677959,
  -0.01655260341619,   0.02161526843274,  -0.02074045215285,   0.00594298065125,
   0.00306428023191,   0.00012025322027,   0.00288463683916                   },
{  0.05418656406430,  -0.02911007808948,  -0.00848709379851,  -0.00851165645469,
  -0.00834990904936,   0.02245293253339,  -0.02596338512915,   0.01624864962975,
  -0.00240879051584,   0.00674613682247,  -0.00187763777362                   },
{  0.15457299681924,  -0.09331049056315,  -0.06247880153653,   0.02163541888798,
  -0.05588393329856,   0.04781476674921,   0.00222312597743,   0.03174092540049,
  -0.01390589421898,   0.00651420667831,  -0.00881362733839                   },
{  0.30296907319327,  -0.22613988682123,  -0.08587323730772,   0.03282930172664,
  -0.00915702933434,  -0.02364141202522,  -0.00584456039913,   0.06276101321749,
  -0.00000828086748,   0.00205861885564,  -0.02950134983287                   },
{  0.33642304856132,  -0.25572241425570,  -0.11828570177555,   0.11921148675203,
  -0.07834489609479,  -0.00469977914380,  -0.00589500224440,   0.05724228140351,
   0.00832043980773,  -0.01635381384540,  -0.01760176568150                   },
{  0.44915256608450,  -0.14351757464547,  -0.22784394429749,  -0.01419140100551,
   0.04078262797139,  -0.12398163381748,   0.04097565135648,   0.10478503600251,
  -0.01863887810927,  -0.03193428438915,   0.00541907748707                   },
{  0.56619470757641,  -0.75464456939302,   0.16242137742230,   0.16744243493672,
  -0.18901604199609,   0.30931782841830,  -0.27562961986224,   0.00647310677246,
   0.08647503780351,  -0.03788984554840,  -0.00588215443421                   },
{  0.58100494960553,  -0.53174909058578,  -0.14289799034253,   0.17520704835522,
   0.02377945217615,   0.15558449135573,  -0.25344790059353,   0.01628462406333,
   0.06920467763959,  -0.03721611395801,  -0.00749618797172                   },
{  0.53648789255105,  -0.42163034350696,  -0.00275953611929,   0.04267842219415,
  -0.10214864179676,   0.14590772289388,  -0.02459864859345,  -0.11202315195388,
  -0.04060034127000,   0.04788665548180,  -0.02217936801134                  }};

const float AButter[N_SUPPORTED_SAMPLE_RATES][BUTTER_TAPS] = {
	{ 1.00000000000000, -1.97223372919527, 0.97261396931306 },
	{ 1.00000000000000, -1.96977855582618, 0.97022847566350 },
	{ 1.00000000000000, -1.95835380975398, 0.95920349965459 },
	{ 1.00000000000000, -1.95002759149878, 0.95124613669835 },
	{ 1.00000000000000, -1.94561023566527, 0.94705070426118 },
	{ 1.00000000000000, -1.92783286977036, 0.93034775234268 },
	{ 1.00000000000000, -1.91858953033784, 0.92177618768381 },
	{ 1.00000000000000, -1.91542108074780, 0.91885558323625 },
	{ 1.00000000000000, -1.88903307939452, 0.89487434461664 },
};

const float BButter[N_SUPPORTED_SAMPLE_RATES][BUTTER_TAPS] = {
	{ 0.98621192462708, -1.97242384925416, 0.98621192462708 },
	{ 0.98500175787242, -1.97000351574484, 0.98500175787242 },
	{ 0.97938932735214, -1.95877865470428, 0.97938932735214 },
	{ 0.97531843204928, -1.95063686409857, 0.97531843204928 },
	{ 0.97316523498161, -1.94633046996323, 0.97316523498161 },
	{ 0.96454515552826, -1.92909031105652, 0.96454515552826 },
	{ 0.96009142950541, -1.92018285901082, 0.96009142950541 },
	{ 0.95856916599601, -1.91713833199203, 0.95856916599601 },
	{ 0.94597685600279, -1.89195371200558, 0.94597685600279 }
};

/**
 * NormalizeFilter accepts any input format (it detects non-float and adds the
 * SampleConverterFilter on it's own. NormalizeFilter again outputs Float.
 */
NormalizeFilter::NormalizeFilter(IAudioSourceRef as, AudioFormat target)
	: IAudioSource(as->getAudioFormat()), src(as)
{
	if(!src->getAudioFormat().Float) { // both IIR filter *and* resample filter expect Float input
		AudioFormat af_float(src->getAudioFormat());
		af_float.Float = true;
		audioformat = af_float;
		dcerr("Auto inserting SampleConverterFilter");
		src = IAudioSourceRef(new SampleConverterFilter(src, af_float));
	}

	int best_diff = SAMPLERATE[0];
	int best_match = 0;
	for(int i = 0; i < N_SUPPORTED_SAMPLE_RATES; ++i) {
		int diff = abs(SAMPLERATE[i] - audioformat.SampleRate);
		if( diff < best_diff ) {
			best_match = i;
			best_diff = diff;
		}
	}

	splitter = boost::shared_ptr<SplitFilter>(new SplitFilter(src, audioformat));
	IIRYule = boost::shared_ptr<IIRFilter>(
		new IIRFilter(splitter,
		              audioformat,
		              vector<float>(BYule[best_match], BYule[best_match] + YULE_TAPS),
		              vector<float>(AYule[best_match], AYule[best_match] + YULE_TAPS)
		             )
	);
	IIRButter = boost::shared_ptr<IIRFilter>(
		new IIRFilter(IIRYule,
		              audioformat,
		              vector<float>(BButter[best_match], BButter[best_match] + BUTTER_TAPS),
		              vector<float>(AButter[best_match], AButter[best_match] + BUTTER_TAPS)
		             )
	);

	rms_period_sample_count = audioformat.SampleRate / 20; // ~1/50sec (or 2205 samples for 44.1khz source)
	position = rms_period_sample_count * audioformat.Channels * sizeof(float);
	peak_amplitude = -std::numeric_limits<float>::max();

	// Clear IIRFilters from noise samples, so that splitter->getData2() returns the same amount of samples as IIRButter->getData()
	uint32 n_noise_sample_bytes = (YULE_TAPS + BUTTER_TAPS) * audioformat.Channels * sizeof(float);
	uint32 toread = n_noise_sample_bytes;
	uint8 buf[n_noise_sample_bytes];
	while((toread>0) && (!IIRButter->exhausted())) {
		toread -= IIRButter->getData(&buf[n_noise_sample_bytes-toread], toread);
	}
}

NormalizeFilter::~NormalizeFilter() {
}

void NormalizeFilter::update_gain() {
	uint32 capacity = rms_period_sample_count * audioformat.Channels;
	std::vector<float> filtered_samples(capacity);
	uint32 bytes_to_read = capacity * sizeof(float);
	uint32 todo = bytes_to_read;
	while(todo>0 && !IIRButter->exhausted()) {
		todo -= IIRButter->getData((uint8*)&filtered_samples[bytes_to_read-todo], todo);
	}
	filtered_samples.resize(capacity-(todo/sizeof(float)));

	float levels[audioformat.Channels];
	for(uint32 sample = 0; sample < filtered_samples.size(); sample += audioformat.Channels) {
		for(uint32 chan = 0; chan < audioformat.Channels; ++chan) {
			float s = filtered_samples[sample + chan];
			s = s * s;
			peak_amplitude = std::max(peak_amplitude, s);
			levels[chan] += s;
		}
	}

	uint32 nsamples = filtered_samples.size() / audioformat.Channels;
	for(uint32 chan = 0; chan < audioformat.Channels; ++chan) {
		levels[chan] /= float(nsamples);
	}

	float level = 0;
	for(uint32 chan = 0; chan < audioformat.Channels; ++chan) {
		level += levels[chan];
	}
	level /= audioformat.Channels;

#if 0
	// NOTE: This occusionally happens, (especially level<0) but I don't know how;
	//       level is calculated from filtered_samples^2 so should never be negative...
	if(level>1.0f || level<0.0f) {
		dcerr("WARNING: FAILFAILFAIL: " << level << "\n");
		level = 0.5f;
	}
#endif

	level = 10.0f * (log10(level + std::numeric_limits<float>::epsilon())); // NOTE: 20 * log10( sqrt( x ) / 1 ) == 10 * log10( x )
	dB_map.insert(dB_map.end(), level); // FIXME: perhaps do something when dB_map gets really big

	std::set<float>::reverse_iterator l = dB_map.rbegin();
	uint32 i = uint32(float(dB_map.size()-1) * 0.05);
	while(i>0) {
		++l;
		--i;
	}
	level = (*l); // TODO: figure out if we should use something like std::max((*l), (REFERENCE_LEVEL*3)) here, or just use something like 100.0f for max_gain

	float max_gain = std::min(1.0f/std::sqrt(peak_amplitude), 100.0f);
	gain  = std::pow(10.0f, (REFERENCE_LEVEL - level) / 20.0f);
	gain *= std::max(std::min((float(dB_map.size())-5.0f / 10.0f), 1.0f), 0.0f);
}

uint32 NormalizeFilter::getData(uint8* buf, uint32 len) {
	// After every 50ms worth of samples we want to look ahead at the next 50ms and update Gain information
	position += len;
	uint32 bytes_per_period = rms_period_sample_count * audioformat.Channels * sizeof(float);
	while(position>bytes_per_period) {
		position -= bytes_per_period;
		update_gain();
	}

	uint32 nsamples = len / sizeof(float);
	vector<float> b(nsamples);
	uint32 toread = nsamples * sizeof(float);
	uint32 done   = 0;
	while((toread>0) && !splitter->exhausted2()) {
		uint32 read = splitter->getData2((uint8*)&b[(done/sizeof(float))*sizeof(float)], toread);
		toread -= read;
		done   += read;
	}
	for(std::vector<float>::iterator i=b.begin(); i!=b.end(); ++i) {
		*i *= gain;
	}
	copy(b.begin(),b.end(), (float*)&buf[0]);
	return done;
}

bool NormalizeFilter::exhausted() {
	return IIRButter->exhausted();
}
