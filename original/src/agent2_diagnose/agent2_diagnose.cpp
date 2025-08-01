#include "../../include/agent2_diagnose.h"
#include "../../include/ai_engine.h"
#include "../../include/master_prompt.h"
#include <sstream>
#include <regex>

DiagnosticReport Agent2_Diagnose::analyze_plan(const ConversationContext& context, const AIModelConfig& model) {
    DiagnosticReport report;
    
    // 检查输入是否为空
    if (context.get_sql().empty() || context.get_explain_plan().empty()) {
        // 如果输入为空，返回一个默认的诊断
        Symptom default_symptom;
        default_symptom.symptom_id = "SYMPTOM_NO_INPUT";
        default_symptom.symptom_description = "未提供有效的SQL或执行计划，无法进行具体分析";
        default_symptom.potential_solution_paths = {"GENERAL_SQL_REFACTORING", "HEURISTIC"};
        report.diagnostics.push_back(default_symptom);
        report.summary = "由于输入信息不足，无法进行详细诊断。";
        return report;
    }
    
    // 构造AI提示词
    std::ostringstream ai_prompt;
    ai_prompt << MasterPrompt::get_master_prompt() << "\n\n";
    ai_prompt << "请分析以下SQL和执行计划，识别性能问题并生成诊断报告：\n\n";
    ai_prompt << "SQL语句：\n" << context.get_sql() << "\n\n";
    ai_prompt << "执行计划：\n" << context.get_explain_plan() << "\n\n";
    ai_prompt << "请以JSON格式输出诊断结果，格式如下：\n";
    ai_prompt << "{\n";
    ai_prompt << "  \"summary\": \"诊断摘要\",\n";
    ai_prompt << "  \"diagnostics\": [\n";
    ai_prompt << "    {\n";
    ai_prompt << "      \"symptom_id\": \"SYMPTOM_1\",\n";
    ai_prompt << "      \"symptom_description\": \"症状描述\",\n";
    ai_prompt << "      \"potential_solution_paths\": [\"DOCUMENTED_HINT\", \"GENERAL_SQL_REFACTORING\"]\n";
    ai_prompt << "    }\n";
    ai_prompt << "  ]\n";
    ai_prompt << "}\n\n";
    ai_prompt << "请确保每个症状都包含具体的性能问题描述和可能的解决路径。";
    
    // 调用AI引擎
    std::string ai_response = call_ai_for_diagnosis(ai_prompt.str(), model);
    
    // 解析AI响应
    try {
        json j = json::parse(ai_response);
        report = parse_diagnostic_json(j);
    } catch (...) {
        // 如果AI调用失败，使用规则基础的诊断
        report.diagnostics = identify_symptoms(context.get_sql(), context.get_explain_plan());
        std::ostringstream summary;
        summary << "AI诊断失败，使用规则基础诊断，共识别出 " << report.diagnostics.size() << " 个性能症状。";
        report.summary = summary.str();
    }
    
    return report;
}

// 调用AI引擎进行诊断分析
std::string Agent2_Diagnose::call_ai_for_diagnosis(const std::string& prompt, const AIModelConfig& model) {
    return call_ai(prompt, model);
}

json Agent2_Diagnose::generate_diagnostic_json(const DiagnosticReport& report) {
    json j;
    j["summary"] = report.summary;
    
    json diagnostics_array = json::array();
    for (const auto& symptom : report.diagnostics) {
        json symptom_json;
        symptom_json["symptom_id"] = symptom.symptom_id;
        symptom_json["symptom_description"] = symptom.symptom_description;
        symptom_json["potential_solution_paths"] = symptom.potential_solution_paths;
        diagnostics_array.push_back(symptom_json);
    }
    j["diagnostics"] = diagnostics_array;
    
    return j;
}

DiagnosticReport Agent2_Diagnose::parse_diagnostic_json(const json& j) {
    DiagnosticReport report;
    report.summary = j.value("summary", "");
    
    if (j.contains("diagnostics") && j["diagnostics"].is_array()) {
        for (const auto& symptom_json : j["diagnostics"]) {
            Symptom symptom;
            symptom.symptom_id = symptom_json.value("symptom_id", "");
            symptom.symptom_description = symptom_json.value("symptom_description", "");
            
            if (symptom_json.contains("potential_solution_paths") && symptom_json["potential_solution_paths"].is_array()) {
                for (const auto& path : symptom_json["potential_solution_paths"]) {
                    symptom.potential_solution_paths.push_back(path.get<std::string>());
                }
            }
            
            report.diagnostics.push_back(symptom);
        }
    }
    
    return report;
}

