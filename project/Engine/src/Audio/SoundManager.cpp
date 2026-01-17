#include "Audio/SoundManager.h"
#include <cassert>

namespace {

#pragma pack(push, 1)
	// RIFFヘッダチャンク
	// struct RiffHeader {
	//  ChunkHeader chunk; //"RIFF"
	//  char type[4];      //"WAVE"
	//};

	struct RiffHeader {
		char chunkId[4]; // "RIFF"
		uint32_t size;   // file size - 8
		char format[4];  // "WAVE"
	};

	// チャンクヘッダ
	struct ChunkHeader {
		char id[4];   // チャンク毎のID
		int32_t size; // チャンクサイズ
	};

#pragma pack(pop)

	inline bool FourCCEquals(const char a[4], const char b[4]) {
		return std::memcmp(a, b, 4) == 0;
	}

	// WAVのチャンクは2バイト境界
	inline uint32_t Align2(uint32_t n) { return (n + 1u) & ~1u; }

	SoundManager::SoundData LoadWavInternal(const char* filename) {
		std::ifstream file(filename, std::ios::binary);
		assert(file && "Failed to open wav file");

		RiffHeader riff{};
		file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
		assert(file && "Failed to read RIFF header");
		assert(FourCCEquals(riff.chunkId, "RIFF") && "Not RIFF");
		assert(FourCCEquals(riff.format, "WAVE") && "Not WAVE");

		SoundManager::SoundData sd{};

		bool foundFmt = false;
		bool foundData = false;

		while (file && (!foundFmt || !foundData)) {
			ChunkHeader ch{};
			file.read(reinterpret_cast<char*>(&ch), sizeof(ch));
			if (!file) {
				break;
			}

			if (FourCCEquals(ch.id, "fmt ")) {
				// fmt chunk（WAVEFORMATEXより大きい場合があるので先頭だけ読む）
				const uint32_t readSize =
					(ch.size < sizeof(WAVEFORMATEX))
					? ch.size
					: static_cast<uint32_t>(sizeof(WAVEFORMATEX));

				file.read(reinterpret_cast<char*>(&sd.wfex), readSize);
				assert(file && "Failed to read fmt chunk");

				if (ch.size > readSize) {
					file.seekg(static_cast<std::streamoff>(ch.size - readSize),
						std::ios::cur);
				}

				const uint32_t pad = Align2(ch.size) - ch.size;
				if (pad) {
					file.seekg(pad, std::ios::cur);
				}

				foundFmt = true;

			} else if (FourCCEquals(ch.id, "data")) {
				sd.bufferSize = ch.size;
				sd.pBuffer = new BYTE[sd.bufferSize];
				file.read(reinterpret_cast<char*>(sd.pBuffer), sd.bufferSize);
				assert(file && "Failed to read data chunk");

				const uint32_t pad = Align2(ch.size) - ch.size;
				if (pad) {
					file.seekg(pad, std::ios::cur);
				}

				foundData = true;

			} else {
				// その他チャンクは飛ばす（JUNK/LIST/fact等）
				file.seekg(static_cast<std::streamoff>(Align2(ch.size)), std::ios::cur);
			}
		}

		assert(foundFmt && "fmt chunk not found");
		assert(foundData && "data chunk not found");

		return sd;
	}

	// FMTチャンク
	// struct FormatChunk {
	//  ChunkHeader chunk; //"fmt"
	//  WAVEFORMATEX fmt;  // 波形フォーマット
	//};
} // namespace

SoundManager* SoundManager::GetInstance() {
	static SoundManager instance;
	return &instance;
}

void SoundManager::Initialize(IXAudio2* xAudio2) {
	assert(xAudio2);
	xAudio2_ = xAudio2;
}

