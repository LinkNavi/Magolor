#include "llvm_codegen.hpp"
#include "stdlib_loader.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
// Handle LLVM version differences for Host.h
#if __has_include(<llvm/TargetParser/Host.h>)
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif
#include <variant>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

LLVMCodeGen::LLVMCodeGen(const std::string& moduleName) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    // Find and initialize stdlib
    std::vector<std::string> searchPaths = {
        "./stdlib",
        "/usr/local/share/magolor/stdlib",
        "/usr/share/magolor/stdlib",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.magolor/stdlib"
    };
    
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            stdlibPath = path;
            break;
        }
    }
    
    if (!stdlibPath.empty()) {
        StdLibLoader::instance().init(stdlibPath);
        stdlibInitialized = true;
    }
    
    // Declare runtime and stdlib functions
    declareRuntimeFunctions();
    if (stdlibInitialized) {
        declareStdLibFunctions();
    }
}

LLVMCodeGen::~LLVMCodeGen() = default;

void LLVMCodeGen::setStdLibPath(const std::string& path) {
    stdlibPath = path;
    if (fs::exists(path)) {
        StdLibLoader::instance().init(path);
        stdlibInitialized = true;
        declareStdLibFunctions();
    }
}

void LLVMCodeGen::initStdLib() {
    if (stdlibInitialized) return;
    
    if (!stdlibPath.empty() && fs::exists(stdlibPath)) {
        StdLibLoader::instance().init(stdlibPath);
        stdlibInitialized = true;
    }
}

