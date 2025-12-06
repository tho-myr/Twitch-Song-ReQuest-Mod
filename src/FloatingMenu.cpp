#include "FloatingMenu.hpp"

#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ScrollView.hpp"
#include "SongListObject.hpp"
#include "SongListTableData.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "assets.hpp"
#include "beatsaverplusplus/shared/BeatSaver.hpp"
#include "bsml/shared/BSML-Lite/Creation/Image.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Misc.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "logging.hpp"
#include "songcore/shared/SongCore.hpp"

#define coro(coroutine)                                                        \
  GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(     \
      custom_types::Helpers::CoroutineHelper::New(coroutine))
DEFINE_TYPE(TSRQ, FloatingMenu);

SafePtrUnity<TSRQ::FloatingMenu> TSRQ::FloatingMenu::instance;

using namespace GlobalNamespace;

// Runs on creation
void TSRQ::FloatingMenu::ctor() { this->cellSize = 14.0f; }

void TSRQ::FloatingMenu::Initialize() {
  if (initialized)
    return;

  auto floatingScreen = BSML::Lite::CreateFloatingScreen(
      UnityEngine::Vector2(80.0f, 76.0f),
      UnityEngine::Vector3(-2.0f, 3.6f, 3.5f),
      UnityEngine::Vector3(-36.0f, -34.0f, 0.0f), 0.0f, true, true,
      BSML::Side::Bottom);

  menu = floatingScreen->get_gameObject();

  BSML::parse_and_construct(IncludedAssets::menu_bsml, menu->get_transform(),
                            this);

  // QuestUI::BeatSaberUI::AddHoverHint(menu->get_transform()->get_gameObject(),
  // "Move by Pressing a trigger");

  BSML::Lite::AddHoverHint(menu, "Move by Pressing a trigger");

  BeatSaver::API::Init(SongCore::API::Loading::GetPreferredCustomLevelPath());

  if (this->songTableData != nullptr &&
      this->songTableData->m_CachedPtr.m_value != nullptr) {
    songTableData->tableView->SetDataSource(
        reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);
    INFO("TSRQ: SongTable Data Initialized");
  }

  // this->PostParse(); no need to call it
  this->RefreshTable();

  // Get coordinators
  soloFreePlayFlowCoordinator =
      UnityEngine::Object::FindObjectOfType<SoloFreePlayFlowCoordinator *>();
  multiplayerLevelSelectionFlowCoordinator =
      UnityEngine::Object::FindObjectOfType<
          MultiplayerLevelSelectionFlowCoordinator *>();

  initialized = true;
}

void TSRQ::FloatingMenu::PostParse() {
  // BSML has a bug that stops getting the correct platform helper and on game
  // reset it dies and the scrollhelper stays invalid and scroll doesn't work
  auto platformHelper = UnityEngine::Resources::FindObjectsOfTypeAll<
                            GlobalNamespace::LevelCollectionTableView *>()
                            ->First()
                            ->GetComponentInChildren<HMUI::ScrollView *>()
                            ->____platformHelper;
  if (platformHelper == nullptr) {
  } else {
    for (auto x : this->GetComponentsInChildren<HMUI::ScrollView *>()) {
      x->____platformHelper = platformHelper;
    }
  }
}

SafePtrUnity<TSRQ::FloatingMenu> TSRQ::FloatingMenu::get_instance() {
  if (instance)
    return instance;

  auto go = UnityEngine::GameObject::New_ctor(StringW("TSRQ Floating Menu"));
  Object::DontDestroyOnLoad(go);
  instance = go->AddComponent<TSRQ::FloatingMenu *>();
  return instance;
}

void TSRQ::FloatingMenu::delete_instance() {
  if (instance) {
    Object::DestroyImmediate(instance->get_gameObject());
    instance = nullptr;
  }
}

float TSRQ::FloatingMenu::CellSize() {
  INFO("TSRQ: Cell size {}", this->cellSize);
  return this->cellSize;
}

int TSRQ::FloatingMenu::NumberOfCells() {
  INFO("TSRQ: Number of cells {}", songList.size());
  return songList.size();
}

HMUI::TableCell *TSRQ::FloatingMenu::CellForIdx(HMUI::TableView *tableView,
                                                int idx) {
  return TSRQ::SongListTableData::GetCell(tableView)->PopulateWithSongData(
      songList[idx]);
}

