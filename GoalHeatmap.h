#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class GoalHeatmap: public BakkesMod::Plugin::BakkesModPlugin
	//,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

private:
	std::shared_ptr<bool> bEnabled;
	std::shared_ptr<int> XPos;
	std::shared_ptr<int> YPos;
	std::shared_ptr<bool> bOnlyOnScoreboard;

	std::shared_ptr<bool> Exclude_freeplay_record;
	std::shared_ptr<std::string> UserOption1;
	std::shared_ptr<std::string> UserOption2;
	std::shared_ptr<std::string> Usercolorscheme;
	std::shared_ptr<float> HeatmapScale;
	std::shared_ptr<float> chosen_second_min;
	std::shared_ptr<float> chosen_second_max;
	std::shared_ptr<int> image_reduction_factor;
	std::shared_ptr<float> alphafactor;
	std::shared_ptr<int> swamp_size;


public:
	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
	void OnRoundStarted();
	void Render(CanvasWrapper canvas);
	void BallHit(BallWrapper HitBall, void* Params);
	void OnBallHitGoal(BallWrapper HitBall, void* Params);
	void RegisterBallPos();
	void AllVariableReset();
	void OnReplayStarted();
	void OnReplayEnded();
	void OnMatchStarted();
	void OnMatchEnded();
	void DrawHeatMap();
	void ButtonRedraw();
	void OpenScoreboard(std::string eventName);
	void CloseScoreboard(std::string eventName);
	void ClearHeatmapData();
	std::vector<unsigned char> GetColor(std::vector<unsigned char> colorRGBA1, std::vector<unsigned char> colorRGBA2, float alphafactor);
	std::vector<Vector> LastCoordinates;

	//playlist IDs taken from https://wiki.bakkesplugins.com/code_snippets/playlist_id/
	std::vector<int> allowed_playlist = {1,2,3,4,6,7,8,9,10,11,12,13,14,21,22,24,30};
	bool isScoreboardOpen = false;
	bool round_started = false;
	bool match_started = false;
	char TeamOfPlayer= 0;
	int tick=0;
	int frequency = 2;
	int max_seconds_recorded = 10;
	bool isInReplay = false;
	bool goalRegistered = false;
	std::shared_ptr<ImageWrapper> heatmapfile;
};