std::vector<Symptom> Agent2_Diagnose::identify_symptoms(const std::string& sql, const std::string& explain_plan) {
    std::vector<Symptom> symptoms;
    
    // 基于执行计划分析识别症状
    std::vector<std::string> symptom_patterns = {
        "Seq Scan", "Hash Join", "Nested Loop", "Sort", "Aggregate", 
        "Index Scan", "Bitmap Heap Scan", "Materialize", "Limit"
    };
    
    int symptom_id = 1;
    
    // 检查全表扫描
    if (explain_plan.find("Seq Scan") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "检测到全表扫描，可能存在索引缺失或查询条件无法利用索引的问题";
        symptom.potential_solution_paths = {"DOCUMENTED_HINT", "GENERAL_SQL_REFACTORING", "DATABASE_DESIGN"};
        symptoms.push_back(symptom);
    }
    
    // 检查Hash Join
    if (explain_plan.find("Hash Join") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "使用Hash Join，可能需要调整连接顺序或考虑其他连接方式";
        symptom.potential_solution_paths = {"DOCUMENTED_HINT", "GENERAL_SQL_REFACTORING"};
        symptoms.push_back(symptom);
    }
    
    // 检查Nested Loop
    if (explain_plan.find("Nested Loop") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "使用Nested Loop连接，可能适合小表连接或需要索引优化";
        symptom.potential_solution_paths = {"DOCUMENTED_HINT", "DATABASE_DESIGN"};
        symptoms.push_back(symptom);
    }
    
    // 检查Sort操作
    if (explain_plan.find("Sort") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "检测到排序操作，可能需要索引优化或调整查询结构";
        symptom.potential_solution_paths = {"DOCUMENTED_HINT", "DATABASE_DESIGN", "GENERAL_SQL_REFACTORING"};
        symptoms.push_back(symptom);
    }
    
    // 检查聚合操作
    if (explain_plan.find("Aggregate") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "检测到聚合操作，可能需要优化聚合策略或使用窗口函数";
        symptom.potential_solution_paths = {"GENERAL_SQL_REFACTORING", "HEURISTIC"};
        symptoms.push_back(symptom);
    }
    
    // 检查Materialize
    if (explain_plan.find("Materialize") != std::string::npos) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_" + std::to_string(symptom_id++);
        symptom.symptom_description = "检测到物化操作，可能需要优化子查询或CTE";
        symptom.potential_solution_paths = {"GENERAL_SQL_REFACTORING", "HEURISTIC"};
        symptoms.push_back(symptom);
    }
    
    // 如果没有识别到具体症状，添加通用症状
    if (symptoms.empty()) {
        Symptom symptom;
        symptom.symptom_id = "SYMPTOM_GENERAL";
        symptom.symptom_description = "需要进一步分析执行计划以识别具体性能问题";
        symptom.potential_solution_paths = {"DOCUMENTED_HINT", "GENERAL_SQL_REFACTORING", "DATABASE_DESIGN", "HEURISTIC"};
        symptoms.push_back(symptom);
    }
    
    return symptoms;
}

std::vector<std::string> Agent2_Diagnose::plan_solution_paths(const Symptom& symptom) {
    std::vector<std::string> paths;
    
    // 根据症状类型规划解决路径
    if (symptom.symptom_description.find("全表扫描") != std::string::npos) {
        paths = {"DOCUMENTED_HINT", "DATABASE_DESIGN"};
    } else if (symptom.symptom_description.find("连接") != std::string::npos) {
        paths = {"DOCUMENTED_HINT", "GENERAL_SQL_REFACTORING"};
    } else if (symptom.symptom_description.find("排序") != std::string::npos) {
        paths = {"DOCUMENTED_HINT", "DATABASE_DESIGN", "GENERAL_SQL_REFACTORING"};
    } else {
        paths = {"DOCUMENTED_HINT", "GENERAL_SQL_REFACTORING", "DATABASE_DESIGN", "HEURISTIC"};
    }
    
    return paths;
}
