/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "dreamweb/dreamweb.h"
#include "dreamweb/dreamgen.h"

#include "audio/mixer.h"
#include "audio/decoders/raw.h"

#include "common/config-manager.h"

namespace DreamGen {

void DreamGenContext::loadSpeech() {
	cancelCh1();
	data.byte(kSpeechloaded) = 0;
	createName();
	const char *name = (const char *)data.ptr(di, 13);
	//warning("name = %s", name);
	if (engine->loadSpeech(name))
		data.byte(kSpeechloaded) = 1;
}

void DreamBase::volumeAdjust() {
	if (data.byte(kVolumedirection) == 0)
		return;
	if (data.byte(kVolume) != data.byte(kVolumeto)) {
		data.byte(kVolumecount) += 64;
		// Only modify the volume every 256/64 = 4th time around
		if (data.byte(kVolumecount) == 0)
			data.byte(kVolume) += data.byte(kVolumedirection);
	} else {
		data.byte(kVolumedirection) = 0;
	}
}

void DreamBase::playChannel0(uint8 index, uint8 repeat) {
	if (data.byte(kSoundint) == 255)
		return;

	data.byte(kCh0playing) = index;
	Sound *soundBank;
	if (index >= 12) {
		soundBank = (Sound *)getSegment(data.word(kSounddata2)).ptr(0, 0);
		index -= 12;
	} else
		soundBank = (Sound *)getSegment(data.word(kSounddata)).ptr(0, 0);

	data.byte(kCh0repeat) = repeat;
	data.word(kCh0emmpage) = soundBank[index].emmPage;
	data.word(kCh0offset) = soundBank[index].offset();
	data.word(kCh0blockstocopy) = soundBank[index].blockCount();
	if (repeat) {
		data.word(kCh0oldemmpage) = data.word(kCh0emmpage);
		data.word(kCh0oldoffset) = data.word(kCh0offset);
		data.word(kCh0oldblockstocopy) = data.word(kCh0blockstocopy);
	}
}

void DreamGenContext::playChannel0() {
	playChannel0(al, ah);
}

void DreamBase::playChannel1(uint8 index) {
	if (data.byte(kSoundint) == 255)
		return;
	if (data.byte(kCh1playing) == 7)
		return;

	data.byte(kCh1playing) = index;
	Sound *soundBank;
	if (index >= 12) {
		soundBank = (Sound *)getSegment(data.word(kSounddata2)).ptr(0, 0);
		index -= 12;
	} else
		soundBank = (Sound *)getSegment(data.word(kSounddata)).ptr(0, 0);

	data.word(kCh1emmpage) = soundBank[index].emmPage;
	data.word(kCh1offset) = soundBank[index].offset();
	data.word(kCh1blockstocopy) = soundBank[index].blockCount();
}

void DreamGenContext::playChannel1() {
	playChannel1(al);
}

void DreamBase::cancelCh0() {
	data.byte(kCh0repeat) = 0;
	data.word(kCh0blockstocopy) = 0;
	data.byte(kCh0playing) = 255;
	engine->stopSound(0);
}

void DreamBase::cancelCh1() {
	data.word(kCh1blockstocopy) = 0;
	data.byte(kCh1playing) = 255;
	engine->stopSound(1);
}

void DreamBase::loadRoomsSample() {
	uint8 sample = data.byte(kRoomssample);

	if (sample == 255 || data.byte(kCurrentsample) == sample)
		return; // loaded already

	assert(sample < 100);
	Common::String sampleName = Common::String::format("DREAMWEB.V%02d", sample);

	uint8 ch0 = data.byte(kCh0playing);
	if (ch0 >= 12 && ch0 != 255)
		cancelCh0();
	uint8 ch1 = data.byte(kCh1playing);
	if (ch1 >= 12)
		cancelCh1();
	engine->loadSounds(1, sampleName.c_str());
}

} // End of namespace DreamGen


