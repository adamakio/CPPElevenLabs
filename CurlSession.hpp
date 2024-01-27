#ifndef CURLSESSION_HPP
#define CURLSESSION_HPP

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <mutex>
#include <cstdlib>
#include <map>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <fstream>

#ifndef CURL_STATICLIB
#include <curl/curl.h>
#else 
#include "curl/curl.h"
#endif

#include <nlohmann/json.hpp>  // nlohmann/json
#include <portaudio.h>

template <typename T>
class LockFreeQueue {
public:
    LockFreeQueue() {}

    void push(const T& item) {
        while (true) {
            auto head = m_head.load(std::memory_order_acquire);
            auto tail = m_tail.load(std::memory_order_relaxed);
            auto next = head + 1;
            if (next == m_capacity) {
                next = 0;
            }
            if (next != tail) {
                if (m_head.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed)) {
                    m_data[head] = item;
                    return;
                }
            }
        }
    }

    bool pop(T* item) {
        while (true) {
            auto tail = m_tail.load(std::memory_order_acquire);
            auto head = m_head.load(std::memory_order_relaxed);
            if (head == tail) {
                return false; // Queue is empty
            }
            *item = m_data[tail];
            auto next = tail + 1;
            if (next == m_capacity) {
                next = 0;
            }
            if (m_tail.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_relaxed)) {
                return true;
            }
        }
    }

private:
    static const size_t m_capacity = 100; // Adjust the capacity as per your requirement
    std::array<T, m_capacity> m_data;
    std::atomic<size_t> m_head = 0;
    std::atomic<size_t> m_tail = 0;
};

PaStream* stream;
LockFreeQueue<int16_t> audioQueue;

// #define   paPrimeOutputBuffersUsingStreamCallback ((PaStreamFlags) 0x00000008)
#define paPrimingOutput    ((PaStreamCallbackFlags) 0x00000010)
#define paOutputUnderflow ((PaStreamCallbackFlags)0x04)
#define paOutputOverflow ((PaStreamCallbackFlags)0x08)
static int AudioCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    int16_t* out = static_cast<int16_t*>(outputBuffer);
    (void)inputBuffer; // Prevent unused variable warning
    int16_t sample;
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        if (audioQueue.pop(&sample)) {
            *out++ = sample;
        }
    }
    return paContinue;
}


void startStream() {
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    // Open an audio I/O stream
    err = Pa_OpenDefaultStream(&stream,
        0,          // No input channels
        1,          // Mono output
        paInt16,    // 16-bit PCM
        24000,      // Sample rate
        2048,        // Frames per buffer
        AudioCallback,      // Callback function
        nullptr);           // No callback data
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }
}

// Remember to close the stream and terminate PortAudio when done
void closeStream() {
    std::cout << "Closing stream...\n";
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}


// Json alias
using Json = nlohmann::json;

struct Response {
    std::string text;
    bool        is_error;
    std::string error_message;
};

class StreamResponse {
public:
    StreamResponse() : is_end_(false) {}

    void reset() {
        std::queue<std::vector<uint8_t>> empty;
        std::swap(chunks_, empty);
        is_end_ = false;
    }

    void set(const std::vector<uint8_t>& data) {
        if (isLikelyMP3Data(data)) {
            std::cout << "Is likely MP3 data\n";
            chunks_.push(data);
            cv_.notify_one(); // Notify one waiting thread
        }
    }

    std::vector<uint8_t> get() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !chunks_.empty(); }); // Wait until chunks_ is not empty
        auto chunk = chunks_.front();
        chunks_.pop();
        return chunk;
    }

    bool is_end() const {
        return is_end_;
    }

private:
    std::queue<std::vector<uint8_t>> chunks_;
    bool is_end_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    bool isLikelyMP3Data(const std::vector<uint8_t>& data) {
        return (data.size() > 200);
    }
};

// Simple curl Session inspired by CPR
class Session {
public:
    Session(bool throw_exception) : throw_exception_{ throw_exception } {
        initCurl();
        ignoreSSL();
    }

    Session(bool throw_exception, std::string proxy_url) : throw_exception_{ throw_exception } {
        initCurl();
        ignoreSSL();
        setProxyUrl(proxy_url);
    }