void SoundManager::Finalize() {
	// 再生中Voiceをまとめて破棄（簡易）
	// for (IXAudio2SourceVoice *v : activeVoices_) {
	//  if (!v) {
	//    continue;
	//  }
	//  v->Stop();
	//  v->FlushSourceBuffers();
	//  v->DestroyVoice();
	//}
	// activeVoices_.clear();

	// xAudio2_ = nullptr;

	StopBGM();

	// SE voice 破棄
	for (IXAudio2SourceVoice* v : seVoices_) {
		if (!v) {
			continue;
		}
		v->Stop();
		v->FlushSourceBuffers();
		v->DestroyVoice();
	}
	seVoices_.clear();

	// WAV解放
	for (auto& [key, sd] : sounds_) {
		delete[] sd.pBuffer;
		sd.pBuffer = nullptr;
		sd.bufferSize = 0;
		sd.wfex = {};
	}
	sounds_.clear();

	xAudio2_ = nullptr;
}

void SoundManager::Update() {
	// 再生が終わったSE voiceを回収
	auto it = seVoices_.begin();
	while (it != seVoices_.end()) {
		IXAudio2SourceVoice* v = *it;
		if (!v) {
			it = seVoices_.erase(it);
			continue;
		}

		XAUDIO2_VOICE_STATE state{};
		v->GetState(&state);

		if (state.BuffersQueued == 0) {
			v->Stop();
			v->FlushSourceBuffers();
			v->DestroyVoice();
			it = seVoices_.erase(it);
		} else {
			++it;
		}
	}
}

void SoundManager::Load(const std::string& key, const std::string& filename) {
	assert(xAudio2_ && "SoundManager::Initialize must be called before Load().");

	if (sounds_.find(key) != sounds_.end()) {
		return; // 既にあるなら何もしない（上書きしたいならここを変える）
	}

	SoundData sd = LoadWavInternal(filename.c_str());
	sounds_.emplace(key, sd);
}

void SoundManager::Unload(const std::string& key) {
	auto it = sounds_.find(key);
	if (it == sounds_.end()) {
		return;
	}

	if (bgmVoice_ && bgmKey_ == key) {
		StopBGM();
	}

	SoundData& sd = it->second;
	delete[] sd.pBuffer;
	sd.pBuffer = nullptr;
	sd.bufferSize = 0;
	sd.wfex = {};

	sounds_.erase(it);
}

void SoundManager::PlaySE(const std::string& key) {
	assert(xAudio2_ &&
		"SoundManager::Initialize must be called before PlaySE().");

	auto it = sounds_.find(key);
	assert(it != sounds_.end() && "Sound key not found. Call Load() first.");

	const SoundData& sd = it->second;
	assert(sd.pBuffer && sd.bufferSize > 0);

	IXAudio2SourceVoice* voice = nullptr;
	HRESULT hr = xAudio2_->CreateSourceVoice(&voice, &sd.wfex);
	assert(SUCCEEDED(hr));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = sd.pBuffer;
	buf.AudioBytes = sd.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	hr = voice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));
	hr = voice->Start();
	assert(SUCCEEDED(hr));

	// 再生終了後Updateで回収
	seVoices_.push_back(voice);
}

void SoundManager::PlayBGM(const std::string& key) {
	assert(xAudio2_ &&
		"SoundManager::Initialize must be called before PlayBGM().");

	auto it = sounds_.find(key);
	assert(it != sounds_.end() && "Sound key not found. Call Load() first.");

	if (bgmVoice_ && bgmKey_ == key) {
		return; // 既に同じBGMが鳴っている
	}

	StopBGM();

	const SoundData& sd = it->second;
	assert(sd.pBuffer && sd.bufferSize > 0);

	HRESULT hr = xAudio2_->CreateSourceVoice(&bgmVoice_, &sd.wfex);
	assert(SUCCEEDED(hr));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = sd.pBuffer;
	buf.AudioBytes = sd.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = XAUDIO2_LOOP_INFINITE;

	hr = bgmVoice_->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));
	hr = bgmVoice_->Start();
	assert(SUCCEEDED(hr));

	bgmKey_ = key;
}

void SoundManager::StopBGM() {
	if (!bgmVoice_) {
		bgmKey_.clear();
		return;
	}
	bgmVoice_->Stop();
	bgmVoice_->FlushSourceBuffers();
	bgmVoice_->DestroyVoice();
	bgmVoice_ = nullptr;
	bgmKey_.clear();
}

bool SoundManager::Exists(const std::string& key) const {
	return sounds_.find(key) != sounds_.end();
}

