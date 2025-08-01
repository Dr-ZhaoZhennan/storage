#pragma once
#include <string>
#include "conversation_context.h"

struct InputData {
    std::string sql;
    std::string explain_result;
};

// Agent 1: 输入处理智能体
class Agent1_Input {
public:
    // 接收并标准化用户输入
    static InputData receive_user_input();
    
    // 验证输入有效性
    static bool validate_input(const InputData& input);
    
    // 处理会话上下文中的输入
    static void process_context_input(ConversationContext& context);
};
