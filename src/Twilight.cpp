#include "Twilight.h"

#include <SimpleIni.h>

extern ENB_API::ENBSDKALT1002* g_ENB;

#define GetSettingFloat(a_section, a_setting) a_setting = (float)ini.GetDoubleValue(a_section, #a_setting, a_setting);
#define SetSettingFloat(a_section, a_setting) ini.SetDoubleValue(a_section, #a_setting, a_setting);

#define GetSettingBool(a_section, a_setting) a_setting = ini.GetBoolValue(a_section, #a_setting, a_setting);
#define SetSettingBool(a_section, a_setting) ini.SetBoolValue(a_section, #a_setting, a_setting);

void Twilight::LoadINI()
{
	std::lock_guard<std::shared_mutex> lk(fileLock);
	CSimpleIniA                        ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\Twilight.ini");

	GetSettingBool("Settings", bEnableTwilight);
	GetSettingBool("Settings", bEnableTwilightWithENB);

	GetSettingBool("Settings", bEnableAmbient);

	GetSettingFloat("Settings", fDawnOffset);
	GetSettingFloat("Settings", fDawnCurve);

	GetSettingFloat("Settings", fDuskOffset);
	GetSettingFloat("Settings", fDuskCurve);
}

void Twilight::SaveINI()
{
	std::lock_guard<std::shared_mutex> lk(fileLock);
	CSimpleIniA                        ini;
	ini.SetUnicode();

	SetSettingBool("Settings", bEnableTwilight);
	SetSettingBool("Settings", bEnableTwilightWithENB);

	GetSettingBool("Settings", bEnableAmbient);

	SetSettingFloat("Settings", fDawnOffset);
	SetSettingFloat("Settings", fDawnCurve);

	SetSettingFloat("Settings", fDuskOffset);
	SetSettingFloat("Settings", fDuskCurve);

	ini.SaveFile(L"Data\\SKSE\\Plugins\\Twilight.ini");
}

RE::NiColor LerpColor(RE::NiColor a, RE::NiColor b, float t)
{
	return a + t * (b - a);
}

float LinearStep(float edge0, float edge1, float x)
{
	return std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
}

void Twilight::Update(RE::Sky* a_sky)
{
	if (bEnableTwilight && a_sky->currentWeather) {

		if (enbEnabled && !bEnableTwilightWithENB) {
			return;
		}

		float gameHour = a_sky->currentGameHour;

		sunriseBegin = GetSunriseBegin(a_sky);
		sunriseMidpoint = GetSunriseMidpoint(a_sky);
		sunriseEnd = GetSunriseEnd(a_sky);

		dawnBegin = sunriseBegin;
		dawnEnd = sunriseMidpoint - (sunriseMidpoint - sunriseBegin) * fDawnOffset;
		dawnMidpoint = (dawnBegin + dawnEnd) / 2;

		sunsetBegin = GetSunsetBegin(a_sky);
		sunsetMidpoint = GetSunsetMidpoint(a_sky);
		sunsetEnd = GetSunsetEnd(a_sky);

		duskBegin = sunsetMidpoint + (sunsetEnd - sunsetMidpoint) * fDuskOffset;
		duskEnd = sunsetEnd;
		duskMidpoint = (duskBegin + duskEnd) / 2;

		// Day or Night
		if ((gameHour > sunriseEnd && gameHour < sunsetBegin)) {
			twilightPercent = 0;
		// Dawn
		} else if (gameHour < sunriseEnd) {
			twilightPercent = LinearStep(dawnEnd, dawnBegin, gameHour);
			twilightPercent = 1 - pow(1 - twilightPercent, fDawnCurve);
		// Dusk
		} else {
			twilightPercent = LinearStep(duskBegin, duskEnd, gameHour);
			twilightPercent = 1 - pow(1 - twilightPercent, fDuskCurve);
		}

		twilightPercent = std::clamp(twilightPercent, 0.0f, 1.0f);
		
		if (twilightPercent > 0.0f) {
			RE::NiColor sunlightDark;
			RE::NiColor ambientDark;
			if (a_sky->lastWeather) {
				sunlightDark = LerpColor(a_sky->lastWeather->colorData[RE::TESWeather::ColorTypes::kSunlight][RE::TESWeather::ColorTime::kNight], a_sky->currentWeather->colorData[RE::TESWeather::ColorTypes::kSunlight][RE::TESWeather::ColorTime::kNight], a_sky->currentWeatherPct);
				ambientDark = LerpColor(a_sky->lastWeather->colorData[RE::TESWeather::ColorTypes::kAmbient][RE::TESWeather::ColorTime::kNight], a_sky->currentWeather->colorData[RE::TESWeather::ColorTypes::kAmbient][RE::TESWeather::ColorTime::kNight], a_sky->currentWeatherPct);
			} else {
				sunlightDark = a_sky->currentWeather->colorData[RE::TESWeather::ColorTypes::kSunlight][RE::TESWeather::ColorTime::kNight];
				ambientDark = a_sky->currentWeather->colorData[RE::TESWeather::ColorTypes::kAmbient][RE::TESWeather::ColorTime::kNight];
			}

			a_sky->skyColor[RE::TESWeather::ColorTypes::kSunlight] = LerpColor(a_sky->skyColor[RE::TESWeather::ColorTypes::kSunlight], sunlightDark, twilightPercent);

			if (bEnableAmbient) {
				a_sky->skyColor[RE::TESWeather::ColorTypes::kAmbient] = LerpColor(a_sky->skyColor[RE::TESWeather::ColorTypes::kAmbient], ambientDark, twilightPercent);
			}

		}
	}
}

