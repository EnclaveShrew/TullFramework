#include "nlohmann/json.hpp"
#include <Windows.h>
#include <chrono>
#include <fstream>
#include "SimpleIni.h"
using namespace RE;
using namespace std::chrono;
using std::vector;
using std::ifstream;
using std::unordered_map;
using std::pair;
using std::string;

char tempbuf[8192] = { 0 };
#pragma region Utilities
char* _MESSAGE(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(tempbuf, sizeof(tempbuf), fmt, args);
	va_end(args);
	spdlog::log(spdlog::level::warn, tempbuf);

	return tempbuf;
}

void Dump(const void* mem, unsigned int size)
{
	const char* p = static_cast<const char*>(mem);
	unsigned char* up = (unsigned char*)p;
	std::stringstream stream;
	int row = 0;
	for (unsigned int i = 0; i < size; i++) {
		stream << std::setfill('0') << std::setw(2) << std::hex << (int)up[i] << " ";
		if (i % 8 == 7) {
			stream << "\t0x"
				   << std::setw(2) << std::hex << (int)up[i]
				   << std::setw(2) << (int)up[i - 1]
				   << std::setw(2) << (int)up[i - 2]
				   << std::setw(2) << (int)up[i - 3]
				   << std::setw(2) << (int)up[i - 4]
				   << std::setw(2) << (int)up[i - 5]
				   << std::setw(2) << (int)up[i - 6]
				   << std::setw(2) << (int)up[i - 7] << std::setfill('0');
			stream << "\t0x" << std::setw(2) << std::hex << row * 8 << std::setfill('0');
			_MESSAGE("%s", stream.str().c_str());
			stream.str(std::string());
			row++;
		}
	}
}