void TSRQ::FloatingMenu::RefreshTable(bool fullReload) {
  INFO("TSRQ: RefreshTable");
  BSML::MainThreadScheduler::Schedule([this, fullReload] {
    // Sort entry list
    // std::stable_sort(downloadEntryList.begin(), downloadEntryList.end(),
    //     [] (DownloadHistoryEntry* entry1, DownloadHistoryEntry* entry2)
    //     {
    //         return (entry1->orderValue() < entry2->orderValue());
    //     }
    // );
    if (fullReload) {
        this->songListTable()->ReloadData();
    } else {
        this->songListTable()->ReloadDataKeepingPosition();
    }
    // coro(this->limitedFullTableReload->Call());
  });
}

void TSRQ::FloatingMenu::SelectSong(HMUI::TableView *table, int id) {
  INFO("TSRQ: Cell is clicked");

  this->songListTable()->ClearSelection();

  if (NumberOfCells() <= id) {
    INFO("How did you even click a non existent cell!!!?!!!??");
  }

  if (songList[id]->downloading == true || songList[id]->isDownloaded == true) {
    if (songList[id]->isDownloaded == true) {

      BeatSaver::Models::Beatmap songToPlay = songList[id]->song.value();

      BSML::MainThreadScheduler::Schedule([this, songToPlay, id] {
        auto versions = songToPlay.GetVersions();
        auto &beatmap = versions.front();
        std::string mapHash = beatmap.GetHash();
        auto level = SongCore::API::Loading::GetLevelByHash(mapHash);
        if (level != nullptr) {
          EnterSolo(level);
        } else {
          INFO("TSRQ: level is empty");
          songList[id]->setIsDownloaded(false);
          songList[id]->setFailed(true);
          this->RefreshTable(false);
          return;
        }
      });
    }

    return;
  }

  songList[id]->downloading = true;
  songList[id]->progress = 0.0f;
  this->RefreshTable(false);

  std::thread([this, id] {
    // Construct dl info and start dl
    auto dlInfo =
        BeatSaver::API::BeatmapDownloadInfo(songList[id]->song.value());
    
    auto progressCallback = [this, id](float progress) {
        BSML::MainThreadScheduler::Schedule([this, id, progress] {
            songList[id]->progress = progress;
            if(songList[id]->progressUpdateCallback) {
                songList[id]->progressUpdateCallback(progress);
            }
        });
    };

    std::optional<std::string> path = BeatSaver::API::DownloadBeatmap(dlInfo, progressCallback);

    if (path.has_value()) {
        auto task = SongCore::API::Loading::RefreshSongs(false);
        // Wait for songs to be refreshed
        task.wait();
    }

    BSML::MainThreadScheduler::Schedule([this, id, path] {
      songList[id]->setIsDownloading(false);
      if (path.has_value()) {
          songList[id]->setIsDownloaded(true);
      } else {
          songList[id]->setFailed(true);
      }
      this->RefreshTable(false);
    });
  }).detach();
}

void TSRQ::FloatingMenu::GoToSongSelect() {
  SafePtrUnity<UnityEngine::GameObject> songSelectButton =
      UnityEngine::GameObject::Find("SoloButton").unsafePtr();
  if (!songSelectButton) {
    songSelectButton = UnityEngine::GameObject::Find(
        "Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton");
  }
  if (!songSelectButton) {
    return;
  }
  songSelectButton->GetComponent<HMUI::NoTransitionsButton *>()->Press();
}

