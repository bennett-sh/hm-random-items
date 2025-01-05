#include "RandomItems.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZScene.h>
#include <Glacier/THashMap.h>
#include <Glacier/ZInventory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZContentKitManager.h>

#include <Glacier/ZGameStats.h>

void RandomItems::OnEngineInitialized() {
    Logger::Info("RandomItems has been initialized!");

    const ZMemberDelegate<RandomItems, void(const SGameUpdateEvent&)> s_Delegate(this, &RandomItems::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

RandomItems::~RandomItems() {
    const ZMemberDelegate<RandomItems, void(const SGameUpdateEvent&)> s_Delegate(this, &RandomItems::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void RandomItems::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_LOCAL_FIRE_DEPARTMENT " Random Items")) {
        m_ShowMessage = !m_ShowMessage;
    }
    
}

void RandomItems::OnDrawUI(bool p_HasFocus) {
    if (m_ShowMessage && p_HasFocus) {
        if (ImGui::Begin(ICON_MD_LOCAL_FIRE_DEPARTMENT " Random Items", &m_ShowMessage)) {
            if (/*m_SpawnInWorld*/ false) {
                ImGui::SetWindowSize(ImVec2(611, 359));
            }
            else {
                ImGui::SetWindowSize(ImVec2(427, 300));
            }

            if (ImGui::Button(m_Running ? "Stop" : "Start")) {
                if (!m_Running) LoadRepositoryProps();
                m_Running = !m_Running;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Starting for the first time might freeze the game for a few seconds.");
            }
            ImGui::InputDouble("Delay (in s)", &m_DelaySeconds);
            ImGui::Checkbox("Spawn in world", &m_SpawnInWorld);
            /*if (m_SpawnInWorld) {
                ImGui::InputFloat3("Item position", m_HitmanItemPosition);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("The position where the item will be spawned relative to the player position.");
                }
            }*/
            ImGui::SeparatorText("Experimental");
            ImGui::Checkbox("Include items without title", &m_IncludeItemsWithoutTitle);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("This will include more items, increasing the time to build the item pool and including some buggy items that can't actually spawn.");
            }
            if (ImGui::Button("Rebuild Item Pool")) {
                LoadRepositoryProps();
            }
        }
        ImGui::End();
    }
}

void RandomItems::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (!m_Running) return;

    m_ElapsedTime += p_UpdateEvent.m_GameTimeDelta.ToSeconds();
    if (m_ElapsedTime >= m_DelaySeconds)
    {
        GiveRandomItem();
        m_ElapsedTime = 0.0;
    }
}

std::pair<const std::string, ZRepositoryID> RandomItems::GetRepositoryPropFromIndex(int s_Index) {
    int s_CurrentIndex = 0;
    for (auto it = m_RepositoryProps.begin(); it != m_RepositoryProps.end(); ++it) {
        if (s_CurrentIndex == s_Index) {
            return *it;
        }
        ++s_CurrentIndex;
    }
    Logger::Error("repo index out of bounds");
    throw std::out_of_range("repo index out of bounds.");
}

void RandomItems::LoadRepositoryProps()
{
    Logger::Info("Loading repository (your game will freeze shortly)");

    std::string s_IncludedCategories[] = {
        "assaultrifle", "sniperrifle", "melee", "explosives", "tool", "pistol", "shotgun", "suitcase", "smg", "distraction", "poison", "container",
        "INVALID_CATEGORY_ICON" // <- debatable, makes it more random but also kind of wonky
    };

    if (m_RepositoryResource.m_nResourceIndex == -1)
    {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
    {
        const auto s_RepositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.GetResourceData());

        for (auto it = s_RepositoryData->begin(); it != s_RepositoryData->end(); ++it)
        {
            const ZDynamicObject* s_DynamicObject = &it->second;
            const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();

            std::string s_Id;

            bool s_HasTitle = false;
            bool s_Included = true;
            std::string s_TitleToAdd;
            ZRepositoryID s_RepoIdToAdd("");

            for (size_t i = 0; i < s_Entries->size(); ++i)
            {
                std::string s_Key = s_Entries->operator[](i).sKey.c_str();

                if (s_Key == "ID_")
                {
                    s_Id = ConvertDynamicObjectValueTString(s_Entries->at(i).value);
                }

                if (s_Key == "Title")
                {
                    s_HasTitle = true;
                    std::string s_Title = ConvertDynamicObjectValueTString(s_Entries->at(i).value);

                    s_TitleToAdd = s_Title;
                    s_RepoIdToAdd = ZRepositoryID(s_Id.c_str());

                    if (s_Title.size() < 1 && !m_IncludeItemsWithoutTitle) s_Included = false;
                }

                if (s_Key == "InventoryCategoryIcon") {
                    std::string s_Category = ConvertDynamicObjectValueTString(s_Entries->at(i).value);
                    bool s_CategoryMatched = false;

                    std::transform(s_Category.begin(), s_Category.end(), s_Category.begin(), ::toupper);

                    for (std::string s_IncludedCategory : s_IncludedCategories) {
                        std::transform(s_IncludedCategory.begin(), s_IncludedCategory.end(), s_IncludedCategory.begin(), ::toupper);
                        if (s_IncludedCategory == s_Category) s_CategoryMatched = true;
                    }

                    if (!s_CategoryMatched) s_Included = false;
                }

                if (s_Key == "IsHitmanSuit") {
                    s_Included = false;
                    break;
                }
            }

            if (s_Included && (s_HasTitle || m_IncludeItemsWithoutTitle)) {
                m_RepositoryProps.insert(std::make_pair(s_TitleToAdd, s_RepoIdToAdd));
            }
        }
    }
}