template <class Ty>
Ty SafeWrite64(uintptr_t addr, Ty data)
{
	DWORD oldProtect;
	size_t len = sizeof(data);

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	Ty olddata = *(Ty*)addr;
	memcpy((void*)addr, &data, len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
	return olddata;
}

template <class Ty>
Ty SafeWrite64Function(uintptr_t addr, Ty data)
{
	DWORD oldProtect;
	void* _d[2];
	memcpy(_d, &data, sizeof(data));
	size_t len = sizeof(_d[0]);

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	Ty olddata;
	memset(&olddata, 0, sizeof(Ty));
	memcpy(&olddata, (void*)addr, len);
	memcpy((void*)addr, &_d[0], len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
	return olddata;
}

ActorValueInfo* GetAVIFByEditorID(std::string editorID)
{
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	BSTArray<ActorValueInfo*> avifs = dh->GetFormArray<ActorValueInfo>();
	for (auto it = avifs.begin(); it != avifs.end(); ++it) {
		if (strcmp((*it)->formEditorID.c_str(), editorID.c_str()) == 0) {
			return (*it);
		}
	}
	return nullptr;
}

BGSExplosion* GetExplosionByFullName(std::string explosionname)
{
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	BSTArray<BGSExplosion*> explosions = dh->GetFormArray<BGSExplosion>();
	for (auto it = explosions.begin(); it != explosions.end(); ++it) {
		if (strcmp((*it)->GetFullName(), explosionname.c_str()) == 0) {
			return (*it);
		}
	}
	return nullptr;
}

SpellItem* GetSpellByFullName(std::string spellname)
{
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	BSTArray<SpellItem*> spells = dh->GetFormArray<SpellItem>();
	for (auto it = spells.begin(); it != spells.end(); ++it) {
		if (strcmp((*it)->GetFullName(), spellname.c_str()) == 0) {
			return (*it);
		}
	}
	return nullptr;
}

TESForm* GetFormFromMod(std::string modname, uint32_t formid)
{
	if (!modname.length() || !formid)
		return nullptr;
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	TESFile* modFile = nullptr;
	for (auto it = dh->files.begin(); it != dh->files.end(); ++it) {
		TESFile* f = *it;
		if (strcmp(f->filename, modname.c_str()) == 0) {
			modFile = f;
			break;
		}
	}
	if (!modFile)
		return nullptr;
	uint8_t modIndex = modFile->compileIndex;
	uint32_t id = formid;
	if (modIndex < 0xFE) {
		id |= ((uint32_t)modIndex) << 24;
	} else {
		uint16_t lightModIndex = modFile->smallFileCompileIndex;
		if (lightModIndex != 0xFFFF) {
			id |= 0xFE000000 | (uint32_t(lightModIndex) << 12);
		}
	}
	return TESForm::GetFormByID(id);
}

bool CheckPA(Actor* a)
{
	if (!a->extraList) {
		return false;
	}
	return a->extraList->HasType(EXTRA_DATA_TYPE::kPowerArmor);
	;
}

namespace F4
{
	struct Unk
	{
		uint32_t unk00 = 0xFFFFFFFF;
		uint32_t unk04 = 0x0;
		uint32_t unk08 = 1;
	};

	bool PlaySound(BGSSoundDescriptorForm* sndr, NiPoint3 pos, NiAVObject* node)
	{
		typedef bool* func_t(Unk, BGSSoundDescriptorForm*, NiPoint3, NiAVObject*);
		REL::Relocation<func_t> func{ REL::ID(376497) };
		Unk u;
		return func(u, sndr, pos, node);
	}

	void ShakeCamera(float mul, NiPoint3 origin, float duration, float strength)
	{
		using func_t = decltype(&F4::ShakeCamera);
		REL::Relocation<func_t> func{ REL::ID(758209) };
		return func(mul, origin, duration, strength);
	}

	void ApplyImageSpaceModifier(TESImageSpaceModifier* imod, float strength, NiAVObject* target)
	{
		using func_t = decltype(&F4::ApplyImageSpaceModifier);
		REL::Relocation<func_t> func{ REL::ID(179769) };
		return func(imod, strength, target);
	}

	class TaskQueueInterface
	{
	public:
		void __fastcall QueueRebuildBendableSpline(TESObjectREFR* ref, bool rebuildCollision, NiAVObject* target)
		{
			using func_t = decltype(&F4::TaskQueueInterface::QueueRebuildBendableSpline);
			REL::Relocation<func_t> func{ REL::ID(198419) };
			return func(this, ref, rebuildCollision, target);
		}
		void QueueShow1stPerson(bool shouldShow)
		{
			using func_t = decltype(&F4::TaskQueueInterface::QueueShow1stPerson);
			REL::Relocation<func_t> func{ REL::ID(994377) };
			return func(this, shouldShow);
		}
	};
	REL::Relocation<TaskQueueInterface**> ptr_TaskQueueInterface{ REL::ID(7491) };
}

#pragma endregion

struct TESEquipEvent
{
	Actor* a;         //00
	uint32_t formId;  //0C
	uint32_t unk08;   //08
	uint64_t flag;    //10 0x00000000ff000000 for unequip
};

class EquipEventSource : public BSTEventSource<TESEquipEvent>
{
public:
	[[nodiscard]] static EquipEventSource* GetSingleton()
	{
		REL::Relocation<EquipEventSource*> singleton{ REL::ID(485633) };
		return singleton.get();
	}
};

struct SideAimSightData
{
	NiPoint3 SideAimZoomData;
	SideAimSightData(NiPoint3 _S)
	{
		SideAimZoomData = _S;
	}

};




#pragma region Variables
	CSimpleIniA ini(true, false, false);
	bool buttonDevMode = false;
	uint32_t XKey = 0x58;
	uint32_t LALTKey = 0xA4; 
	int FireSelectorState = 0;
	uint32_t lastKey;
	int keyPressedCount = 0;
	std::chrono::system_clock::duration lastKeyTime;
	int numKeyPress = 2;
	float keyTimeout = 0.1f;
	PlayerCharacter* p;
	PlayerCamera* pcam;
	BGSKeyword* TullAKKeyword;
	BGSKeyword* s_30_Auto;
	BGSKeyword* WeaponTypeAutomatic;
	BGSKeyword* sideAimAnimationKeyword;
	BGSKeyword* TullFrameworkSideAimSightKeyword;
	bool isLALTPress;
	bool sideAim = false;
	const F4SE::TaskInterface* taskInterface;
	bool SideAimReady;
	NiPoint3 Old_ZoomData;
	NiPoint3 FinalZoomData;
	float Old_FOVMult;
	unordered_map<TESForm*, vector<SideAimSightData>> CustomSideAimSightData;
	BGSSoundDescriptorForm* FireSelectorSound;
	SpellItem* FireSelectorStatusAutoMessageSpell;
	SpellItem* FireSelectorStatusSingleMessageSpell;
	uint32_t FireSelectorKey = 0x58;
	uint32_t SideAimKey1 = 0xA4;
	uint32_t SideAimKey2 = 0x58;
	bool EnableSideAimCombineKey = true;
#pragma endregion

void NotifyWeaponGraphManager(std::string evn)
	{
		if (p->GetBiped(true)) {
			IAnimationGraphManagerHolder* holder =
				(IAnimationGraphManagerHolder*)p->GetBiped(true)->object[(uint32_t)BIPED_OBJECT::kWeaponGun].objectGraphManager.get();
			if (holder) {
				holder->NotifyAnimationGraphImpl(evn);
			}
		}
	}

	class SideAimDelegate : public F4SE::ITaskDelegate
	{
	public:
		virtual void Run() override
		{
			p->NotifyAnimationGraphImpl("00NextClip");
		}
	};

	class SideAimDelegate2 : public F4SE::ITaskDelegate
	{
	public:
		virtual void Run() override
		{
			p->flags |= 0x100;
			if (sideAim) {
				NotifyWeaponGraphManager("Extend");
			}
		}
	};

SideAimDelegate2* SideAimSTS;

void SwitchFireMode()
	{
		if (p->currentProcess && p->currentProcess->middleHigh) {
			BSTArray<EquippedItem> equipped = p->currentProcess->middleHigh->equippedItems;
			if (equipped.size() != 0 && equipped[0].item.instanceData) {
				TESObjectWEAP::InstanceData* instance = (TESObjectWEAP::InstanceData*)equipped[0].item.instanceData.get();
				if (instance) {
					if ((instance->flags & 0x8000) == 0x8000) {
						instance->flags &= ~0x8000;
						if (instance->keywords->HasKeyword(s_30_Auto, instance)) {
							instance->keywords->RemoveKeyword(s_30_Auto);
							instance->keywords->RemoveKeyword(WeaponTypeAutomatic);
							BGSObjectInstance* obj = &equipped[0].item;
							((EquippedWeaponData*)(equipped[0].data.get()))->SetupFireSounds(*p, *(BGSObjectInstanceT<TESObjectWEAP>*)obj);
							F4::PlaySound(FireSelectorSound, p->data.location, pcam->cameraRoot.get());
							FireSelectorStatusSingleMessageSpell->Cast(p,p);
							_MESSAGE("Auto keyword removed");
						}
						_MESSAGE("Switched to semi-auto");
					} else {
						instance->flags |= 0x8000;
						if (!instance->keywords->HasKeyword(s_30_Auto, instance)) {
							instance->keywords->AddKeyword(s_30_Auto);
							instance->keywords->AddKeyword(WeaponTypeAutomatic);
							BGSObjectInstance* obj = &equipped[0].item;
							((EquippedWeaponData*)(equipped[0].data.get()))->SetupFireSounds(*p, *(BGSObjectInstanceT<TESObjectWEAP>*)obj);
							F4::PlaySound(FireSelectorSound, p->data.location, pcam->cameraRoot.get());
							FireSelectorStatusAutoMessageSpell->Cast(p,p);
							_MESSAGE("Auto keyword added");
						}
						_MESSAGE("Switched to auto");
					}
				}
			}
		}
	}

void SwitchSideAim()
	{
		if (p->currentProcess && p->currentProcess->middleHigh) {
			BSTArray<EquippedItem> equipped = p->currentProcess->middleHigh->equippedItems;
			if (equipped.size() != 0 && equipped[0].item.instanceData) {
				TESObjectWEAP::InstanceData* instance = (TESObjectWEAP::InstanceData*)equipped[0].item.instanceData.get();
				if (instance && instance->keywords->HasKeyword(TullFrameworkSideAimSightKeyword, instance)) {
					if (instance->keywords->HasKeyword(sideAimAnimationKeyword, instance)) {
						instance->keywords->RemoveKeyword(sideAimAnimationKeyword);
						instance->zoomData->zoomData.fovMult = Old_FOVMult;
						instance->zoomData->zoomData.cameraOffset = Old_ZoomData;
						NotifyWeaponGraphManager("Retract");
						_MESSAGE("Switched to normal");
						sideAim = false;
					} else {
						instance->keywords->AddKeyword(sideAimAnimationKeyword);	
						Old_ZoomData = instance->zoomData->zoomData.cameraOffset;
						Old_FOVMult = instance->zoomData->zoomData.fovMult;
						instance->zoomData->zoomData.fovMult = 1;
						instance->zoomData->zoomData.cameraOffset = FinalZoomData;
						_MESSAGE("FinalZoomData %f %f %f\n", FinalZoomData.x, FinalZoomData.y, FinalZoomData.z);
						NotifyWeaponGraphManager("Extend");
						_MESSAGE("Switched to side aim");
						sideAim = true;
					}
					p->HandleItemEquip(false);
					if (pcam->currentState == pcam->cameraStates[CameraState::kFirstPerson]) {
						p->flags &= ~0x100;
						p->NotifyAnimationGraphImpl("00NextClip");
						std::thread([]() -> void {
							std::this_thread::sleep_for(std::chrono::milliseconds(20));
							taskInterface->AddTask(new SideAimDelegate());
							std::this_thread::sleep_for(std::chrono::milliseconds(30));
							F4::TaskQueueInterface* queue = *F4::ptr_TaskQueueInterface;
							queue->QueueShow1stPerson(false);
							queue->QueueShow1stPerson(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							taskInterface->AddTask(new SideAimDelegate2());
						}).detach();
					}
				}
			}
		}
	}


class TullAKFireSelectorHandler : public BSInputEventReceiver
{
public:
	typedef void (TullAKFireSelectorHandler::*FnPerformInputProcessing)(const InputEvent* a_queueHead);

	void HandleMultipleButtonEvent(const ButtonEvent* evn)
	{
		if (evn->eventType != INPUT_EVENT_TYPE::kButton) {
			if (evn->next)
				HandleMultipleButtonEvent((ButtonEvent*)evn->next);
			return;
		}
		uint32_t id = evn->idCode;
		if (evn->device == INPUT_DEVICE::kMouse)
			id += 256;

		if (evn->value == 1.0f && evn->heldDownSecs == 0.0f && id == FireSelectorKey && SideAimReady == false) {
			SwitchFireMode();
		} 

		if (EnableSideAimCombineKey && evn->value == 1.0f && evn->heldDownSecs >= 0.0f && id == SideAimKey1) {
			isLALTPress = true;
			SideAimReady = true;

		} else if (EnableSideAimCombineKey && evn->value == 0.0f && id == SideAimKey1) {
			isLALTPress = false;
			SideAimReady = false;
		}

		if (EnableSideAimCombineKey && evn->value == 1.0f && evn->heldDownSecs == 0.0f && id == SideAimKey2 && isLALTPress == true && SideAimReady == true) {
			if (p->gunState == 0) {
				SwitchSideAim();
			}
		}

		if (!EnableSideAimCombineKey && evn->value == 1.0f && evn->heldDownSecs == 0.0f && id == SideAimKey1) {
			if (p->gunState == 0) {
				SwitchSideAim();
			}
		}

		if (evn->next)
			HandleMultipleButtonEvent((ButtonEvent*)evn->next);
	}

	void HookedPerformInputProcessing(const InputEvent* a_queueHead)
	{
		if (!UI::GetSingleton()->menuMode && a_queueHead) {
			HandleMultipleButtonEvent((ButtonEvent*)a_queueHead);
		}
		FnPerformInputProcessing fn = fnHash.at(*(uint64_t*)this);
		if (fn) {
			(this->*fn)(a_queueHead);
		}
	}

	void HookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end()) {
			FnPerformInputProcessing fn = SafeWrite64Function(vtable, &TullAKFireSelectorHandler::HookedPerformInputProcessing);
			fnHash.insert(std::pair<uint64_t, FnPerformInputProcessing>(vtable, fn));
		}
	}

	void UnHookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end())
			return;
		SafeWrite64Function(vtable, it->second);
		fnHash.erase(it);
	}

