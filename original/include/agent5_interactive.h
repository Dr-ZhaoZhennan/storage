#pragma once
#include <string>
#include <vector>
#include "conversation_context.h"
#include "agent4_report.h"

// 追问问题结构
struct FollowUpQuestion {
    std::string question_id;
    std::string question_text;
    std::string reasoning;
    std::string expected_value_type;
};

// 交互分析结果
struct InteractiveAnalysisResult {
    std::vector<FollowUpQuestion> questions;
    std::string summary;
    bool should_continue_conversation;
};

// Agent 5: 交互式问答智能体 - 对话策略与上下文富化引擎
class Agent5_Interactive {
public:
    // 分析当前报告并生成追问问题
    static InteractiveAnalysisResult analyze_and_generate_questions(const ConversationContext& context, 
                                                                  const std::string& latest_report);
    
    // 生成完整的交互提示文本
    static std::string generate_interaction_prompt(const InteractiveAnalysisResult& analysis);
    
    // 检查是否需要继续对话
    static bool should_continue_conversation(const std::string& user_response);
    
    // 处理用户响应并更新上下文
    static void process_user_response(ConversationContext& context, const std::string& user_response);
    
private:
    // 分析报告中的信息缺口
    static std::vector<std::string> identify_information_gaps(const std::string& report);
    
    // 基于信息缺口生成问题
    static std::vector<FollowUpQuestion> generate_questions_from_gaps(const std::vector<std::string>& gaps);
    
    // 分析假设性建议
    static std::vector<std::string> identify_assumptions(const std::string& report);
    
    // 生成基于假设的问题
    static std::vector<FollowUpQuestion> generate_questions_from_assumptions(const std::vector<std::string>& assumptions);
    
    // 格式化问题列表
    static std::string format_questions(const std::vector<FollowUpQuestion>& questions);
};