std::string RandomItems::ConvertDynamicObjectValueTString(const ZDynamicObject& p_DynamicObject)
{
    std::string s_Result;
    const IType* s_Type = p_DynamicObject.m_pTypeID->typeInfo();

    if (strcmp(s_Type->m_pTypeName, "ZString") == 0)
    {
        const auto s_Value = p_DynamicObject.As<ZString>();
        s_Result = s_Value->c_str();
    }
    else if (strcmp(s_Type->m_pTypeName, "bool") == 0)
    {
        if (*p_DynamicObject.As<bool>())
        {
            s_Result = "true";
        }
        else
        {
            s_Result = "false";
        }
    }
    else if (strcmp(s_Type->m_pTypeName, "float64") == 0)
    {
        double value = *p_DynamicObject.As<double>();

        s_Result = std::to_string(value).c_str();
    }
    else
    {
        s_Result = s_Type->m_pTypeName;
    }

    return s_Result;
}

void RandomItems::GiveRandomItem()
{
    if (m_RepositoryProps.size() == 0)
    {
        Logger::Info("loading repository props (your game might freeze shortly)");
        LoadRepositoryProps();
    }
    
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    size_t s_RandomIndex = std::rand() % m_RepositoryProps.size();
    auto s_PropPair = GetRepositoryPropFromIndex(s_RandomIndex);

    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    if(m_SpawnInWorld) {
        Logger::Info("Spawning in world: {}", s_PropPair.first);
        ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

        const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;
        if (!s_Scene) {
            Logger::Warn("no scene loaded");
            return;
        }

        const auto s_ItemSpawnerID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
        const auto s_ItemRepoKeyID = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

        TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

        Globals::ResourceManager->GetResourcePtr(s_Resource, s_ItemSpawnerID, 0);
        Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ItemRepoKeyID, 0);

        if (!s_Resource)
        {
            Logger::Error("resource not loaded");
            return;
        }

        ZEntityRef s_ItemSpawnerEntity, s_ItemRepoKey;

        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemSpawnerEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);
        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemRepoKey, "", s_Resource2, s_Scene.m_ref, nullptr, -1);

        if (!s_ItemSpawnerEntity)
        {
            Logger::Error("failed to spawn item spawner");
            return;
        }

        if (!s_ItemRepoKey)
        {
            Logger::Error("failed to spawn item repo key entity");
            return;
        }

        const auto s_ItemSpawner = s_ItemSpawnerEntity.QueryInterface<ZItemSpawner>();

        s_ItemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_KINEMATIC;
        s_ItemSpawner->m_rMainItemKey.m_ref = s_ItemRepoKey;
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef = s_ItemRepoKey.QueryInterface<ZItemRepositoryKeyEntity>();
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = s_PropPair.second;
        s_ItemSpawner->m_bUsePlacementAttach = false;
        s_ItemSpawner->m_eDisposalTypeOverwrite = EDisposalType::DISPOSAL_HIDE;
        s_ItemSpawner->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

        Functions::ZItemSpawner_RequestContentLoad->Call(s_ItemSpawner);
    }  else {
        Logger::Info("Adding to inventory: {} {}", s_PropPair.first, s_PropPair.second.ToString());
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(s_Inventory, s_PropPair.second, "", &s_ModifierIds, 2);
    }
}

DECLARE_ZHM_PLUGIN(RandomItems);