protected:
	static unordered_map<uint64_t, FnPerformInputProcessing> fnHash;
};
unordered_map<uint64_t, TullAKFireSelectorHandler::FnPerformInputProcessing> TullAKFireSelectorHandler::fnHash;


void RegisterZoomData(auto iter, TESForm* form, unordered_map<TESForm*, vector<SideAimSightData>>& map)
{
	float x = 0;
	float y = 0;
	float z = 0;
	for (auto Zoomdata = iter.value().begin(); Zoomdata != iter.value().end(); ++Zoomdata) {

		if (Zoomdata.key() == "X") {
			x = Zoomdata.value().get<float>();
		} else if (Zoomdata.key() == "Y") {
			y = Zoomdata.value().get<float>();
		} else if (Zoomdata.key() == "Z") {
			z = Zoomdata.value().get<float>();
		}

		auto existit = map.find(form);
		if (existit == map.end()) {
			map.insert(pair<TESForm*, vector<SideAimSightData>>(form, vector<SideAimSightData>{SideAimSightData(NiPoint3(x, y, z)) }));
		} else {
			existit->second.push_back(SideAimSightData(NiPoint3(x, y, z)));
		}
	}
}


void CompareEquippedWeaponModsAndMap(TESObjectWEAP* currwep)
{
	
	if (!p->inventoryList) {
		return;
	}
	for (auto invitem = p->inventoryList->data.begin(); invitem != p->inventoryList->data.end(); ++invitem) {
		if (invitem->object->formType == ENUM_FORM_ID::kWEAP) {
			TESObjectWEAP* wep = (TESObjectWEAP*)(invitem->object);
			if (invitem->stackData->IsEquipped() && wep == currwep) {
				if (invitem->stackData->extra) {
					BGSObjectInstanceExtra* extraData = (BGSObjectInstanceExtra*)invitem->stackData->extra->GetByType(EXTRA_DATA_TYPE::kObjectInstance);
					if (extraData) {
						auto data = extraData->values;
						if (data && data->buffer) {
							uintptr_t buf = (uintptr_t)(data->buffer);
							for (uint32_t i = 0; i < data->size / 0x8; i++) {
								TESForm* omod = TESForm::GetFormByID(*(uint32_t*)(buf + i * 0x8));
								auto omodit = CustomSideAimSightData.find(omod);
								if (omodit != CustomSideAimSightData.end()) {
									for (auto it = omodit->second.begin(); it != omodit->second.end(); ++it) {
										FinalZoomData.x = it->SideAimZoomData.x;
										_MESSAGE("x : %f", it->SideAimZoomData.x);
										FinalZoomData.y = it->SideAimZoomData.y;
										_MESSAGE("y : %f", it->SideAimZoomData.y);
										FinalZoomData.z = it->SideAimZoomData.z;
										_MESSAGE("z : %f\n", it->SideAimZoomData.z);
									}
								}

							}
						}
					}
				}
			}
		}
	}
}

