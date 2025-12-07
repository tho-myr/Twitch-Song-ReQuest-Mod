#pragma once


#include "SongListCell.hpp"
#include "HMUI/Touchable.hpp"
#include "bsml/shared/BSML.hpp"
#include "main.hpp"
#include "assets.hpp"
#include "beatsaverplusplus/shared/Models/Beatmap.hpp"

#include "UnityEngine/Sprite.hpp"

namespace TSRQ
{
    class SongListObject {
    public:
        std::optional<BeatSaver::Models::Beatmap> song;
        bool downloading = false;
        bool isDownloaded = false;
        bool failed = false;
        bool songNotFound = false;
        float progress = 0.0f;
        SafePtrUnity<UnityEngine::Sprite> cover;
        std::function<void(float)> progressUpdateCallback;

        void setSong(std::optional<BeatSaver::Models::Beatmap> song) {
            this->song = song;
        }

        void setIsDownloading(bool downloading) {
            this->downloading = downloading;
            if (downloading) {
                this->failed = false;
                this->progress = 0.0f;
            }
        }

        void setIsDownloaded(bool isDownloaded) {
            this->isDownloaded = isDownloaded;
        }
        
        void setFailed(bool failed) {
            this->failed = failed;
        }
    };
}