# 指定C++编译器，这里使用g++
CXX = g++

# 编译选项：
# -std=c++11：使用C++11标准
# -Iinclude：添加头文件搜索路径为include目录
# -Wall：启用所有警告
# -O2：优化级别2
CXXFLAGS = -std=c++11 -Iinclude -Wall -O2

# 链接选项：
# -lcurl：链接libcurl库用于HTTP请求
# nlohmann/json是header-only库，不需要链接
LDFLAGS = -lcurl

# 源文件列表，包含主程序和各个功能模块的实现文件
SRC = main.cpp \
    src/conversation_context.cpp \
    src/master_prompt.cpp \
    src/agent1_input/agent1_input.cpp \
    src/agent2_diagnose/agent2_diagnose.cpp \
    src/agent3_strategy/agent3_strategy.cpp \
    src/agent4_report/agent4_report.cpp \
    src/agent5_interactive/agent5_interactive.cpp \
    src/ai_engine/ai_engine.cpp \
    src/utils/utils.cpp

# 对应的目标文件列表（.cpp替换为.o）
OBJ = $(SRC:.cpp=.o)

# 生成主程序main的规则
# 依赖于所有源文件
main: $(SRC)
	$(CXX) $(CXXFLAGS) -o main $(SRC) $(LDFLAGS)

# 清理编译生成的文件
clean:
	rm -f main *.o src/*/*.o src/*.o

# 安装依赖（Ubuntu/Debian系统）
install-deps:
	sudo apt-get update
	sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev

# 检查Ollama服务状态
check-ollama:
	@echo "检查Ollama服务状态..."
	@curl -s http://localhost:11434/api/tags > /dev/null && echo "Ollama服务运行正常" || echo "警告：Ollama服务未运行，请启动Ollama服务"

# 编译并运行
run: main
	./main

# 开发模式编译（带调试信息）
debug: CXXFLAGS += -g -DDEBUG
debug: main

# 发布模式编译（优化级别3）
release: CXXFLAGS += -O3 -DNDEBUG
release: main