void GameLoadZoomDataset()
{
	if (p->currentProcess && p->currentProcess->middleHigh) {
		BSTArray<EquippedItem> equipped = p->currentProcess->middleHigh->equippedItems;
		if (equipped.size() != 0 && equipped[0].item.instanceData) {
			TESObjectWEAP* weap = (TESObjectWEAP*)equipped[0].item.object;
			TESObjectWEAP::InstanceData* instance = (TESObjectWEAP::InstanceData*)equipped[0].item.instanceData.get();
			if (instance) {
				CompareEquippedWeaponModsAndMap(weap);
			}
		}
	}
}

class EquipWatcher : public BSTEventSink<TESEquipEvent>
{
public:
	virtual BSEventNotifyControl ProcessEvent(const TESEquipEvent& evn, BSTEventSource<TESEquipEvent>*)
	{

		if (evn.flag != 0x00000000ff000000) {
			if (evn.a == p && p->currentProcess && p->currentProcess->middleHigh) {
				BSTArray<EquippedItem> equipped = p->currentProcess->middleHigh->equippedItems;
				if (equipped.size() != 0 && equipped[0].item.instanceData) {
					TESObjectWEAP* weap = (TESObjectWEAP*)equipped[0].item.object;
					TESObjectWEAP::InstanceData* instance = (TESObjectWEAP::InstanceData*)equipped[0].item.instanceData.get();
					if (instance) {
						if ((instance->keywords->HasKeyword(sideAimAnimationKeyword, instance) == false)) {
							sideAim = false;
							NotifyWeaponGraphManager("Retract");
						}
						CompareEquippedWeaponModsAndMap(weap);
					}
				}
			}
		}
		return BSEventNotifyControl::kContinue;
	}
	F4_HEAP_REDEFINE_NEW(EquipEventSink);
};



