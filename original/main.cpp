#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <agent1_input.h>
#include <agent2_diagnose.h>
#include <agent3_strategy.h>
#include <agent4_report.h>
#include <agent5_interactive.h>
#include <ai_engine.h>
#include <conversation_context.h>
#include <master_prompt.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 函数声明
int find_model_index(const std::vector<AIModelConfig>& models, const std::string& name);
std::string generate_basic_analysis(const ConversationContext& context);

// 辅助函数：多行输入，END/#END/两次空行结束
std::string multiline_input(const std::string& prompt, bool allow_exit = false) {
    std::cout << prompt << std::endl;
    std::string result, line;
    int empty_count = 0;
    while (true) {
        std::getline(std::cin, line);
        std::string lower_line = line;
        std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
        if (allow_exit && (lower_line == "exit" || lower_line == "quit")) {
            return "__USER_EXIT__";
        }
        if (line == "END" || line == "#END") break;
        if (line.empty()) {
            empty_count++;
            if (empty_count >= 2) break;
            continue;
        } else {
            empty_count = 0;
        }
        result += line + "\n";
    }
    return result;
}

// 获取默认模型名称
std::string get_default_model_name(const std::string& config_path) {
    std::ifstream fin(config_path);
    if (!fin.is_open()) return "";
    
    try {
        json j;
        fin >> j;
        return j.value("default_model", "");
    } catch (...) {
        return "";
    }
}

// 根据名称查找模型配置
AIModelConfig find_model_by_name(const std::vector<AIModelConfig>& models, const std::string& name) {
    for (const auto& model : models) {
        if (model.name == name) {
            return model;
        }
    }
    return models[0]; // 如果没找到，返回第一个
}

// 根据名称查找模型在列表中的索引
int find_model_index(const std::vector<AIModelConfig>& models, const std::string& name) {
    for (size_t i = 0; i < models.size(); ++i) {
        if (models[i].name == name) {
            return i;
        }
    }
    return -1; // 未找到
}

// 简化的快速分析模式
std::string execute_quick_analysis(ConversationContext& context, const std::vector<AIModelConfig>& models, int selected_model_index) {
    std::cout << "正在执行快速分析模式..." << std::endl;
    
    std::ostringstream prompt;
    prompt << "请分析以下SQL并提供优化建议：\n\n";
    prompt << "SQL: " << context.get_sql() << "\n\n";
    prompt << "执行计划: " << context.get_explain_plan() << "\n\n";
    prompt << "请提供：\n";
    prompt << "1. 性能问题分析\n";
    prompt << "2. 优化建议\n";
    prompt << "3. 优化后的SQL\n";
    prompt << "4. 预期效果\n";
    
    std::string result = call_ai(prompt.str(), models[selected_model_index]);
    
    if (result.find("[AI调用失败]") != std::string::npos) {
        return generate_basic_analysis(context);
    }
    
    return result;
}

// 完整的Agent工作流程（带超时控制）
std::string execute_full_agent_workflow(ConversationContext& context, const std::vector<AIModelConfig>& models, int selected_model_index) {
    std::cout << "正在执行完整的Agent工作流程..." << std::endl;
    
    try {
        // Agent 1: 输入处理
        std::cout << "【Agent 1】输入处理..." << std::endl;
        Agent1_Input::process_context_input(context);
        
        // Agent 2: 诊断分析
        std::cout << "【Agent 2】诊断分析..." << std::endl;
        DiagnosticReport diagnostic_report;
        try {
            diagnostic_report = Agent2_Diagnose::analyze_plan(context, models[selected_model_index]);
        } catch (const std::exception& e) {
            std::cerr << "Agent 2异常: " << e.what() << std::endl;
            // 创建默认诊断报告
            Symptom default_symptom;
            default_symptom.symptom_id = "SYMPTOM_DEFAULT";
            default_symptom.symptom_description = "无法进行详细诊断，使用基本分析";
            default_symptom.potential_solution_paths = {"GENERAL_SQL_REFACTORING"};
            diagnostic_report.diagnostics.push_back(default_symptom);
            diagnostic_report.summary = "诊断分析失败，使用基本分析。";
        }
        
        // Agent 3: 策略生成
        std::cout << "【Agent 3】策略生成..." << std::endl;
        StrategyGenerationResult strategies;
        try {
            strategies = Agent3_Strategy::generate_strategies(diagnostic_report, models[selected_model_index]);
        } catch (const std::exception& e) {
            std::cerr << "Agent 3异常: " << e.what() << std::endl;
            // 创建默认策略
            OptimizationStrategy default_strategy;
            default_strategy.strategy_id = "STRATEGY_DEFAULT";
            default_strategy.title = "基本优化建议";
            default_strategy.recommendation = "检查索引和查询条件";
            default_strategy.source_tag = "[来源: 通用优化]";
            default_strategy.optimized_sql = context.get_sql();
            default_strategy.reasoning = "由于策略生成失败，提供基本优化建议";
            strategies.strategies.push_back(default_strategy);
            strategies.summary = "策略生成失败，使用基本策略。";
        }
        
        // Agent 4: 报告生成
        std::cout << "【Agent 4】报告生成..." << std::endl;
        std::string final_report;
        try {
            final_report = Agent4_Report::generate_layered_report(strategies);
        } catch (const std::exception& e) {
            std::cerr << "Agent 4异常: " << e.what() << std::endl;
            final_report = generate_basic_analysis(context);
        }
        
        return final_report;
    } catch (const std::exception& e) {
        std::cerr << "Agent工作流程异常: " << e.what() << std::endl;
        return "Agent工作流程执行失败，使用快速分析模式...\n\n" + execute_quick_analysis(context, models, selected_model_index);
    }
}