// !Run in the main thread
void TSRQ::FloatingMenu::EnterSolo(GlobalNamespace::BeatmapLevel *level) {

  // If we are in the song selection screen, we can just select the song
  SafePtrUnity<GlobalNamespace::LevelCollectionNavigationController>
      levelCollectionNavigationController =
          UnityEngine::Resources::FindObjectsOfTypeAll<
              GlobalNamespace::LevelCollectionNavigationController *>()
              ->FirstOrDefault();
  if (levelCollectionNavigationController) {
    INFO("TSRQ: levelCollectionNavigationController is not null");
    if (levelCollectionNavigationController->get_isActiveAndEnabled()) {
      INFO("TSRQ: levelCollectionNavigationController is active and enabled");
      // levelCollectionNavigationController->Selectp(level);

      // FIXME: Level selection does not reselect the level if it is already
      // selected
      levelCollectionNavigationController->SelectLevel(level);
      return;
    }
  }

  auto customLevelsPack = SongCore::API::Loading::GetCustomLevelPack();
  if (customLevelsPack == nullptr) {
    ERROR("CustomLevelsPack is null, refusing to continue");
    return;
  }
  if (customLevelsPack->___beatmapLevels->get_Length() == 0) {
    ERROR("CustomLevelsPack has no levels, refusing to continue");
    return;
  }

  auto category = SelectLevelCategoryViewController::LevelCategory(
      SelectLevelCategoryViewController::LevelCategory::All);

  auto levelCategory =
      System::Nullable_1<SelectLevelCategoryViewController::LevelCategory>();
  levelCategory.value = category;
  levelCategory.hasValue = true;

  auto state = LevelSelectionFlowCoordinator::State::New_ctor(
      customLevelsPack, static_cast<GlobalNamespace::BeatmapLevel *>(level));

  state->___levelCategory = levelCategory;

  multiplayerLevelSelectionFlowCoordinator
      ->LevelSelectionFlowCoordinator::Setup(state);
  soloFreePlayFlowCoordinator->Setup(state);

  GoToSongSelect();

  // For some reason setup does not work for multiplayer so I have to use this
  // method to workaround
  if (multiplayerLevelSelectionFlowCoordinator &&
      multiplayerLevelSelectionFlowCoordinator
          ->___levelSelectionNavigationController &&
      multiplayerLevelSelectionFlowCoordinator
          ->___levelSelectionNavigationController
          ->____levelCollectionNavigationController) {
    DEBUG("Selecting level in multiplayer");

    multiplayerLevelSelectionFlowCoordinator
        ->___levelSelectionNavigationController
        ->____levelCollectionNavigationController->SelectLevel(
            static_cast<GlobalNamespace::BeatmapLevel *>(level));
  };
}

void TSRQ::FloatingMenu::push(TSRQ::SongListObject *songListObject) {
  std::optional<BeatSaver::Models::Beatmap> song = songListObject->song;
  if (!song.has_value())
    return;
  
  if (song->GetVersions().empty()) {
      ERROR("Song has no versions!");
      return;
  }

  INFO("TSRQ: Pushing song {}", song->GetMetadata().GetSongName());

  SongCore::SongLoader::CustomBeatmapLevel *local =
      SongCore::API::Loading::GetLevelByHash(
          song->GetVersions().front().GetHash());

  if (local != nullptr) {
    // Verify that the level actually exists on disk
    std::string customLevelPath(local->get_customLevelPath());
    if (std::filesystem::exists(customLevelPath)) {
        songListObject->isDownloaded = true;
    }
  }

  // Fetch cover image
  std::thread([songListObject, this] {
      auto coverFuture = BeatSaver::API::GetCoverImageAsync(songListObject->song.value().GetVersions().front());
      auto coverData = coverFuture.get();
      if (coverData.IsSuccessful() && coverData.responseData.has_value()) {
           std::vector<uint8_t> bytes = coverData.responseData.value();
           BSML::MainThreadScheduler::Schedule([songListObject, bytes, this]() mutable {
               Array<uint8_t>* byteArray = il2cpp_utils::vectorToArray(bytes);
               UnityEngine::Texture2D* texture = UnityEngine::Texture2D::New_ctor(2, 2, UnityEngine::TextureFormat::RGBA32, false);
               UnityEngine::ImageConversion::LoadImage(texture, byteArray);
               UnityEngine::Sprite* sprite = UnityEngine::Sprite::Create(texture, UnityEngine::Rect(0, 0, texture->get_width(), texture->get_height()), UnityEngine::Vector2(0.5f, 0.5f), 100.0f, 0, UnityEngine::SpriteMeshType::FullRect, UnityEngine::Vector4::get_zero(), false);
               songListObject->cover = sprite;
               this->RefreshTable(false);
           });
      }
  }).detach();

  this->songList.insert(this->songList.begin(), songListObject);
  this->RefreshTable(false);
}