    ~Session() {
        curl_easy_cleanup(curl_);
        curl_global_cleanup();
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
        }
    }

    void initCurl() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
        if (curl_ == nullptr) {
            throw std::runtime_error("curl cannot initialize"); // here we throw it shouldn't happen
        }
    }

    void ignoreSSL() {
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    void setUrl(const std::string& url) { url_ = url; }

    void setToken(const std::string& token, const std::string& organization) {
        token_ = token;
        organization_ = organization;
    }
    void setProxyUrl(const std::string& url) {
        proxy_url_ = url;
        curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_url_.c_str());

    }

    void setBody(const std::string& data);
    void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath, const std::map<std::string, std::string>& fields);

    Response getPrepare(const std::string& authorizationHeader = "", const std::string& accept = "");
    Response postPrepare(
        const std::string& contentType = "", 
        const std::string& authorizationHeader = "Authorization: Bearer ", 
        const std::string& accept = "", 
        StreamResponse* response = nullptr
    );
    Response deletePrepare();
    Response makeRequest(
        const std::string& contentType = "", 
        const std::string& authorizationHeader = "Authorization: Bearer ", 
        const std::string& accept = "",
        StreamResponse* response = nullptr
   );
    std::string easyEscape(const std::string& text);

private:
    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    static size_t writeStreamFunction(char* ptr, size_t size, size_t nmemb, void* userdata) {
        size_t real_size = size * nmemb;
        if (real_size > 200) {
            std::cout << "Received audio chunk...\n";
            // Assuming incoming audio data is in the form of int16_t samples
            int16_t* audioData = reinterpret_cast<int16_t*>(ptr);
            for (size_t i = 0; i < real_size / sizeof(int16_t); i++) { 
                audioQueue.push(audioData[i]);
            }
        }

        return real_size;
    }

private:
    CURL* curl_;
    CURLcode    res_;
    curl_mime* mime_form_ = nullptr;
    std::string url_;
    std::string proxy_url_;
    std::string token_;
    std::string organization_;

    bool        throw_exception_;
    std::mutex  mutex_request_;
};

inline void Session::setBody(const std::string& data) {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    }
}

inline void Session::setMultiformPart(const std::pair<std::string, std::string>& fieldfield_and_filepath, const std::map<std::string, std::string>& fields) {
    // https://curl.se/libcurl/c/curl_mime_init.html
    if (curl_) {
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
            mime_form_ = nullptr;
        }
        curl_mimepart* field = nullptr;

        mime_form_ = curl_mime_init(curl_);

        field = curl_mime_addpart(mime_form_);
        curl_mime_name(field, fieldfield_and_filepath.first.c_str());
        curl_mime_filedata(field, fieldfield_and_filepath.second.c_str());

        for (const auto& field_pair : fields) {
            field = curl_mime_addpart(mime_form_);
            curl_mime_name(field, field_pair.first.c_str());
            curl_mime_data(field, field_pair.second.c_str(), CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, mime_form_);
    }
}

inline Response Session::getPrepare(const std::string& authorizationHeader, const std::string& accept) {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
    }
    return makeRequest("", authorizationHeader, accept);
}

inline Response Session::postPrepare(const std::string& contentType, const std::string& authorizationHeader, const std::string& accept, StreamResponse* response) {
    return makeRequest(contentType, authorizationHeader, accept, response);
}

inline Response Session::deletePrepare() {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    return makeRequest();
}

inline Response Session::makeRequest(const std::string& contentType, const std::string& authorizationHeader, const std::string& accept, StreamResponse* response) {
    std::lock_guard<std::mutex> lock(mutex_request_);

    struct curl_slist* headers = NULL;
    if (!contentType.empty()) {
        headers = curl_slist_append(headers, std::string{ "Content-Type: " + contentType }.c_str());
        if (contentType == "multipart/form-data") {
            headers = curl_slist_append(headers, "Expect:");
        }
    }
    headers = curl_slist_append(headers, std::string{ authorizationHeader + token_ }.c_str());
    if (!accept.empty()) {
        headers = curl_slist_append(headers, std::string{ "accept: " + accept }.c_str());
    }

    if (!organization_.empty()) {
        headers = curl_slist_append(headers, std::string{ "OpenAI-Organization: " + organization_ }.c_str());
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

    std::string response_string;
    std::string header_string;
    if (response != nullptr) {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeStreamFunction);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*) response);
    }
    else {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    }
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_string);

    res_ = curl_easy_perform(curl_);

    bool is_error = false;
    std::string error_msg{};
    if (res_ != CURLE_OK) {
        is_error = true;
        error_msg = "ElevenLabs curl_easy_perform() failed: " + std::string{ curl_easy_strerror(res_) };
        if (throw_exception_) {
            throw std::runtime_error(error_msg);
        }
        else {
            std::cerr << error_msg << '\n';
        }
    }
    if (response == nullptr) {
        return { response_string, is_error, error_msg };
    }
    else {
		return { "", is_error, error_msg };
	}
}

inline std::string Session::easyEscape(const std::string& text) {
    char* encoded_output = curl_easy_escape(curl_, text.c_str(), static_cast<int>(text.length()));
    const auto str = std::string{ encoded_output };
    curl_free(encoded_output);
    return str;
}


#endif // CURLSESSION_HPP