// 调用AI引擎进行优化分析（带备用模型机制）
std::string call_ai_for_optimization_with_fallback(const ConversationContext& context, const std::vector<AIModelConfig>& models, int selected_model_index) {
    std::ostringstream prompt;
    prompt << MasterPrompt::get_master_prompt() << "\n\n";
    prompt << "请分析以下SQL和执行计划，提供详细的优化建议：\n\n";
    prompt << "SQL语句：\n" << context.get_sql() << "\n\n";
    prompt << "执行计划：\n" << context.get_explain_plan() << "\n\n";
    
    if (context.get_iteration_count() > 1) {
        prompt << "历史对话信息：\n" << context.get_conversation_summary() << "\n\n";
        prompt << "补充信息：\n" << context.get_additional_info_summary() << "\n\n";
    }
    
    prompt << "请提供以下格式的优化分析：\n";
    prompt << "1. 基于官方文档的优化建议（使用openGauss Plan Hint）\n";
    prompt << "2. 通用SQL优化建议\n";
    prompt << "3. 数据库设计优化建议\n";
    prompt << "4. 启发式优化建议\n";
    prompt << "5. 优化后SQL汇总\n";
    prompt << "6. 预期优化效果\n\n";
    prompt << "请确保每个建议都标注来源，格式如：[来源: 官方文档]";
    
    std::string prompt_str = prompt.str();
    
    // 首先尝试选定的模型
    std::cout << "正在使用主要模型: " << models[selected_model_index].name << std::endl;
    std::string result = call_ai(prompt_str, models[selected_model_index]);
    
    // 如果主要模型失败，尝试其他模型
    if (result.find("[AI调用失败]") != std::string::npos || result.find("Timeout") != std::string::npos) {
        std::cout << "\n主要模型调用失败，正在尝试备用模型..." << std::endl;
        
        for (size_t i = 0; i < models.size(); ++i) {
            if (static_cast<int>(i) == selected_model_index) continue; // 跳过已经失败的模型
            
            std::cout << "尝试备用模型: " << models[i].name << std::endl;
            result = call_ai(prompt_str, models[i]);
            
            if (result.find("[AI调用失败]") == std::string::npos && result.find("Timeout") == std::string::npos) {
                std::cout << "备用模型 " << models[i].name << " 调用成功！" << std::endl;
                break;
            }
        }
        
        // 如果所有模型都失败了，返回一个基本的分析结果
        if (result.find("[AI调用失败]") != std::string::npos || result.find("Timeout") != std::string::npos) {
            std::cout << "\n所有AI模型都调用失败，提供基本分析..." << std::endl;
            result = generate_basic_analysis(context);
        }
    }
    
    return result;
}

