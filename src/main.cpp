inline void sendConsoleCommand(std::string a_command)
{
	const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
	const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
	if (script) {
		const auto selectedRef = RE::Console::GetSelectedRef();
		script->SetCommand(a_command);
		script->CompileAndRun(selectedRef.get());
		delete script;
	}
}

class CombatMusicFix {
private:
	static inline const std::string stopCombatMusic = "removemusic MUScombat";
public:
	static void fix() {
		auto asyncFunc = []() {
			std::this_thread::sleep_for(std::chrono::seconds(5));
			sendConsoleCommand(stopCombatMusic);
		};
		std::jthread t(asyncFunc);
		t.detach();
	}

};
class CombatEventHandler : public RE::BSTEventSink<RE::TESCombatEvent>
{
public:
	using EventResult = RE::BSEventNotifyControl;

	virtual EventResult ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>* a_eventSource) {
		if (a_event->actor && a_event->actor->IsPlayerRef()) {
			if (a_event->newState == RE::ACTOR_COMBAT_STATE::kNone) {
				CombatMusicFix::fix();
			}
		}
		return EventResult::kContinue;
	}


	static bool Register()
	{
		

		static CombatEventHandler singleton;
		INFO("Registering {}...", typeid(singleton).name());
		auto ScriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
		
		if (!ScriptEventSource) {
			ERROR("Script event source not found");
			return false;
		}

		ScriptEventSource->AddEventSink(&singleton);

		INFO("..done");

		return true;
	}

};



void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		INFO("Data Loaded");
		CombatEventHandler::Register();
	case SKSE::MessagingInterface::kPostLoadGame:
		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc && !pc->IsInCombat()) {
			CombatMusicFix::fix();
		}
		break;
	}
}

#if ANNIVERSARY_EDITION

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
{
	SKSE::PluginVersionData data{};

	data.PluginVersion(Version::MAJOR);
	data.PluginName(Version::NAME);
	data.AuthorName("dTry"sv);

	data.CompatibleVersions({ SKSE::RUNTIME_LATEST });
	data.UsesAddressLibrary(true);

	return data;
}();

#else

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	DKUtil::Logger::Init(Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		ERROR("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		ERROR("Unable to load this plugin, incompatible runtime version!\nExpected: Newer than 1-5-39-0 (A.K.A Special Edition)\nDetected: {}", ver.string());
		return false;
	}

	return true;
}

#endif


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#if ANNIVERSARY_EDITION

	DKUtil::Logger::Init(Version::PROJECT, Version::NAME);

	if (REL::Module::get().version() < SKSE::RUNTIME_1_6_317) {
		ERROR("Unable to load this plugin, incompatible runtime version!\nExpected: Newer than 1-6-317-0 (A.K.A Anniversary Edition)\nDetected: {}", REL::Module::get().version().string());
		return false;
	}

#endif

	INFO("{} v{} loaded", Version::PROJECT, Version::NAME);

	SKSE::Init(a_skse);
	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}
	return true;
}