void Twilight::UpdateENB()
{
	if (g_ENB) {
		BOOL         res;
		ENBParameter param;
		res = g_ENB->GetParameter("enbseries.ini", "GLOBAL", "UseEffect", &param);
		if ((res == TRUE) && (param.Type == ENB_SDK::ENBParameterType::ENBParam_BOOL)) {
			BOOL bvalue = FALSE;
			memcpy(&bvalue, param.Data, 4);
			enbEnabled = bvalue;
			return;
		}
	}
	enbEnabled = false;
}

#define TWDEF "group='MOD:Twilight' precision=2 step=0.01 "

void Twilight::RefreshUI()
{
	auto bar = g_ENB->TwGetBarByEnum(!REL::Module::IsVR() ? ENB_API::ENBWindowType::EditorBarEffects : ENB_API::ENBWindowType::EditorBarObjects);  // ENB misnames its own bar, whoops!

	g_ENB->TwAddVarRW(bar, "EnableTwilight", ETwType::TW_TYPE_BOOLCPP, &bEnableTwilight, TWDEF);
	g_ENB->TwAddVarRW(bar, "EnableTwilightWithENB", ETwType::TW_TYPE_BOOLCPP, &bEnableTwilightWithENB, TWDEF);

	g_ENB->TwAddVarRW(bar, "TwilightModifyAmbient", ETwType::TW_TYPE_BOOLCPP, &bEnableAmbient, TWDEF);

	g_ENB->TwAddVarRW(bar, "TwilightDawnOffset", ETwType::TW_TYPE_FLOAT, &fDawnOffset, TWDEF);
	g_ENB->TwAddVarRW(bar, "TwilightDawnCurve", ETwType::TW_TYPE_FLOAT, &fDawnCurve, TWDEF);
	g_ENB->TwAddVarRW(bar, "TwilightDuskOffset", ETwType::TW_TYPE_FLOAT, &fDuskOffset, TWDEF);
	g_ENB->TwAddVarRW(bar, "TwilightDuskCurve", ETwType::TW_TYPE_FLOAT, &fDuskCurve, TWDEF);

	g_ENB->TwAddVarRW(bar, "TwilightPercent", ETwType::TW_TYPE_FLOAT, &twilightPercent, TWDEF "readonly=true");

	g_ENB->TwAddVarRW(bar, "TwilightDawnBegin", ETwType::TW_TYPE_FLOAT, &dawnBegin, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightDawnMidpoint", ETwType::TW_TYPE_FLOAT, &dawnMidpoint, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightDawnEnd", ETwType::TW_TYPE_FLOAT, &dawnEnd, TWDEF "readonly=true");

	g_ENB->TwAddVarRW(bar, "TwilightSunriseBegin", ETwType::TW_TYPE_FLOAT, &sunriseBegin, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightSunriseMidpoint", ETwType::TW_TYPE_FLOAT, &sunriseMidpoint, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightSunriseEnd", ETwType::TW_TYPE_FLOAT, &sunriseEnd, TWDEF "readonly=true");

	g_ENB->TwAddVarRW(bar, "TwilightSunsetBegin", ETwType::TW_TYPE_FLOAT, &sunsetBegin, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightSunsetMidpoint", ETwType::TW_TYPE_FLOAT, &sunsetMidpoint, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightSunsetEnd", ETwType::TW_TYPE_FLOAT, &sunsetEnd, TWDEF "readonly=true");

	g_ENB->TwAddVarRW(bar, "TwilightDuskBegin", ETwType::TW_TYPE_FLOAT, &duskBegin, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightDuskMidpoint", ETwType::TW_TYPE_FLOAT, &duskMidpoint, TWDEF "readonly=true");
	g_ENB->TwAddVarRW(bar, "TwilightDuskEnd", ETwType::TW_TYPE_FLOAT, &duskEnd, TWDEF "readonly=true");

	g_ENB->TwDefine("EditorBarEffects/'MOD:Twilight' opened=false");
}

float ConvertClimateTimeToGameTime(std::uint8_t a_time)
{
	float hours = (float)((a_time * 10) / 60);
	float minutes = (float)((a_time * 10) % 60);
	return hours + (minutes * (100 / 60) / 100);
}

float Twilight::GetSunriseBegin(RE::Sky* a_sky)
{
	return ConvertClimateTimeToGameTime(a_sky->currentClimate->timing.sunrise.begin);
}

float Twilight::GetSunriseEnd(RE::Sky* a_sky)
{
	return ConvertClimateTimeToGameTime(a_sky->currentClimate->timing.sunrise.end);
}

float Twilight::GetSunriseMidpoint(RE::Sky* a_sky)
{
	static auto fSunAlphaTransTime = RE::GameSettingCollection::GetSingleton()->GetSetting("fSunAlphaTransTime");
	return (GetSunriseBegin(a_sky) + GetSunriseEnd(a_sky) + fSunAlphaTransTime->GetFloat()) / 2;
}

float Twilight::GetSunsetBegin(RE::Sky* a_sky)
{
	return ConvertClimateTimeToGameTime(a_sky->currentClimate->timing.sunset.begin);
}

float Twilight::GetSunsetEnd(RE::Sky* a_sky)
{
	return ConvertClimateTimeToGameTime(a_sky->currentClimate->timing.sunset.end);
}

float Twilight::GetSunsetMidpoint(RE::Sky* a_sky)
{
	static auto fSunAlphaTransTime = RE::GameSettingCollection::GetSingleton()->GetSetting("fSunAlphaTransTime");
	return (GetSunsetBegin(a_sky) + GetSunsetEnd(a_sky) - fSunAlphaTransTime->GetFloat()) / 2;
}

float Twilight::GetLinearBumpStep(float edge0, float edge1, float x)
{
	return 1.0f - abs(std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f) - 0.5f) * 2.0f;
}