// 生成基本分析（当所有AI模型都失败时）
std::string generate_basic_analysis(const ConversationContext& context) {
    std::ostringstream analysis;
    analysis << "=== 基本SQL优化分析 ===\n\n";
    
    analysis << "【执行计划分析】\n";
    analysis << "从提供的执行计划可以看出：\n";
    analysis << "1. 查询涉及多个表的连接操作\n";
    analysis << "2. 存在大量的数据扫描和传输\n";
    analysis << "3. 可能需要优化索引和连接策略\n\n";
    
    analysis << "【优化建议】\n";
    analysis << "1. [来源: 通用优化] 检查相关表的索引情况，确保连接字段有适当的索引\n";
    analysis << "2. [来源: 通用优化] 考虑使用EXPLAIN ANALYZE查看实际的执行时间\n";
    analysis << "3. [来源: 通用优化] 检查WHERE条件的选择性，优化过滤条件\n";
    analysis << "4. [来源: 通用优化] 考虑是否可以使用视图或物化视图优化\n\n";
    
    analysis << "【建议的下一步】\n";
    analysis << "1. 请提供表结构信息（CREATE TABLE语句）\n";
    analysis << "2. 请提供相关表的索引信息\n";
    analysis << "3. 请提供数据量级信息\n";
    analysis << "4. 请提供具体的性能要求\n\n";
    
    analysis << "【注意】\n";
    analysis << "由于AI模型暂时不可用，以上为基本分析。建议：\n";
    analysis << "1. 检查网络连接\n";
    analysis << "2. 验证API密钥是否有效\n";
    analysis << "3. 确认AI服务是否正常运行\n";
    analysis << "4. 稍后重试或联系技术支持\n";
    
    return analysis.str();
}

// 调用AI引擎进行优化分析
std::string call_ai_for_optimization(const ConversationContext& context, const AIModelConfig& model) {
    std::ostringstream prompt;
    prompt << MasterPrompt::get_master_prompt() << "\n\n";
    prompt << "请分析以下SQL和执行计划，提供详细的优化建议：\n\n";
    prompt << "SQL语句：\n" << context.get_sql() << "\n\n";
    prompt << "执行计划：\n" << context.get_explain_plan() << "\n\n";
    
    if (context.get_iteration_count() > 1) {
        prompt << "历史对话信息：\n" << context.get_conversation_summary() << "\n\n";
        prompt << "补充信息：\n" << context.get_additional_info_summary() << "\n\n";
    }
    
    prompt << "请提供以下格式的优化分析：\n";
    prompt << "1. 基于官方文档的优化建议（使用openGauss Plan Hint）\n";
    prompt << "2. 通用SQL优化建议\n";
    prompt << "3. 数据库设计优化建议\n";
    prompt << "4. 启发式优化建议\n";
    prompt << "5. 优化后SQL汇总\n";
    prompt << "6. 预期优化效果\n\n";
    prompt << "请确保每个建议都标注来源，格式如：[来源: 官方文档]";
    
    return call_ai(prompt.str(), model);
}

// 循环迭代式优化会话（带备用模型机制）
void run_optimization_session(const std::vector<AIModelConfig>& models, int selected_model_index) {
    // 初始化会话上下文
    ConversationContext context;
    
    // 获取用户初始输入
    std::cout << "\n【步骤1】请输入SQL语句（可多行，END/#END/两次空行结束）：" << std::endl;
    std::string sql = multiline_input("请粘贴或输入SQL，结束后输入END/#END或连续两次空行：");
    std::cout << "\n已收到SQL语句，内容如下：\n" << sql << std::endl;
    
    std::cout << "\n【步骤2】请输入查询计划分析（EXPLAIN(ANALYZE)结果，可多行，END/#END/两次空行结束）：" << std::endl;
    std::string explain = multiline_input("请粘贴或输入执行计划，结束后输入END/#END或连续两次空行：");
    std::cout << "\n已收到查询计划分析，内容如下：\n" << explain << std::endl;
    
    // 设置初始上下文
    context.set_sql(sql);
    context.set_explain_plan(explain);
    
    // 主循环迭代
    while (true) {
        context.increment_iteration();
        std::cout << "\n===== 第 " << context.get_iteration_count() << " 轮优化分析 =====" << std::endl;
        
        // 1. 执行完整的Agent工作流程 (Agent 1-4)
        std::cout << "\n【步骤3】正在进行智能分析，请稍候……" << std::endl;
        
        // 添加时间戳到上下文，确保每次调用都有不同的缓存键
        context.add_additional_info("iteration_timestamp", std::to_string(context.get_iteration_count()));
        
        // 执行完整的Agent工作流程
        std::string ai_analysis = execute_full_agent_workflow(context, models, selected_model_index);
        
        // 2. 向用户展示报告
        std::cout << "\n===== 当前轮次优化分析报告 =====" << std::endl;
        std::cout << ai_analysis << std::endl;
        context.add_to_history("System", ai_analysis);
        
        // 3. 生成并提出追问问题 (Agent 5)
        InteractiveAnalysisResult interaction = Agent5_Interactive::analyze_and_generate_questions(context, ai_analysis);
        
        if (!interaction.should_continue_conversation) {
            std::cout << "\n===== 优化分析完成 =====" << std::endl;
            std::cout << "当前方案已足够完整，无需进一步补充信息。" << std::endl;
            break;
        }
        
        std::cout << "\n===== 需要您补充信息以继续优化 =====" << std::endl;
        std::string follow_up_prompt = Agent5_Interactive::generate_interaction_prompt(interaction);
        std::cout << follow_up_prompt << std::endl;
        
        // 4. 获取用户输入并决策
        std::string user_response = multiline_input("请输入您的补充信息或问题（多行，END/#END/两次空行结束，或输入exit/quit退出）：", true);
        
        if (user_response == "__USER_EXIT__") {
            std::cout << "\n【用户已选择退出，感谢使用SQL优化助手！】" << std::endl;
            break;
        }
        
        // 处理用户响应并更新上下文
        Agent5_Interactive::process_user_response(context, user_response);
        context.add_to_history("User", user_response);
        
        std::cout << "\n感谢您提供的信息，正在进行新一轮分析...\n" << std::endl;
    }
}

