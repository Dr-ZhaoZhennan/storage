#include "../include/conversation_context.h"
#include <sstream>

ConversationContext::ConversationContext() : iteration_count(0) {}

void ConversationContext::set_sql(const std::string& sql) {
    original_sql = sql;
}

void ConversationContext::set_explain_plan(const std::string& explain) {
    original_explain_plan = explain;
}

void ConversationContext::add_to_history(const std::string& role, const std::string& content) {
    conversation_history.push_back({role, content});
}

std::string ConversationContext::get_conversation_summary() const {
    std::ostringstream oss;
    oss << "=== 会话历史摘要 ===\n";
    for (const auto& entry : conversation_history) {
        oss << "[" << entry.first << "]: " << entry.second << "\n";
    }
    return oss.str();
}

void ConversationContext::add_additional_info(const std::string& key, const std::string& value) {
    additional_info[key] = value;
}

std::string ConversationContext::get_additional_info_summary() const {
    std::ostringstream oss;
    oss << "=== 补充信息摘要 ===\n";
    for (const auto& entry : additional_info) {
        oss << entry.first << ": " << entry.second << "\n";
    }
    return oss.str();
}

std::string ConversationContext::get_full_context() const {
    std::ostringstream oss;
    oss << "=== 完整会话上下文 ===\n";
    oss << "迭代次数: " << iteration_count << "\n\n";
    oss << "原始SQL:\n" << original_sql << "\n";
    oss << "执行计划:\n" << original_explain_plan << "\n\n";
    oss << get_conversation_summary() << "\n";
    oss << get_additional_info_summary() << "\n";
    return oss.str();
} 