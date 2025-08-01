#include <iostream>
#include <vector>
#include <ai_engine.h>

int main() {
    std::cout << "=== AI配置测试程序 ===" << std::endl;
    
    // 测试加载AI配置
    std::vector<AIModelConfig> models;
    if (!load_ai_config("config/ai_models.json", models)) {
        std::cerr << "错误：无法加载AI配置文件" << std::endl;
        return 1;
    }
    
    std::cout << "成功加载 " << models.size() << " 个AI模型：" << std::endl;
    for (const auto& model : models) {
        std::cout << "  - " << model.name << " (" << model.type << ")" << std::endl;
        std::cout << "    URL: " << model.url << std::endl;
        std::cout << "    模型ID: " << model.model_id << std::endl;
        std::cout << "    API密钥: " << (model.api_key.empty() ? "无" : "已设置") << std::endl;
        std::cout << std::endl;
    }
    
    // 测试默认模型
    std::cout << "默认模型配置：" << std::endl;
    if (!models.empty()) {
        const auto& default_model = models[0];
        std::cout << "  - 名称: " << default_model.name << std::endl;
        std::cout << "  - 类型: " << default_model.type << std::endl;
        std::cout << "  - 端口: 11434 (Ollama默认端口)" << std::endl;
    }
    
    std::cout << "\n配置测试完成！" << std::endl;
    return 0;
} 