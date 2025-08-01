#include "../../include/agent3_strategy.h"
#include "../../include/ai_engine.h"
#include "../../include/master_prompt.h"
#include <sstream>
#include <algorithm>

StrategyGenerationResult Agent3_Strategy::generate_strategies(const DiagnosticReport& diagnostic_report, const AIModelConfig& model) {
    StrategyGenerationResult result;
    
    // 构造AI提示词
    std::ostringstream ai_prompt;
    ai_prompt << MasterPrompt::get_master_prompt() << "\n\n";
    ai_prompt << "请根据以下诊断报告生成优化策略：\n\n";
    
    // 添加诊断信息
    ai_prompt << "诊断摘要：" << diagnostic_report.summary << "\n\n";
    ai_prompt << "识别的问题：\n";
    for (const auto& symptom : diagnostic_report.diagnostics) {
        ai_prompt << "- " << symptom.symptom_description << "\n";
        ai_prompt << "  解决路径：" << std::accumulate(symptom.potential_solution_paths.begin(), 
                                                      symptom.potential_solution_paths.end(), 
                                                      std::string(), 
                                                      [](const std::string& a, const std::string& b) {
                                                          return a + (a.empty() ? "" : ", ") + b;
                                                      }) << "\n\n";
    }
    
    ai_prompt << "请为每个问题生成多种来源的优化策略，包括：\n";
    ai_prompt << "1. 基于官方文档的策略（使用openGauss Plan Hint）\n";
    ai_prompt << "2. 通用SQL优化策略\n";
    ai_prompt << "3. 数据库设计策略\n";
    ai_prompt << "4. 启发式优化策略\n\n";
    
    ai_prompt << "请以JSON格式输出策略，格式如下：\n";
    ai_prompt << "{\n";
    ai_prompt << "  \"summary\": \"策略生成摘要\",\n";
    ai_prompt << "  \"optimization_strategies\": [\n";
    ai_prompt << "    {\n";
    ai_prompt << "      \"strategy_id\": \"STRATEGY_1\",\n";
    ai_prompt << "      \"symptom_ref\": \"SYMPTOM_1\",\n";
    ai_prompt << "      \"title\": \"策略标题\",\n";
    ai_prompt << "      \"recommendation\": \"具体建议\",\n";
    ai_prompt << "      \"source_tag\": \"[来源: 官方文档]\",\n";
    ai_prompt << "      \"optimized_sql\": \"优化后的SQL\",\n";
    ai_prompt << "      \"reasoning\": \"优化理由\"\n";
    ai_prompt << "    }\n";
    ai_prompt << "  ]\n";
    ai_prompt << "}\n\n";
    ai_prompt << "请确保每个策略都有明确的来源标签和具体的优化SQL。";
    
    // 调用AI引擎
    std::string ai_response = call_ai_for_strategies(ai_prompt.str(), model);
    
    // 解析AI响应
    try {
        json j = json::parse(ai_response);
        result = parse_strategies_json(j);
    } catch (...) {
        // 如果AI调用失败，使用规则基础的策略生成
        for (const auto& symptom : diagnostic_report.diagnostics) {
            // 为每个症状生成多种来源的策略
            std::vector<OptimizationStrategy> official_strategies = generate_official_document_strategies(symptom);
            std::vector<OptimizationStrategy> general_strategies = generate_general_sql_strategies(symptom);
            std::vector<OptimizationStrategy> design_strategies = generate_database_design_strategies(symptom);
            std::vector<OptimizationStrategy> heuristic_strategies = generate_heuristic_strategies(symptom);
            
            // 合并所有策略
            result.strategies.insert(result.strategies.end(), official_strategies.begin(), official_strategies.end());
            result.strategies.insert(result.strategies.end(), general_strategies.begin(), general_strategies.end());
            result.strategies.insert(result.strategies.end(), design_strategies.begin(), design_strategies.end());
            result.strategies.insert(result.strategies.end(), heuristic_strategies.begin(), heuristic_strategies.end());
        }
        
        // 生成摘要
        std::ostringstream summary;
        summary << "AI策略生成失败，使用规则基础策略，共生成 " << result.strategies.size() << " 个优化策略。";
        result.summary = summary.str();
    }
    
    return result;
}

