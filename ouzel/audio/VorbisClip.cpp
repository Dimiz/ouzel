// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#include <stdexcept>
#include "VorbisClip.hpp"
#include "Audio.hpp"
#include "mixer/Data.hpp"
#include "mixer/Stream.hpp"
#include "utils/Utils.hpp"

#include "stb_vorbis.c"

namespace ouzel
{
    namespace audio
    {
        class VorbisData;

        class VorbisSource final: public mixer::Stream
        {
        public:
            VorbisSource(VorbisData& vorbisData);

            ~VorbisSource()
            {
                if (vorbisStream)
                    stb_vorbis_close(vorbisStream);
            }

            void reset() override
            {
                stb_vorbis_seek_start(vorbisStream);
            }

            void getSamples(uint32_t frames, std::vector<float>& samples) override;

        private:
            stb_vorbis* vorbisStream = nullptr;
            std::vector<float> samples;
        };

        class VorbisData final: public mixer::Data
        {
        public:
            VorbisData(const std::vector<uint8_t>& initData):
                data(initData)
            {
                stb_vorbis* vorbisStream = stb_vorbis_open_memory(data.data(), static_cast<int>(data.size()), nullptr, nullptr);

                if (!vorbisStream)
                    throw std::runtime_error("Failed to load Vorbis stream");

                stb_vorbis_info info = stb_vorbis_get_info(vorbisStream);

                channels = static_cast<uint16_t>(info.channels);
                sampleRate = info.sample_rate;

                stb_vorbis_close(vorbisStream);
            }

            const std::vector<uint8_t>& getData() const { return data; }

            std::unique_ptr<mixer::Stream> createStream() override
            {
                return std::unique_ptr<mixer::Stream>(new VorbisSource(*this));
            }

        private:
            std::vector<uint8_t> data;
        };

        VorbisSource::VorbisSource(VorbisData& vorbisData):
            Stream(vorbisData)
        {
            vorbisStream = stb_vorbis_open_memory(vorbisData.getData().data(),
                                                  static_cast<int>(vorbisData.getData().size()),
                                                  nullptr, nullptr);
        }

        void VorbisSource::getSamples(uint32_t frames, std::vector<float>& samples)
        {
            uint32_t neededSize = frames * data.getChannels();
            samples.resize(neededSize);

            uint32_t copyFrames = 0;

            if (neededSize > 0)
            {
                if (vorbisStream->eof)
                    reset();

                samples.resize(neededSize);

                std::vector<float*> channelData(data.getChannels());
                for (uint32_t channel = 0; channel < data.getChannels(); ++channel)
                    channelData[channel] = &samples[channel * frames];

                int resultFrames = stb_vorbis_get_samples_float(vorbisStream, data.getChannels(),
                                                                channelData.data(), static_cast<int>(frames));
                copyFrames = static_cast<uint32_t>(resultFrames);

                switch (data.getChannels())
                {
                    case 1:
                        samples = samples;
                        break;
                    case 2:
                        for (uint32_t frame = 0; frame < frames; ++frame)
                        {
                            samples[0 * frames + frame] = channelData[0][frame];
                            samples[1 * frames + frame] = channelData[1][frame];
                        }
                        break;
                    case 4:
                        for (uint32_t frame = 0; frame < frames; ++frame)
                        {
                            samples[0 * frames + frame] = channelData[0][frame];
                            samples[1 * frames + frame] = channelData[1][frame];
                            samples[2 * frames + frame] = channelData[2][frame];
                            samples[3 * frames + frame] = channelData[3][frame];
                        }
                        break;
                    case 6:
                        for (uint32_t frame = 0; frame < frames; ++frame)
                        {
                            samples[0 * frames + frame] = channelData[0][frame];
                            samples[1 * frames + frame] = channelData[2][frame];
                            samples[2 * frames + frame] = channelData[1][frame];
                            samples[3 * frames + frame] = channelData[5][frame];
                            samples[4 * frames + frame] = channelData[3][frame];
                            samples[5 * frames + frame] = channelData[4][frame];
                        }
                        break;
                    default:
                        throw std::runtime_error("Unsupported channel count");
                }
            }

            if (vorbisStream->eof)
            {
                playing = false; // TODO: fire event
                reset();
            }

            for (uint32_t channel = 0; channel < data.getChannels(); ++channel)
                for (uint32_t frame = copyFrames; frame < frames; ++frame)
                    samples[channel * frames + frame] = 0.0F;
        }

        VorbisClip::VorbisClip(Audio& initAudio, const std::vector<uint8_t>& initData):
            Sound(initAudio, initAudio.initData([initData](){
                return std::unique_ptr<mixer::Data>(new VorbisData(initData));
            }))
        {
        }
    } // namespace audio
} // namespace ouzel