class AnimationGraphEventWatcher : public BSTEventSink<BSAnimationGraphEvent>
{
public:
	typedef BSEventNotifyControl (AnimationGraphEventWatcher::*FnProcessEvent)(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* dispatcher);

	BSEventNotifyControl HookedProcessEvent(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* src)
	{
		if (sideAim == true && evn.animEvent == std::string("FirstPersonInitialized") || evn.animEvent == std::string("PowerFirstPersonInitialized")) {
			NotifyWeaponGraphManager("Extend");
		} 
		FnProcessEvent fn = fnHash.at(*(uint64_t*)this);
		return fn ? (this->*fn)(evn, src) : BSEventNotifyControl::kContinue;
	}

	void HookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end()) {
			FnProcessEvent fn = SafeWrite64(vtable + 0x8, &AnimationGraphEventWatcher::HookedProcessEvent);
			fnHash.insert(std::pair<uint64_t, FnProcessEvent>(vtable, fn));
			_MESSAGE("AnimationGraphEventWatcher");
		}
	}

	void UnHookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end())
			return;
		SafeWrite64(vtable + 0x8, it->second);
		fnHash.erase(it);
	}

protected:
	static unordered_map<uint64_t, FnProcessEvent> fnHash;
};
unordered_map<uint64_t, AnimationGraphEventWatcher::FnProcessEvent> AnimationGraphEventWatcher::fnHash;