// 调用AI引擎生成优化策略
std::string Agent3_Strategy::call_ai_for_strategies(const std::string& prompt, const AIModelConfig& model) {
    return call_ai(prompt, model);
}

json Agent3_Strategy::generate_strategies_json(const StrategyGenerationResult& result) {
    json j;
    j["summary"] = result.summary;
    
    json strategies_array = json::array();
    for (const auto& strategy : result.strategies) {
        json strategy_json;
        strategy_json["strategy_id"] = strategy.strategy_id;
        strategy_json["symptom_ref"] = strategy.symptom_ref;
        strategy_json["title"] = strategy.title;
        strategy_json["recommendation"] = strategy.recommendation;
        strategy_json["source_tag"] = strategy.source_tag;
        strategy_json["optimized_sql"] = strategy.optimized_sql;
        strategy_json["reasoning"] = strategy.reasoning;
        strategies_array.push_back(strategy_json);
    }
    j["optimization_strategies"] = strategies_array;
    
    return j;
}

StrategyGenerationResult Agent3_Strategy::parse_strategies_json(const json& j) {
    StrategyGenerationResult result;
    result.summary = j.value("summary", "");
    
    if (j.contains("optimization_strategies") && j["optimization_strategies"].is_array()) {
        for (const auto& strategy_json : j["optimization_strategies"]) {
            OptimizationStrategy strategy;
            strategy.strategy_id = strategy_json.value("strategy_id", "");
            strategy.symptom_ref = strategy_json.value("symptom_ref", "");
            strategy.title = strategy_json.value("title", "");
            strategy.recommendation = strategy_json.value("recommendation", "");
            strategy.source_tag = strategy_json.value("source_tag", "");
            strategy.optimized_sql = strategy_json.value("optimized_sql", "");
            strategy.reasoning = strategy_json.value("reasoning", "");
            
            result.strategies.push_back(strategy);
        }
    }
    
    return result;
}

std::vector<OptimizationStrategy> Agent3_Strategy::generate_official_document_strategies(const Symptom& symptom) {
    std::vector<OptimizationStrategy> strategies;
    
    // 基于官方文档知识生成策略
    if (symptom.symptom_description.find("全表扫描") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "OFFICIAL", 1);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "使用索引扫描Hint优化全表扫描";
        strategy.recommendation = "建议使用 `/*+ indexscan(table_name index_name) */` 强制使用索引扫描";
        strategy.source_tag = MasterPrompt::SOURCE_OFFICIAL_DOC;
        strategy.optimized_sql = "SELECT /*+ indexscan(table_name index_name) */ * FROM table_name WHERE condition;";
        strategy.reasoning = "根据openGauss官方文档，可以使用indexscan hint强制使用索引扫描，避免全表扫描";
        strategies.push_back(strategy);
    }
    
    if (symptom.symptom_description.find("连接") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "OFFICIAL", 2);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "使用Leading Hint优化连接顺序";
        strategy.recommendation = "建议使用 `/*+ leading(table1 table2 table3) */` 指定连接顺序";
        strategy.source_tag = MasterPrompt::SOURCE_OFFICIAL_DOC;
        strategy.optimized_sql = "SELECT /*+ leading(table1 table2 table3) */ * FROM table1, table2, table3 WHERE ...;";
        strategy.reasoning = "根据openGauss官方文档，leading hint可以指定表的连接顺序，优化查询性能";
        strategies.push_back(strategy);
    }
    
    return strategies;
}