int main() {
    std::cout << "================ Copilot SQL 优化助手 V4.0 ================\n";
    std::cout << "本工具可帮助你分析GaussDB SQL及其执行计划，自动生成优化建议和优化后SQL。" << std::endl;
    std::cout << "\n【新功能特性】" << std::endl;
    std::cout << "✓ 循环迭代式优化：通过多轮对话逐步完善优化方案" << std::endl;
    std::cout << "✓ 双知识源支持：结合官方文档和通用优化知识" << std::endl;
    std::cout << "✓ 分层标注报告：清晰标识每个建议的来源和优先级" << std::endl;
    std::cout << "✓ 智能追问机制：自动识别信息缺口并引导用户补充" << std::endl;
    std::cout << "\n【使用说明】" << std::endl;
    std::cout << "1. 先输入SQL语句（可多行，END/#END/两次空行结束）" << std::endl;
    std::cout << "2. 再输入查询计划分析（EXPLAIN(ANALYZE)结果，可多行，END/#END/两次空行结束）" << std::endl;
    std::cout << "3. 程序将自动调用AI分析并输出建议，如需补充信息会自动提问。" << std::endl;
    std::cout << "4. 在补充信息环节，输入exit或quit并回车可随时结束会诊。" << std::endl;
    std::cout << "====================================================\n";

    // 加载AI模型配置
    std::cout << "\n【步骤1】正在加载AI模型配置……" << std::endl;
    std::vector<AIModelConfig> models;
    if (!load_ai_config("config/ai_models.json", models) || models.empty()) {
        std::cerr << "AI模型配置加载失败！请检查config/ai_models.json。" << std::endl;
        return 1;
    }
    
    // 获取默认模型
    std::string default_model_name = get_default_model_name("config/ai_models.json");
    AIModelConfig model = find_model_by_name(models, default_model_name);
    
    // 显示可用模型列表并让用户选择
    std::cout << "\n可用AI模型列表：" << std::endl;
    for (size_t i = 0; i < models.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << models[i].name << " (" << models[i].type << ")" << std::endl;
    }
    std::cout << "  " << (models.size() + 1) << ". 测试所有AI模型连接" << std::endl;
    
    // 用户选择模型
    int choice = 0;
    while (choice < 1 || choice > static_cast<int>(models.size() + 1)) {
        std::cout << "\n请选择要使用的AI模型 (1-" << (models.size() + 1) << "，默认选择" << (find_model_index(models, default_model_name) + 1) << "): ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) {
            // 使用默认模型
            choice = find_model_index(models, default_model_name) + 1;
        } else {
            try {
                choice = std::stoi(input);
            } catch (...) {
                choice = 0;
            }
        }
        
        if (choice < 1 || choice > static_cast<int>(models.size() + 1)) {
            std::cout << "无效选择，请输入1-" << (models.size() + 1) << "之间的数字。" << std::endl;
        }
    }
    
    // 如果用户选择了测试连接选项
    if (choice == static_cast<int>(models.size() + 1)) {
        test_all_ai_connections(models);
        std::cout << "\n连接测试完成。请重新运行程序选择AI模型。" << std::endl;
        return 0;
    }
    
    model = models[choice - 1];
    std::cout << "\n已选择AI模型：" << model.name << "（类型：" << model.type << "）" << std::endl;
    
    if (model.type == "ollama") {
        std::cout << "注意：使用本地Ollama模型，请确保Ollama服务已启动在端口11434" << std::endl;
        std::cout << "如果Ollama未安装，请访问 https://ollama.ai 安装，或选择其他在线模型。" << std::endl;
    }
    
    // 启动循环迭代式优化会话（带备用模型机制）
    run_optimization_session(models, choice - 1);
    
    // 清理资源
    cleanup_ai_engine();
    
    return 0;
}
