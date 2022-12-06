#include "pch.h"
#include "GoalHeatmap.h"
#include <heatmap.h>
#include <lodepng.h>
#include <colorschemes/Spectral.h>
#include <colorschemes/YlGnBu.h>
BAKKESMOD_PLUGIN(GoalHeatmap, "Show a heatmap of ball positions before goals", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void GoalHeatmap::onLoad()
{
	_globalCvarManager = cvarManager;

	LOG("Goal Heatmap plugin loaded! LETS GO");

	bEnabled = std::make_shared<bool>(true);
	XPos = std::make_shared<int>(0);
	YPos = std::make_shared<int>(0);
	bOnlyOnScoreboard = std::make_shared<bool>(false);

	Exclude_freeplay_record = std::make_shared<bool>(true);
	UserOption1 = std::make_shared<std::string>("opt1");
	UserOption2 = std::make_shared<std::string>("opt3");
	Usercolorscheme = std::make_shared<std::string>("Spectralsoft");
	HeatmapScale = std::make_shared<float>(0.4f);
	swamp_size = std::make_shared <int>(128);
	chosen_second_min = std::make_shared <float>(1.0f);
	chosen_second_max = std::make_shared <float>(4.0f);
	image_reduction_factor = std::make_shared <int>(8);
	alphafactor = std::make_shared <float>(0.8f);

	DrawHeatMap();

	cvarManager->registerNotifier("DrawHeatmapButton", [this](std::vector<std::string> params) {GoalHeatmap::ButtonRedraw();}, "Regenerate the heatmap", PERMISSION_ALL);
	cvarManager->registerNotifier("ClearHeatmapDataButton", [this](std::vector<std::string> params) {GoalHeatmap::ClearHeatmapData();}, "Clear saved goals data | CAN'T BE UNDONE", PERMISSION_ALL);
	cvarManager->registerCvar("GHM_Enable", "1", "Show goal speed anywhere", true, true, 0, true, 1).bindTo(bEnabled);
	cvarManager->registerCvar("GHM_X_Position", "0", "Goal speed anywhere X position", true, true, 0, true, 100).bindTo(XPos);
	cvarManager->registerCvar("GHM_Y_Position", "0", "Goal speed anywhere Y position", true, true, 0, true, 100).bindTo(YPos);
	cvarManager->registerCvar("GHM_onlyonscoreboard", "0", "Display Only On Scoreboard?", true, true, 0, true, 1).bindTo(bOnlyOnScoreboard);

	cvarManager->registerCvar("GHM_Exclude_freeplay_record", "1", "Exclude freeplay goals", true, true, 0, true, 1).bindTo(Exclude_freeplay_record);
	cvarManager->registerCvar("GHM_UserOption1", "opt1", "Sample data from", true).bindTo(UserOption1);
	cvarManager->registerCvar("GHM_UserOption2", "opt3", "Shot taken or scored", true).bindTo(UserOption2);
	cvarManager->registerCvar("GHM_Usercolorscheme", "Spectralsoft", "Colorscheme", true).bindTo(Usercolorscheme);
	cvarManager->registerCvar("GHM_HeatmapScale", "0.4f", "Scale of the Heatmap", true, true, 0.01f, true, 2.0f).bindTo(HeatmapScale);
	cvarManager->registerCvar("GHM_swamp_size", "128", "Size of swamp (how much colors scatter around coordinates)", true, true, 1, true, 200).bindTo(swamp_size);
	cvarManager->registerCvar("GHM_chosen_second_min", "1.0f", "Min number of seconds before goals", true, true, 0, true, 10).bindTo(chosen_second_min);
	cvarManager->registerCvar("GHM_chosen_second_max", "4.0f", "Min number of seconds before goals", true, true, 0, true, 10).bindTo(chosen_second_max);
	//	cvarManager->registerCvar("GHM_image_reduction_factor", "8", "Level of compression of the image", true, true, 1, true, 10).bindTo(image_reduction_factor);
	cvarManager->registerCvar("GHM_alphafactor", "0.8f", "Opacity of the heatmap", true, true, 0.1f, true, 1.0f).bindTo(alphafactor);

	gameWrapper->RegisterDrawable(bind(&GoalHeatmap::Render, this, std::placeholders::_1));

	
	gameWrapper->HookEvent("Function TAGame.Team_TA.PostBeginPlay", std::bind(&GoalHeatmap::OnMatchStarted, this));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&GoalHeatmap::OnRoundStarted, this));
	//gameWrapper->HookEventWithCallerPost<BallWrapper>("Function TAGame.Ball_TA.RecordCarHit", std::bind(&GoalHeatmap::BallHit, this, std::placeholders::_1, std::placeholders::_2));
	gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnHitGoal", std::bind(&GoalHeatmap::OnBallHitGoal, this, std::placeholders::_1, std::placeholders::_2));
	gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput", std::bind(&GoalHeatmap::RegisterBallPos, this));

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&GoalHeatmap::OnReplayStarted, this));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", std::bind(&GoalHeatmap::OnReplayEnded, this));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&GoalHeatmap::OnMatchEnded, this));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&GoalHeatmap::OnMatchEnded, this));


	gameWrapper->HookEvent("Function TAGame.GFxHUD_TA.OpenScoreboard", std::bind(&GoalHeatmap::OpenScoreboard, this, std::placeholders::_1)); // In Game when user views Scoreboard
	gameWrapper->HookEvent("Function TAGame.GFxHUD_TA.CloseScoreboard", std::bind(&GoalHeatmap::CloseScoreboard, this, std::placeholders::_1)); // In Game when user stops viewing Scoreboard
}

