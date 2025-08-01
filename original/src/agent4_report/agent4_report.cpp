#include "../../include/agent4_report.h"
#include "../../include/master_prompt.h"
#include <sstream>
#include <map>

std::string Agent4_Report::generate_layered_report(const StrategyGenerationResult& strategies) {
    std::ostringstream report;
    
    report << "# SQL优化分析报告\n\n";
    
    // 按来源标签分组策略
    auto grouped_strategies = group_strategies_by_source(strategies.strategies);
    
    // 1. 官方文档来源的建议（最高优先级）
    if (grouped_strategies.find(MasterPrompt::SOURCE_OFFICIAL_DOC) != grouped_strategies.end()) {
        report << format_official_document_suggestions(grouped_strategies[MasterPrompt::SOURCE_OFFICIAL_DOC]);
    }
    
    // 2. 通用SQL优化建议
    if (grouped_strategies.find(MasterPrompt::SOURCE_GENERAL_SQL) != grouped_strategies.end()) {
        report << format_general_sql_suggestions(grouped_strategies[MasterPrompt::SOURCE_GENERAL_SQL]);
    }
    
    // 3. 数据库设计建议
    if (grouped_strategies.find(MasterPrompt::SOURCE_DATABASE_DESIGN) != grouped_strategies.end()) {
        report << format_database_design_suggestions(grouped_strategies[MasterPrompt::SOURCE_DATABASE_DESIGN]);
    }
    
    // 4. 启发式设计建议
    if (grouped_strategies.find(MasterPrompt::SOURCE_HEURISTIC) != grouped_strategies.end()) {
        report << format_heuristic_suggestions(grouped_strategies[MasterPrompt::SOURCE_HEURISTIC]);
    }
    
    // 5. 优化后SQL汇总
    report << generate_optimized_sql_summary(strategies.strategies);
    
    // 6. 预期效果评估
    report << generate_expected_effects(strategies.strategies);
    
    return report.str();
}

std::string Agent4_Report::format_official_document_suggestions(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 1. 基于官方文档的优化建议 " << MasterPrompt::SOURCE_OFFICIAL_DOC << "\n\n";
    
    for (size_t i = 0; i < strategies.size(); ++i) {
        section << "### 1." << (i + 1) << " " << strategies[i].title << "\n";
        section << "**建议**: " << strategies[i].recommendation << "\n";
        section << "**理由**: " << strategies[i].reasoning << "\n";
        section << "**优化SQL**:\n```sql\n" << strategies[i].optimized_sql << "\n```\n\n";
    }
    
    return section.str();
}

std::string Agent4_Report::format_general_sql_suggestions(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 2. 通用SQL优化建议 " << MasterPrompt::SOURCE_GENERAL_SQL << "\n\n";
    
    for (size_t i = 0; i < strategies.size(); ++i) {
        section << "### 2." << (i + 1) << " " << strategies[i].title << "\n";
        section << "**建议**: " << strategies[i].recommendation << "\n";
        section << "**理由**: " << strategies[i].reasoning << "\n";
        section << "**优化SQL**:\n```sql\n" << strategies[i].optimized_sql << "\n```\n\n";
    }
    
    return section.str();
}

std::string Agent4_Report::format_database_design_suggestions(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 3. 数据库设计优化建议 " << MasterPrompt::SOURCE_DATABASE_DESIGN << "\n\n";
    
    for (size_t i = 0; i < strategies.size(); ++i) {
        section << "### 3." << (i + 1) << " " << strategies[i].title << "\n";
        section << "**建议**: " << strategies[i].recommendation << "\n";
        section << "**理由**: " << strategies[i].reasoning << "\n";
        section << "**优化SQL**:\n```sql\n" << strategies[i].optimized_sql << "\n```\n\n";
    }
    
    return section.str();
}

