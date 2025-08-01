#pragma once
#include <string>

// 预留AI调用接口
std::string call_ai(const std::string& prompt, const std::string& model_name);
void load_ai_config(const std::string& config_path); 