void InitializePlugin()
{
	p = PlayerCharacter::GetSingleton();
	((AnimationGraphEventWatcher*)((uint64_t)p + 0x38))->HookSink();
	pcam = PlayerCamera::GetSingleton();
	((TullAKFireSelectorHandler*)((uint64_t)pcam + 0x38))->HookSink();
	_MESSAGE("PlayerCharacter %llx", p);
	_MESSAGE("UI %llx", UI::GetSingleton());
	_MESSAGE("PlayerCamera %llx", pcam);
	TullAKKeyword = (BGSKeyword*)GetFormFromMod("Tull_Framework.esp", 0x801);
	s_30_Auto = (BGSKeyword*)TESForm::GetFormByID(0x0A191C);
	WeaponTypeAutomatic = (BGSKeyword*)TESForm::GetFormByID(0x04A0A2);
	sideAimAnimationKeyword = (BGSKeyword*)GetFormFromMod("Tull_Framework.esp", 0x804);
	TullFrameworkSideAimSightKeyword = (BGSKeyword*)GetFormFromMod("Tull_Framework.esp", 0x800);
	FireSelectorSound = (BGSSoundDescriptorForm*)GetFormFromMod("Tull_Framework.esp", 0xB74);
	FireSelectorStatusAutoMessageSpell = (SpellItem*)GetFormFromMod("Tull_Framework.esp", 0xBA3);
	FireSelectorStatusSingleMessageSpell = (SpellItem*)GetFormFromMod("Tull_Framework.esp", 0xBA1);
}

