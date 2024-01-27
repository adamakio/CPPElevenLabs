#ifndef ELEVENLABS_API_HPP
#define ELEVENLABS_API_HPP

#include <string>
#include "CurlSession.hpp"

#define ELEVENLABS_VERBOSE_OUTPUT 1

namespace elevenlabs {
    // forward declaration for category structures
    class  ElevenLabs;

    // https://elevenlabs.io/docs/api-reference/text-to-speech
    // Convert Text to Speech using ElevenLabs API
    struct TextToSpeech {
        Json create(const std::string& text, const std::string& voice_id, const std::string& model_id);
        void stream(const std::string& text, const std::string& voice_id, const std::string& model_id, StreamResponse* stream_response);

        TextToSpeech(ElevenLabs& elevenlabs) : elevenlabs_{ elevenlabs } {}
    private:
        ElevenLabs& elevenlabs_;
    };


    // https://elevenlabs.io/docs/api-reference/models-get
    // Get a list of models
    struct Models {
        Json list();

        Models(ElevenLabs& elevenlabs) : elevenlabs_{ elevenlabs } {}
    private:
        ElevenLabs& elevenlabs_;
    };

    // https://elevenlabs.io/docs/api-reference/voices
    // Get a list of voices
    struct Voices {
		Json list();
        Json defaultSettings();
        Json voiceSettings(const std::string& voice_id);
        Json editVoiceSettings(const std::string& voice_id, const Json& json);
        Json getVoice(const std::string& voice_id, bool include_settings = false);

		Voices(ElevenLabs& elevenlabs) : elevenlabs_{ elevenlabs } {}
    private:
        ElevenLabs& elevenlabs_;
    };

    // ElevenLabs
    class ElevenLabs {
    public:
        ElevenLabs(const std::string& token = "", const std::string& organization = "", bool throw_exception = true, const std::string& api_base_url = "")
            : session_{ throw_exception }, token_{ token }, organization_{ organization }, throw_exception_{ throw_exception } {
            if (token.empty()) {
                if (const char* env_p = std::getenv("ELEVENLABS_API_KEY")) {
                    token_ = std::string{ env_p };
                    std::cout << "ELEVEN LABS TOKEN: " << token << '\n';
                }
            }
            if (api_base_url.empty()) {
                if (const char* env_p = std::getenv("ELEVENLABS_API_BASE")) {
                    base_url = std::string{ env_p } + "/";
                }
                else {
                    base_url = "https://api.elevenlabs.io/v1/";
                }
            }
            else {
                base_url = api_base_url;
            }
            session_.setUrl(base_url);
            session_.setToken(token_, organization_);
        }

        ElevenLabs(const ElevenLabs&) = delete;
        ElevenLabs& operator=(const ElevenLabs&) = delete;
        ElevenLabs(ElevenLabs&&) = delete;
        ElevenLabs& operator=(ElevenLabs&&) = delete;

        void setProxy(const std::string& url) { session_.setProxyUrl(url); }

        // void change_token(const std::string& token) { token_ = token; };
        void setThrowException(bool throw_exception) { throw_exception_ = throw_exception; }

        void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath, const std::map<std::string, std::string>& fields) { session_.setMultiformPart(filefield_and_filepath, fields); }

        Json post(const std::string& suffix, const std::string& data, const std::string& contentType, const std::string& accept, StreamResponse* stream_response = nullptr) {
            setParameters(suffix, data, contentType);
            std::string authorizationHeader = "xi-api-key: ";
            auto response = session_.postPrepare(contentType, authorizationHeader, accept, stream_response);
            if (response.is_error) {
                trigger_error(response.error_message);
            }

            Json json{};
            if (isJson(response.text)) {
                json = Json::parse(response.text);
                checkResponse(json);
            }
            else {
#if ELEVENLABS_VERBOSE_OUTPUT
                std::cerr << "Response is not a valid JSON";
                std::cout << "<< " << response.text << "\n";
#endif
            }

            return json;
        }

        Json get(const std::string& suffix, const std::string& data = "") {
            setParameters(suffix, data);
            std::string authorizationHeader = "xi-api-key: ";
            std::string accept = "application/json";
            auto response = session_.getPrepare(authorizationHeader, accept);
            if (response.is_error) { trigger_error(response.error_message); }

            Json json{};
            if (isJson(response.text)) {
                json = Json::parse(response.text);
                checkResponse(json);
            }
            else {
#if ELEVENLABS_VERBOSE_OUTPUT
                std::cerr << "Response is not a valid JSON\n";
                std::cout << "<< " << response.text << "\n";
#endif
            }
            return json;
        }

        Json post(const std::string& suffix, const Json& json, const std::string& contentType = "application/json", const std::string& accept = "application/json", StreamResponse * response = nullptr) {
            return post(suffix, json.dump(), contentType, accept, response);
        }

        Json del(const std::string& suffix) {
            setParameters(suffix, "");
            auto response = session_.deletePrepare();
            if (response.is_error) { trigger_error(response.error_message); }

            Json json{};
            if (isJson(response.text)) {
                json = Json::parse(response.text);
                checkResponse(json);
            }
            else {
#if ELEVENLABS_VERBOSE_OUTPUT
                std::cerr << "Response is not a valid JSON\n";
                std::cout << "<< " << response.text << "\n";
#endif
            }
            return json;
        }

        std::string easyEscape(const std::string& text) { return session_.easyEscape(text); }

        void debug() const { std::cout << token_ << '\n'; }

        void setBaseUrl(const std::string& url) {
            base_url = url;
        }

        std::string getBaseUrl() const {
            return base_url;
        }

    private:
        std::string base_url;

