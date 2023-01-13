#pragma once

#include "ENB/ENBSeriesAPI.h"
#include <shared_mutex>

class Twilight
{
public:
	static Twilight* GetSingleton()
	{
		static Twilight handler;
		return &handler;
	}

	static void InstallHooks()
	{
		Hooks::Install();
	}

	std::shared_mutex       fileLock;

	float twilightPercent = 0;
	bool  enbEnabled = false;

	float dawnBegin = 0;
	float dawnMidpoint = 0;
	float dawnEnd = 0;
	float sunriseBegin = 0;
	float sunriseMidpoint = 0;
	float sunriseEnd = 0;
	float sunsetBegin = 0;
	float sunsetMidpoint = 0;
	float sunsetEnd = 0;
	float duskBegin = 0;
	float duskMidpoint = 0;
	float duskEnd = 0;

	bool bEnableTwilight = true;
	bool bEnableTwilightWithENB = false;
	bool bEnableAmbient = true;

	float fDawnOffset = 0.0f;
	float fDawnCurve = 10.0f;

	float fDuskOffset = 0.0f;
	float fDuskCurve = 10.0f;

	float GetSunriseBegin(RE::Sky* a_sky);
	float GetSunriseEnd(RE::Sky* a_sky);
	float GetSunriseMidpoint(RE::Sky* a_sky);

	float GetSunsetBegin(RE::Sky* a_sky);
	float GetSunsetEnd(RE::Sky* a_sky);
	float GetSunsetMidpoint(RE::Sky* a_sky);

	float GetLinearBumpStep(float edge0, float edge1, float x);

	void LoadINI();
	void SaveINI();

	void Update(RE::Sky* a_sky);
	void UpdateENB();

	// ENB UI

	void RefreshUI();

protected:
	struct Hooks
	{
		struct Sky_Update_UpdateColors
		{
			static void thunk(RE::Sky* This)
			{
				func(This);
				GetSingleton()->Update(This);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			stl::write_thunk_call<Sky_Update_UpdateColors>(REL::RelocationID(25682, 26229).address() + REL::Relocate(0x2AC, 0x3F6));
		}
	};

private:
	Twilight()
	{
		LoadINI();
	};

	Twilight(const Twilight&) = delete;
	Twilight(Twilight&&) = delete;

	~Twilight() = default;

	Twilight& operator=(const Twilight&) = delete;
	Twilight& operator=(Twilight&&) = delete;
};
