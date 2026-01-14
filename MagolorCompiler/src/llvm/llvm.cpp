// codegen_llvm.cpp - LLVM IR Code Generator for Magolor
// Part 1: Core initialization, type conversion, and class generation

#include "codegen_llvm.hpp"
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <sstream>

// =============================================================================
// Constructor / Destructor
// =============================================================================

LLVMCodeGen::LLVMCodeGen()
    : context(std::make_unique<llvm::LLVMContext>()), module(nullptr),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      currentFunction(nullptr), currentThis(nullptr),
      currentLoopContinue(nullptr), currentLoopBreak(nullptr), tempCounter(0) {
  initializeTarget();
}

LLVMCodeGen::~LLVMCodeGen() = default;

// =============================================================================
// Initialization
// =============================================================================

void LLVMCodeGen::initializeTarget() {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();
}

void LLVMCodeGen::initializeRuntime() {
  declareRuntimeFunctions();
  generateRuntimeHelpers();
}

void LLVMCodeGen::declareRuntimeFunctions() {
  auto *voidTy = llvm::Type::getVoidTy(*context);
  auto *i8Ty = llvm::Type::getInt8Ty(*context);
  auto *i8PtrTy = llvm::PointerType::get(*context, 0);
  auto *i32Ty = llvm::Type::getInt32Ty(*context);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *doubleTy = llvm::Type::getDoubleTy(*context);
  auto *boolTy = llvm::Type::getInt1Ty(*context);

  // String type is a pointer to char
  auto *stringTy = i8PtrTy;

  // printf for output
  auto *printfTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, true);
  module->getOrInsertFunction("printf", printfTy);

  // puts for simple string output
  auto *putsTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
  module->getOrInsertFunction("puts", putsTy);

  // getline for input
  auto *getlineTy = llvm::FunctionType::get(
      i64Ty,
      {llvm::PointerType::get(*context, 0), llvm::PointerType::get(*context, 0),
       llvm::PointerType::get(*context, 0)},
      false);
  module->getOrInsertFunction("getline", getlineTy);

  // Memory functions
  auto *mallocTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
  runtime.mallocFunc = module->getOrInsertFunction("malloc", mallocTy);

  auto *freeTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
  runtime.freeFunc = module->getOrInsertFunction("free", freeTy);

  auto *memcpyTy =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy, i64Ty}, false);
  runtime.memcpyFunc = module->getOrInsertFunction("memcpy", memcpyTy);

  auto *memsetTy =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i32Ty, i64Ty}, false);
  runtime.memsetFunc = module->getOrInsertFunction("memset", memsetTy);

  // String functions
  auto *strlenTy = llvm::FunctionType::get(i64Ty, {i8PtrTy}, false);
  module->getOrInsertFunction("strlen", strlenTy);

  auto *strcpyTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false);
  module->getOrInsertFunction("strcpy", strcpyTy);

  auto *strcatTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false);
  module->getOrInsertFunction("strcat", strcatTy);

  auto *strcmpTy = llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, false);
  module->getOrInsertFunction("strcmp", strcmpTy);

  auto *strncpyTy =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy, i64Ty}, false);
  module->getOrInsertFunction("strncpy", strncpyTy);

  // Math functions
  auto *powTy = llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false);
  runtime.powFunc = module->getOrInsertFunction("pow", powTy);

  auto *sqrtTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.sqrtFunc = module->getOrInsertFunction("sqrt", sqrtTy);

  auto *sinTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.sinFunc = module->getOrInsertFunction("sin", sinTy);

  auto *cosTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.cosFunc = module->getOrInsertFunction("cos", cosTy);

  auto *tanTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.tanFunc = module->getOrInsertFunction("tan", tanTy);

  auto *logTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.logFunc = module->getOrInsertFunction("log", logTy);

  auto *expTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.expFunc = module->getOrInsertFunction("exp", expTy);

  auto *floorTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.floorFunc = module->getOrInsertFunction("floor", floorTy);

  auto *ceilTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.ceilFunc = module->getOrInsertFunction("ceil", ceilTy);

  auto *fabsTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
  runtime.fabsFunc = module->getOrInsertFunction("fabs", fabsTy);

  // File I/O
  auto *fopenTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false);
  module->getOrInsertFunction("fopen", fopenTy);

  auto *fcloseTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
  module->getOrInsertFunction("fclose", fcloseTy);

  auto *freadTy =
      llvm::FunctionType::get(i64Ty, {i8PtrTy, i64Ty, i64Ty, i8PtrTy}, false);
  module->getOrInsertFunction("fread", freadTy);

  auto *fwriteTy =
      llvm::FunctionType::get(i64Ty, {i8PtrTy, i64Ty, i64Ty, i8PtrTy}, false);
  module->getOrInsertFunction("fwrite", fwriteTy);

  auto *fgetsTy =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i32Ty, i8PtrTy}, false);
  module->getOrInsertFunction("fgets", fgetsTy);

  auto *fputsTy = llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, false);
  module->getOrInsertFunction("fputs", fputsTy);

  // Time functions

  auto *timeTy = llvm::FunctionType::get(
      i64Ty, {llvm::PointerType::get(*context, 0)}, false);

  // Exit
  auto *exitTy = llvm::FunctionType::get(voidTy, {i32Ty}, false);
  module->getOrInsertFunction("exit", exitTy);

  // sprintf for number to string
  auto *sprintfTy = llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, true);
  module->getOrInsertFunction("sprintf", sprintfTy);

  // atoi, atof for string to number
  auto *atoiTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
  module->getOrInsertFunction("atoi", atoiTy);

  auto *atofTy = llvm::FunctionType::get(doubleTy, {i8PtrTy}, false);
  module->getOrInsertFunction("atof", atofTy);

  auto *strtolTy = llvm::FunctionType::get(
      i64Ty, {i8PtrTy, llvm::PointerType::get(*context, 0), i32Ty}, false);

  module->getOrInsertFunction("strtol", strtolTy);
}