void LoadINIConfigs()
{
	ini.LoadFile("Data\\F4SE\\Plugins\\TullFramework.ini");
	FireSelectorKey = std::stoi(ini.GetValue("General", "FireSelectorKey", "0x58"), 0, 16);
	EnableSideAimCombineKey = std::stoi(ini.GetValue("General", "EnableSideAimCombineKey", "1")) > 0;
	SideAimKey1 = std::stoi(ini.GetValue("General", "SideAImKey1", "0xA4"), 0, 16);
	SideAimKey2 = std::stoi(ini.GetValue("General", "SideAImKey2", "0x58"), 0, 16);
	

}

void LoadConfigs()
{
	namespace fs = std::filesystem;
	fs::path jsonPath = fs::current_path();
	jsonPath += "\\Data\\F4SE\\Plugins\\TullFrameworkData";
	std::stringstream stream;
	fs::directory_entry jsonEntry{ jsonPath };
	if (!jsonEntry.exists()) {
		logger::warn("TullFrameworkData directory does not exist!"sv);
		return;
	}
	for (auto& it : fs::directory_iterator(jsonEntry)) {
		if (it.path().extension().compare(".json") == 0) {
			CustomSideAimSightData.clear();
			stream << it.path().filename();
			_MESSAGE("Loading Side AIm Sight Zoom data %s", stream.str().c_str());
			stream.str(std::string());
			std::ifstream reader;
			reader.open(it.path());
			nlohmann::json customData;
			reader >> customData;
			for (auto Pluginit = customData.begin(); Pluginit != customData.end(); ++Pluginit) {
				for (auto formit = Pluginit.value().begin(); formit != Pluginit.value().end(); ++formit) {
					TESForm* form = GetFormFromMod(Pluginit.key(), std::stoi(formit.key(), 0, 16));
					if (form) {
						if (form->GetFormType() == ENUM_FORM_ID::kOMOD) {
							RegisterZoomData(formit, form, CustomSideAimSightData);
						}
					}
				}
			}
		}
	}
	

}


extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "TullAKFireSelectorF4SE.log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = "TullAKFireSelectorF4SE";
	a_info->version = 1;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor"sv);
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}"sv, ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	taskInterface = F4SE::GetTaskInterface();
	message->RegisterListener([](F4SE::MessagingInterface::Message* msg) -> void {
		if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
			InitializePlugin();
			LoadConfigs();
			LoadINIConfigs();
			EquipWatcher* ew = new EquipWatcher();
			EquipEventSource::GetSingleton()->RegisterSink(ew);
		} else if (msg->type == F4SE::MessagingInterface::kPostLoadGame) {
			NotifyWeaponGraphManager("Retract");
			LoadConfigs();
			GameLoadZoomDataset();
			LoadINIConfigs();
		} else if (msg->type == F4SE::MessagingInterface::kNewGame){
			LoadConfigs();
		}
	});
	return true;
}