void LLVMCodeGen::declareRuntimeFunctions() {
    auto voidTy = llvm::Type::getVoidTy(*context);
    auto int8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    auto int8Ty = llvm::Type::getInt8Ty(*context);
    auto int1Ty = llvm::Type::getInt1Ty(*context);
    auto int32Ty = llvm::Type::getInt32Ty(*context);
    auto int64Ty = llvm::Type::getInt64Ty(*context);
    auto doubleTy = llvm::Type::getDoubleTy(*context);
    
    // ========================================================================
    // Memory Management
    // ========================================================================
    module->getOrInsertFunction("malloc", 
        llvm::FunctionType::get(int8PtrTy, {int64Ty}, false));
    module->getOrInsertFunction("free", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_alloc_simple", 
        llvm::FunctionType::get(int8PtrTy, {int64Ty}, false));
    module->getOrInsertFunction("mg_free_simple", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_incref", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_decref", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    
    // ========================================================================
    // String Operations
    // ========================================================================
    module->getOrInsertFunction("mg_string_create", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_create_len", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int32Ty}, false));
    module->getOrInsertFunction("mg_string_destroy", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_concat", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_length", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_compare", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_equals", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_substring", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_string_index_of", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_starts_with", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_ends_with", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_contains", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_trim", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_to_lower", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_to_upper", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_replace", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_repeat", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int32Ty}, false));
    module->getOrInsertFunction("mg_string_char_at", 
        llvm::FunctionType::get(int8Ty, {int8PtrTy, int32Ty}, false));
    module->getOrInsertFunction("mg_string_split", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8Ty}, false));
    module->getOrInsertFunction("mg_string_join", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy}, false));
    
    // ========================================================================
    // Array Operations
    // ========================================================================
    module->getOrInsertFunction("mg_array_create", 
        llvm::FunctionType::get(int8PtrTy, {int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_array_destroy", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_length", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_push", 
        llvm::FunctionType::get(voidTy, {llvm::PointerType::get(int8PtrTy, 0), int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_pop", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_get", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int32Ty}, false));
    module->getOrInsertFunction("mg_array_set", 
        llvm::FunctionType::get(voidTy, {int8PtrTy, int32Ty, int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_clear", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_array_reverse", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    
    // ========================================================================
    // Option Operations
    // ========================================================================
    module->getOrInsertFunction("mg_option_some", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int32Ty}, false));
    module->getOrInsertFunction("mg_option_none", 
        llvm::FunctionType::get(int8PtrTy, {}, false));
    module->getOrInsertFunction("mg_option_destroy", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_option_is_some", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_option_is_none", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_option_unwrap", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_option_unwrap_or", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy}, false));
    
    // ========================================================================
    // I/O Operations
    // ========================================================================
    module->getOrInsertFunction("mg_print", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_println", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_print_int", 
        llvm::FunctionType::get(voidTy, {int32Ty}, false));
    module->getOrInsertFunction("mg_println_int", 
        llvm::FunctionType::get(voidTy, {int32Ty}, false));
    module->getOrInsertFunction("mg_print_float", 
        llvm::FunctionType::get(voidTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_println_float", 
        llvm::FunctionType::get(voidTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_print_bool", 
        llvm::FunctionType::get(voidTy, {int1Ty}, false));
    module->getOrInsertFunction("mg_println_bool", 
        llvm::FunctionType::get(voidTy, {int1Ty}, false));
    module->getOrInsertFunction("mg_eprint", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_eprintln", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_readLine", 
        llvm::FunctionType::get(int8PtrTy, {}, false));
    module->getOrInsertFunction("mg_readChar", 
        llvm::FunctionType::get(int8Ty, {}, false));
    
    // ========================================================================
    // Type Conversions
    // ========================================================================
    module->getOrInsertFunction("mg_int_to_string", 
        llvm::FunctionType::get(int8PtrTy, {int32Ty}, false));
    module->getOrInsertFunction("mg_float_to_string", 
        llvm::FunctionType::get(int8PtrTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_bool_to_string", 
        llvm::FunctionType::get(int8PtrTy, {int1Ty}, false));
    module->getOrInsertFunction("mg_char_to_string", 
        llvm::FunctionType::get(int8PtrTy, {int8Ty}, false));
    module->getOrInsertFunction("mg_string_to_int", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_to_float", 
        llvm::FunctionType::get(doubleTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_string_to_bool", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_parse_int", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_parse_float", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    
    // ========================================================================
    // Math Functions
    // ========================================================================
    module->getOrInsertFunction("mg_abs_int", 
        llvm::FunctionType::get(int32Ty, {int32Ty}, false));
    module->getOrInsertFunction("mg_abs_float", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_sqrt", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_cbrt", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_pow", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_exp", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_log", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_log10", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_log2", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_sin", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_cos", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_tan", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_asin", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_acos", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_atan", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_atan2", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_floor", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_ceil", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_round", 
        llvm::FunctionType::get(doubleTy, {doubleTy}, false));
    module->getOrInsertFunction("mg_fmod", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_min_int", 
        llvm::FunctionType::get(int32Ty, {int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_max_int", 
        llvm::FunctionType::get(int32Ty, {int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_min_float", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_max_float", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_clamp_int", 
        llvm::FunctionType::get(int32Ty, {int32Ty, int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_clamp_float", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy, doubleTy}, false));
    
    // ========================================================================
    // Random Number Generation
    // ========================================================================
    module->getOrInsertFunction("mg_random_init", 
        llvm::FunctionType::get(voidTy, {}, false));
    module->getOrInsertFunction("mg_random_int", 
        llvm::FunctionType::get(int32Ty, {int32Ty, int32Ty}, false));
    module->getOrInsertFunction("mg_random_float", 
        llvm::FunctionType::get(doubleTy, {}, false));
    module->getOrInsertFunction("mg_random_float_range", 
        llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false));
    module->getOrInsertFunction("mg_random_bool", 
        llvm::FunctionType::get(int1Ty, {}, false));
    
    // ========================================================================
    // File System Functions
    // ========================================================================
    module->getOrInsertFunction("mg_file_exists", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_is_file", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_is_directory", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_read", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_write", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_append", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_delete", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_dir_create", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_dir_delete", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_file_size", 
        llvm::FunctionType::get(int64Ty, {int8PtrTy}, false));
    
    // ========================================================================
    // Time Functions
    // ========================================================================
    module->getOrInsertFunction("mg_time_now", 
        llvm::FunctionType::get(int64Ty, {}, false));
    module->getOrInsertFunction("mg_time_now_millis", 
        llvm::FunctionType::get(int64Ty, {}, false));
    module->getOrInsertFunction("mg_time_sleep", 
        llvm::FunctionType::get(voidTy, {int32Ty}, false));
    module->getOrInsertFunction("mg_time_format", 
        llvm::FunctionType::get(int8PtrTy, {int64Ty, int8PtrTy}, false));
    
    // ========================================================================
    // System Functions
    // ========================================================================
    module->getOrInsertFunction("mg_env_get", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_env_set", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_system_exec", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_exit", 
        llvm::FunctionType::get(voidTy, {int32Ty}, false));
    
    // ========================================================================
    // Map Functions
    // ========================================================================
    module->getOrInsertFunction("mg_map_create", 
        llvm::FunctionType::get(int8PtrTy, {int32Ty}, false));
    module->getOrInsertFunction("mg_map_destroy", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_set", 
        llvm::FunctionType::get(voidTy, {int8PtrTy, int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_get", 
        llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_has", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_remove", 
        llvm::FunctionType::get(int1Ty, {int8PtrTy, int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_size", 
        llvm::FunctionType::get(int32Ty, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_map_clear", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    
    // ========================================================================
    // Runtime Lifecycle
    // ========================================================================
    module->getOrInsertFunction("mg_runtime_init", 
        llvm::FunctionType::get(voidTy, {}, false));
    module->getOrInsertFunction("mg_runtime_shutdown", 
        llvm::FunctionType::get(voidTy, {}, false));
    
    // ========================================================================
    // Panic/Assert
    // ========================================================================
    module->getOrInsertFunction("mg_panic", 
        llvm::FunctionType::get(voidTy, {int8PtrTy}, false));
    module->getOrInsertFunction("mg_assert", 
        llvm::FunctionType::get(voidTy, {int1Ty, int8PtrTy}, false));
}

void LLVMCodeGen::declareStdLibFunctions() {
    if (!stdlibInitialized) return;
    
    auto voidTy = llvm::Type::getVoidTy(*context);
    auto int8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    auto int32Ty = llvm::Type::getInt32Ty(*context);
    auto doubleTy = llvm::Type::getDoubleTy(*context);
    
    // Load common stdlib modules
    std::vector<std::string> modules_to_load = {
        "Std.Core.Prelude",
        "Std.IO",
        "Std.String",
        "Std.Array",
        "Std.Math"
    };
    
    for (const auto& modulePath : modules_to_load) {
        auto stdFunctions = StdLibLoader::instance().getFunctions(modulePath);
        
        for (const auto& func : stdFunctions) {
            if (func.isConstant) continue; // Skip constants
            
            // Convert Magolor types to LLVM types
            std::vector<llvm::Type*> paramTypes;
            for (const auto& ptype : func.paramTypes) {
                if (ptype == "int") paramTypes.push_back(int32Ty);
                else if (ptype == "float") paramTypes.push_back(doubleTy);
                else if (ptype == "string") paramTypes.push_back(int8PtrTy);
                else if (ptype == "bool") paramTypes.push_back(llvm::Type::getInt1Ty(*context));
                else paramTypes.push_back(int8PtrTy); // Default to pointer
            }
            
            llvm::Type* returnType = voidTy;
            if (func.returnType == "int") returnType = int32Ty;
            else if (func.returnType == "float") returnType = doubleTy;
            else if (func.returnType == "string") returnType = int8PtrTy;
            else if (func.returnType == "bool") returnType = llvm::Type::getInt1Ty(*context);
            else if (func.returnType != "void") returnType = int8PtrTy;
            
            // Create function declaration
            auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
            std::string mangledName = "mg_std_" + modulePath + "_" + func.name;
            std::replace(mangledName.begin(), mangledName.end(), '.', '_');
            
            module->getOrInsertFunction(mangledName, funcType);
            // Store in our functions map
            functions[func.name] = module->getFunction(mangledName);
        }
    }
}

bool LLVMCodeGen::isStdLibFunction(const std::string& name) {
    if (!stdlibInitialized) return false;
    return StdLibLoader::instance().isStdFunction(name);
}

llvm::Function* LLVMCodeGen::getStdLibFunction(const std::string& name) {
    if (!stdlibInitialized) return nullptr;
    
    auto modulePath = StdLibLoader::instance().getModuleForFunction(name);
    if (modulePath.empty()) return nullptr;
    
    std::string mangledName = "mg_std_" + modulePath + "_" + name;
    std::replace(mangledName.begin(), mangledName.end(), '.', '_');
    
    return module->getFunction(mangledName);
}

llvm::Function* LLVMCodeGen::getRuntimeFunction(const std::string& name) {
    return module->getFunction(name);
}

bool LLVMCodeGen::emitObjectFile(const std::string& filename) {
    // Initialize target
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);
    
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target lookup error: " << error << std::endl;
        return false;
    }
    
    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, llvm::Reloc::PIC_
    );
    
    module->setDataLayout(targetMachine->createDataLayout());
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    llvm::legacy::PassManager pass;
    
    // Handle different LLVM versions for CodeGenFileType
#if LLVM_VERSION_MAJOR >= 18
    auto fileType = llvm::CodeGenFileType::ObjectFile;
#elif LLVM_VERSION_MAJOR >= 10
    auto fileType = llvm::CGFT_ObjectFile;
#else
    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
#endif
    
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "TargetMachine can't emit object file\n";
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}

bool LLVMCodeGen::emitLLVMIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    module->print(dest, nullptr);
    return true;
}

bool LLVMCodeGen::emitAssembly(const std::string& filename) {
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);
    
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target lookup error: " << error << std::endl;
        return false;
    }
    
    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, llvm::Reloc::PIC_
    );
    
    module->setDataLayout(targetMachine->createDataLayout());
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    llvm::legacy::PassManager pass;
    
#if LLVM_VERSION_MAJOR >= 18
    auto fileType = llvm::CodeGenFileType::AssemblyFile;
#elif LLVM_VERSION_MAJOR >= 10
    auto fileType = llvm::CGFT_AssemblyFile;
#else
    auto fileType = llvm::TargetMachine::CGFT_AssemblyFile;
#endif
    
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "TargetMachine can't emit assembly file\n";
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}

bool LLVMCodeGen::linkExecutable(const std::string& objectFile, const std::string& outputFile) {
    // Compile runtime.c
    std::string runtimeObj = "runtime.o";
    std::string compileRuntime = "gcc -c runtime.c -o " + runtimeObj + " 2>&1";
    if (std::system(compileRuntime.c_str()) != 0) {
        std::cerr << "Failed to compile runtime library\n";
        return false;
    }
    
    // Link everything together
    std::string linkCmd = "gcc " + objectFile + " " + runtimeObj + 
                         " -o " + outputFile + " -lm -lstdc++ 2>&1";
    
    FILE* pipe = popen(linkCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run linker\n";
        return false;
    }
    
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int returnCode = pclose(pipe);
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << "Linking failed\n";
        return false;
    }
    
    // Clean up intermediate files
    fs::remove(runtimeObj);
    
    return true;
}

// Type conversion implementation
llvm::Type* LLVMCodeGen::convertType(TypePtr type) {
    if (!type) return llvm::Type::getVoidTy(*context);
    
    switch (type->kind) {
        case Type::INT:
            return llvm::Type::getInt32Ty(*context);
        case Type::FLOAT:
            return llvm::Type::getDoubleTy(*context);
        case Type::BOOL:
            return llvm::Type::getInt1Ty(*context);
        case Type::STRING:
            return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
        case Type::VOID:
            return llvm::Type::getVoidTy(*context);
        case Type::CLASS:
            if (classes.count(type->className)) {
                return llvm::PointerType::get(classes[type->className], 0);
            }
            return llvm::PointerType::get(
                llvm::StructType::create(*context, type->className), 0
            );
        case Type::ARRAY:
            return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
        case Type::OPTION:
            return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
        case Type::FUNCTION: {
            auto retType = convertType(type->returnType);
            std::vector<llvm::Type*> paramTypes;
            for (const auto& pt : type->paramTypes) {
                paramTypes.push_back(convertType(pt));
            }
            return llvm::PointerType::get(
                llvm::FunctionType::get(retType, paramTypes, false), 0
            );
        }
        default:
            return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    }
}

llvm::FunctionType* LLVMCodeGen::convertFunctionType(TypePtr returnType, const std::vector<TypePtr>& paramTypes) {
    std::vector<llvm::Type*> llvmParamTypes;
    for (const auto& pt : paramTypes) {
        llvmParamTypes.push_back(convertType(pt));
    }
    return llvm::FunctionType::get(convertType(returnType), llvmParamTypes, false);
}

bool LLVMCodeGen::generate(const Program& prog) {
    initStdLib();
    
    // Declare all classes
    for (const auto& cls : prog.classes) {
        std::vector<llvm::Type*> fieldTypes;
        for (const auto& field : cls.fields) {
            if (!field.isStatic) {
                fieldTypes.push_back(convertType(field.type));
            }
        }
        auto structType = llvm::StructType::create(*context, fieldTypes, cls.name);
        classes[cls.name] = structType;
    }
    
    // Declare all functions
    for (const auto& fn : prog.functions) {
        genFunctionDecl(fn);
    }
    
    // Generate class methods
    for (const auto& cls : prog.classes) {
        genClass(cls);
    }
    
    // Generate function bodies
    for (const auto& fn : prog.functions) {
        genFunction(fn);
    }
    
    // Verify module
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyModule(*module, &errorStream)) {
        std::cerr << "Module verification failed:\n" << error << std::endl;
        return false;
    }
    
    return true;
}

void LLVMCodeGen::genClass(const ClassDecl& cls) {
    currentClass = const_cast<ClassDecl*>(&cls);
    
    for (const auto& field : cls.fields) {
        if (field.isStatic && field.initValue) {
            genGlobalVar(mangleName(field.name, cls.name), field.type, field.initValue);
        }
    }
    
    for (const auto& method : cls.methods) {
        genFunction(method, mangleName(method.name, cls.name));
    }
    
    currentClass = nullptr;
}

llvm::Function* LLVMCodeGen::genFunctionDecl(const FnDecl& fn, const std::string& mangledName) {
    std::string funcName = mangledName.empty() ? fn.name : mangledName;
    
    if (functions.count(funcName)) {
        return functions[funcName];
    }
    
    std::vector<TypePtr> paramTypes;
    for (const auto& param : fn.params) {
        paramTypes.push_back(param.type);
    }
    
    auto funcType = convertFunctionType(fn.returnType, paramTypes);
    auto func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        funcName,
        module.get()
    );
    
    size_t idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(fn.params[idx++].name);
    }
    
    functions[funcName] = func;
    return func;
}

void LLVMCodeGen::genFunction(const FnDecl& fn, const std::string& mangledName) {
    std::string funcName = mangledName.empty() ? fn.name : mangledName;
    auto func = functions[funcName];
    
    if (!func) {
        func = genFunctionDecl(fn, mangledName);
    }
    
    auto entryBB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entryBB);
    
    currentFunction = func;
    namedValues.clear();
    
    for (auto& arg : func->args()) {
        auto alloca = createEntryBlockAlloca(func, std::string(arg.getName()), arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    
    for (const auto& stmt : fn.body) {
        genStmt(stmt);
    }
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (fn.returnType->kind == Type::VOID) {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(llvm::Constant::getNullValue(convertType(fn.returnType)));
        }
    }
    
    currentFunction = nullptr;
}

llvm::AllocaInst* LLVMCodeGen::createEntryBlockAlloca(llvm::Function* fn, const std::string& varName, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

llvm::Value* LLVMCodeGen::createString(const std::string& str) {
    auto strConstant = llvm::ConstantDataArray::getString(*context, str);
    auto strGlobal = new llvm::GlobalVariable(
        *module,
        strConstant->getType(),
        true,
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );
    
    return builder->CreatePointerCast(strGlobal, llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0));
}

std::string LLVMCodeGen::mangleName(const std::string& name, const std::string& className) {
    if (className.empty()) {
        return name;
    }
    return className + "_" + name;
}

// Statement generation
void LLVMCodeGen::genStmt(const StmtPtr& stmt) {
    std::visit([this](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        
        if constexpr (std::is_same_v<T, LetStmt>) {
            genLetStmt(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            genReturnStmt(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            genExpr(s.expr);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            genIfStmt(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            genWhileStmt(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            genForStmt(s);
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            genMatchStmt(s);
        }
    }, stmt->data);
}

void LLVMCodeGen::genLetStmt(const LetStmt& stmt) {
    auto initVal = genExpr(stmt.init);
    if (!initVal) return;
    
    auto alloca = createEntryBlockAlloca(currentFunction, stmt.name, initVal->getType());
    builder->CreateStore(initVal, alloca);
    namedValues[stmt.name] = alloca;
}

void LLVMCodeGen::genReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        auto retVal = genExpr(stmt.value);
        if (retVal) {
            builder->CreateRet(retVal);
        } else {
            builder->CreateRetVoid();
        }
    } else {
        builder->CreateRetVoid();
    }
}

void LLVMCodeGen::genIfStmt(const IfStmt& stmt) {
    auto condVal = genExpr(stmt.cond);
    if (!condVal) return;
    
    if (condVal->getType() != llvm::Type::getInt1Ty(*context)) {
        condVal = builder->CreateICmpNE(
            condVal,
            llvm::Constant::getNullValue(condVal->getType()),
            "ifcond"
        );
    }
    
    auto thenBB = llvm::BasicBlock::Create(*context, "then", currentFunction);
    auto elseBB = llvm::BasicBlock::Create(*context, "else", currentFunction);
    auto mergeBB = llvm::BasicBlock::Create(*context, "ifcont", currentFunction);
    
    builder->CreateCondBr(condVal, thenBB, elseBB);
    
    builder->SetInsertPoint(thenBB);
    for (const auto& s : stmt.thenBody) {
        genStmt(s);
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    builder->SetInsertPoint(elseBB);
    if (!stmt.elseBody.empty()) {
        for (const auto& s : stmt.elseBody) {
            genStmt(s);
        }
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    builder->SetInsertPoint(mergeBB);
}

void LLVMCodeGen::genWhileStmt(const WhileStmt& stmt) {
    auto condBB = llvm::BasicBlock::Create(*context, "whilecond", currentFunction);
    auto loopBB = llvm::BasicBlock::Create(*context, "whileloop", currentFunction);
    auto afterBB = llvm::BasicBlock::Create(*context, "afterloop", currentFunction);
    
    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);
    
    auto condVal = genExpr(stmt.cond);
    if (!condVal) return;
    
    if (condVal->getType() != llvm::Type::getInt1Ty(*context)) {
        condVal = builder->CreateICmpNE(
            condVal,
            llvm::Constant::getNullValue(condVal->getType()),
            "loopcond"
        );
    }
    
    builder->CreateCondBr(condVal, loopBB, afterBB);
    
    builder->SetInsertPoint(loopBB);
    
    for (const auto& s : stmt.body) {
        genStmt(s);
    }
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }
    
    builder->SetInsertPoint(afterBB);
}

void LLVMCodeGen::genForStmt(const ForStmt& stmt) {
    // For now, generate a simple loop structure
    // Full iterator support would require more work
    
    auto condBB = llvm::BasicBlock::Create(*context, "forcond", currentFunction);
    auto loopBB = llvm::BasicBlock::Create(*context, "forloop", currentFunction);
    auto afterBB = llvm::BasicBlock::Create(*context, "afterfor", currentFunction);
    
    // Evaluate iterable
    auto iterable = genExpr(stmt.iterable);
    if (!iterable) return;
    
    // Create loop counter
    auto counterAlloca = createEntryBlockAlloca(currentFunction, "__iter_counter", llvm::Type::getInt32Ty(*context));
    builder->CreateStore(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)), counterAlloca);
    
    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);
    
    // For now, just create a simple condition (counter < 10 as placeholder)
    auto counter = builder->CreateLoad(llvm::Type::getInt32Ty(*context), counterAlloca, "counter");
    auto cond = builder->CreateICmpSLT(counter, llvm::ConstantInt::get(*context, llvm::APInt(32, 10)), "forcond");
    
    builder->CreateCondBr(cond, loopBB, afterBB);
    
    builder->SetInsertPoint(loopBB);
    
    // Store current element (placeholder - would need array indexing)
    namedValues[stmt.var] = counterAlloca;
    
    for (const auto& s : stmt.body) {
        genStmt(s);
    }
    
    // Increment counter
    auto nextCounter = builder->CreateAdd(counter, llvm::ConstantInt::get(*context, llvm::APInt(32, 1)), "nextcounter");
    builder->CreateStore(nextCounter, counterAlloca);
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }
    
    builder->SetInsertPoint(afterBB);
}

void LLVMCodeGen::genMatchStmt(const MatchStmt& stmt) {
    auto exprVal = genExpr(stmt.expr);
    if (!exprVal) return;
    
    auto mergeBB = llvm::BasicBlock::Create(*context, "matchend", currentFunction);
    
    llvm::BasicBlock* nextCaseBB = nullptr;
    
    for (size_t i = 0; i < stmt.arms.size(); i++) {
        const auto& arm = stmt.arms[i];
        
        auto caseBB = llvm::BasicBlock::Create(*context, "case", currentFunction);
        nextCaseBB = (i + 1 < stmt.arms.size()) 
            ? llvm::BasicBlock::Create(*context, "nextcase", currentFunction)
            : mergeBB;
        
        if (i == 0) {
            builder->CreateBr(caseBB);
        }
        
        builder->SetInsertPoint(caseBB);
        
        // Generate case body
        for (const auto& s : arm.body) {
            genStmt(s);
        }
        
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
        
        if (nextCaseBB != mergeBB) {
            builder->SetInsertPoint(nextCaseBB);
        }
    }
    
    builder->SetInsertPoint(mergeBB);
}

// Expression generation
llvm::Value* LLVMCodeGen::genExpr(const ExprPtr& expr) {
    return std::visit([this](auto&& e) -> llvm::Value* {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return genIntLit(e);
        } else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            return genFloatLit(e);
        } else if constexpr (std::is_same_v<T, StringLitExpr>) {
            return genStringLit(e);
        } else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return genBoolLit(e);
        } else if constexpr (std::is_same_v<T, IdentExpr>) {
            return genIdent(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return genBinaryExpr(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return genUnaryExpr(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return genCallExpr(e);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return genMemberExpr(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return genIndexExpr(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return genAssignExpr(e);
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
            return genLambdaExpr(e);
        } else if constexpr (std::is_same_v<T, NewExpr>) {
            return genNewExpr(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return genArrayExpr(e);
        } else if constexpr (std::is_same_v<T, ThisExpr>) {
            return genThisExpr(e);
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
            // Generate Option Some value
            return genExpr(e.value);
        } else if constexpr (std::is_same_v<T, NoneExpr>) {
            // Generate null pointer for None
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)
            );
        }
        
        return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(*context));
    }, expr->data);
}

llvm::Value* LLVMCodeGen::genIntLit(const IntLitExpr& expr) {
    return llvm::ConstantInt::get(*context, llvm::APInt(32, expr.value, true));
}

llvm::Value* LLVMCodeGen::genFloatLit(const FloatLitExpr& expr) {
    return llvm::ConstantFP::get(*context, llvm::APFloat(expr.value));
}

llvm::Value* LLVMCodeGen::genStringLit(const StringLitExpr& expr) {
    return createString(expr.value);
}

llvm::Value* LLVMCodeGen::genBoolLit(const BoolLitExpr& expr) {
    return llvm::ConstantInt::get(*context, llvm::APInt(1, expr.value ? 1 : 0));
}

llvm::Value* LLVMCodeGen::genIdent(const IdentExpr& expr) {
    auto it = namedValues.find(expr.name);
    if (it == namedValues.end()) {
        // Check if it's a function
        auto funcIt = functions.find(expr.name);
        if (funcIt != functions.end()) {
            return funcIt->second;
        }
        return nullptr;
    }
    
    llvm::Value* val = it->second;
    
    // If it's an alloca, load from it
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(val)) {
        return builder->CreateLoad(allocaInst->getAllocatedType(), val, expr.name);
    }
    
    return val;
}

llvm::Value* LLVMCodeGen::genBinaryExpr(const BinaryExpr& expr) {
    auto L = genExpr(expr.left);
    auto R = genExpr(expr.right);
    
    if (!L || !R) return nullptr;
    
    if (expr.op == "+") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateAdd(L, R, "addtmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFAdd(L, R, "addtmp");
        }
    } else if (expr.op == "-") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateSub(L, R, "subtmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFSub(L, R, "subtmp");
        }
    } else if (expr.op == "*") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateMul(L, R, "multmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFMul(L, R, "multmp");
        }
    } else if (expr.op == "/") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateSDiv(L, R, "divtmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFDiv(L, R, "divtmp");
        }
    } else if (expr.op == "%") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateSRem(L, R, "modtmp");
        }
    } else if (expr.op == "<") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpSLT(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpOLT(L, R, "cmptmp");
        }
    } else if (expr.op == ">") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpSGT(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpOGT(L, R, "cmptmp");
        }
    } else if (expr.op == "<=") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpSLE(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpOLE(L, R, "cmptmp");
        }
    } else if (expr.op == ">=") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpSGE(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpOGE(L, R, "cmptmp");
        }
    } else if (expr.op == "==") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpEQ(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpOEQ(L, R, "cmptmp");
        }
    } else if (expr.op == "!=") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpNE(L, R, "cmptmp");
        } else if (L->getType()->isDoubleTy()) {
            return builder->CreateFCmpONE(L, R, "cmptmp");
        }
    } else if (expr.op == "&&") {
        return builder->CreateAnd(L, R, "andtmp");
    } else if (expr.op == "||") {
        return builder->CreateOr(L, R, "ortmp");
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::genUnaryExpr(const UnaryExpr& expr) {
    auto operand = genExpr(expr.operand);
    if (!operand) return nullptr;
    
    if (expr.op == "-") {
        if (operand->getType()->isIntegerTy()) {
            return builder->CreateNeg(operand, "negtmp");
        } else if (operand->getType()->isDoubleTy()) {
            return builder->CreateFNeg(operand, "negtmp");
        }
    } else if (expr.op == "!") {
        return builder->CreateNot(operand, "nottmp");
    }
    
    return operand;
}

llvm::Value* LLVMCodeGen::genCallExpr(const CallExpr& expr) {
    llvm::Function* func = nullptr;
    
    if (auto* ident = std::get_if<IdentExpr>(&expr.callee->data)) {
        func = getStdLibFunction(ident->name);
        if (!func) {
            func = getRuntimeFunction("mg_" + ident->name);
        }
        if (!func) {
            auto it = functions.find(ident->name);
            if (it != functions.end()) {
                func = it->second;
            }
        }
    } else if (auto* member = std::get_if<MemberExpr>(&expr.callee->data)) {
        // Handle member function calls
        func = getStdLibFunction(member->member);
        if (!func) {
            auto it = functions.find(member->member);
            if (it != functions.end()) {
                func = it->second;
            }
        }
    }
    
    if (!func) {
        return nullptr;
    }
    
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.args) {
        auto val = genExpr(arg);
        if (val) {
            args.push_back(val);
        }
    }
    
    if (func->getReturnType()->isVoidTy()) {
        builder->CreateCall(func, args);
        return nullptr;
    }
    
    return builder->CreateCall(func, args, "calltmp");
}

