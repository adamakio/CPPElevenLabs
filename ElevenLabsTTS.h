/*****************************************************************//**
 * \file   ElevenLabsTTS.h
 * \brief  Header file for ElevenLabsTTS.cpp
 * 
 * \author 16478
 * \date   November 2023
 *********************************************************************/

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include "ElevenLabsAPI.hpp"

struct Model {
	std::string id = "";
	std::string name = "";
	std::string description = "";
};

// {"accent":"american","age":"young","description":"calm","gender":"female","use case":"narration"}
struct Voice {
	std::string id = "";
	std::string name = "";
	Json labels;
};

struct VoiceSettings {
	std::string voice_id = "";
	double similarity_boost = 0.75;
	double stability = 0.5;
	double style = 0.0;
	bool use_speaker_boost = true;
};

std::vector<Model> listModels();

std::vector<Voice> listVoices();

VoiceSettings getDefaultVoiceSettings();

VoiceSettings getVoiceSettings(const std::string& voice_id);