void GoalHeatmap::BallHit(BallWrapper HitBall, void* Params)
{
	LOG("BALL HIT!");
	Vector ball_coordinates = HitBall.GetLocation();
	LOG("X:" + std::to_string(ball_coordinates.X) + "Y: " + std::to_string(ball_coordinates.Y) + "Z: " + std::to_string(ball_coordinates.Z));
}

void GoalHeatmap::ButtonRedraw()
{
	DrawHeatMap();
}


void GoalHeatmap::OnReplayStarted() {
	isInReplay = true;
}

void GoalHeatmap::OnReplayEnded() {
	isInReplay = false;
	AllVariableReset();
}
void GoalHeatmap::OnMatchStarted() {
	match_started = true;
}

void GoalHeatmap::OnMatchEnded() {
	DrawHeatMap();
	AllVariableReset();
	match_started = false;
}

void GoalHeatmap::OnRoundStarted()
{
	if (gameWrapper->IsInReplay()) return;
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return;
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (!playlist) return;
	int playlistID = playlist.GetPlaylistId();

	if (std::find(allowed_playlist.begin(), allowed_playlist.end(), playlistID) != allowed_playlist.end() == false) {
		return;
	}
	else {
	CarWrapper LocalCar = gameWrapper->GetLocalCar();
	TeamOfPlayer = LocalCar.GetTeamNum2();
	round_started = true;
	match_started = true;
	}
}