llvm::Value* LLVMCodeGen::genMemberExpr(const MemberExpr& expr) {
    auto obj = genExpr(expr.object);
    if (!obj) return nullptr;
    
    // For now, just return the object
    // Full struct member access would require GEP instructions
    return obj;
}

llvm::Value* LLVMCodeGen::genIndexExpr(const IndexExpr& expr) {
    auto obj = genExpr(expr.object);
    auto idx = genExpr(expr.index);
    
    if (!obj || !idx) return nullptr;
    
    // Generate array access using GEP
    auto elemPtr = builder->CreateGEP(
        llvm::Type::getInt8Ty(*context),
        obj,
        idx,
        "elemptr"
    );
    
    return builder->CreateLoad(llvm::Type::getInt8Ty(*context), elemPtr, "elem");
}

llvm::Value* LLVMCodeGen::genAssignExpr(const AssignExpr& expr) {
    auto val = genExpr(expr.value);
    if (!val) return nullptr;
    
    if (auto* ident = std::get_if<IdentExpr>(&expr.target->data)) {
        auto it = namedValues.find(ident->name);
        if (it != namedValues.end()) {
            builder->CreateStore(val, it->second);
            return val;
        }
    }
    
    return val;
}

llvm::Value* LLVMCodeGen::genLambdaExpr(const LambdaExpr& expr) {
    // Generate lambda as a nested function
    std::vector<TypePtr> paramTypes;
    for (const auto& param : expr.params) {
        paramTypes.push_back(param.type);
    }
    
    auto funcType = convertFunctionType(expr.returnType, paramTypes);
    
    static int lambdaCount = 0;
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCount++);
    
    auto func = llvm::Function::Create(
        funcType,
        llvm::Function::InternalLinkage,
        lambdaName,
        module.get()
    );
    
    auto prevBB = builder->GetInsertBlock();
    auto prevFunc = currentFunction;
    
    auto entryBB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entryBB);
    currentFunction = func;
    
    size_t idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(expr.params[idx].name);
        auto alloca = createEntryBlockAlloca(func, expr.params[idx].name, arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[expr.params[idx].name] = alloca;
        idx++;
    }
    
    for (const auto& stmt : expr.body) {
        genStmt(stmt);
    }
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (expr.returnType && expr.returnType->kind != Type::VOID) {
            builder->CreateRet(llvm::Constant::getNullValue(convertType(expr.returnType)));
        } else {
            builder->CreateRetVoid();
        }
    }
    
    builder->SetInsertPoint(prevBB);
    currentFunction = prevFunc;
    
    return func;
}