namespace DreamWeb {

void DreamWebEngine::playSound(uint8 channel, uint8 id, uint8 loops) {
	debug(1, "playSound(%u, %u, %u)", channel, id, loops);

	int bank = 0;
	bool speech = false;
	Audio::Mixer::SoundType type = channel == 0?
		Audio::Mixer::kMusicSoundType: Audio::Mixer::kSFXSoundType;

	if (id >= 12) {
		id -= 12;
		bank = 1;
		if (id == 50) {
			speech = true;
			type = Audio::Mixer::kSpeechSoundType;
		}
	}
	const SoundData &data = _soundData[bank];

	Audio::SeekableAudioStream *raw;
	if (!speech) {
		if (id >= data.samples.size() || data.samples[id].size == 0) {
			warning("invalid sample #%u played", id);
			return;
		}

		const Sample &sample = data.samples[id];
		uint8 *buffer = (uint8 *)malloc(sample.size);
		if (!buffer)
			error("out of memory: cannot allocate memory for sound(%u bytes)", sample.size);
		memcpy(buffer, data.data.begin() + sample.offset, sample.size);

		raw = Audio::makeRawStream(
			buffer,
			sample.size, 22050, Audio::FLAG_UNSIGNED);
	} else {
		uint8 *buffer = (uint8 *)malloc(_speechData.size());
		if (!buffer)
			error("out of memory: cannot allocate memory for sound(%u bytes)", _speechData.size());
		memcpy(buffer, _speechData.begin(), _speechData.size());
		raw = Audio::makeRawStream(
			buffer,
			_speechData.size(), 22050, Audio::FLAG_UNSIGNED);

	}

	Audio::AudioStream *stream;
	if (loops > 1) {
		stream = new Audio::LoopingAudioStream(raw, loops < 255? loops: 0);
	} else
		stream = raw;

	if (_mixer->isSoundHandleActive(_channelHandle[channel]))
		_mixer->stopHandle(_channelHandle[channel]);
	_mixer->playStream(type, &_channelHandle[channel], stream);
}

void DreamWebEngine::stopSound(uint8 channel) {
	debug(1, "stopSound(%u)", channel);
	assert(channel == 0 || channel == 1);
	_mixer->stopHandle(_channelHandle[channel]);
	if (channel == 0)
		_channel0 = 0;
	else
		_channel1 = 0;
}

bool DreamWebEngine::loadSpeech(const Common::String &filename) {
	if (ConfMan.getBool("speech_mute"))
		return false;

	Common::File file;
	if (!file.open("speech/" + filename))
		return false;

	debug(1, "loadSpeech(%s)", filename.c_str());

	uint size = file.size();
	_speechData.resize(size);
	file.read(_speechData.begin(), size);
	file.close();
	return true;
}

void DreamWebEngine::soundHandler() {
	_base.data.byte(DreamGen::kSubtitles) = ConfMan.getBool("subtitles");
	_base.volumeAdjust();

	uint volume = _base.data.byte(DreamGen::kVolume);
	//.vol file loaded into soundbuf:0x4000
	//volume table at (volume * 0x100 + 0x3f00)
	//volume value could be from 1 to 7
	//1 - 0x10-0xff
	//2 - 0x1f-0xdf
	//3 - 0x2f-0xd0
	//4 - 0x3e-0xc1
	//5 - 0x4d-0xb2
	//6 - 0x5d-0xa2
	//7 - 0x6f-0x91
	if (volume >= 8)
		volume = 7;
	volume = (8 - volume) * Audio::Mixer::kMaxChannelVolume / 8;
	_mixer->setChannelVolume(_channelHandle[0], volume);

	uint8 ch0 = _base.data.byte(DreamGen::kCh0playing);
	if (ch0 == 255)
		ch0 = 0;
	uint8 ch1 = _base.data.byte(DreamGen::kCh1playing);
	if (ch1 == 255)
		ch1 = 0;
	uint8 ch0loop = _base.data.byte(DreamGen::kCh0repeat);

	if (_channel0 != ch0) {
		_channel0 = ch0;
		if (ch0) {
			playSound(0, ch0, ch0loop);
		}
	}
	if (_channel1 != ch1) {
		_channel1 = ch1;
		if (ch1) {
			playSound(1, ch1, 1);
		}
	}
	if (!_mixer->isSoundHandleActive(_channelHandle[0])) {
		_base.data.byte(DreamGen::kCh0playing) = 255;
		_channel0 = 0;
	}
	if (!_mixer->isSoundHandleActive(_channelHandle[1])) {
		_base.data.byte(DreamGen::kCh1playing) = 255;
		_channel1 = 0;
	}

}

void DreamWebEngine::loadSounds(uint bank, const Common::String &filename) {
	debug(1, "loadSounds(%u, %s)", bank, filename.c_str());
	Common::File file;
	if (!file.open(filename)) {
		warning("cannot open %s", filename.c_str());
		return;
	}

	uint8 header[0x60];
	file.read(header, sizeof(header));
	uint tablesize = READ_LE_UINT16(header + 0x32);
	debug(1, "table size = %u", tablesize);

	SoundData &soundData = _soundData[bank];
	soundData.samples.resize(tablesize / 6);
	uint total = 0;
	for(uint i = 0; i < tablesize / 6; ++i) {
		uint8 entry[6];
		Sample &sample = soundData.samples[i];
		file.read(entry, sizeof(entry));
		sample.offset = entry[0] * 0x4000 + READ_LE_UINT16(entry + 1);
		sample.size = READ_LE_UINT16(entry + 3) * 0x800;
		total += sample.size;
		debug(1, "offset: %08x, size: %u", sample.offset, sample.size);
	}
	soundData.data.resize(total);
	file.read(soundData.data.begin(), total);
	file.close();
}

} // End of namespace DreamWeb