#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "agent2_diagnose.h"
#include "master_prompt.h"
#include "ai_engine.h"

using json = nlohmann::json;

// 优化策略结构
struct OptimizationStrategy {
    std::string strategy_id;
    std::string symptom_ref;
    std::string title;
    std::string recommendation;
    std::string source_tag;
    std::string optimized_sql;
    std::string reasoning;
};

// 策略生成结果
struct StrategyGenerationResult {
    std::vector<OptimizationStrategy> strategies;
    std::string summary;
};

// Agent 3: 策略生成智能体 - 多策略并行生成器
class Agent3_Strategy {
public:
    // 根据诊断报告生成带来源标签的优化策略
    static StrategyGenerationResult generate_strategies(const DiagnosticReport& diagnostic_report, const AIModelConfig& model);
    
    // 生成JSON格式的策略报告
    static json generate_strategies_json(const StrategyGenerationResult& result);
    
    // 从JSON解析策略报告
    static StrategyGenerationResult parse_strategies_json(const json& j);
    
private:
    // 为症状生成基于官方文档的策略
    static std::vector<OptimizationStrategy> generate_official_document_strategies(const Symptom& symptom);
    
    // 为症状生成基于通用SQL优化的策略
    static std::vector<OptimizationStrategy> generate_general_sql_strategies(const Symptom& symptom);
    
    // 为症状生成基于数据库设计的策略
    static std::vector<OptimizationStrategy> generate_database_design_strategies(const Symptom& symptom);
    
    // 为症状生成基于启发式设计的策略
    static std::vector<OptimizationStrategy> generate_heuristic_strategies(const Symptom& symptom);
    
    // 生成策略ID
    static std::string generate_strategy_id(const std::string& symptom_id, const std::string& source_type, int index);
    
    // 调用AI引擎生成优化策略
    static std::string call_ai_for_strategies(const std::string& prompt, const AIModelConfig& model);
};