SoundManager::SoundData SoundManager::SoundLoadWave(const char* filename) {

	return LoadWavInternal(filename);

	///* ファイルオープン
	//----------------------*/

	//// ファイル入力ストリームのインスタンス
	// std::ifstream file;

	////.wavファイルをバイナリモードで開く
	// file.open(filename, std::ios_base::binary);

	//// ファイルが開けなかったら
	// assert(file.is_open());

	///* .wavデータ読み込み
	//----------------------*/

	//// RIFFヘッダーの読み込み
	// RiffHeader riff;

	// file.read((char *)&riff, sizeof(riff));

	//// 開いたファイルがRIFFであるかを確認する
	// if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
	//   assert(0);
	// }

	//// タイプがWAVEか確認
	// if (strncmp(riff.type, "WAVE", 4) != 0) {
	//   assert(0);
	// }

	//// formatチャンクの読み込み
	// FormatChunk format = {};

	//// チャンクヘッダーの確認
	// file.read((char *)&format, sizeof(ChunkHeader));

	// if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
	//   assert(0);
	// }

	//// チャンク本体の読み込み
	// assert(format.chunk.size <= sizeof(format.fmt));
	// file.read((char *)&format.fmt, format.chunk.size);

	//// Dataチャンクの読み込み
	// ChunkHeader data;

	// file.read((char *)&data, sizeof(data));

	//// Junkチャンク(パディング？)を検出した場合
	// if (strncmp(data.id, "JUNK", 4) == 0) {

	//  // 読み取り位置をJunkチャンクの終わりまで進める
	//  file.seekg(data.size, std::ios_base::cur);

	//  // 飛ばした後に再度読み込み
	//  file.read((char *)&data, sizeof(data));
	//}

	// if (strncmp(data.id, "data", 4) != 0) {
	//   assert(0);
	// }

	//// Dataチャンクのデータ部(波形データ)の読み込み
	// char *pBuffer = new char[data.size];
	// file.read(pBuffer, data.size);

	//// Waveファイルを閉じる
	// file.close();

	///* 読み込んだ音声データをreturn
	//-------------------------------*/

	//// returnするための音声データ
	// SoundData soundData = {};

	// soundData.wfex = format.fmt;
	// soundData.pBuffer = reinterpret_cast<BYTE *>(pBuffer);
	// soundData.bufferSize = data.size;

	// return soundData;
}

void SoundManager::SoundUnload(SoundData* soundData) {
	if (!soundData) {
		return;
	}
	// バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = nullptr;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

void SoundManager::SoundPlayWave(IXAudio2* xAudio2,
	const SoundData& soundData) {
	assert(xAudio2);
	assert(soundData.pBuffer && soundData.bufferSize > 0);

	IXAudio2SourceVoice* voice = nullptr;
	HRESULT hr = xAudio2->CreateSourceVoice(&voice, &soundData.wfex);
	assert(SUCCEEDED(hr));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	hr = voice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));
	hr = voice->Start();
	assert(SUCCEEDED(hr));

	seVoices_.push_back(voice);

	// HRESULT result;

	//// 波形フォーマットを元にSourceVoiceの生成
	// IXAudio2SourceVoice *pSourceVoice = nullptr;
	// result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	// assert(SUCCEEDED(result));

	//// 再生する波形データの設定
	// XAUDIO2_BUFFER buf{};
	// buf.pAudioData = soundData.pBuffer;
	// buf.AudioBytes = soundData.bufferSize;
	// buf.Flags = XAUDIO2_END_OF_STREAM;

	//// 波形データの再生
	// result = pSourceVoice->SubmitSourceBuffer(&buf);
	// result = pSourceVoice->Start();

	// activeVoices_.push_back(pSourceVoice);
}

void SoundManager::SoundPlayWave(const SoundData& soundData) {
	assert(xAudio2_ &&
		"SoundManager::Initialize must be called before SoundPlayWave().");
	SoundPlayWave(xAudio2_, soundData);
}

void SoundManager::StopAllSE() {
	// いま鳴っているSEをすべて停止し、voiceも破棄する
	for (IXAudio2SourceVoice* v : seVoices_) {
		if (!v) { continue; }
		v->Stop();
		v->FlushSourceBuffers();
		v->DestroyVoice();
	}
	seVoices_.clear();
}