std::vector<OptimizationStrategy> Agent3_Strategy::generate_general_sql_strategies(const Symptom& symptom) {
    std::vector<OptimizationStrategy> strategies;
    
    // 基于通用SQL优化知识生成策略
    if (symptom.symptom_description.find("聚合") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "GENERAL", 1);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "使用窗口函数优化聚合操作";
        strategy.recommendation = "考虑使用窗口函数替代自连接进行聚合计算";
        strategy.source_tag = MasterPrompt::SOURCE_GENERAL_SQL;
        strategy.optimized_sql = "SELECT *, ROW_NUMBER() OVER (PARTITION BY col ORDER BY other_col) as rn FROM table;";
        strategy.reasoning = "窗口函数可以避免自连接，提高聚合操作的性能";
        strategies.push_back(strategy);
    }
    
    if (symptom.symptom_description.find("子查询") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "GENERAL", 2);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "将子查询改写为JOIN";
        strategy.recommendation = "将相关子查询改写为JOIN操作，提高查询效率";
        strategy.source_tag = MasterPrompt::SOURCE_GENERAL_SQL;
        strategy.optimized_sql = "SELECT t1.*, t2.col FROM table1 t1 JOIN table2 t2 ON t1.id = t2.id;";
        strategy.reasoning = "JOIN操作通常比子查询更高效，特别是对于大数据集";
        strategies.push_back(strategy);
    }
    
    return strategies;
}

std::vector<OptimizationStrategy> Agent3_Strategy::generate_database_design_strategies(const Symptom& symptom) {
    std::vector<OptimizationStrategy> strategies;
    
    // 基于数据库设计知识生成策略
    if (symptom.symptom_description.find("全表扫描") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "DESIGN", 1);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "创建复合索引优化查询";
        strategy.recommendation = "为经常查询的列组合创建复合索引";
        strategy.source_tag = MasterPrompt::SOURCE_DATABASE_DESIGN;
        strategy.optimized_sql = "CREATE INDEX idx_table_col1_col2 ON table(col1, col2);";
        strategy.reasoning = "复合索引可以同时支持多个查询条件，减少全表扫描";
        strategies.push_back(strategy);
    }
    
    if (symptom.symptom_description.find("排序") != std::string::npos) {
        OptimizationStrategy strategy;
        strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "DESIGN", 2);
        strategy.symptom_ref = symptom.symptom_id;
        strategy.title = "创建覆盖索引避免排序";
        strategy.recommendation = "创建包含所有查询列的覆盖索引，避免额外的排序操作";
        strategy.source_tag = MasterPrompt::SOURCE_DATABASE_DESIGN;
        strategy.optimized_sql = "CREATE INDEX idx_table_covering ON table(col1, col2) INCLUDE (col3, col4);";
        strategy.reasoning = "覆盖索引包含查询所需的所有列，避免回表操作和排序";
        strategies.push_back(strategy);
    }
    
    return strategies;
}

std::vector<OptimizationStrategy> Agent3_Strategy::generate_heuristic_strategies(const Symptom& symptom) {
    std::vector<OptimizationStrategy> strategies;
    
    // 基于启发式知识生成策略
    OptimizationStrategy strategy;
    strategy.strategy_id = generate_strategy_id(symptom.symptom_id, "HEURISTIC", 1);
    strategy.symptom_ref = symptom.symptom_id;
    strategy.title = "更新统计信息优化查询";
    strategy.recommendation = "定期更新表统计信息，确保优化器生成准确的执行计划";
    strategy.source_tag = MasterPrompt::SOURCE_HEURISTIC;
    strategy.optimized_sql = "ANALYZE table_name;";
    strategy.reasoning = "准确的统计信息是优化器生成高效执行计划的基础";
    strategies.push_back(strategy);
    
    return strategies;
}

std::string Agent3_Strategy::generate_strategy_id(const std::string& symptom_id, const std::string& source_type, int index) {
    return symptom_id + "_" + source_type + "_" + std::to_string(index);
}