std::string Agent4_Report::format_heuristic_suggestions(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 4. 启发式优化建议 " << MasterPrompt::SOURCE_HEURISTIC << "\n\n";
    
    for (size_t i = 0; i < strategies.size(); ++i) {
        section << "### 4." << (i + 1) << " " << strategies[i].title << "\n";
        section << "**建议**: " << strategies[i].recommendation << "\n";
        section << "**理由**: " << strategies[i].reasoning << "\n";
        section << "**优化SQL**:\n```sql\n" << strategies[i].optimized_sql << "\n```\n\n";
    }
    
    return section.str();
}

std::string Agent4_Report::generate_optimized_sql_summary(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 5. 优化后SQL汇总\n\n";
    
    // 按来源标签分组，优先显示官方文档的建议
    auto grouped_strategies = group_strategies_by_source(strategies);
    
    std::vector<std::string> priority_order = {
        MasterPrompt::SOURCE_OFFICIAL_DOC,
        MasterPrompt::SOURCE_GENERAL_SQL,
        MasterPrompt::SOURCE_DATABASE_DESIGN,
        MasterPrompt::SOURCE_HEURISTIC
    };
    
    for (const auto& source_tag : priority_order) {
        if (grouped_strategies.find(source_tag) != grouped_strategies.end()) {
            section << "### " << source_tag << " 优化SQL\n\n";
            for (const auto& strategy : grouped_strategies[source_tag]) {
                section << "**" << strategy.title << "**\n";
                section << "```sql\n" << strategy.optimized_sql << "\n```\n\n";
            }
        }
    }
    
    return section.str();
}

std::string Agent4_Report::generate_expected_effects(const std::vector<OptimizationStrategy>& strategies) {
    std::ostringstream section;
    section << "## 6. 预期优化效果\n\n";
    
    // 统计各种优化策略的预期效果
    int official_count = 0, general_count = 0, design_count = 0, heuristic_count = 0;
    
    for (const auto& strategy : strategies) {
        if (strategy.source_tag == MasterPrompt::SOURCE_OFFICIAL_DOC) official_count++;
        else if (strategy.source_tag == MasterPrompt::SOURCE_GENERAL_SQL) general_count++;
        else if (strategy.source_tag == MasterPrompt::SOURCE_DATABASE_DESIGN) design_count++;
        else if (strategy.source_tag == MasterPrompt::SOURCE_HEURISTIC) heuristic_count++;
    }
    
    section << "### 优化策略统计\n";
    section << "- 基于官方文档的策略: " << official_count << " 个\n";
    section << "- 通用SQL优化策略: " << general_count << " 个\n";
    section << "- 数据库设计策略: " << design_count << " 个\n";
    section << "- 启发式优化策略: " << heuristic_count << " 个\n\n";
    
    section << "### 预期性能提升\n";
    section << "- **官方文档策略**: 通常可带来显著的性能提升，建议优先实施\n";
    section << "- **通用SQL优化**: 可改善查询逻辑，减少不必要的计算\n";
    section << "- **数据库设计**: 长期优化方案，需要数据库结构调整\n";
    section << "- **启发式优化**: 辅助性优化，建议结合其他策略使用\n\n";
    
    section << "### 实施建议\n";
    section << "1. 优先实施基于官方文档的优化策略\n";
    section << "2. 根据业务情况选择性地应用其他优化策略\n";
    section << "3. 在测试环境中验证优化效果后再应用到生产环境\n";
    section << "4. 定期监控查询性能，根据实际情况调整优化策略\n";
    
    return section.str();
}

std::map<std::string, std::vector<OptimizationStrategy>> Agent4_Report::group_strategies_by_source(const std::vector<OptimizationStrategy>& strategies) {
    std::map<std::string, std::vector<OptimizationStrategy>> grouped;
    
    for (const auto& strategy : strategies) {
        grouped[strategy.source_tag].push_back(strategy);
    }
    
    return grouped;
}

std::string Agent4_Report::format_single_strategy(const OptimizationStrategy& strategy) {
    std::ostringstream formatted;
    formatted << "**" << strategy.title << "** " << strategy.source_tag << "\n";
    formatted << strategy.recommendation << "\n";
    formatted << "```sql\n" << strategy.optimized_sql << "\n```\n";
    return formatted.str();
}
