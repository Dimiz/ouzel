// Copyright 2015-2018 Elviss Strazdins. All rights reserved.

#include <stdexcept>
#include "VorbisSound.hpp"
#include "Audio.hpp"
#include "mixer/Source.hpp"
#include "mixer/SourceData.hpp"
#include "utils/Utils.hpp"

#include "stb_vorbis.c"

namespace ouzel
{
    namespace audio
    {
        class VorbisData;

        class VorbisSource: public Source
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

            void getData(std::vector<float>& samples, uint16_t& channels,
                         uint32_t& sampleRate, Vector3& position) override
            {
                uint32_t neededSize = static_cast<uint32_t>(samples.size());
                uint32_t totalSize = 0;

                while (neededSize > 0)
                {
                    if (vorbisStream->eof)
                        reset();

                    int resultFrames = stb_vorbis_get_samples_float_interleaved(vorbisStream, channels, samples.data(), static_cast<int>(neededSize));
                    totalSize += static_cast<uint32_t>(resultFrames) * channels;
                    neededSize -= static_cast<uint32_t>(resultFrames) * channels;

                    if (!isRepeating()) break;
                }

                if (vorbisStream->eof)
                    reset();

                std::fill(samples.begin() + totalSize, samples.end(), 0.0F);
            }

        private:
            stb_vorbis* vorbisStream = nullptr;
        };

        class VorbisData: public SourceData
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

            std::unique_ptr<Source> createSource() override
            {
                return std::unique_ptr<Source>(new VorbisSource(*this));
            }

        private:
            std::vector<uint8_t> data;
        };

        VorbisSource::VorbisSource(VorbisData& vorbisData):
            Source(vorbisData)
        {
            const std::vector<uint8_t>& data = vorbisData.getData();
            vorbisStream = stb_vorbis_open_memory(data.data(), static_cast<int>(data.size()), nullptr, nullptr);
        }

        VorbisSound::VorbisSound(Audio& initAudio, const std::vector<uint8_t>& initData):
            Sound(initAudio, initAudio.initSourceData(std::unique_ptr<SourceData>(new VorbisData(initData))))
        {

        }
    } // namespace audio
} // namespace ouzel
