/*****************************************************************//**
 * \file   ElevenLabsTTS.cpp
 * \brief  Implementation file  for ElevenLabsAPI.hpp 
 * 
 * Testing out the ElevenLabs API using curl and portaudio
 * 
 * \author 16478
 * \date   November 2023
 *********************************************************************/
#include "ElevenLabsTTS.h"

std::vector<Model> listModels() {
	std::vector<Model> models;
	auto model_jsons = elevenlabs::models().list();
	for (const auto& model_json : model_jsons) {
		Model model;
		model.id = model_json.at("model_id").get<std::string>();
		model.name = model_json.at("name").get<std::string>();
		model.description = model_json.at("description").get<std::string>();
		models.emplace_back(model);
	}
	
	return models;
}

std::vector<Voice> listVoices() {
	std::vector<Voice> voices;
	auto voice_jsons = elevenlabs::voices().list().at("voices");
	for (const auto& voice_json : voice_jsons) {
		Voice voice;
		voice.id = voice_json.at("voice_id").get<std::string>();
		voice.name = voice_json.at("name").get<std::string>();
		voice.labels = voice_json.at("labels");
		voices.emplace_back(voice);
	}
	return voices;
}


VoiceSettings getDefaultVoiceSettings() {
	VoiceSettings voice_settings;
	auto default_settings = elevenlabs::voices().defaultSettings();
	voice_settings.similarity_boost = default_settings.at("similarity_boost").get<double>();
	voice_settings.stability = default_settings.at("stability").get<double>();
	voice_settings.style = default_settings.at("style").get<double>();
	voice_settings.use_speaker_boost = default_settings.at("use_speaker_boost").get<bool>();
	return voice_settings;
}

VoiceSettings getVoiceSettings(const std::string& voice_id) {
	VoiceSettings voice_settings;
	auto voice_settings_json = elevenlabs::voices().voiceSettings(voice_id);
	voice_settings.voice_id = voice_id;
	voice_settings.similarity_boost = voice_settings_json.at("similarity_boost").get<double>();
	voice_settings.stability = voice_settings_json.at("stability").get<double>();
	voice_settings.style = voice_settings_json.at("style").get<double>();
	voice_settings.use_speaker_boost = voice_settings_json.at("use_speaker_boost").get<bool>();
	return voice_settings;
}

Json editVoiceSettings(const std::string& voice_id, const VoiceSettings& voice_settings) {
	Json voice_settings_json;
	voice_settings_json["similarity_boost"] = voice_settings.similarity_boost;
	voice_settings_json["stability"] = voice_settings.stability;
	voice_settings_json["style"] = voice_settings.style;
	voice_settings_json["use_speaker_boost"] = (int) voice_settings.use_speaker_boost;
	return elevenlabs::voices().editVoiceSettings(voice_id, voice_settings_json);
}

int main()
{
	auto& ElevenLabs = elevenlabs::start("your_api_key_here");

	std::cout << "Starting...\n";

	StreamResponse* response = new StreamResponse();

	std::string long_text = "Why is there still static I defined all the flags now";
	elevenlabs::text_to_speech().stream(long_text, "your_voice_id_here", "eleven_turbo_v2", response);
	closeStream();
	std::cout << "Press Enter to close stream...";
	std::cin.get();

	delete response;
	return 0;
}