llvm::Value* LLVMCodeGen::genNewExpr(const NewExpr& expr) {
    auto it = classes.find(expr.className);
    if (it == classes.end()) {
        return nullptr;
    }
    
    auto structType = it->second;
    auto dataLayout = module->getDataLayout();
    auto size = dataLayout.getTypeAllocSize(structType);
    
    // Allocate memory
    auto mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) return nullptr;
    
    auto sizeVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), size);
    auto mem = builder->CreateCall(mallocFunc, {sizeVal}, "newtmp");
    
    // Cast to struct pointer
    auto structPtr = builder->CreatePointerCast(
        mem,
        llvm::PointerType::get(structType, 0),
        "objptr"
    );
    
    return structPtr;
}

llvm::Value* LLVMCodeGen::genArrayExpr(const ArrayExpr& expr) {
    if (expr.elements.empty()) {
        return llvm::ConstantPointerNull::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)
        );
    }
    
    // Determine element type from first element
    auto firstVal = genExpr(expr.elements[0]);
    if (!firstVal) return nullptr;
    
    auto elemType = firstVal->getType();
    
    // Allocate array
    auto dataLayout = module->getDataLayout();
    auto elemSize = dataLayout.getTypeAllocSize(elemType);
    auto totalSize = elemSize * expr.elements.size();
    
    auto mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) return nullptr;
    
    auto sizeVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), totalSize);
    auto mem = builder->CreateCall(mallocFunc, {sizeVal}, "arraytmp");
    
    auto arrayPtr = builder->CreatePointerCast(
        mem,
        llvm::PointerType::get(elemType, 0),
        "arrayptr"
    );
    
    // Store elements
    for (size_t i = 0; i < expr.elements.size(); i++) {
        auto val = genExpr(expr.elements[i]);
        if (!val) continue;
        
        auto elemPtr = builder->CreateGEP(
            elemType,
            arrayPtr,
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i),
            "elemptr"
        );
        builder->CreateStore(val, elemPtr);
    }
    
    return arrayPtr;
}

