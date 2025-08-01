#pragma once
#include <string>

// 主提示词管理类，包含双知识源和分层标注功能
class MasterPrompt {
private:
    static const std::string OFFICIAL_DOCUMENT_KNOWLEDGE;
    static const std::string GENERAL_OPTIMIZATION_KNOWLEDGE;
    
public:
    // 获取完整的主提示词（包含双知识源）
    static std::string get_master_prompt();
    
    // 获取官方文档知识部分
    static std::string get_official_document_knowledge();
    
    // 获取通用优化知识部分
    static std::string get_general_optimization_knowledge();
    
    // 生成带来源标签的提示词
    static std::string generate_tagged_prompt(const std::string& base_prompt, 
                                            const std::string& source_tag);
    
    // 来源标签常量
    static const std::string SOURCE_OFFICIAL_DOC;
    static const std::string SOURCE_GENERAL_SQL;
    static const std::string SOURCE_DATABASE_DESIGN;
    static const std::string SOURCE_HEURISTIC;
}; 