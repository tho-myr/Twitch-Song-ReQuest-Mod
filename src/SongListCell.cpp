#include "SongListCell.hpp"

#include "UnityEngine/RectTransform.hpp"
#include "main.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"

DEFINE_TYPE(TSRQ, CustomSongListTableCell)
namespace TSRQ {
CustomSongListTableCell *CustomSongListTableCell::PopulateWithSongData(
    TSRQ::SongListObject *songListObject) {

  std::optional<BeatSaver::Models::Beatmap> song = songListObject->song;

  songName->set_text(song.value().GetName());
  levelAuthorName->set_text(song.value().GetMetadata().GetLevelAuthorName());

  if (songListObject->cover) {
      coverImage->set_sprite(songListObject->cover);
  } else {
      coverImage->set_sprite(nullptr);
  }

  songListObject->progressUpdateCallback = [this](float progress) {
       BSML::MainThreadScheduler::Schedule([this, progress] {
           UpdateProgress(progress);
       });
  };

  if (songListObject->isDownloaded) {
    statusLabel->set_text("In Collection");
  } else if (songListObject->downloading) {
    statusLabel->set_text(fmt::format("Downloading... {:.0f}%", songListObject->progress * 100));
  } else if (songListObject->failed) {
    statusLabel->set_text("Download Failed");
  } else {
    statusLabel->set_text("Click to download");
  }

  this->entry = songListObject;
  return this;
}

void CustomSongListTableCell::UpdateProgress(float progress) {
    if (entry && entry->downloading) {
        statusLabel->set_text(fmt::format("Downloading... {:.0f}%", progress * 100));
    }
}

void CustomSongListTableCell::RefreshBgState() {
  bgContainer->set_color(
      UnityEngine::Color(0, 0, 0, highlighted ? 0.8f : 0.45f));
}

// void CustomSongListTableCell::RefreshBar() {
//     if(!entry)
//         return;
//     auto clr = entry->status == DownloadHistoryEntry::Failed ?
//     UnityEngine::Color::get_red() : entry->status !=
//     DownloadHistoryEntry::DownloadStatus::Queued ?
//     UnityEngine::Color::get_green() : UnityEngine::Color::get_gray(); clr.a =
//     0.5f + (entry->downloadProgress * 0.4f); bgProgress->set_color(clr);

//     auto x =
//     reinterpret_cast<UnityEngine::RectTransform*>(bgProgress->get_gameObject()->get_transform());
//     if(!x)
//         return;
//     x->set_anchorMax(UnityEngine::Vector2(entry->downloadProgress, 1));
//     static auto forceUpdate = reinterpret_cast<function_ptr_t<void,
//     UnityEngine::RectTransform*>>(il2cpp_functions::resolve_icall("UnityEngine.RectTransform::ForceUpdateRectTransforms"));
//     forceUpdate(x);
// }

// void CustomSongListTableCell::UpdateProgress() {
//     statusLabel->set_text(entry->statusMessage());
//     RefreshBar();
// }

void CustomSongListTableCell::SelectionDidChange(
    HMUI::SelectableCell::TransitionType transitionType) {
  RefreshBgState();
}

void CustomSongListTableCell::HighlightDidChange(
    HMUI::SelectableCell::TransitionType transitionType) {
  RefreshBgState();
}

void CustomSongListTableCell::WasPreparedForReuse() {
  if (entry) {
      entry->progressUpdateCallback = nullptr;
  }
}
} // namespace TSRQ
