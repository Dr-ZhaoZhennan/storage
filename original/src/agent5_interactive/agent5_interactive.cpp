#include "../../include/agent5_interactive.h"
#include "../../include/master_prompt.h"
#include <sstream>
#include <regex>
#include <unordered_set>

InteractiveAnalysisResult Agent5_Interactive::analyze_and_generate_questions(const ConversationContext& context, 
                                                                          const std::string& latest_report) {
    InteractiveAnalysisResult result;
    
    // 如果迭代次数过多，停止继续提问
    if (context.get_iteration_count() > 5) {
        result.should_continue_conversation = false;
        result.summary = "已达到最大迭代次数，建议用户接受当前方案或退出。";
        return result;
    }
    
    // 分析报告中的信息缺口
    std::vector<std::string> information_gaps = identify_information_gaps(latest_report);
    
    // 分析假设性建议
    std::vector<std::string> assumptions = identify_assumptions(latest_report);
    
    // 基于信息缺口生成问题
    std::vector<FollowUpQuestion> gap_questions = generate_questions_from_gaps(information_gaps);
    
    // 基于假设生成问题
    std::vector<FollowUpQuestion> assumption_questions = generate_questions_from_assumptions(assumptions);
    
    // 合并所有问题并去重
    std::vector<FollowUpQuestion> all_questions;
    all_questions.insert(all_questions.end(), gap_questions.begin(), gap_questions.end());
    all_questions.insert(all_questions.end(), assumption_questions.begin(), assumption_questions.end());
    
    // 去重逻辑：基于question_id去重
    std::unordered_set<std::string> seen_ids;
    for (const auto& question : all_questions) {
        if (seen_ids.find(question.question_id) == seen_ids.end()) {
            result.questions.push_back(question);
            seen_ids.insert(question.question_id);
        }
    }
    
    // 判断是否需要继续对话
    result.should_continue_conversation = !result.questions.empty();
    
    // 生成摘要
    std::ostringstream summary;
    summary << "分析完成，识别出 " << information_gaps.size() << " 个信息缺口和 " 
            << assumptions.size() << " 个假设性建议，生成了 " << result.questions.size() << " 个追问问题。";
    result.summary = summary.str();
    
    return result;
}

std::string Agent5_Interactive::generate_interaction_prompt(const InteractiveAnalysisResult& analysis) {
    std::ostringstream prompt;
    
    prompt << "## 需要您补充的信息\n\n";
    
    if (analysis.questions.empty()) {
        prompt << "当前优化分析已经足够完整，无需进一步补充信息。\n";
        prompt << "您可以：\n";
        prompt << "- 输入 'accept' 接受当前优化方案\n";
        prompt << "- 输入 'exit' 退出优化会话\n";
        prompt << "- 继续提供其他信息以进一步优化\n";
    } else {
        prompt << "为了提供更精准的优化建议，请您补充以下信息：\n\n";
        
        for (size_t i = 0; i < analysis.questions.size(); ++i) {
            const auto& question = analysis.questions[i];
            prompt << "**问题 " << (i + 1) << "**: " << question.question_text << "\n";
            prompt << "**原因**: " << question.reasoning << "\n";
            prompt << "**期望格式**: " << question.expected_value_type << "\n\n";
        }
        
        prompt << "## 如何继续\n";
        prompt << "- 请按上述问题提供相关信息\n";
        prompt << "- 输入 'accept' 接受当前优化方案\n";
        prompt << "- 输入 'exit' 退出优化会话\n";
    }
    
    return prompt.str();
}

bool Agent5_Interactive::should_continue_conversation(const std::string& user_response) {
    std::string lower_response = user_response;
    std::transform(lower_response.begin(), lower_response.end(), lower_response.begin(), ::tolower);
    
    return !(lower_response.find("accept") != std::string::npos || 
             lower_response.find("exit") != std::string::npos ||
             lower_response.find("quit") != std::string::npos);
}

void Agent5_Interactive::process_user_response(ConversationContext& context, const std::string& user_response) {
    // 将用户响应添加到会话历史
    context.add_to_history("User", user_response);
    
    // 提取关键信息并添加到补充信息中
    // 这里可以根据需要实现更复杂的信息提取逻辑
    context.add_additional_info("user_response_" + std::to_string(context.get_iteration_count()), user_response);
}

