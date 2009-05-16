#include "filter_normalize.h"

#include "../cross-platform.h"
#include "../util/StrFormat.h"
#include "../error-handling.h"
#include "filter_sampleconverter.h"
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
uint32 SAMPLERATE[N_SUPPORTED_SAMPLE_RATES] = {48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025};
const float AYule[N_SUPPORTED_SAMPLE_RATES][YULE_TAPS] = {
{  1.00000000000000f,  -3.84664617118067f,   7.81501653005538f, -11.34170355132042f,
  13.05504219327545f, -12.28759895145294f,   9.48293806319790f,  -5.87257861775999f,
   2.75465861874613f,  -0.86984376593551f,   0.13919314567432f                    },
{  1.00000000000000f,  -3.47845948550071f,   6.36317777566148f,  -8.54751527471874f,
   9.47693607801280f,  -8.81498681370155f,   6.85401540936998f,  -4.39470996079559f,
   2.19611684890774f,  -0.75104302451432f,   0.13149317958808f                    },
{  1.00000000000000f,  -2.37898834973084f,   2.84868151156327f,  -2.64577170229825f,
   2.23697657451713f,  -1.67148153367602f,   1.00595954808547f,  -0.45953458054983f,
   0.16378164858596f,  -0.05032077717131f,   0.02347897407020f                    },
{  1.00000000000000f,  -1.61273165137247f,   1.07977492259970f,  -0.25656257754070f,
  -0.16276719120440f,  -0.22638893773906f,   0.39120800788284f,  -0.22138138954925f,
   0.04500235387352f,   0.02005851806501f,   0.00302439095741f                    },
{  1.00000000000000f,  -1.49858979367799f,   0.87350271418188f,   0.12205022308084f,
  -0.80774944671438f,   0.47854794562326f,  -0.12453458140019f,  -0.04067510197014f,
   0.08333755284107f,  -0.04237348025746f,   0.02977207319925f                    },
{  1.00000000000000f,  -0.62820619233671f,   0.29661783706366f,  -0.37256372942400f,
   0.00213767857124f,  -0.42029820170918f,   0.22199650564824f,   0.00613424350682f,
   0.06747620744683f,   0.05784820375801f,   0.03222754072173f                    },
{  1.00000000000000f,  -1.04800335126349f,   0.29156311971249f,  -0.26806001042947f,
   0.00819999645858f,   0.45054734505008f,  -0.33032403314006f,   0.06739368333110f,
  -0.04784254229033f,   0.01639907836189f,   0.01807364323573f                    },
{  1.00000000000000f,  -0.51035327095184f,  -0.31863563325245f,  -0.20256413484477f,
   0.14728154134330f,   0.38952639978999f,  -0.23313271880868f,  -0.05246019024463f,
  -0.02505961724053f,   0.02442357316099f,   0.01818801111503f                    },
{  1.00000000000000f,  -0.25049871956020f,  -0.43193942311114f,  -0.03424681017675f,
  -0.04678328784242f,   0.26408300200955f,   0.15113130533216f,  -0.17556493366449f,
  -0.18823009262115f,   0.05477720428674f,   0.04704409688120f                   }};

