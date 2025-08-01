#pragma once
#include <string>
#include <vector>

struct AIModelConfig {
    std::string name;
    std::string url;
    std::string api_key;
    std::string model_id;
    std::string type;  // "ollama" 或 "openai"
};

bool load_ai_config(const std::string& path, std::vector<AIModelConfig>& models);
std::string call_ai(const std::string& prompt, const AIModelConfig& model);
bool test_ai_connection(const AIModelConfig& model);
void test_all_ai_connections(const std::vector<AIModelConfig>& models);
void cleanup_ai_engine();
void clear_cache();