        void setParameters(const std::string& suffix, const std::string& data, const std::string& contentType = "") {
            auto complete_url = base_url + suffix;
            session_.setUrl(complete_url);

            if (contentType != "multipart/form-data") {
                session_.setBody(data);
            }

#if ELEVENLABS_VERBOSE_OUTPUT
            std::cout << "<< request: " << complete_url << "  " << data << '\n';
#endif
        }

        void checkResponse(const Json& json) {
            if (json.count("error")) {
                auto reason = json["error"].dump();
                trigger_error(reason);

#if ELEVENLABS_VERBOSE_OUTPUT
                std::cerr << ">> response error :\n" << json.dump(2) << "\n";
#endif
            }
        }

        // as of now the only way
        bool isJson(const std::string& data) {
            bool rc = true;
            try {
                auto json = Json::parse(data); // throws if no json 
            }
            catch (std::exception&) {
                rc = false;
            }
            return(rc);
        }

        void trigger_error(const std::string& msg) {
            if (throw_exception_) {
                throw std::runtime_error(msg);
            }
            else {
                std::cerr << "[OpenAI] error. Reason: " << msg << '\n';
            }
        }

    public:
        TextToSpeech           text_to_speech{ *this };
        Models                 models{ *this };
        Voices                 voices{ *this };

    private:
        Session                 session_;
        std::string             token_;
        std::string             organization_;
        bool                    throw_exception_;
    };


    inline std::string bool_to_string(const bool b) {
        std::ostringstream ss;
        ss << std::boolalpha << b;
        return ss.str();
    }

    inline ElevenLabs& start(const std::string& token = "", const std::string& organization = "", bool throw_exception = true) {
        static ElevenLabs instance{ token, organization, throw_exception };
        return instance;
    }

    inline ElevenLabs& instance() {
        return start();
    }

    inline Json post(const std::string& suffix, const Json& json) {
        return instance().post(suffix, json);
    }

    inline Json get(const std::string& suffix/*, const Json& json*/) {
        return instance().get(suffix);
    }

    // Helper functions to get category structures instance()

    inline TextToSpeech& text_to_speech() {
        return instance().text_to_speech;
    }

    inline Models& models() {
		return instance().models;
	}

    inline Voices& voices() {
		return instance().voices;
	}

    // Definitions of category methods
    // POST 'https://api.elevenlabs.io/v1/text-to-speech/<voice-id>'
    // Creates a new text-to-speech request
    inline Json TextToSpeech::create(const std::string& text, const std::string& voice_id, const std::string& model_id) {
		Json json;
        json["text"] = text;
		json["model_id"] = model_id;
		return elevenlabs_.post("text-to-speech/" + voice_id, json, "application/json", "audio/mpeg");
	}

    // Function to add query parameters to the URL
    std::string buildUrlWithParams(const std::string& baseUrl, const std::map<std::string, std::string>& params) {
        std::string url = baseUrl;
        if (!params.empty()) {
            url += "?";
            for (const auto& param : params) {
                url += param.first + "=" + param.second + "&";
            }
            url.pop_back(); // Remove the last '&' character
        }
        return url;
    }

    // POST 'https://api.elevenlabs.io/v1/text-to-speech/<voice-id>/stream'
    inline void TextToSpeech::stream(const std::string& text, const std::string& voice_id, const std::string& model_id, StreamResponse* stream_response) {
		Json json;
		json["text"] = text;
		json["model_id"] = model_id;
        Json voice_settings;
        voice_settings["similarity_boost"] = 0.75;
        voice_settings["stability"] = 0.5;
        voice_settings["style"] = 0.0;
        voice_settings["use_speaker_boost"] = true;
        json["voice_settings"] = voice_settings;

        // Add query parameters
        std::map<std::string, std::string> queryParams;
        queryParams["optimize_streaming_latency"] = "3";
        queryParams["output_format"] = "pcm_24000"; // Set the desired output format

        // Build the URL with query parameters
        std::string urlWithParams = buildUrlWithParams("text-to-speech/" + voice_id + "/stream", queryParams);


		startStream();
        elevenlabs_.post(urlWithParams, json, "application/json", "audio/mpeg", stream_response);
    }

    // GET 'https://api.elevenlabs.io/v1/models'
    // Lists the currently available models, and provides information abut each one:
    inline Json Models::list() {
        return elevenlabs_.get("models");
    }

    // GET 'https://api.elevenlabs.io/v1/voices'
    // Lists the currently available voices, and provides information abut each one:
    inline Json Voices::list() {
		return elevenlabs_.get("voices");
	}

    // GET 'https://api.elevenlabs.io/v1/voices/settings/default'
    // Returns the default voice settings
    inline Json Voices::defaultSettings() {
        return elevenlabs_.get("voices/settings/default");
    }

    // GET 'https://api.elevenlabs.io/v1/voices/{voice_id}/settings'
    // Returns the voice settings for the given voice
    inline Json Voices::voiceSettings(const std::string& voice_id) {
		return elevenlabs_.get("voices/" + voice_id + "/settings");
	}

    // POST 'https://api.elevenlabs.io/v1/voices/{voice_id}/settings'
    // Updates the voice settings for the given voice
    inline Json Voices::editVoiceSettings(const std::string& voice_id, const Json& json) {
		return elevenlabs_.post("voices/" + voice_id + "/settings/edit", json);
	}

    // GET 'https://api.elevenlabs.io/v1/voices/{voice_id}?with_settings=false'
    // Returns the voice Json
    inline Json Voices::getVoice(const std::string& voice_id, bool include_settings) {
        std::string with_settings = include_settings ? "true" : "false";
        return elevenlabs_.get("voices/" + voice_id + "?with_settings=" + with_settings);
    }

    // 
} // namespace elevenlabs
#endif // !ELEVENLABS_API_HPP
