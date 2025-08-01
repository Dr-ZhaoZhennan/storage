#include <ai_engine.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <memory>

using json = nlohmann::json;

// 全局缓存和连接池
static std::unordered_map<std::string, std::string> ai_response_cache;
static std::mutex cache_mutex;
static CURL* global_curl_handle = nullptr;
static std::mutex curl_mutex;

// 初始化全局CURL句柄
void init_global_curl() {
    std::lock_guard<std::mutex> lock(curl_mutex);
    if (!global_curl_handle) {
        global_curl_handle = curl_easy_init();
        if (global_curl_handle) {
            // 设置全局CURL选项
            curl_easy_setopt(global_curl_handle, CURLOPT_TIMEOUT, 60L);
            curl_easy_setopt(global_curl_handle, CURLOPT_CONNECTTIMEOUT, 15L);
            curl_easy_setopt(global_curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(global_curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(global_curl_handle, CURLOPT_USERAGENT, "Copilot-SQL-Optimizer/4.0");
            curl_easy_setopt(global_curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(global_curl_handle, CURLOPT_TCP_KEEPIDLE, 120L);
            curl_easy_setopt(global_curl_handle, CURLOPT_TCP_KEEPINTVL, 60L);
        }
    }
}

// 清理全局CURL句柄
void cleanup_global_curl() {
    std::lock_guard<std::mutex> lock(curl_mutex);
    if (global_curl_handle) {
        curl_easy_cleanup(global_curl_handle);
        global_curl_handle = nullptr;
    }
}

// 生成缓存键
std::string generate_cache_key(const std::string& prompt, const std::string& model_name) {
    // 更复杂的缓存键生成，包含时间戳避免重复缓存
    std::hash<std::string> hasher;
    auto now = std::chrono::system_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    
    std::string cache_key_input = prompt + model_name + std::to_string(millis / 60000); // 每分钟更新一次
    return std::to_string(hasher(cache_key_input));
}

// 检查缓存
std::string check_cache(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = ai_response_cache.find(cache_key);
    if (it != ai_response_cache.end()) {
        std::cout << "✓ 使用缓存结果" << std::endl;
        return it->second;
    }
    return "";
}

// 存储到缓存
void store_cache(const std::string& cache_key, const std::string& response) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    ai_response_cache[cache_key] = response;
}

// 清理缓存
void clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    ai_response_cache.clear();
}

bool load_ai_config(const std::string& path, std::vector<AIModelConfig>& models) {
    std::ifstream fin(path);
    if (!fin.is_open()) return false;
    json j;
    fin >> j;
    if (!j.contains("models")) return false;
    for (const auto& m : j["models"]) {
        AIModelConfig cfg;
        cfg.name = m.value("name", "");
        cfg.url = m.value("url", "");
        cfg.api_key = m.value("api_key", "");
        cfg.model_id = m.value("model_id", "");
        cfg.type = m.value("type", "openai");  // 默认为openai类型
        models.push_back(cfg);
    }
    return !models.empty();
}

// libcurl回调，收集AI回复
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 优化的AI调用函数
std::string call_ai_optimized(const std::string& prompt, const AIModelConfig& model) {
    // 生成缓存键
    std::string cache_key = generate_cache_key(prompt, model.name);
    
    // 检查缓存
    std::string cached_response = check_cache(cache_key);
    if (!cached_response.empty()) {
        return cached_response;
    }
    
    // 初始化全局CURL
    init_global_curl();
    
    std::lock_guard<std::mutex> lock(curl_mutex);
    if (!global_curl_handle) {
        return "[AI调用失败] CURL初始化失败";
    }
    
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    
    // 根据模型类型设置不同的请求头
    if (model.type == "ollama") {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    } else {
        // OpenAI兼容接口
        if (!model.api_key.empty()) {
            headers = curl_slist_append(headers, ("Authorization: Bearer " + model.api_key).c_str());
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    // 根据模型类型构造不同的请求体
    json body;
    if (model.type == "ollama") {
        // Ollama API格式
        body["model"] = model.model_id;
        body["messages"] = {
            { {"role", "system"}, {"content", "你是GaussDB SQL优化专家。请根据用户输入的SQL和EXPLAIN(ANALYZE)结果，输出详细优化建议和优化后SQL。"} },
            { {"role", "user"}, {"content", prompt} }
        };
        body["stream"] = false;
    } else {
        // OpenAI兼容格式
        body["model"] = model.model_id;
        body["messages"] = {
            { {"role", "system"}, {"content", "你是GaussDB SQL优化专家。请根据用户输入的SQL和EXPLAIN(ANALYZE)结果，输出详细优化建议和优化后SQL。"} },
            { {"role", "user"}, {"content", prompt} }
        };
        body["stream"] = false;
    }
    
    std::string body_str = body.dump();

    curl_easy_setopt(global_curl_handle, CURLOPT_URL, model.url.c_str());
    curl_easy_setopt(global_curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(global_curl_handle, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(global_curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(global_curl_handle, CURLOPT_WRITEDATA, &readBuffer);
    
    // 优化超时设置
    curl_easy_setopt(global_curl_handle, CURLOPT_TIMEOUT, 45L);  // 减少到45秒
    curl_easy_setopt(global_curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);  // 减少到10秒
    
    CURLcode res = curl_easy_perform(global_curl_handle);
    long http_code = 0;
    curl_easy_getinfo(global_curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        return std::string("[AI调用失败] ") + curl_easy_strerror(res);
    }
    
    // 检查HTTP状态码
    if (http_code != 200) {
        return std::string("[AI调用失败] HTTP状态码: ") + std::to_string(http_code) + " - " + readBuffer;
    }
    
    // 解析AI回复
    try {
        auto j = json::parse(readBuffer);
        std::string response_content;
        
        if (model.type == "ollama") {
            // Ollama响应格式
            if (j.contains("message") && j["message"].contains("content")) {
                response_content = j["message"]["content"].get<std::string>();
            } else {
                return "[Ollama回复解析失败] " + readBuffer;
            }
        } else {
            // OpenAI兼容响应格式
            if (j.contains("choices") && j["choices"].size() > 0 && j["choices"][0]["message"].contains("content")) {
                response_content = j["choices"][0]["message"]["content"].get<std::string>();
            } else {
                return "[AI回复解析失败] " + readBuffer;
            }
        }
        
        // 存储到缓存
        store_cache(cache_key, response_content);
        return response_content;
        
    } catch (const std::exception& e) {
        return "[AI回复JSON解析异常] " + readBuffer;
    }
}

// 重试机制：最多重试3次，每次间隔递增
std::string call_ai_with_retry(const std::string& prompt, const AIModelConfig& model, int max_retries = 3) {
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        std::cout << "正在调用AI模型: " << model.name << " (第" << attempt << "次尝试)..." << std::endl;
        
        std::string result = call_ai_optimized(prompt, model);
        
        if (result.find("[AI调用失败]") == std::string::npos && result.find("Timeout") == std::string::npos) {
            std::cout << "AI调用成功！" << std::endl;
            return result;
        }
        
        if (attempt < max_retries) {
            std::cout << "等待 " << (attempt * 2) << " 秒后重试..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(attempt * 2));
        }
    }
    
    return "[AI调用失败] 所有重试都失败了";
}

std::string call_ai(const std::string& prompt, const AIModelConfig& model) {
    return call_ai_with_retry(prompt, model);
}

// 测试AI模型连接
bool test_ai_connection(const AIModelConfig& model) {
    std::cout << "正在测试AI模型连接: " << model.name << " (" << model.url << ")" << std::endl;
    
    init_global_curl();
    
    std::lock_guard<std::mutex> lock(curl_mutex);
    if (!global_curl_handle) {
        std::cerr << "curl初始化失败" << std::endl;
        return false;
    }
    
    // 构造简单的测试请求
    json test_body;
    if (model.type == "ollama") {
        test_body["model"] = model.model_id;
        test_body["messages"] = {
            { {"role", "user"}, {"content", "Hello"} }
        };
        test_body["stream"] = false;
    } else {
        test_body["model"] = model.model_id;
        test_body["messages"] = {
            { {"role", "user"}, {"content", "Hello"} }
        };
        test_body["stream"] = false;
    }
    
    std::string body_str = test_body.dump();
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    
    if (model.type == "ollama") {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    } else {
        if (!model.api_key.empty()) {
            headers = curl_slist_append(headers, ("Authorization: Bearer " + model.api_key).c_str());
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    
    curl_easy_setopt(global_curl_handle, CURLOPT_URL, model.url.c_str());
    curl_easy_setopt(global_curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(global_curl_handle, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(global_curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(global_curl_handle, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(global_curl_handle, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(global_curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(global_curl_handle);
    long http_code = 0;
    curl_easy_getinfo(global_curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        std::cerr << "连接测试失败: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    if (http_code == 200) {
        std::cout << "✓ 连接测试成功！" << std::endl;
        return true;
    } else {
        std::cerr << "连接测试失败，HTTP状态码: " << http_code << std::endl;
        return false;
    }
}

// 测试所有AI模型连接
void test_all_ai_connections(const std::vector<AIModelConfig>& models) {
    std::cout << "\n=== AI模型连接测试 ===" << std::endl;
    
    bool any_success = false;
    for (size_t i = 0; i < models.size(); ++i) {
        std::cout << "\n[" << (i + 1) << "/" << models.size() << "] ";
        if (test_ai_connection(models[i])) {
            any_success = true;
        }
    }
    
    if (any_success) {
        std::cout << "\n✓ 至少有一个AI模型可以正常连接" << std::endl;
    } else {
        std::cout << "\n✗ 所有AI模型都无法连接，请检查：" << std::endl;
        std::cout << "1. 网络连接是否正常" << std::endl;
        std::cout << "2. API密钥是否有效" << std::endl;
        std::cout << "3. AI服务是否正常运行" << std::endl;
        std::cout << "4. 防火墙设置是否允许连接" << std::endl;
    }
}

// 程序退出时清理资源
void cleanup_ai_engine() {
    cleanup_global_curl();
    clear_cache();
}
