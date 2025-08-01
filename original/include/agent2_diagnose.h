#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "conversation_context.h"
#include "ai_engine.h"

using json = nlohmann::json;

// 症状结构
struct Symptom {
    std::string symptom_id;
    std::string symptom_description;
    std::vector<std::string> potential_solution_paths;
};

// 诊断报告结构
struct DiagnosticReport {
    std::vector<Symptom> diagnostics;
    std::string summary;
};

// Agent 2: 诊断分析智能体 - 症状分类与路径规划师
class Agent2_Diagnose {
public:
    // 分析执行计划，输出结构化的症状和解决路径
    static DiagnosticReport analyze_plan(const ConversationContext& context, const AIModelConfig& model);
    
    // 生成JSON格式的诊断报告
    static json generate_diagnostic_json(const DiagnosticReport& report);
    
    // 从JSON解析诊断报告
    static DiagnosticReport parse_diagnostic_json(const json& j);
    
private:
    // 识别性能症状
    static std::vector<Symptom> identify_symptoms(const std::string& sql, const std::string& explain_plan);
    
    // 为症状规划解决路径
    static std::vector<std::string> plan_solution_paths(const Symptom& symptom);
    
    // 调用AI引擎进行诊断分析
    static std::string call_ai_for_diagnosis(const std::string& prompt, const AIModelConfig& model);
};
