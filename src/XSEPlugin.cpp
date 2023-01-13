#include "Twilight.h"

ENB_API::ENBSDKALT1002* g_ENB = nullptr;

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		g_ENB = reinterpret_cast<ENB_API::ENBSDKALT1002*>(ENB_API::RequestENBAPI(ENB_API::SDKVersion::V1002));
		if (g_ENB) {
			logger::info("Obtained ENB API");
			g_ENB->SetCallbackFunction([](ENBCallbackType calltype) {
				switch (calltype) {
				case ENBCallbackType::ENBCallback_PostLoad:
					Twilight::GetSingleton()->RefreshUI();
					Twilight::GetSingleton()->LoadINI();
					break;
				case ENBCallbackType::ENBCallback_PreSave:
					Twilight::GetSingleton()->SaveINI();
					break;
				case ENBCallbackType::ENBCallback_BeginFrame:
					Twilight::GetSingleton()->UpdateENB();
					break;
				}
			});
		} else
			logger::info("Unable to acquire ENB API");
		break;
	}
}

void Load()
{
	Twilight::InstallHooks();
	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
}
