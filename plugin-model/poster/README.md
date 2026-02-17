## UOS AI模型插件

1. DemoPlugin是插件类，用于插件的加载，模型的创建；

2. DemoLlm是模型类，用于调用模型接口、处理回复数据、错误处理、中断机制。
3. include文件夹中的两个头文件需放在项目文件中。
4. DemoLLM::generate()方法。模型接口调用、数据收发处理都在此方法中。具体实现方法已在代码中注释。
5. 插件编译后会生成的.so文件需放在{LIB_PLATFORM_DIR}uos-ai-assistant/llm/目录中。如：/usr/local/lib/x86_64-linux-gnu/uos-ai-assistant/llm/libdemo.so
6. 启动UOS-AI便可加载模型插件。