void GoalHeatmap::ClearHeatmapData() {
	std::ofstream myfile;
	myfile.open(gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/GoalRecords.csv", std::ios::out);
	myfile << "#X;Y;Z;MATCH_ID;ROUND_NUM;WIN_LOOSE;TIME_BEFORE_GOAL;DATE\n";
	myfile.close();
}

void GoalHeatmap::AllVariableReset() {
	round_started = false;
	LastCoordinates.clear();
	LastCoordinates.resize(0);
	goalRegistered = false;
	isInReplay = false;
	tick = 0;
	isScoreboardOpen = false;
}

void GoalHeatmap::OpenScoreboard(std::string eventName) {
	if (!(*bEnabled) || !round_started) return;
	isScoreboardOpen = true;
}

void GoalHeatmap::CloseScoreboard(std::string eventName) {
	if (!(*bEnabled) || !round_started) return;
	isScoreboardOpen = false;
}


void GoalHeatmap::RegisterBallPos()
{
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return;

	//Register ball position, only if the round started, not in replay and if in correct playlist
	if (round_started == true && isInReplay == false) {
		//120 is the number of tick each second.
		if (tick == 120 / frequency) {
			BallWrapper BallOfTheGame = server.GetBall();
			Vector ball_coordinates = BallOfTheGame.GetLocation();
			int number_of_records = LastCoordinates.size();

			if (number_of_records < max_seconds_recorded * frequency) {
				LastCoordinates.push_back(ball_coordinates);
			}
			else {
				LastCoordinates.erase(LastCoordinates.begin());
				LastCoordinates.push_back(ball_coordinates);
			}
		}
		tick += 1;

		if (tick > 120 / frequency) {
			tick = 0;
		}
	}
}



void GoalHeatmap::OnBallHitGoal(BallWrapper HitBall, void* Params)
{
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return;

	//PLAYLIST ID
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (!playlist) return;
	int playlistID = playlist.GetPlaylistId();

	if (round_started == true && goalRegistered == false && std::find(allowed_playlist.begin(), allowed_playlist.end(), playlistID) != allowed_playlist.end()) {

		//MATCH GUID
		std::string MatchGUID = server.GetMatchGUID();
		if (MatchGUID == "") {
			MatchGUID = "Not defined";
		}
		//DATE OF THE MATCH
		const int MAXLEN = 80;
		char MatchDate[MAXLEN];
		time_t t = time(0);
		strftime(MatchDate, MAXLEN, "%Y-%m-%d", localtime(&t));
		LOG(MatchDate);


		Vector ball_coordinates = HitBall.GetLocation();
		LastCoordinates.push_back(ball_coordinates);


		int TeamScorer = 0;
		if (ball_coordinates.Y < 0) {
			TeamScorer = 1;
		}
		int switch_side = 1; //invert ball coordinates in symmetry in order to have the shoot on the same sides
		if (LastCoordinates[LastCoordinates.size() - 1].Y < 0) {
			switch_side = -1;
		}


		std::ofstream myfile;
		myfile.open(gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/GoalRecords.csv", std::ios::out | std::ios::app);

		for (int i = 0; i < LastCoordinates.size(); i++) {
			if (TeamScorer == TeamOfPlayer) {
				myfile << std::to_string(LastCoordinates[i].X * switch_side) + ";" + std::to_string(LastCoordinates[i].Y * switch_side) + ";" + std::to_string(LastCoordinates[i].Z) + ";" + MatchGUID + ";" + std::to_string(server.GetRoundNum()) + ";win;" + std::to_string((float)(LastCoordinates.size() - 1 - i) / frequency) + ";" + MatchDate + ";" + std::to_string(playlistID) + "\n";
			}
			else {
				myfile << std::to_string(LastCoordinates[i].X * switch_side) + ";" + std::to_string(LastCoordinates[i].Y * switch_side) + ";" + std::to_string(LastCoordinates[i].Z) + ";" + MatchGUID + ";" + std::to_string(server.GetRoundNum()) + ";loose;" + std::to_string((float)(LastCoordinates.size() - 1 - i) / frequency) + ";" + MatchDate +";"+ std::to_string(playlistID) + "\n";
			}
		}
		myfile.close();

		round_started = false;
		goalRegistered = true;
	}
}

void GoalHeatmap::DrawHeatMap() {
	std::ifstream  myfile(gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/GoalRecords.csv");
	std::string line;
	int terrain_width = 4120 * 2;
	int terrain_height = 5140 * 2;
	heatmap_stamp_t* stamp = heatmap_stamp_gen(*swamp_size);
	static const size_t w = terrain_width / (*image_reduction_factor), h = terrain_height / (*image_reduction_factor);
	std::vector< std::vector<std::string>> dataframe;

	//DATE OF TODAY
	const int MAXLEN = 80;
	char TodayDate[MAXLEN];
	time_t t = time(0);
	strftime(TodayDate, MAXLEN, "%Y-%m-%d", localtime(&t));

	std::string firstline;
	std::getline(myfile, firstline); // skip the first line

	while (std::getline(myfile, line))
	{
		if (line.size() == 0) {
			break;
		}
		std::stringstream  lineStream(line);
		std::string        cell;
		std::vector<std::string> cells;

		while (std::getline(lineStream, cell, ';'))
		{
			cells.push_back(cell);
		}

		if (std::stof(cells[6]) <= (*chosen_second_max) && std::stof(cells[6]) >= (*chosen_second_min)) {
			if (std::stof(cells[0]) == 0.0f && std::stof(cells[1]) == 0.0f) {
				//do nothing if x=0 and y=0 (ball at center of the map)
			}
			else if (*UserOption1 == "opt1" || (*UserOption1 == "opt2" && cells[7] == TodayDate)) {
				if ((*UserOption2 == "opt1" && cells[5] == "loose") || (*UserOption2 == "opt2" && cells[5] == "win") || (*UserOption2 == "opt3")) {
					if ((*Exclude_freeplay_record == 0) || (*Exclude_freeplay_record == 1 && cells[3] != "Not defined")) {
						dataframe.push_back(cells);
					}
				}
			}
			else {
				//do nothing
			}
		}
	}


	// Create the heatmap object with the given dimensions (in pixel).
	heatmap_t* hm = heatmap_new(w, h);

	for (unsigned i = 0; i < dataframe.size(); ++i) {
		int switch_side = 1;
		if (dataframe[i][5] == "win") {
			switch_side = -1;
		}
		heatmap_add_point_with_stamp(hm, (switch_side * std::stof(dataframe[i][0]) + (terrain_width / 2)) / (*image_reduction_factor), (switch_side * std::stof(dataframe[i][1]) + terrain_height / 2) / (*image_reduction_factor), stamp);
	}

	// This creates an image out of the heatmap.
	// `image` now contains the image data in 32-bit RGBA.
	std::vector<unsigned char> imageFG(w * h * 4);
	heatmap_colorscheme_t chosenColorscheme = *heatmap_cs_Spectral_soft;
	if (*Usercolorscheme == "SpectralDiscrete") {
		chosenColorscheme = *heatmap_cs_Spectral_discrete;
	}
	else if (*Usercolorscheme == "YGBDiscrete") {
		chosenColorscheme = *heatmap_cs_YlGnBu_discrete;
	}
	else if (*Usercolorscheme == "YGBSoft") {
		chosenColorscheme = *heatmap_cs_YlGnBu_soft;
	}


	heatmap_render_to(hm, &chosenColorscheme, &imageFG[0]);

	// Now that we've got a finished heatmap picture, we don't need the map anymore.
	heatmap_free(hm);


	//DECODE BG
	std::vector<unsigned char> imageBG; //the raw pixels
	unsigned bgwidth, bgheight;
	unsigned error = lodepng::decode(imageBG, bgwidth, bgheight, gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/rlmap.png");

	std::vector<unsigned char> imageFINAL; //the raw pixels result

	int nbyte = 0;
	std::vector<unsigned char> pixel_value1;
	std::vector<unsigned char> pixel_value2;
	for (int pixel = 0; pixel < imageBG.size(); pixel++) {
		nbyte += 1;
		pixel_value1.push_back(imageBG[pixel]);
		pixel_value2.push_back(imageFG[pixel]);

		if (nbyte == 4) {
			std::vector<unsigned char> mergedColor = GetColor(pixel_value1, pixel_value2, (*alphafactor));
			imageFINAL.push_back(mergedColor[0]);
			imageFINAL.push_back(mergedColor[1]);
			imageFINAL.push_back(mergedColor[2]);
			imageFINAL.push_back(mergedColor[3]);
			pixel_value1.clear();
			pixel_value2.clear();
			nbyte = 0;
		}

	}
	// Finally, we use the fantastic lodepng library to save it as an image.
	if (unsigned error = lodepng::encode(gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/heatmap.png", imageFINAL, w, h)) {
		LOG("encoder error " + error);
	}
	heatmapfile = std::make_shared<ImageWrapper>(gameWrapper->GetBakkesModPath().generic_string() + "/data/GoalHeatmap/heatmap.png", true, true);
}


std::vector<unsigned char> GoalHeatmap::GetColor(std::vector<unsigned char> colorRGBA1, std::vector<unsigned char> colorRGBA2, float alphafactor) {
	std::vector<unsigned char> result;
	colorRGBA2[3] = static_cast<int>(std::round((float)colorRGBA2[3] * alphafactor));
	unsigned char alpha = 255 - ((255 - colorRGBA1[3]) * (255 - colorRGBA2[3]) / 255);
	if (colorRGBA1[3] == 0) {
		alpha = 0;
	}
	unsigned char red = (colorRGBA1[0] * (255 - colorRGBA2[3]) + colorRGBA2[0] * colorRGBA2[3]) / 255;
	unsigned char green = (colorRGBA1[1] * (255 - colorRGBA2[3]) + colorRGBA2[1] * colorRGBA2[3]) / 255;
	unsigned char blue = (colorRGBA1[2] * (255 - colorRGBA2[3]) + colorRGBA2[2] * colorRGBA2[3]) / 255;
	result.push_back(red);
	result.push_back(green);
	result.push_back(blue);
	result.push_back(alpha);

	return (result);
}



void GoalHeatmap::Render(CanvasWrapper canvas)
{
	if (!(*bEnabled) || gameWrapper->IsInGame() || (gameWrapper->IsInOnlineGame() && match_started ==true)|| gameWrapper->IsInFreeplay() || gameWrapper->IsInCustomTraining() || gameWrapper->IsInReplay()) {
		return;
	}
	else {
	int x_screen = (gameWrapper->GetScreenSize().X * (*XPos)) / 100;
	int y_screen = (gameWrapper->GetScreenSize().Y * (*YPos)) / 100;
	canvas.SetPosition(Vector2{ x_screen,y_screen });
	canvas.DrawTexture(heatmapfile.get(), (*HeatmapScale));
	}

}