const float BYule[N_SUPPORTED_SAMPLE_RATES][YULE_TAPS] = {{
   0.03857599435200f,  -0.02160367184185f,  -0.00123395316851f,  -0.00009291677959f,
  -0.01655260341619f,   0.02161526843274f,  -0.02074045215285f,   0.00594298065125f,
   0.00306428023191f,   0.00012025322027f,   0.00288463683916f                    },
{  0.05418656406430f,  -0.02911007808948f,  -0.00848709379851f,  -0.00851165645469f,
  -0.00834990904936f,   0.02245293253339f,  -0.02596338512915f,   0.01624864962975f,
  -0.00240879051584f,   0.00674613682247f,  -0.00187763777362f                    },
{  0.15457299681924f,  -0.09331049056315f,  -0.06247880153653f,   0.02163541888798f,
  -0.05588393329856f,   0.04781476674921f,   0.00222312597743f,   0.03174092540049f,
  -0.01390589421898f,   0.00651420667831f,  -0.00881362733839f                    },
{  0.30296907319327f,  -0.22613988682123f,  -0.08587323730772f,   0.03282930172664f,
  -0.00915702933434f,  -0.02364141202522f,  -0.00584456039913f,   0.06276101321749f,
  -0.00000828086748f,   0.00205861885564f,  -0.02950134983287f                    },
{  0.33642304856132f,  -0.25572241425570f,  -0.11828570177555f,   0.11921148675203f,
  -0.07834489609479f,  -0.00469977914380f,  -0.00589500224440f,   0.05724228140351f,
   0.00832043980773f,  -0.01635381384540f,  -0.01760176568150f                    },
{  0.44915256608450f,  -0.14351757464547f,  -0.22784394429749f,  -0.01419140100551f,
   0.04078262797139f,  -0.12398163381748f,   0.04097565135648f,   0.10478503600251f,
  -0.01863887810927f,  -0.03193428438915f,   0.00541907748707f                    },
{  0.56619470757641f,  -0.75464456939302f,   0.16242137742230f,   0.16744243493672f,
  -0.18901604199609f,   0.30931782841830f,  -0.27562961986224f,   0.00647310677246f,
   0.08647503780351f,  -0.03788984554840f,  -0.00588215443421f                    },
{  0.58100494960553f,  -0.53174909058578f,  -0.14289799034253f,   0.17520704835522f,
   0.02377945217615f,   0.15558449135573f,  -0.25344790059353f,   0.01628462406333f,
   0.06920467763959f,  -0.03721611395801f,  -0.00749618797172f                    },
{  0.53648789255105f,  -0.42163034350696f,  -0.00275953611929f,   0.04267842219415f,
  -0.10214864179676f,   0.14590772289388f,  -0.02459864859345f,  -0.11202315195388f,
  -0.04060034127000f,   0.04788665548180f,  -0.02217936801134f                   }};

const float AButter[N_SUPPORTED_SAMPLE_RATES][BUTTER_TAPS] = {
	{ 1.00000000000000f, -1.97223372919527f, 0.97261396931306f },
	{ 1.00000000000000f, -1.96977855582618f, 0.97022847566350f },
	{ 1.00000000000000f, -1.95835380975398f, 0.95920349965459f },
	{ 1.00000000000000f, -1.95002759149878f, 0.95124613669835f },
	{ 1.00000000000000f, -1.94561023566527f, 0.94705070426118f },
	{ 1.00000000000000f, -1.92783286977036f, 0.93034775234268f },
	{ 1.00000000000000f, -1.91858953033784f, 0.92177618768381f },
	{ 1.00000000000000f, -1.91542108074780f, 0.91885558323625f },
	{ 1.00000000000000f, -1.88903307939452f, 0.89487434461664f },
};

const float BButter[N_SUPPORTED_SAMPLE_RATES][BUTTER_TAPS] = {
	{ 0.98621192462708f, -1.97242384925416f, 0.98621192462708f },
	{ 0.98500175787242f, -1.97000351574484f, 0.98500175787242f },
	{ 0.97938932735214f, -1.95877865470428f, 0.97938932735214f },
	{ 0.97531843204928f, -1.95063686409857f, 0.97531843204928f },
	{ 0.97316523498161f, -1.94633046996323f, 0.97316523498161f },
	{ 0.96454515552826f, -1.92909031105652f, 0.96454515552826f },
	{ 0.96009142950541f, -1.92018285901082f, 0.96009142950541f },
	{ 0.95856916599601f, -1.91713833199203f, 0.95856916599601f },
	{ 0.94597685600279f, -1.89195371200558f, 0.94597685600279f }
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

	uint32 best_diff = SAMPLERATE[0];
	uint32 best_match = 0;
	for(int i = 0; i < N_SUPPORTED_SAMPLE_RATES; ++i) {
		int64 diff64 = int64(SAMPLERATE[i]) - int64(audioformat.SampleRate);
		int32 diff32 = int32(diff64);
		uint32 diff = abs(diff32);
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
	boost::shared_array<uint8> buf(new uint8[n_noise_sample_bytes]);
	while((toread>0) && (!IIRButter->exhausted())) {
		toread -= IIRButter->getData(&buf[n_noise_sample_bytes-toread], toread);
	}
}

NormalizeFilter::~NormalizeFilter() {
	float pdb = 10.0f * log10(peak_amplitude); // peak_amplitude is squared
	float db  = 20.0f * log10(gain);
	cout << "Recommended gain: " << gain << " / " << showpos << db << noshowpos << " dB, peak amplitude: " << peak_amplitude << " / " << pdb << " dB FS" << endl;
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

	std::vector<float> levels(audioformat.Channels);
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
	gain  = std::min(gain, max_gain);
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
