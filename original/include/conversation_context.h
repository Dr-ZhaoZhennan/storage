#pragma once
#include <string>
#include <vector>
#include <map>

// 会话上下文类，用于管理多轮对话的状态
class ConversationContext {
private:
    std::string original_sql;
    std::string original_explain_plan;
    std::vector<std::pair<std::string, std::string>> conversation_history; // role, content
    std::map<std::string, std::string> additional_info; // 用户补充的信息
    int iteration_count;

public:
    ConversationContext();
    
    // 设置初始数据
    void set_sql(const std::string& sql);
    void set_explain_plan(const std::string& explain);
    
    // 会话历史管理
    void add_to_history(const std::string& role, const std::string& content);
    std::string get_conversation_summary() const;
    
    // 补充信息管理
    void add_additional_info(const std::string& key, const std::string& value);
    std::string get_additional_info_summary() const;
    
    // 获取当前状态
    std::string get_sql() const { return original_sql; }
    std::string get_explain_plan() const { return original_explain_plan; }
    int get_iteration_count() const { return iteration_count; }
    void increment_iteration() { iteration_count++; }
    
    // 获取完整的上下文信息（用于AI分析）
    std::string get_full_context() const;
}; 