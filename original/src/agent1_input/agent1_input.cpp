#include "../../include/agent1_input.h"
#include <iostream>
#include <sstream>
#include <algorithm>

InputData Agent1_Input::receive_user_input() {
    InputData input;
    
    std::cout << "请输入SQL语句：" << std::endl;
    std::getline(std::cin, input.sql);
    
    std::cout << "请输入执行计划：" << std::endl;
    std::getline(std::cin, input.explain_result);
    
    return input;
}

bool Agent1_Input::validate_input(const InputData& input) {
    if (input.sql.empty() || input.explain_result.empty()) {
        return false;
    }
    
    // 检查SQL是否包含基本的关键字
    std::string sql_lower = input.sql;
    std::transform(sql_lower.begin(), sql_lower.end(), sql_lower.begin(), ::tolower);
    
    if (sql_lower.find("select") == std::string::npos) {
        return false;
    }
    
    // 检查执行计划是否包含基本的关键字
    std::string explain_lower = input.explain_result;
    std::transform(explain_lower.begin(), explain_lower.end(), explain_lower.begin(), ::tolower);
    
    if (explain_lower.find("query plan") == std::string::npos && 
        explain_lower.find("explain") == std::string::npos) {
        return false;
    }
    
    return true;
}

void Agent1_Input::process_context_input(ConversationContext& context) {
    // 这个方法在循环迭代工作流中用于处理上下文中的输入
    // 当前实现为空，因为输入处理主要在main函数中完成
    // 可以根据需要添加额外的输入处理逻辑
}
