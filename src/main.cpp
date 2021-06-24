#include "v8.h"
#include "cffi.h"
#include "libplatform/libplatform.h"
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iterator>
extern "C" {
#include <dlfcn.h>
}

// 当前模块的绝对路径
v8::Persistent<v8::String> currentModuleId;


/**
 * 技术路径path相对于 dir的绝对路径
 * @param path 文件路径
 * @param dir_name 目录
 * @return
 */
std::string getAbsolutePath(const std::string& path,
                            const std::string& dir) {
    std::string absolute_path;
    // 判断是否为绝对路径。在linux 上下。文件以 / 开头
    if ( path[0] == '/') {
        absolute_path = path;
    } else {
        absolute_path = dir + '/' + path;
    }
    std::replace(absolute_path.begin(), absolute_path.end(), '\\', '/');
    std::vector<std::string> segments;
    std::istringstream segment_stream(absolute_path);
    std::string segment;
    while (std::getline(segment_stream, segment, '/')) {
        if (segment == "..") {
            segments.pop_back();
        } else if (segment != ".") {
            segments.push_back(segment);
        }
    }
    std::ostringstream os;
    std::copy(segments.begin(), segments.end() - 1,
              std::ostream_iterator<std::string>(os, "/"));
    os << *segments.rbegin();
    return os.str();
}

/**
 * 读取文件
 * @param path
 * @return
 */
v8::Local<v8::String>  readFile (std::string& path) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    // 读取主模块文件
    std::ifstream in(path.c_str());
    // 如果打开文件失败
    if (!in.is_open()) {
        return v8::Local<v8::String>();
    }
    std::string source;
    char buffer[256];
    // 如果没有读取到文件结束符位置。
    while(!in.eof()){
        in.getline(buffer,256);
        source.append(buffer);
    };
    return v8::String::NewFromUtf8(isolate, source.c_str()).ToLocalChecked();
}

/**
 * 判断字符串是否以某个 后缀为结尾
 * @param str
 * @param suffix
 * @return
 */
inline bool has_suffix(const std::string &str, const std::string &suffix){
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * 获取文件名
 * @param filePath
 * @return
 */
inline std::string getFileName(std::string& filePath) {
    if (filePath.find(std::string("."))  != -1) {
        int index = filePath.find_last_of("/");
        return index == -1 ? filePath : filePath.substr( index+1);
    }
    return std::string("");
}



v8::MaybeLocal<v8::Module> resolveModule (v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
                                          v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer) {
    v8::Isolate* isolate = context->GetIsolate();
    std::string filePath(*v8::String::Utf8Value(isolate, specifier));
    std::string fileName = getFileName(filePath);
    // 共享库
    // 如果为共享库模块，使用粘合模块实现模块的重新组转
    if (has_suffix(fileName, ".so")) {
        // 如果没有 import assert,或者import assert 不为偶数
        if (import_assertions->Length() == 0 || import_assertions->Length() % 2 != 0 || import_assertions->Length() > 12) {
            isolate->ThrowException(v8::String::NewFromUtf8Literal(isolate, "共享库类型参数错误"));
            return {};
        }
        // 打开共享库
        void* handle = dlopen(fileName.c_str(),  RTLD_LAZY);
        for (int index = 0; index < import_assertions->Length();) {
            std::string funName(*v8::String::Utf8Value(isolate, import_assertions->Get(context, 1).As<v8::String>()));
            void* fun = dlsym( handle, fileName.c_str());
            if (!fun) {
                isolate->ThrowException(v8::String::NewFromUtf8(isolate, (std::string(fileName) + "函数不在共享库" + fileName + "中").c_str() ).ToLocalChecked());
                dlclose(handle);
                return {};
            }
            v8::TryCatch tryCatch(isolate);
            // 读取类型信息
            v8::MaybeLocal<v8::Value> importAssert = v8::JSON::Parse(context, import_assertions->Get(context, 1).As<v8::String>());
            // 解析json错误
            if (tryCatch.HasCaught()) {
                isolate->ThrowException(tryCatch.Message()->Get());
                dlclose(handle);
                return {};
            }

            // 根据import assert获取函数的类型信息，从而构建cffi
            v8::Local<v8::Object> typeObject = importAssert.ToLocalChecked().As<v8::Object>();
            int cffiParamsCount = 0;
            void* cffiParams [6] = {};
            v8::MaybeLocal<v8::Value> maybeParams = typeObject->Get(context, v8::String::NewFromUtf8Literal(isolate, "params"));
            if (!maybeParams.IsEmpty()) {
                v8::Local<v8::Array> params = maybeParams.ToLocalChecked().As<v8::Array>();
                if (!params.IsEmpty() && (cffiParamsCount = static_cast<int>(params->Length()))) {
                    for (int paramsIndex = 0; paramsIndex < cffiParamsCount; ++paramsIndex) {

                    }
                }
            }
            index+=2;
        }
    } else if (has_suffix(fileName, ".js")) {

    } else if (has_suffix(fileName, ".json")) {

    } else {
        isolate->ThrowException(v8::String::NewFromUtf8Literal(isolate, "错误文件"));
        return {};
    }
}



int main(int argc, char** argv){

    // 如果没有入口文件
    if (argv[1] == nullptr) {
        argv[1] = const_cast<char*>("./resource/bar.js");
    }
    char workDirBuffer[255];

    // linux 获取工作目录
    getcwd(workDirBuffer,sizeof(workDirBuffer));

    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    // 启动Import Assertions


    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    v8::V8::SetFlagsFromString("--harmony_import_assertions");
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);

        // 把当前目录的目录缓存起来
        currentModuleId.Reset(isolate, v8::String::NewFromUtf8(isolate, workDirBuffer).ToLocalChecked());
        std::string moduleAbsolutePath = getAbsolutePath(argv[1], std::string(workDirBuffer));
        // 读取文件
        v8::Local<v8::String> scriptSource = readFile(moduleAbsolutePath);
        if (scriptSource .IsEmpty()) {
            isolate->ThrowException(v8::String::NewFromUtf8(isolate, (moduleAbsolutePath + "找不到").c_str()).ToLocalChecked());
        }
        // 获取文件名称
        std::string fileName = getFileName(moduleAbsolutePath);
        v8::TryCatch tryCatch(isolate);
        // 构建编译条件
        v8::ScriptOrigin origin(v8::String::NewFromUtf8(isolate, fileName.c_str()).ToLocalChecked(),  0, 0, false, -1, v8::Local<v8::Value>(), false, false, true);
        // 构建编译source
        v8::ScriptCompiler::Source source(scriptSource, origin);
        // 编译模块
        v8::MaybeLocal<v8::Module> module = v8::ScriptCompiler::CompileModule(isolate, &source);
        if (!tryCatch.HasCaught()) {
            // 实例化模块
            module.ToLocalChecked()->InstantiateModule(context, resolveModule).FromJust();
            // 执行模块
            module.ToLocalChecked()->Evaluate(context).ToLocalChecked();
        } else {
            std::cout << *v8::String::Utf8Value(isolate, tryCatch.Message()->Get()) << std::endl;
        }

    }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}