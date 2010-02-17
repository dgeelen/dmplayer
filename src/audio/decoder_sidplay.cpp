#include "decoder_sidplay.h"
#include <sidplay/sid2types.h>
#include <sidplay/builders/resid.h>
#include "../error-handling.h"

#define MAX_SONG_DURATION (3*60) // Default to 3:00 minutes (like foo_sidplay default)

SidplayDecoder::SidplayDecoder(IDataSourceRef source)
: IDecoder(AudioFormat())
, datasource(source)
{
	// NOTE: We use 64k as an upperbound on the maximum size of SIDs
	//       that we will load, as this is the maximum C64 memory size.
	//       I don't know if larger sids exist (they could exist at 
	//       least in theory), but then they would not play on a real C64
	//       either.
	std::vector<uint8> buffer(64 * 1024); 
	uint32 pos = 0;
	datasource->reset();
	while((!datasource->exhausted()) && (pos < buffer.size())) {
		pos += datasource->getData(&buffer[pos], buffer.size() - pos);
	}
	tune = boost::shared_ptr<SidTune>(new SidTune(&buffer[0], pos));
	if((!tune) || !(*tune))
		throw Exception("Could not load tune");

	audioformat.BitsPerSample = 16;
	audioformat.Channels      = tune->isStereo() ? 2 : 1;
	audioformat.Float         = false;
	audioformat.LittleEndian  = true;
	audioformat.SignedSample  = true;
	#ifdef DEBUG
	audioformat.SampleRate    = 44100;
	#else
	audioformat.SampleRate    = 48000;
	#endif

	tune->selectSong(1);
	player.load(tune.get());
	dcerr("Playing sub-tune " << tune->getInfo().currentSong << "/" << tune->getInfo().songs);

	boost::shared_ptr<ReSIDBuilder> resid = boost::shared_ptr<ReSIDBuilder>(new ReSIDBuilder("ReSID"));
	if(!resid)
		throw Exception("Could not create ReSIDBuilder");

	resid->create(player.info().maxsids);
	#ifdef DEBUG
	resid->filter(false);
	#else
	resid->filter(true);
	#endif
	resid->sampling(audioformat.SampleRate);

	sidemu = resid;

	sid2_config_t cfg(player.config());
	cfg.playback      = tune->isStereo() ? sid2_stereo : sid2_mono;
	cfg.emulateStereo = tune->isStereo();
	cfg.frequency     = audioformat.SampleRate;
	cfg.sampleFormat  = SID2_LITTLE_SIGNED;
	cfg.precision     = 16;
	/* These should be configurable ... */
	cfg.clockSpeed    = SID2_CLOCK_CORRECT;
	cfg.clockForced   = true;
	cfg.sidModel      = SID2_MODEL_CORRECT;
#ifdef DEBUG
	cfg.optimisation  = SID2_MAX_OPTIMISATION;
#else
	cfg.optimisation  = SID2_DEFAULT_OPTIMISATION;
#endif
	cfg.sidSamples    = true;
	cfg.clockDefault  = SID2_CLOCK_PAL;
	cfg.sidDefault    = SID2_MOS6581;
	cfg.sidEmulation  = sidemu.get();
	player.config(cfg);	
}

SidplayDecoder::~SidplayDecoder() {
}

IDecoderRef SidplayDecoder::tryDecode(IDataSourceRef ds) {
	uint8 buf[4];
	if(ds->getData(buf, 4) == 4) {
		// Check for 'PSID' or 'RSID' marker
		if((buf[0] == 'P' || buf[0] == 'R') && buf[1] == 'S' && buf[2] == 'I' && buf[3] == 'D') {
			try {
				return IDecoderRef(new SidplayDecoder(ds));
			} catch(Exception e) {}
		}
	}
	return IDecoderRef();
}

bool SidplayDecoder::exhausted() {
	return 
		(tune->getInfo().currentSong == tune->getInfo().songs)
		&&
		((player.time() / player.timebase()) > MAX_SONG_DURATION);
}

uint32 SidplayDecoder::getData(unsigned char* buf, unsigned long len) {
	if(exhausted()) return 0;
	if((player.time() / player.timebase()) > MAX_SONG_DURATION) {
		// Not exhausted, so there are either still more songs to play,
		// or the current song still has time left to play. However since
		// the current song has elapsed it's time, there must be another
		// song left to play. So select it.
		tune->selectSong(tune->getInfo().currentSong + 1);
		player.stop();
		player.load(tune.get());
		dcerr("Playing sub-tune " << tune->getInfo().currentSong << "/" << tune->getInfo().songs);
	}
	long bytes_per_frame = audioformat.Channels * sizeof(int16);
	long nframes         = len / bytes_per_frame;
	return player.play(buf, nframes * audioformat.Channels * sizeof(int16));
}