void LLVMCodeGen::generateRuntimeHelpers() {
  // Generate helper functions that bridge Magolor semantics to C runtime

  auto *i8PtrTy = llvm::PointerType::get(*context, 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);

  // mg_print_str - Print a string without newline
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_print_str", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *fmt = createString("%s");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printStr = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_println_str - Print a string with newline
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_println_str", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *puts = module->getFunction("puts");
    builder->CreateCall(puts, {fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printlnStr = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_print_int - Print an int64
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i64Ty}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_print_int", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *fmt = createString("%lld");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printInt = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_println_int - Print an int64 with newline
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i64Ty}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_println_int", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *fmt = createString("%lld\n");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printlnInt = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_print_float - Print a double
  {
    auto *doubleTy = llvm::Type::getDoubleTy(*context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {doubleTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_print_float", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *fmt = createString("%g");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printFloat = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_println_float - Print a double with newline
  {
    auto *doubleTy = llvm::Type::getDoubleTy(*context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {doubleTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_println_float", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *fmt = createString("%g\n");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateRetVoid();

    runtime.printlnFloat = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_print_bool - Print a bool
  {
    auto *boolTy = llvm::Type::getInt1Ty(*context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {boolTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_print_bool", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *trueStr = createString("true");
    auto *falseStr = createString("false");
    auto *str = builder->CreateSelect(fn->getArg(0), trueStr, falseStr);
    auto *fmt = createString("%s");
    builder->CreateCall(printf, {fmt, str});
    builder->CreateRetVoid();

    runtime.printBool = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_println_bool - Print a bool with newline
  {
    auto *boolTy = llvm::Type::getInt1Ty(*context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {boolTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_println_bool", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *trueStr = createString("true");
    auto *falseStr = createString("false");
    auto *str = builder->CreateSelect(fn->getArg(0), trueStr, falseStr);
    auto *fmt = createString("%s\n");
    builder->CreateCall(printf, {fmt, str});
    builder->CreateRetVoid();

    runtime.printlnBool = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_string_concat - Concatenate two strings

  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_string_concat", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto strlen = module->getOrInsertFunction(
        "strlen", llvm::FunctionType::get(i64Ty, {i8PtrTy}, false));
    auto malloc = runtime.mallocFunc;
    auto strcpy = module->getOrInsertFunction(
        "strcpy", llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false));
    auto strcat = module->getOrInsertFunction(
        "strcat", llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false));

    auto *len1 = builder->CreateCall(strlen, {fn->getArg(0)});
    auto *len2 = builder->CreateCall(strlen, {fn->getArg(1)});
    auto *totalLen = builder->CreateAdd(len1, len2);
    auto *totalLenPlus1 =
        builder->CreateAdd(totalLen, llvm::ConstantInt::get(i64Ty, 1));

    auto *buf = builder->CreateCall(malloc, {totalLenPlus1});
    builder->CreateCall(strcpy, {buf, fn->getArg(0)});
    builder->CreateCall(strcat, {buf, fn->getArg(1)});
    builder->CreateRet(buf);

    runtime.stringConcat = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_string_length - Get string length
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i8PtrTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_string_length", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *strlen = module->getFunction("strlen");
    auto *len = builder->CreateCall(strlen, {fn->getArg(0)});
    builder->CreateRet(len);

    runtime.stringLength = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_int_to_string - Convert int to string
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_int_to_string", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto malloc = runtime.mallocFunc;
    auto *sprintf = module->getFunction("sprintf");

    auto *buf =
        builder->CreateCall(malloc, {llvm::ConstantInt::get(i64Ty, 32)});
    auto *fmt = createString("%lld");
    builder->CreateCall(sprintf, {buf, fmt, fn->getArg(0)});
    builder->CreateRet(buf);

    runtime.intToString = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_float_to_string - Convert float to string
  {
    auto *doubleTy = llvm::Type::getDoubleTy(*context);
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {doubleTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_float_to_string", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto malloc = runtime.mallocFunc;
    auto *sprintf = module->getFunction("sprintf");

    auto *buf =
        builder->CreateCall(malloc, {llvm::ConstantInt::get(i64Ty, 64)});
    auto *fmt = createString("%g");
    builder->CreateCall(sprintf, {buf, fmt, fn->getArg(0)});
    builder->CreateRet(buf);

    runtime.floatToString = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_bool_to_string - Convert bool to string
  {
    auto *boolTy = llvm::Type::getInt1Ty(*context);
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {boolTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_bool_to_string", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *trueStr = createString("true");
    auto *falseStr = createString("false");
    auto *result = builder->CreateSelect(fn->getArg(0), trueStr, falseStr);
    builder->CreateRet(result);

    runtime.boolToString = llvm::FunctionCallee(fnTy, fn);
  }

  // mg_throw_error - Throw a runtime error and exit
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                      "mg_throw_error", module.get());
    fn->addFnAttr(llvm::Attribute::NoReturn);
    auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);

    auto *printf = module->getFunction("printf");
    auto *exit = module->getFunction("exit");
    auto *fmt = createString("Runtime Error: %s\n");
    builder->CreateCall(printf, {fmt, fn->getArg(0)});
    builder->CreateCall(
        exit, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1)});
    builder->CreateUnreachable();

    runtime.throwError = llvm::FunctionCallee(fnTy, fn);
  }
}

// =============================================================================
// Type Conversion
// =============================================================================

llvm::Type *LLVMCodeGen::toLLVMType(const TypePtr &type) {
  if (!type) {
    return llvm::Type::getVoidTy(*context);
  }

  switch (type->kind) {
  case Type::INT:
    return llvm::Type::getInt64Ty(*context);

  case Type::FLOAT:
    return llvm::Type::getDoubleTy(*context);

  case Type::STRING:
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);

  case Type::BOOL:
    return llvm::Type::getInt1Ty(*context);

  case Type::VOID:
    return llvm::Type::getVoidTy(*context);

  case Type::CLASS:
    return getClassType(type->className);

  case Type::OPTION:
    return getOptionType(type->innerType);

  case Type::ARRAY:
    return getArrayType(type->innerType);

  case Type::GENERIC: {
    if (type->className == "Array" && type->genericArgs.size() == 1) {
      return getArrayType(type->genericArgs[0]);
    }
    if (type->className == "Map" && type->genericArgs.size() == 2) {
      return getMapType(type->genericArgs[0], type->genericArgs[1]);
    }
    if (type->className == "Set" && type->genericArgs.size() == 1) {
      // Set is implemented as Map<T, bool>
      auto boolType = std::make_shared<Type>();
      boolType->kind = Type::BOOL;
      return getMapType(type->genericArgs[0], boolType);
    }
    if (type->className == "Option" && type->genericArgs.size() == 1) {
      return getOptionType(type->genericArgs[0]);
    }
    // Unknown generic - return as pointer
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  }

  case Type::FUNCTION:
    return llvm::PointerType::get(getFunctionType(type), 0);

  default:
    return llvm::Type::getVoidTy(*context);
  }
}

llvm::Type *LLVMCodeGen::getStringType() {
  return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
}

llvm::Type *LLVMCodeGen::getArrayType(const TypePtr &elemType) {
  // Array is represented as a struct: { i8* data, i64 length, i64 capacity }
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);

  std::string name = "mg_array_" + std::to_string(tempCounter++);




  auto *structTy = llvm::StructType::create(*context, name);
  structTy->setBody({i8PtrTy, i64Ty, i64Ty});

  return llvm::PointerType::get(structTy, 0);
}

llvm::Type *LLVMCodeGen::getMapType(const TypePtr &keyType,
                                    const TypePtr &valType) {
  // Map is represented as an opaque pointer (implementation uses hash table
  // internally)
  return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
}

llvm::Type *LLVMCodeGen::getOptionType(const TypePtr &innerType) {
  // Option is a struct: { i1 has_value, T value }
  auto *innerLLVMType = toLLVMType(innerType);
  auto *boolTy = llvm::Type::getInt1Ty(*context);

  std::string name = "mg_option_" + std::to_string(tempCounter++);

  auto *structTy = llvm::StructType::create(*context, name);
  structTy->setBody({boolTy, innerLLVMType});

  return structTy;
}

llvm::Type *LLVMCodeGen::getClassType(const std::string &className) {
  auto it = classLayouts.find(className);
  if (it != classLayouts.end()) {
    return llvm::PointerType::get(it->second.structType, 0);
  }

  // Return opaque pointer for unknown classes
  return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
}

llvm::FunctionType *LLVMCodeGen::getFunctionType(const TypePtr &type) {
  if (!type || type->kind != Type::FUNCTION) {
    return llvm::FunctionType::get(llvm::Type::getVoidTy(*context), {}, false);
  }

  auto *retType = toLLVMType(type->returnType);
  std::vector<llvm::Type *> paramTypes;

  for (const auto &paramType : type->paramTypes) {
    paramTypes.push_back(toLLVMType(paramType));
  }

  return llvm::FunctionType::get(retType, paramTypes, false);
}

// =============================================================================
// Class Generation
// =============================================================================

void LLVMCodeGen::generateClassLayouts(const std::vector<ClassDecl> &classes) {
  // First pass: create all struct types (forward declarations)
  for (const auto &cls : classes) {
    auto *structTy = llvm::StructType::create(*context, "class." + cls.name);
    ClassLayout layout;
    layout.structType = structTy;
    layout.parentClass = cls.parent;
    layout.hasVTable = false;
    classLayouts[cls.name] = layout;
  }

  // Second pass: fill in struct bodies
  for (const auto &cls : classes) {
    generateClassLayout(cls);
  }
}

void LLVMCodeGen::generateClassLayout(const ClassDecl &cls) {
  auto &layout = classLayouts[cls.name];
  std::vector<llvm::Type *> fieldTypes;

  // If there's a parent class, include its fields first
  if (!cls.parent.empty()) {
    auto parentIt = classLayouts.find(cls.parent);
    if (parentIt != classLayouts.end()) {
      // Include parent struct as first field for inheritance
      fieldTypes.push_back(parentIt->second.structType);
      layout.fieldIndices["__parent"] = 0;
    }
  }

  // Add this class's fields
  size_t index = fieldTypes.size();
  for (const auto &field : cls.fields) {
    if (!field.isStatic) {
      auto *fieldType = toLLVMType(field.type);
      fieldTypes.push_back(fieldType);
      layout.fieldIndices[field.name] = index;
      layout.fieldTypes[field.name] = field.type;
      layout.fieldOrder.push_back(field.name);
      index++;
    }
  }

  // Set the struct body
  if (fieldTypes.empty()) {
    // Empty struct needs at least one field
    fieldTypes.push_back(llvm::Type::getInt8Ty(*context));
  }

  layout.structType->setBody(fieldTypes);
}

void LLVMCodeGen::generateClass(const ClassDecl &cls) {
  // Generate static fields as global variables
  for (const auto &field : cls.fields) {
    if (field.isStatic) {
      auto *type = toLLVMType(field.type);
      auto *gv = new llvm::GlobalVariable(*module, type,
                                          false, // isConstant
                                          llvm::GlobalValue::ExternalLinkage,
                                          getDefaultValue(type),
                                          cls.name + "." + field.name);

      if (field.initValue) {
        // We'll initialize in a global constructor
      }
    }
  }

  // Generate method declarations
  generateClassMethods(cls);

  // Generate constructor
  generateClassConstructor(cls);
}

void LLVMCodeGen::generateClassMethods(const ClassDecl &cls) {
  currentClassName = cls.name;

  for (const auto &method : cls.methods) {
    generateFunction(method, cls.name);
  }

  currentClassName.clear();
}

void LLVMCodeGen::generateClassConstructor(const ClassDecl &cls) {
  auto &layout = classLayouts[cls.name];

  // Find the create method if it exists
  const FnDecl *createMethod = nullptr;
  for (const auto &method : cls.methods) {
    if (method.name == "create") {
      createMethod = &method;
      break;
    }
  }

  // Build constructor function type
  std::vector<llvm::Type *> paramTypes;
  if (createMethod) {
    for (const auto &param : createMethod->params) {
      paramTypes.push_back(toLLVMType(param.type));
    }
  }

  auto *retType = llvm::PointerType::get(layout.structType, 0);
  auto *fnType = llvm::FunctionType::get(retType, paramTypes, false);

  std::string fnName = getMangledName(cls.name, "__new");
  auto *fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage,
                                    fnName, module.get());

  auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
  builder->SetInsertPoint(entry);

  // Allocate the object
  auto *size = llvm::ConstantExpr::getSizeOf(layout.structType);
  auto *sizeInt =
      builder->CreatePtrToInt(size, llvm::Type::getInt64Ty(*context));
  auto *mem = builder->CreateCall(runtime.mallocFunc, {sizeInt});
  auto *obj = builder->CreateBitCast(mem, retType);

  // Initialize all fields to default values
  for (size_t i = 0; i < layout.fieldOrder.size(); i++) {
    const auto &fieldName = layout.fieldOrder[i];
    auto fieldIdx = layout.fieldIndices[fieldName];
    auto *fieldPtr = builder->CreateStructGEP(layout.structType, obj, fieldIdx);
    auto *fieldType = layout.structType->getElementType(fieldIdx);
    builder->CreateStore(getDefaultValue(fieldType), fieldPtr);
  }

  // Call the create method if it exists
  if (createMethod) {
    pushScope();

    // Set up 'this' pointer
    currentThis = obj;

    // Declare parameters
    size_t argIdx = 0;
    for (auto &arg : fn->args()) {
      const auto &param = createMethod->params[argIdx];
      auto *alloca = createEntryBlockAlloca(fn, param.name, arg.getType());
      builder->CreateStore(&arg, alloca);
      declareVar(param.name, alloca, arg.getType(), param.type, false);
      argIdx++;
    }

    // Generate create method body
    for (const auto &stmt : createMethod->body) {
      genStmt(stmt);
    }

    currentThis = nullptr;
    popScope();
  }

  builder->CreateRet(obj);

  functions[fnName] = fn;
}

llvm::Value *
LLVMCodeGen::createClassInstance(const std::string &className,
                                 const std::vector<llvm::Value *> &args) {
  auto it = classLayouts.find(className);
  if (it == classLayouts.end()) {
    emitError("Unknown class: " + className);
    return llvm::Constant::getNullValue(
        llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0));
  }

  std::string fnName = getMangledName(className, "__new");
  auto *fn = functions[fnName];
  if (!fn) {
    fn = module->getFunction(fnName);
  }

  if (!fn) {
    emitError("Constructor not found for class: " + className);
    return llvm::Constant::getNullValue(
        llvm::PointerType::get(it->second.structType, 0));
  }

  return builder->CreateCall(fn, args);
}

// =============================================================================
// Helper Functions Implementation
// =============================================================================

llvm::AllocaInst *LLVMCodeGen::createEntryBlockAlloca(llvm::Function *fn,
                                                      const std::string &name,
                                                      llvm::Type *type) {
  llvm::IRBuilder<> tempBuilder(&fn->getEntryBlock(),
                                fn->getEntryBlock().begin());
  return tempBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value *LLVMCodeGen::createString(const std::string &str) {
  auto it = stringConstants.find(str);
  if (it != stringConstants.end()) {
    return builder->CreateBitCast(it->second,
                                  llvm::PointerType::get(*context, 0));
  }

  auto *strConstant = llvm::ConstantDataArray::getString(*context, str);
  auto *global = new llvm::GlobalVariable(*module, strConstant->getType(), true,
                                          llvm::GlobalValue::PrivateLinkage,
                                          strConstant, ".str");
  global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

  stringConstants[str] = global;

  return builder->CreateBitCast(global, llvm::PointerType::get(*context, 0));
}

llvm::Value *LLVMCodeGen::createStringFromValue(llvm::Value *val,
                                                const TypePtr &type) {
  if (!type) {
    return createString("<unknown>");
  }

  switch (type->kind) {
  case Type::INT:
    return builder->CreateCall(runtime.intToString, {val});
  case Type::FLOAT:
    return builder->CreateCall(runtime.floatToString, {val});
  case Type::BOOL:
    return builder->CreateCall(runtime.boolToString, {val});
  case Type::STRING:
    return val;
  default:
    return createString("<object>");
  }
}

llvm::Value *LLVMCodeGen::loadIfPointer(llvm::Value *val) {
  if (auto *allocaInst = llvm::dyn_cast<llvm::AllocaInst>(val)) {
    return builder->CreateLoad(allocaInst->getAllocatedType(), val);
  }
  return val;
}

llvm::Value *LLVMCodeGen::castValue(llvm::Value *val, llvm::Type *targetType) {
  auto *srcType = val->getType();

  if (srcType == targetType) {
    return val;
  }

  // Integer conversions
  if (srcType->isIntegerTy() && targetType->isIntegerTy()) {
    return builder->CreateIntCast(val, targetType, true);
  }

  // Int to float
  if (srcType->isIntegerTy() && targetType->isFloatingPointTy()) {
    return builder->CreateSIToFP(val, targetType);
  }

  // Float to int
  if (srcType->isFloatingPointTy() && targetType->isIntegerTy()) {
    return builder->CreateFPToSI(val, targetType);
  }

  // Pointer casts
  if (srcType->isPointerTy() && targetType->isPointerTy()) {
    return builder->CreateBitCast(val, targetType);
  }

  // Int to pointer
  if (srcType->isIntegerTy() && targetType->isPointerTy()) {
    return builder->CreateIntToPtr(val, targetType);
  }

  // Pointer to int
  if (srcType->isPointerTy() && targetType->isIntegerTy()) {
    return builder->CreatePtrToInt(val, targetType);
  }

  return val;
}

llvm::Constant *LLVMCodeGen::getDefaultValue(llvm::Type *type) {
  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(type, 0);
  }
  if (type->isFloatingPointTy()) {
    return llvm::ConstantFP::get(type, 0.0);
  }
  if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  }
  if (type->isStructTy()) {
    return llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(type), {});
  }
  return llvm::UndefValue::get(type);
}

llvm::Constant *LLVMCodeGen::getDefaultValue(const TypePtr &type) {
  return getDefaultValue(toLLVMType(type));
}
std::string LLVMCodeGen::getMangledName(const std::string &className,
                                        const std::string &methodName) {
  return className + "_" + methodName;
}

std::string LLVMCodeGen::getUniqueName(const std::string &base) {
  return base + "_" + std::to_string(tempCounter++);
}

void LLVMCodeGen::pushScope() { scopeStack.push_back({}); }

void LLVMCodeGen::popScope() {
  if (!scopeStack.empty()) {
    scopeStack.pop_back();
  }
}

void LLVMCodeGen::declareVar(const std::string &name, llvm::AllocaInst *alloca,
                             llvm::Type *type, const TypePtr &astType,
                             bool isMut) {
  if (scopeStack.empty()) {
    pushScope();
  }
  scopeStack.back()[name] = {alloca, type, astType, isMut};
}

LLVMVar *LLVMCodeGen::lookupVar(const std::string &name) {
  for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

llvm::Function *LLVMCodeGen::lookupFunction(const std::string &name) {
  auto it = functions.find(name);
  if (it != functions.end()) {
    return it->second;
  }
  return module->getFunction(name);
}

void LLVMCodeGen::emitError(const std::string &msg) {
  llvm::errs() << "CodeGen Error: " << msg << "\n";
}

llvm::Value *LLVMCodeGen::emitRuntimeError(const std::string &msg) {
  auto *errMsg = createString(msg);
  return builder->CreateCall(runtime.throwError, {errMsg});
}

std::string LLVMCodeGen::getIRString() const {
  std::string str;
  llvm::raw_string_ostream os(str);
  module->print(os, nullptr);
  return str;
}

bool LLVMCodeGen::writeIRToFile(const std::string& filename) const {
    if (!module) {
        llvm::errs() << "ERROR: module is null in writeIRToFile!\n";
        return false;
    }
    std::error_code ec;
    llvm::raw_fd_ostream file(filename, ec);
    if (ec) {
        llvm::errs() << "Error opening file: " << ec.message() << "\n";
        return false;
    }
    module->print(file, nullptr);
    return true;
}

bool LLVMCodeGen::verify() const {
  std::string errors;
  llvm::raw_string_ostream os(errors);
  if (llvm::verifyModule(*module, &os)) {
    llvm::errs() << "Module verification failed:\n" << errors << "\n";
    return false;
  }
  return true;
}

std::vector<std::string> LLVMCodeGen::collectLinkFlags(const Program &prog) {
  std::vector<std::string> flags;
  // Add standard C library
  flags.push_back("-lm"); // Math library
  // Add other libraries based on used modules
  return flags;
}

bool LLVMCodeGen::writeObjectFile(const std::string& filename) {
    // Initialize target
    std::string targetTriple = llvm::sys::getDefaultTargetTriple();
    
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    
    if (!target) {
        llvm::errs() << "Error: " << error << "\n";
        return false;
    }
    
    auto CPU = "generic";
    auto features = "";
    
    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, CPU, features, opt, std::nullopt);
    
    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(llvm::Triple(targetTriple));
    
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    
    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message() << "\n";
        return false;
    }
    
    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;
    
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type\n";
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}
