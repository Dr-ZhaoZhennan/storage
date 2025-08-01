#pragma once
#include <string>
#include "agent3_strategy.h"

// Agent 4: 报告生成智能体 - 分层报告格式化与呈现器
class Agent4_Report {
public:
    // 将策略列表格式化为分层报告
    static std::string generate_layered_report(const StrategyGenerationResult& strategies);
    
    // 格式化官方文档来源的建议
    static std::string format_official_document_suggestions(const std::vector<OptimizationStrategy>& strategies);
    
    // 格式化通用SQL优化建议
    static std::string format_general_sql_suggestions(const std::vector<OptimizationStrategy>& strategies);
    
    // 格式化数据库设计建议
    static std::string format_database_design_suggestions(const std::vector<OptimizationStrategy>& strategies);
    
    // 格式化启发式设计建议
    static std::string format_heuristic_suggestions(const std::vector<OptimizationStrategy>& strategies);
    
    // 生成优化后SQL汇总
    static std::string generate_optimized_sql_summary(const std::vector<OptimizationStrategy>& strategies);
    
    // 生成预期效果评估
    static std::string generate_expected_effects(const std::vector<OptimizationStrategy>& strategies);
    
private:
    // 按来源标签分组策略
    static std::map<std::string, std::vector<OptimizationStrategy>> group_strategies_by_source(const std::vector<OptimizationStrategy>& strategies);
    
    // 格式化单个策略
    static std::string format_single_strategy(const OptimizationStrategy& strategy);
};