llvm::Value* LLVMCodeGen::genThisExpr(const ThisExpr&) {
    auto it = namedValues.find("this");
    if (it != namedValues.end()) {
        return builder->CreateLoad(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
            it->second,
            "this"
        );
    }
    return nullptr;
}

void LLVMCodeGen::genGlobalVar(const std::string& name, TypePtr type, ExprPtr initExpr) {
    auto llvmType = convertType(type);
    
    llvm::Constant* initializer = nullptr;
    if (initExpr) {
        if (auto* intLit = std::get_if<IntLitExpr>(&initExpr->data)) {
            initializer = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), intLit->value);
        } else if (auto* floatLit = std::get_if<FloatLitExpr>(&initExpr->data)) {
            initializer = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), floatLit->value);
        } else if (auto* boolLit = std::get_if<BoolLitExpr>(&initExpr->data)) {
            initializer = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), boolLit->value ? 1 : 0);
        }
    }
    
    if (!initializer) {
        initializer = llvm::Constant::getNullValue(llvmType);
    }
    
    auto global = new llvm::GlobalVariable(
        *module,
        llvmType,
        false,
        llvm::GlobalValue::InternalLinkage,
        initializer,
        name
    );
    
    globals[name] = global;
}

void LLVMCodeGen::genIncRef(llvm::Value* ptr) {
    // Simplified reference counting - would need runtime support
    (void)ptr;
}

void LLVMCodeGen::genDecRef(llvm::Value* ptr) {
    // Simplified reference counting - would need runtime support
    (void)ptr;
}