std::vector<std::string> Agent5_Interactive::identify_information_gaps(const std::string& report) {
    std::vector<std::string> gaps;
    
    // 检查是否缺少表信息
    if (report.find("table_name") != std::string::npos || report.find("index_name") != std::string::npos) {
        gaps.push_back("缺少具体的表名和索引信息");
    }
    
    // 检查是否缺少数据量信息
    if (report.find("全表扫描") != std::string::npos && report.find("数据量") == std::string::npos) {
        gaps.push_back("缺少表的数据量信息");
    }
    
    // 检查是否缺少业务约束
    if (report.find("优化") != std::string::npos && report.find("业务约束") == std::string::npos) {
        gaps.push_back("缺少业务约束信息");
    }
    
    // 检查是否缺少性能要求
    if (report.find("性能") != std::string::npos && report.find("性能要求") == std::string::npos) {
        gaps.push_back("缺少具体的性能要求");
    }
    
    return gaps;
}

std::vector<FollowUpQuestion> Agent5_Interactive::generate_questions_from_gaps(const std::vector<std::string>& gaps) {
    std::vector<FollowUpQuestion> questions;
    
    for (const auto& gap : gaps) {
        FollowUpQuestion question;
        
        if (gap.find("表名") != std::string::npos) {
            question.question_id = "Q_TABLE_INFO";
            question.question_text = "请提供具体的表名和索引名称，以便生成更精确的优化建议。";
            question.reasoning = "需要具体的表名和索引信息来生成准确的Hint和索引建议。";
            question.expected_value_type = "表名: xxx, 索引名: xxx";
        } else if (gap.find("数据量") != std::string::npos) {
            question.question_id = "Q_DATA_VOLUME";
            question.question_text = "请提供相关表的数据量信息（行数、大小等），以便评估优化效果。";
            question.reasoning = "数据量信息有助于评估索引效果和选择合适的优化策略。";
            question.expected_value_type = "表名: xxx, 行数: xxx, 大小: xxx";
        } else if (gap.find("业务约束") != std::string::npos) {
            question.question_id = "Q_BUSINESS_CONSTRAINTS";
            question.question_text = "请说明是否有业务约束（如不能建索引、不能修改表结构等）。";
            question.reasoning = "业务约束会影响可选的优化策略。";
            question.expected_value_type = "约束描述: xxx";
        } else if (gap.find("性能要求") != std::string::npos) {
            question.question_id = "Q_PERFORMANCE_REQUIREMENTS";
            question.question_text = "请说明具体的性能要求（如响应时间、并发量等）。";
            question.reasoning = "性能要求是优化目标的重要参考。";
            question.expected_value_type = "性能要求: xxx";
        }
        
        questions.push_back(question);
    }
    
    return questions;
}

std::vector<std::string> Agent5_Interactive::identify_assumptions(const std::string& report) {
    std::vector<std::string> assumptions;
    
    // 检查是否包含假设性建议
    std::vector<std::string> assumption_keywords = {
        "建议", "可以考虑", "如果", "假设", "可能", "通常"
    };
    
    for (const auto& keyword : assumption_keywords) {
        if (report.find(keyword) != std::string::npos) {
            assumptions.push_back("包含基于假设的建议: " + keyword);
        }
    }
    
    return assumptions;
}

std::vector<FollowUpQuestion> Agent5_Interactive::generate_questions_from_assumptions(const std::vector<std::string>& assumptions) {
    std::vector<FollowUpQuestion> questions;
    
    // 只为第一个假设生成一个问题，避免重复
    if (!assumptions.empty()) {
        FollowUpQuestion question;
        question.question_id = "Q_ASSUMPTION_CONFIRM";
        question.question_text = "请确认上述建议是否符合您的实际情况，如有不符请说明。";
        question.reasoning = "需要确认假设性建议的适用性。";
        question.expected_value_type = "确认或说明: xxx";
        questions.push_back(question);
    }
    
    return questions;
}

std::string Agent5_Interactive::format_questions(const std::vector<FollowUpQuestion>& questions) {
    std::ostringstream formatted;
    
    for (size_t i = 0; i < questions.size(); ++i) {
        const auto& question = questions[i];
        formatted << "问题 " << (i + 1) << ": " << question.question_text << "\n";
        formatted << "原因: " << question.reasoning << "\n";
        formatted << "期望格式: " << question.expected_value_type << "\n\n";
    }
    
    return formatted.str();
}
