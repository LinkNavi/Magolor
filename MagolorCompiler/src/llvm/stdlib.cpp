// codegen_llvm_stdlib.cpp - LLVM IR Code Generator for Magolor
// Part 4: Standard Library Generation and Helper Functions

#include "codegen_llvm.hpp"
#include <sstream>
#include <iostream>
// =============================================================================
// Standard Library Generation
// =============================================================================

void LLVMCodeGen::generateStdLib() {
  // Always generate core IO functions since they're commonly used
  generateStdIO();
  generateStdMath();

  // Generate other modules based on what's used
  for (const auto &mod : usedModules) {
    if (mod.find("String") != std::string::npos) {
      generateStdString();
    }
    if (mod.find("Array") != std::string::npos) {
      generateStdArray();
    }
    if (mod.find("Map") != std::string::npos) {
      generateStdMap();
    }
    if (mod.find("File") != std::string::npos) {
      generateStdFile();
    }
    if (mod.find("Time") != std::string::npos) {
      generateStdTime();
    }
    if (mod.find("Random") != std::string::npos) {
      generateStdRandom();
    }
    if (mod.find("System") != std::string::npos) {
      generateStdSystem();
    }
    if (mod.find("Network") != std::string::npos) {
      generateStdNetwork();
    }
if (mod.find("Network") != std::string::npos) {
    std::cerr << "[DEBUG] Calling generateStdNetwork()" << std::endl;
    generateStdNetwork();
    
    // Check if it worked
    auto *testFn = module->getFunction("Sockets_listen");
    std::cerr << "[DEBUG] Sockets_listen exists: " << (testFn ? "YES" : "NO") << std::endl;
}
    if (mod.find("Crypto") != std::string::npos) {
      generateStdCrypto();
    }
    if (mod.find("Option") != std::string::npos) {
      generateStdOption();
    }
  }
}

void LLVMCodeGen::generateStdIO() {
  auto *voidTy = llvm::Type::getVoidTy(*context);
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);

  // IO_readLine
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {}, false);
    if (!module->getFunction("IO_readLine")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "IO_readLine", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *buf = builder->CreateCall(runtime.mallocFunc,
                                      {llvm::ConstantInt::get(i64Ty, 4096)});

      auto *fgets = module->getFunction("fgets");
      if (!fgets) {
        auto *fgetsTy = llvm::FunctionType::get(
            i8PtrTy, {i8PtrTy, llvm::Type::getInt32Ty(*context), i8PtrTy},
            false);
        fgets = llvm::Function::Create(fgetsTy, llvm::Function::ExternalLinkage,
                                       "fgets", module.get());
      }

      auto *stdinGV = module->getOrInsertGlobal("stdin", i8PtrTy);
      auto *stdinPtr = builder->CreateLoad(i8PtrTy, stdinGV);

      builder->CreateCall(
          fgets,
          {buf, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 4096),
           stdinPtr});

      auto *strlen = module->getFunction("strlen");
      auto *len = builder->CreateCall(strlen, {buf});
      auto *lenMinus1 =
          builder->CreateSub(len, llvm::ConstantInt::get(i64Ty, 1));
      auto *lastCharPtr =
          builder->CreateGEP(llvm::Type::getInt8Ty(*context), buf, lenMinus1);
      builder->CreateStore(
          llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0),
          lastCharPtr);

      builder->CreateRet(buf);
    }
  }
}

void LLVMCodeGen::generateStdMath() {
  auto *doubleTy = llvm::Type::getDoubleTy(*context);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);

  // Math_abs (int)
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty}, false);
    if (!module->getFunction("Math_abs_int")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_abs_int", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *val = fn->getArg(0);
      auto *zero = llvm::ConstantInt::get(i64Ty, 0);
      auto *isNeg = builder->CreateICmpSLT(val, zero);
      auto *negVal = builder->CreateNeg(val);
      auto *result = builder->CreateSelect(isNeg, negVal, val);
      builder->CreateRet(result);
    }
  }

  // Math_abs (float)
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_abs")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_abs", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.fabsFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_sqrt
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_sqrt")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_sqrt", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.sqrtFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_pow
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy, doubleTy}, false);
    if (!module->getFunction("Math_pow")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_pow", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result =
          builder->CreateCall(runtime.powFunc, {fn->getArg(0), fn->getArg(1)});
      builder->CreateRet(result);
    }
  }

  // Math_sin
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_sin")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_sin", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.sinFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_cos
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_cos")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_cos", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.cosFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_tan
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_tan")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_tan", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.tanFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_log
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_log")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_log", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.logFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_exp
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_exp")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_exp", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.expFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_floor
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_floor")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_floor", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.floorFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_ceil
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {doubleTy}, false);
    if (!module->getFunction("Math_ceil")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_ceil", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.ceilFunc, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // Math_min (int)
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
    if (!module->getFunction("Math_min_int")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_min_int", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *a = fn->getArg(0);
      auto *b = fn->getArg(1);
      auto *cmp = builder->CreateICmpSLT(a, b);
      auto *result = builder->CreateSelect(cmp, a, b);
      builder->CreateRet(result);
    }
  }

  // Math_max (int)
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
    if (!module->getFunction("Math_max_int")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Math_max_int", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *a = fn->getArg(0);
      auto *b = fn->getArg(1);
      auto *cmp = builder->CreateICmpSGT(a, b);
      auto *result = builder->CreateSelect(cmp, a, b);
      builder->CreateRet(result);
    }
  }
}

void LLVMCodeGen::generateStdString() {
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *boolTy = llvm::Type::getInt1Ty(*context);

  // String_length
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i8PtrTy}, false);
    if (!module->getFunction("String_length")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "String_length", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.stringLength, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // String_isEmpty
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy}, false);
    if (!module->getFunction("String_isEmpty")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "String_isEmpty", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *len = builder->CreateCall(runtime.stringLength, {fn->getArg(0)});
      auto *result =
          builder->CreateICmpEQ(len, llvm::ConstantInt::get(i64Ty, 0));
      builder->CreateRet(result);
    }
  }

  // String_concat
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i8PtrTy}, false);
    if (!module->getFunction("String_concat")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "String_concat", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *result = builder->CreateCall(runtime.stringConcat,
                                         {fn->getArg(0), fn->getArg(1)});
      builder->CreateRet(result);
    }
  }
}

void LLVMCodeGen::generateStdArray() {
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);

  auto *arrayStructTy =
      llvm::StructType::get(*context, {i8PtrTy, i64Ty, i64Ty});
  auto *arrayPtrTy = llvm::PointerType::get(arrayStructTy, 0);

  // Array_length
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {arrayPtrTy}, false);
    if (!module->getFunction("Array_length")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Array_length", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *lenPtr = builder->CreateStructGEP(arrayStructTy, fn->getArg(0), 1);
      auto *len = builder->CreateLoad(i64Ty, lenPtr);
      builder->CreateRet(len);
    }
  }

  // Array_push
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {arrayPtrTy, i64Ty}, false);
    if (!module->getFunction("Array_push")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Array_push", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *arr = fn->getArg(0);
      auto *val = fn->getArg(1);

      auto *lenPtr = builder->CreateStructGEP(arrayStructTy, arr, 1);
      auto *len = builder->CreateLoad(i64Ty, lenPtr);
      auto *capPtr = builder->CreateStructGEP(arrayStructTy, arr, 2);
      auto *cap = builder->CreateLoad(i64Ty, capPtr);
      auto *dataPtr = builder->CreateStructGEP(arrayStructTy, arr, 0);
      auto *data = builder->CreateLoad(i8PtrTy, dataPtr);

      auto *needResize = builder->CreateICmpSGE(len, cap);

      auto *resizeBB = llvm::BasicBlock::Create(*context, "resize", fn);
      auto *storeBB = llvm::BasicBlock::Create(*context, "store", fn);

      builder->CreateCondBr(needResize, resizeBB, storeBB);

      builder->SetInsertPoint(resizeBB);
      auto *newCap = builder->CreateMul(cap, llvm::ConstantInt::get(i64Ty, 2));
      auto *newSize =
          builder->CreateMul(newCap, llvm::ConstantInt::get(i64Ty, 8));
      auto *newData = builder->CreateCall(runtime.mallocFunc, {newSize});
      auto *oldSize = builder->CreateMul(len, llvm::ConstantInt::get(i64Ty, 8));
      builder->CreateCall(runtime.memcpyFunc, {newData, data, oldSize});
      builder->CreateCall(runtime.freeFunc, {data});
      builder->CreateStore(newData, dataPtr);
      builder->CreateStore(newCap, capPtr);
      builder->CreateBr(storeBB);

      builder->SetInsertPoint(storeBB);
      auto *finalData = builder->CreateLoad(i8PtrTy, dataPtr);
      auto *offset = builder->CreateMul(len, llvm::ConstantInt::get(i64Ty, 8));
      auto *elemPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context),
                                         finalData, offset);
      auto *castPtr =
          builder->CreateBitCast(elemPtr, llvm::PointerType::get(i64Ty, 0));
      builder->CreateStore(val, castPtr);

      auto *newLen = builder->CreateAdd(len, llvm::ConstantInt::get(i64Ty, 1));
      builder->CreateStore(newLen, lenPtr);

      builder->CreateRetVoid();
    }
  }

  // Array_pop
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {arrayPtrTy}, false);
    if (!module->getFunction("Array_pop")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Array_pop", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *arr = fn->getArg(0);
      auto *lenPtr = builder->CreateStructGEP(arrayStructTy, arr, 1);
      auto *len = builder->CreateLoad(i64Ty, lenPtr);
      auto *dataPtr = builder->CreateStructGEP(arrayStructTy, arr, 0);
      auto *data = builder->CreateLoad(i8PtrTy, dataPtr);

      auto *lastIdx = builder->CreateSub(len, llvm::ConstantInt::get(i64Ty, 1));
      auto *offset =
          builder->CreateMul(lastIdx, llvm::ConstantInt::get(i64Ty, 8));
      auto *elemPtr =
          builder->CreateGEP(llvm::Type::getInt8Ty(*context), data, offset);
      auto *castPtr =
          builder->CreateBitCast(elemPtr, llvm::PointerType::get(i64Ty, 0));
      auto *val = builder->CreateLoad(i64Ty, castPtr);

      builder->CreateStore(lastIdx, lenPtr);

      builder->CreateRet(val);
    }
  }
}

void LLVMCodeGen::generateStdMap() {
  // Map requires hash table implementation - stub for now
}

void LLVMCodeGen::generateStdOption() {
  auto *boolTy = llvm::Type::getInt1Ty(*context);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *optionTy = llvm::StructType::get(*context, {boolTy, i64Ty});

  // Option_isSome
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {optionTy}, false);
    if (!module->getFunction("Option_isSome")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Option_isSome", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *hasVal = builder->CreateExtractValue(fn->getArg(0), 0);
      builder->CreateRet(hasVal);
    }
  }

  // Option_isNone
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {optionTy}, false);
    if (!module->getFunction("Option_isNone")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Option_isNone", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *hasVal = builder->CreateExtractValue(fn->getArg(0), 0);
      auto *result = builder->CreateNot(hasVal);
      builder->CreateRet(result);
    }
  }

  // Option_unwrap
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {optionTy}, false);
    if (!module->getFunction("Option_unwrap")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Option_unwrap", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *okBB = llvm::BasicBlock::Create(*context, "ok", fn);

      builder->SetInsertPoint(entry);
      auto *hasVal = builder->CreateExtractValue(fn->getArg(0), 0);
      builder->CreateCondBr(hasVal, okBB, errorBB);

      builder->SetInsertPoint(errorBB);
      auto *errMsg = createString("Called unwrap on None value");
      builder->CreateCall(runtime.throwError, {errMsg});
      builder->CreateUnreachable();

      builder->SetInsertPoint(okBB);
      auto *val = builder->CreateExtractValue(fn->getArg(0), 1);
      builder->CreateRet(val);
    }
  }

  // Option_unwrapOr
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {optionTy, i64Ty}, false);
    if (!module->getFunction("Option_unwrapOr")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Option_unwrapOr", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);
      auto *hasVal = builder->CreateExtractValue(fn->getArg(0), 0);
      auto *val = builder->CreateExtractValue(fn->getArg(0), 1);
      auto *result = builder->CreateSelect(hasVal, val, fn->getArg(1));
      builder->CreateRet(result);
    }
  }
}

void LLVMCodeGen::generateStdFile() {
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *boolTy = llvm::Type::getInt1Ty(*context);
  auto *i32Ty = llvm::Type::getInt32Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);

  // File_exists
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy}, false);
    if (!module->getFunction("File_exists")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_exists", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *accessTy = llvm::FunctionType::get(i32Ty, {i8PtrTy, i32Ty}, false);
      auto access = module->getOrInsertFunction("access", accessTy);
      auto *result = builder->CreateCall(
          access, {fn->getArg(0), llvm::ConstantInt::get(i32Ty, 0)});
      auto *exists =
          builder->CreateICmpEQ(result, llvm::ConstantInt::get(i32Ty, 0));
      builder->CreateRet(exists);
    }
  }

  // File_readAll
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("File_readAll")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_readAll", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *fopen = module->getFunction("fopen");
      auto *mode = createString("r");
      auto *file = builder->CreateCall(fopen, {fn->getArg(0), mode});

      auto *isNull =
          builder->CreateICmpEQ(file, llvm::Constant::getNullValue(i8PtrTy));

      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *readBB = llvm::BasicBlock::Create(*context, "read", fn);
      builder->CreateCondBr(isNull, errorBB, readBB);

      builder->SetInsertPoint(errorBB);
      builder->CreateRet(createString(""));

      builder->SetInsertPoint(readBB);

      // Get file size
      auto *fseekTy =
          llvm::FunctionType::get(i32Ty, {i8PtrTy, i64Ty, i32Ty}, false);
      auto fseek = module->getOrInsertFunction("fseek", fseekTy);
      auto *ftellTy = llvm::FunctionType::get(i64Ty, {i8PtrTy}, false);
      auto ftell = module->getOrInsertFunction("ftell", ftellTy);

      // Seek to end
      builder->CreateCall(fseek, {file, llvm::ConstantInt::get(i64Ty, 0),
                                  llvm::ConstantInt::get(i32Ty, 2)});
      auto *size = builder->CreateCall(ftell, {file});
      // Seek back to start
      builder->CreateCall(fseek, {file, llvm::ConstantInt::get(i64Ty, 0),
                                  llvm::ConstantInt::get(i32Ty, 0)});

      auto *sizePlus1 =
          builder->CreateAdd(size, llvm::ConstantInt::get(i64Ty, 1));
      auto *buf = builder->CreateCall(runtime.mallocFunc, {sizePlus1});

      auto *fread = module->getFunction("fread");
      builder->CreateCall(fread,
                          {buf, llvm::ConstantInt::get(i64Ty, 1), size, file});

      // Null terminate
      auto *endPtr =
          builder->CreateGEP(llvm::Type::getInt8Ty(*context), buf, size);
      builder->CreateStore(
          llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0), endPtr);

      auto *fclose = module->getFunction("fclose");
      builder->CreateCall(fclose, {file});

      builder->CreateRet(buf);
    }
  }

  // File_writeAll
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy, i8PtrTy}, false);
    if (!module->getFunction("File_writeAll")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_writeAll", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *fopen = module->getFunction("fopen");
      auto *mode = createString("w");
      auto *file = builder->CreateCall(fopen, {fn->getArg(0), mode});

      auto *isNull =
          builder->CreateICmpEQ(file, llvm::Constant::getNullValue(i8PtrTy));

      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *writeBB = llvm::BasicBlock::Create(*context, "write", fn);
      builder->CreateCondBr(isNull, errorBB, writeBB);

      builder->SetInsertPoint(errorBB);
      builder->CreateRet(llvm::ConstantInt::getFalse(*context));

      builder->SetInsertPoint(writeBB);
      auto *fputs = module->getFunction("fputs");
      builder->CreateCall(fputs, {fn->getArg(1), file});

      auto *fclose = module->getFunction("fclose");
      builder->CreateCall(fclose, {file});

      builder->CreateRet(llvm::ConstantInt::getTrue(*context));
    }
  }

  // File_append
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy, i8PtrTy}, false);
    if (!module->getFunction("File_append")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_append", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *fopen = module->getFunction("fopen");
      auto *mode = createString("a");
      auto *file = builder->CreateCall(fopen, {fn->getArg(0), mode});

      auto *isNull =
          builder->CreateICmpEQ(file, llvm::Constant::getNullValue(i8PtrTy));

      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *writeBB = llvm::BasicBlock::Create(*context, "write", fn);
      builder->CreateCondBr(isNull, errorBB, writeBB);

      builder->SetInsertPoint(errorBB);
      builder->CreateRet(llvm::ConstantInt::getFalse(*context));

      builder->SetInsertPoint(writeBB);
      auto *fputs = module->getFunction("fputs");
      builder->CreateCall(fputs, {fn->getArg(1), file});

      auto *fclose = module->getFunction("fclose");
      builder->CreateCall(fclose, {file});

      builder->CreateRet(llvm::ConstantInt::getTrue(*context));
    }
  }

  // File_delete
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy}, false);
    if (!module->getFunction("File_delete")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_delete", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *removeTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
      auto remove = module->getOrInsertFunction("remove", removeTy);
      auto *result = builder->CreateCall(remove, {fn->getArg(0)});
      auto *success =
          builder->CreateICmpEQ(result, llvm::ConstantInt::get(i32Ty, 0));
      builder->CreateRet(success);
    }
  }

  // File_copy
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy, i8PtrTy}, false);
    if (!module->getFunction("File_copy")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_copy", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      // Read source file
      auto *readFn = module->getFunction("File_readAll");
      auto *content = builder->CreateCall(readFn, {fn->getArg(0)});

      // Write to destination
      auto *writeFn = module->getFunction("File_writeAll");
      auto *result = builder->CreateCall(writeFn, {fn->getArg(1), content});

      builder->CreateRet(result);
    }
  }

  // File_move
  {
    auto *fnTy = llvm::FunctionType::get(boolTy, {i8PtrTy, i8PtrTy}, false);
    if (!module->getFunction("File_move")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "File_move", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *renameTy =
          llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, false);
      auto rename = module->getOrInsertFunction("rename", renameTy);
      auto *result =
          builder->CreateCall(rename, {fn->getArg(0), fn->getArg(1)});
      auto *success =
          builder->CreateICmpEQ(result, llvm::ConstantInt::get(i32Ty, 0));
      builder->CreateRet(success);
    }
  }
}

void LLVMCodeGen::generateStdTime() {
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);

  // Time_now
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {}, false);
    if (!module->getFunction("Time_now")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Time_now", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      // FIXED: Declare the time() function properly
      auto *timeTy = llvm::FunctionType::get(
          i64Ty, {llvm::PointerType::get(i64Ty, 0)}, false);
      auto time = module->getOrInsertFunction("time", timeTy);
      
      auto *result = builder->CreateCall(
          time,
          {llvm::Constant::getNullValue(llvm::PointerType::get(i64Ty, 0))});
      auto *ms =
          builder->CreateMul(result, llvm::ConstantInt::get(i64Ty, 1000));
      builder->CreateRet(ms);
    }
  }

  // Time_sleep
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i64Ty}, false);
    if (!module->getFunction("Time_sleep")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Time_sleep", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *usleepTy =
          llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                  {llvm::Type::getInt32Ty(*context)}, false);
      auto usleep = module->getOrInsertFunction("usleep", usleepTy);
      auto *us = builder->CreateMul(fn->getArg(0),
                                    llvm::ConstantInt::get(i64Ty, 1000));
      auto *us32 = builder->CreateTrunc(us, llvm::Type::getInt32Ty(*context));
      builder->CreateCall(usleep, {us32});
      builder->CreateRetVoid();
    }
  }
}void LLVMCodeGen::generateStdRandom() {
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *doubleTy = llvm::Type::getDoubleTy(*context);

  // Random_int
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
    if (!module->getFunction("Random_int")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Random_int", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *randTy =
          llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
      auto rand = module->getOrInsertFunction("rand", randTy);
      auto *randVal = builder->CreateCall(rand, {});
      auto *randVal64 = builder->CreateSExt(randVal, i64Ty);

      auto *min = fn->getArg(0);
      auto *max = fn->getArg(1);
      auto *range = builder->CreateSub(max, min);
      auto *mod = builder->CreateSRem(randVal64, range);
      auto *result = builder->CreateAdd(min, mod);
      builder->CreateRet(result);
    }
  }

  // Random_float
  {
    auto *fnTy = llvm::FunctionType::get(doubleTy, {}, false);
    if (!module->getFunction("Random_float")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Random_float", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *randTy =
          llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
      auto rand = module->getOrInsertFunction("rand", randTy);
      auto *randVal = builder->CreateCall(rand, {});
      auto *randFloat = builder->CreateSIToFP(randVal, doubleTy);
      auto *randMax = llvm::ConstantFP::get(doubleTy, 2147483647.0);
      auto *result = builder->CreateFDiv(randFloat, randMax);
      builder->CreateRet(result);
    }
  }
}

void LLVMCodeGen::generateStdSystem() {
  auto *i32Ty = llvm::Type::getInt32Ty(*context);
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *voidTy = llvm::Type::getVoidTy(*context);

  // System_exit
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i32Ty}, false);
    if (!module->getFunction("System_exit")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "System_exit", module.get());
      fn->addFnAttr(llvm::Attribute::NoReturn);
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *exit = module->getFunction("exit");
      builder->CreateCall(exit, {fn->getArg(0)});
      builder->CreateUnreachable();
    }
  }

  // System_getenv
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("System_getenv")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "System_getenv", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *getenvTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
      auto getenv = module->getOrInsertFunction("getenv", getenvTy);
      auto *result = builder->CreateCall(getenv, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }

  // System_exec
  {
    auto *fnTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
    if (!module->getFunction("System_exec")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "System_exec", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *systemTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
      auto system = module->getOrInsertFunction("system", systemTy);
      auto *result = builder->CreateCall(system, {fn->getArg(0)});
      builder->CreateRet(result);
    }
  }
}

void LLVMCodeGen::generateStdNetwork() {
    generateStdNetworkFull();
}

void LLVMCodeGen::generateStdCrypto() {
    generateStdCryptoFull();
}

llvm::Value *
LLVMCodeGen::callStdLibFunction(const std::string &module_name,
                                const std::string &func,
                                const std::vector<llvm::Value *> &args) {
  std::string funcName = module_name + "_" + func;

  auto *fn = module->getFunction(funcName);
  if (!fn) {
    fn = module->getFunction("mg_" + funcName);
  }

  if (!fn) {
    // Handle built-in mappings with automatic type detection
    if (module_name == "IO") {
      if (func == "print" && args.size() == 1) {
        auto *argType = args[0]->getType();
        if (argType->isPointerTy()) {
          return builder->CreateCall(runtime.printStr, args);
        } else if (argType->isIntegerTy(64)) {
          return builder->CreateCall(runtime.printInt, args);
        } else if (argType->isDoubleTy()) {
          return builder->CreateCall(runtime.printFloat, args);
        } else if (argType->isIntegerTy(1)) {
          return builder->CreateCall(runtime.printBool, args);
        }
      }
      if (func == "println" && args.size() == 1) {
        auto *argType = args[0]->getType();
        if (argType->isPointerTy()) {
          return builder->CreateCall(runtime.printlnStr, args);
        } else if (argType->isIntegerTy(64)) {
          return builder->CreateCall(runtime.printlnInt, args);
        } else if (argType->isDoubleTy()) {
          return builder->CreateCall(runtime.printlnFloat, args);
        } else if (argType->isIntegerTy(1)) {
          return builder->CreateCall(runtime.printlnBool, args);
        }
      }
      if (func == "readLine" && args.size() == 0) {
        auto *readLineFn = module->getFunction("IO_readLine");
        if (readLineFn) {
          return builder->CreateCall(readLineFn, {});
        }
      }
    }

    if (module_name == "Math") {
      if (func == "sqrt" && args.size() == 1) {
        auto *arg = args[0];
        if (!arg->getType()->isDoubleTy()) {
          arg = builder->CreateSIToFP(arg, llvm::Type::getDoubleTy(*context));
        }
        return builder->CreateCall(runtime.sqrtFunc, {arg});
      }
      if (func == "pow" && args.size() == 2) {
        auto *base = args[0];
        auto *exp = args[1];
        if (!base->getType()->isDoubleTy()) {
          base = builder->CreateSIToFP(base, llvm::Type::getDoubleTy(*context));
        }
        if (!exp->getType()->isDoubleTy()) {
          exp = builder->CreateSIToFP(exp, llvm::Type::getDoubleTy(*context));
        }
        return builder->CreateCall(runtime.powFunc, {base, exp});
      }
      if (func == "sin" && args.size() == 1) {
        auto *arg = args[0];
        if (!arg->getType()->isDoubleTy()) {
          arg = builder->CreateSIToFP(arg, llvm::Type::getDoubleTy(*context));
        }
        return builder->CreateCall(runtime.sinFunc, {arg});
      }
      if (func == "cos" && args.size() == 1) {
        auto *arg = args[0];
        if (!arg->getType()->isDoubleTy()) {
          arg = builder->CreateSIToFP(arg, llvm::Type::getDoubleTy(*context));
        }
        return builder->CreateCall(runtime.cosFunc, {arg});
      }
    }

    if (module_name == "String") {
      if (func == "length" && args.size() == 1) {
        return builder->CreateCall(runtime.stringLength, args);
      }
    }
if (module_name == "Network") {
    if (func == "socketsListen") {
      auto *fn = module->getFunction("Sockets_listen");
      if (!fn) {
        emitError("Sockets_listen function not generated");
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
      }
      return builder->CreateCall(fn, args);
    }
    if (func == "socketsAccept") {
      auto *fn = module->getFunction("Sockets_accept");
      if (!fn) {
        emitError("Sockets_accept function not generated");
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
      }
      return builder->CreateCall(fn, args);
    }
    if (func == "socketsConnect") {
      auto *fn = module->getFunction("Sockets_connect");
      if (!fn) {
        emitError("Sockets_connect function not generated");
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
      }
      return builder->CreateCall(fn, args);
    }
    if (func == "socketsSend") {
      auto *fn = module->getFunction("Sockets_send");
      if (!fn) {
        emitError("Sockets_send function not generated");
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
      }
      return builder->CreateCall(fn, args);
    }
    if (func == "socketsReceive") {
      auto *fn = module->getFunction("Sockets_receive");
      if (!fn) {
        emitError("Sockets_receive function not generated");
       return llvm::Constant::getNullValue(llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0));      }
      return builder->CreateCall(fn, args);
    }
    if (func == "socketsClose") {
      auto *fn = module->getFunction("Sockets_close");
      if (!fn) {
        emitError("Sockets_close function not generated");
        return nullptr;
      }
      return builder->CreateCall(fn, args);
    }
    if (func == "httpGet") {
      auto *fn = module->getFunction("Http_get");
      if (!fn) {
        emitError("Http_get function not generated");
        return llvm::Constant::getNullValue(llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0));
      }
      return builder->CreateCall(fn, args);
    }
  }

  // Crypto module
  if (module_name == "Crypto") {
    if (func == "sha256") {
      auto *fn = module->getFunction("Crypto_sha256");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "md5") {
      auto *fn = module->getFunction("Crypto_md5");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "base64Encode") {
      auto *fn = module->getFunction("Crypto_base64Encode");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "base64Decode") {
      auto *fn = module->getFunction("Crypto_base64Decode");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "randomBytes") {
      auto *fn = module->getFunction("Crypto_randomBytes");
      if (fn) return builder->CreateCall(fn, args);
    }
  }

  // Time module
  if (module_name == "Time") {
    if (func == "now" && args.size() == 0) {
      auto *fn = module->getFunction("Time_now");
      if (fn) return builder->CreateCall(fn, {});
    }
    if (func == "sleep") {
      auto *fn = module->getFunction("Time_sleep");
      if (fn) {
        builder->CreateCall(fn, args);
        return nullptr;
      }
    }
    if (func == "sleepSeconds") {
      // Convert seconds to milliseconds
      auto *fn = module->getFunction("Time_sleep");
      if (fn && args.size() == 1) {
        auto *ms = builder->CreateMul(args[0], 
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1000));
        builder->CreateCall(fn, {ms});
        return nullptr;
      }
    }
  }

  // Array module additions
  if (module_name == "Array") {
    if (func == "length") {
      auto *fn = module->getFunction("Array_length");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "push") {
      auto *fn = module->getFunction("Array_push");
      if (fn) {
        builder->CreateCall(fn, args);
        return nullptr;
      }
    }
    if (func == "pop") {
      auto *fn = module->getFunction("Array_pop");
      if (fn) return builder->CreateCall(fn, args);
    }
  }

  // Option module
  if (module_name == "Option") {
    if (func == "isSome") {
      auto *fn = module->getFunction("Option_isSome");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "isNone") {
      auto *fn = module->getFunction("Option_isNone");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "unwrap") {
      auto *fn = module->getFunction("Option_unwrap");
      if (fn) return builder->CreateCall(fn, args);
    }
    if (func == "unwrapOr") {
      auto *fn = module->getFunction("Option_unwrapOr");
      if (fn) return builder->CreateCall(fn, args);
    }
  }
    emitError("Unknown stdlib function: " + module_name + "." + func);
    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
  }

  // If function exists, cast arguments to match parameter types
  std::vector<llvm::Value *> castArgs = args;
  auto *fnType = fn->getFunctionType();
  for (size_t i = 0; i < castArgs.size() && i < fnType->getNumParams(); i++) {
    if (castArgs[i]->getType() != fnType->getParamType(i)) {
      castArgs[i] = castValue(castArgs[i], fnType->getParamType(i));
    }
  }

  return builder->CreateCall(fn, castArgs);
}

// =============================================================================
// Crypto Implementation (OpenSSL)
// =============================================================================

void LLVMCodeGen::generateStdCryptoFull() {
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i32Ty = llvm::Type::getInt32Ty(*context);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);
  auto *i8Ty = llvm::Type::getInt8Ty(*context);

  // Declare OpenSSL functions

  // SHA256
  auto *sha256Ty =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i64Ty, i8PtrTy}, false);
  auto SHA256_func = module->getOrInsertFunction("SHA256", sha256Ty);

  // MD5
  auto *md5Ty =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i64Ty, i8PtrTy}, false);
  auto MD5_func = module->getOrInsertFunction("MD5", md5Ty);

  // EVP functions for base64
  auto *EVP_EncodeBlock_Ty =
      llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy, i32Ty}, false);
  auto EVP_EncodeBlock =
      module->getOrInsertFunction("EVP_EncodeBlock", EVP_EncodeBlock_Ty);

  auto *EVP_DecodeBlock_Ty =
      llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy, i32Ty}, false);
  auto EVP_DecodeBlock =
      module->getOrInsertFunction("EVP_DecodeBlock", EVP_DecodeBlock_Ty);

  // RAND_bytes for random generation
  auto *RAND_bytes_Ty = llvm::FunctionType::get(i32Ty, {i8PtrTy, i32Ty}, false);
  auto RAND_bytes = module->getOrInsertFunction("RAND_bytes", RAND_bytes_Ty);

  // Helper: convert binary to hex string
  auto *binToHexTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i64Ty}, false);
  llvm::Function *binToHexFn = nullptr;
  if (!module->getFunction("_mg_bin_to_hex")) {
    binToHexFn =
        llvm::Function::Create(binToHexTy, llvm::Function::ExternalLinkage,
                               "_mg_bin_to_hex", module.get());
    auto *entry = llvm::BasicBlock::Create(*context, "entry", binToHexFn);
    builder->SetInsertPoint(entry);

    auto *data = binToHexFn->getArg(0);
    auto *len = binToHexFn->getArg(1);

    // Allocate hex string (2 chars per byte + null terminator)
    auto *hexLen = builder->CreateMul(len, llvm::ConstantInt::get(i64Ty, 2));
    auto *hexLenPlus1 =
        builder->CreateAdd(hexLen, llvm::ConstantInt::get(i64Ty, 1));
    auto *hexStr = builder->CreateCall(runtime.mallocFunc, {hexLenPlus1});

    // Convert each byte to hex
    auto *loopVar = createEntryBlockAlloca(binToHexFn, "i", i64Ty);
    builder->CreateStore(llvm::ConstantInt::get(i64Ty, 0), loopVar);

    auto *loopBB = llvm::BasicBlock::Create(*context, "loop", binToHexFn);
    auto *bodyBB = llvm::BasicBlock::Create(*context, "body", binToHexFn);
    auto *endBB = llvm::BasicBlock::Create(*context, "end", binToHexFn);

    builder->CreateBr(loopBB);

    builder->SetInsertPoint(loopBB);
    auto *i = builder->CreateLoad(i64Ty, loopVar);
    auto *cond = builder->CreateICmpSLT(i, len);
    builder->CreateCondBr(cond, bodyBB, endBB);

    builder->SetInsertPoint(bodyBB);
    auto *bytePtr = builder->CreateGEP(i8Ty, data, i);
    auto *byte = builder->CreateLoad(i8Ty, bytePtr);
    auto *byteExt = builder->CreateZExt(byte, i32Ty);

    // High nibble
    auto *high = builder->CreateLShr(byteExt, llvm::ConstantInt::get(i32Ty, 4));
    auto *highChar =
        builder->CreateAdd(high, llvm::ConstantInt::get(i32Ty, 48)); // '0'
    auto *isHighAlpha =
        builder->CreateICmpUGT(high, llvm::ConstantInt::get(i32Ty, 9));
    auto *highCharAlpha =
        builder->CreateAdd(high, llvm::ConstantInt::get(i32Ty, 87)); // 'a' - 10
    auto *highFinal =
        builder->CreateSelect(isHighAlpha, highCharAlpha, highChar);
    auto *highByte = builder->CreateTrunc(highFinal, i8Ty);

    // Low nibble
    auto *low = builder->CreateAnd(byteExt, llvm::ConstantInt::get(i32Ty, 15));
    auto *lowChar = builder->CreateAdd(low, llvm::ConstantInt::get(i32Ty, 48));
    auto *isLowAlpha =
        builder->CreateICmpUGT(low, llvm::ConstantInt::get(i32Ty, 9));
    auto *lowCharAlpha =
        builder->CreateAdd(low, llvm::ConstantInt::get(i32Ty, 87));
    auto *lowFinal = builder->CreateSelect(isLowAlpha, lowCharAlpha, lowChar);
    auto *lowByte = builder->CreateTrunc(lowFinal, i8Ty);

    // Store to hex string
    auto *hexIdx = builder->CreateMul(i, llvm::ConstantInt::get(i64Ty, 2));
    auto *hexPtr1 = builder->CreateGEP(i8Ty, hexStr, hexIdx);
    builder->CreateStore(highByte, hexPtr1);
    auto *hexIdx2 =
        builder->CreateAdd(hexIdx, llvm::ConstantInt::get(i64Ty, 1));
    auto *hexPtr2 = builder->CreateGEP(i8Ty, hexStr, hexIdx2);
    builder->CreateStore(lowByte, hexPtr2);

    auto *nextI = builder->CreateAdd(i, llvm::ConstantInt::get(i64Ty, 1));
    builder->CreateStore(nextI, loopVar);
    builder->CreateBr(loopBB);

    builder->SetInsertPoint(endBB);
    // Null terminate
    auto *nullPos = builder->CreateGEP(i8Ty, hexStr, hexLen);
    builder->CreateStore(llvm::ConstantInt::get(i8Ty, 0), nullPos);
    builder->CreateRet(hexStr);
  } else {
    binToHexFn = module->getFunction("_mg_bin_to_hex");
  }

  // Crypto_sha256
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("Crypto_sha256")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Crypto_sha256", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *input = fn->getArg(0);
      auto *strlen = module->getFunction("strlen");
      auto *inputLen = builder->CreateCall(strlen, {input});

      // Allocate 32 bytes for SHA256 hash
      auto *hash = builder->CreateCall(runtime.mallocFunc,
                                       {llvm::ConstantInt::get(i64Ty, 32)});

      // Compute SHA256
      builder->CreateCall(SHA256_func, {input, inputLen, hash});

      // Convert to hex string
      auto *hexStr = builder->CreateCall(
          binToHexFn, {hash, llvm::ConstantInt::get(i64Ty, 32)});

      builder->CreateCall(runtime.freeFunc, {hash});
      builder->CreateRet(hexStr);
    }
  }

  // Crypto_md5
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("Crypto_md5")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Crypto_md5", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *input = fn->getArg(0);
      auto *strlen = module->getFunction("strlen");
      auto *inputLen = builder->CreateCall(strlen, {input});

      // Allocate 16 bytes for MD5 hash
      auto *hash = builder->CreateCall(runtime.mallocFunc,
                                       {llvm::ConstantInt::get(i64Ty, 16)});

      // Compute MD5
      builder->CreateCall(MD5_func, {input, inputLen, hash});

      // Convert to hex string
      auto *hexStr = builder->CreateCall(
          binToHexFn, {hash, llvm::ConstantInt::get(i64Ty, 16)});

      builder->CreateCall(runtime.freeFunc, {hash});
      builder->CreateRet(hexStr);
    }
  }

  // Crypto_base64Encode
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("Crypto_base64Encode")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Crypto_base64Encode", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *input = fn->getArg(0);
      auto *strlen = module->getFunction("strlen");
      auto *inputLen = builder->CreateCall(strlen, {input});
      auto *inputLen32 = builder->CreateTrunc(inputLen, i32Ty);

      // Calculate output size: ((len + 2) / 3) * 4 + 1
      auto *inputLenPlus2 =
          builder->CreateAdd(inputLen, llvm::ConstantInt::get(i64Ty, 2));
      auto *div3 =
          builder->CreateUDiv(inputLenPlus2, llvm::ConstantInt::get(i64Ty, 3));
      auto *mul4 = builder->CreateMul(div3, llvm::ConstantInt::get(i64Ty, 4));
      auto *outLen = builder->CreateAdd(mul4, llvm::ConstantInt::get(i64Ty, 1));

      auto *output = builder->CreateCall(runtime.mallocFunc, {outLen});

      // Encode
      builder->CreateCall(EVP_EncodeBlock, {output, input, inputLen32});

      builder->CreateRet(output);
    }
  }

  // Crypto_base64Decode
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("Crypto_base64Decode")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Crypto_base64Decode", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *input = fn->getArg(0);
      auto *strlen = module->getFunction("strlen");
      auto *inputLen = builder->CreateCall(strlen, {input});
      auto *inputLen32 = builder->CreateTrunc(inputLen, i32Ty);

      // Output will be at most (3 * len / 4) + 1
      auto *mul3 =
          builder->CreateMul(inputLen, llvm::ConstantInt::get(i64Ty, 3));
      auto *div4 = builder->CreateUDiv(mul3, llvm::ConstantInt::get(i64Ty, 4));
      auto *outLen = builder->CreateAdd(div4, llvm::ConstantInt::get(i64Ty, 1));

      auto *output = builder->CreateCall(runtime.mallocFunc, {outLen});

      // Decode
      builder->CreateCall(EVP_DecodeBlock, {output, input, inputLen32});

      builder->CreateRet(output);
    }
  }

  // Crypto_randomBytes
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
    if (!module->getFunction("Crypto_randomBytes")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Crypto_randomBytes", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *count = fn->getArg(0);
      auto *count32 = builder->CreateTrunc(count, i32Ty);
      auto *buf = builder->CreateCall(runtime.mallocFunc, {count});

      // Generate cryptographically secure random bytes
      builder->CreateCall(RAND_bytes, {buf, count32});

      builder->CreateRet(buf);
    }
  }
}

// =============================================================================
// Network Implementation (Cross-platform sockets)
// =============================================================================

void LLVMCodeGen::generateStdNetworkFull() {
  auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto *i32Ty = llvm::Type::getInt32Ty(*context);
  auto *i64Ty = llvm::Type::getInt64Ty(*context);
  auto *i16Ty = llvm::Type::getInt16Ty(*context);
  auto *voidTy = llvm::Type::getVoidTy(*context);
  auto *i8Ty = llvm::Type::getInt8Ty(*context);
  if (!runtime.mallocFunc.getCallee()) {
    auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    auto *i64Ty = llvm::Type::getInt64Ty(*context);
    auto *mallocTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
    runtime.mallocFunc = module->getOrInsertFunction("malloc", mallocTy);
  }
  
  if (!runtime.freeFunc.getCallee()) {
    auto *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    auto *voidTy = llvm::Type::getVoidTy(*context);
    auto *freeTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    runtime.freeFunc = module->getOrInsertFunction("free", freeTy);
  }
  // Declare socket functions (POSIX compatible, works on Linux/macOS/BSD)
  // Windows would need Winsock but we'll use ifdef in the linking stage

  auto *socketTy = llvm::FunctionType::get(i32Ty, {i32Ty, i32Ty, i32Ty}, false);
  auto socket_func = module->getOrInsertFunction("socket", socketTy);

  auto *bindTy = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy, i32Ty}, false);
  auto bind_func = module->getOrInsertFunction("bind", bindTy);

  auto *listenTy = llvm::FunctionType::get(i32Ty, {i32Ty, i32Ty}, false);
  auto listen_func = module->getOrInsertFunction("listen", listenTy);

  auto *acceptTy =
      llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy, i8PtrTy}, false);
  auto accept_func = module->getOrInsertFunction("accept", acceptTy);

  auto *connectTy =
      llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrTy, i32Ty}, false);
  auto connect_func = module->getOrInsertFunction("connect", connectTy);

  auto *sendTy =
      llvm::FunctionType::get(i64Ty, {i32Ty, i8PtrTy, i64Ty, i32Ty}, false);
  auto send_func = module->getOrInsertFunction("send", sendTy);

  auto *recvTy =
      llvm::FunctionType::get(i64Ty, {i32Ty, i8PtrTy, i64Ty, i32Ty}, false);
  auto recv_func = module->getOrInsertFunction("recv", recvTy);

  auto *closeTy = llvm::FunctionType::get(i32Ty, {i32Ty}, false);
  auto close_func = module->getOrInsertFunction("close", closeTy);

  auto *htonsTy = llvm::FunctionType::get(i16Ty, {i16Ty}, false);
  auto htons_func = module->getOrInsertFunction("htons", htonsTy);

  auto *inet_addrTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
  auto inet_addr_func = module->getOrInsertFunction("inet_addr", inet_addrTy);

  auto *gethostbynameTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
  auto gethostbyname_func =
      module->getOrInsertFunction("gethostbyname", gethostbynameTy);

  auto *memsetTy =
      llvm::FunctionType::get(i8PtrTy, {i8PtrTy, i32Ty, i64Ty}, false);
  auto memset_func = module->getOrInsertFunction("memset", memsetTy);

  // Socket constants
  // AF_INET = 2, SOCK_STREAM = 1, IPPROTO_TCP = 6
  auto *AF_INET = llvm::ConstantInt::get(i32Ty, 2);
  auto *SOCK_STREAM = llvm::ConstantInt::get(i32Ty, 1);
  auto *IPPROTO_TCP = llvm::ConstantInt::get(i32Ty, 6);

  // sockaddr_in struct size (16 bytes typically)
  auto *SOCKADDR_SIZE = llvm::ConstantInt::get(i32Ty, 16);

  // Sockets_listen
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty}, false);
    if (!module->getFunction("Sockets_listen")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_listen", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *port = fn->getArg(0);
      auto *port16 = builder->CreateTrunc(port, i16Ty);

      // Create socket
      auto *sock =
          builder->CreateCall(socket_func, {AF_INET, SOCK_STREAM, IPPROTO_TCP});

      // Check if socket creation failed
      auto *sockFailed =
          builder->CreateICmpSLT(sock, llvm::ConstantInt::get(i32Ty, 0));
      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *successBB = llvm::BasicBlock::Create(*context, "success", fn);
      builder->CreateCondBr(sockFailed, errorBB, successBB);

      builder->SetInsertPoint(errorBB);
      builder->CreateRet(llvm::ConstantInt::get(i64Ty, -1));

      builder->SetInsertPoint(successBB);

      // Create sockaddr_in structure
      auto *addr = builder->CreateCall(runtime.mallocFunc,
                                       {llvm::ConstantInt::get(i64Ty, 16)});
      builder->CreateCall(memset_func, {addr, llvm::ConstantInt::get(i32Ty, 0),
                                        llvm::ConstantInt::get(i64Ty, 16)});

      // Set sin_family = AF_INET (offset 0, 2 bytes)
      auto *familyPtr =
          builder->CreateBitCast(addr, llvm::PointerType::get(i16Ty, 0));
      builder->CreateStore(builder->CreateTrunc(AF_INET, i16Ty), familyPtr);

      // Set sin_port = htons(port) (offset 2, 2 bytes)
      auto *portPtr = builder->CreateGEP(i16Ty, familyPtr,
                                         llvm::ConstantInt::get(i64Ty, 1));
      auto *portNetwork = builder->CreateCall(htons_func, {port16});
      builder->CreateStore(portNetwork, portPtr);

      // Set sin_addr = INADDR_ANY (0) (offset 4, 4 bytes)
      auto *addrPtr =
          builder->CreateGEP(i8Ty, addr, llvm::ConstantInt::get(i64Ty, 4));
      auto *addrPtr32 =
          builder->CreateBitCast(addrPtr, llvm::PointerType::get(i32Ty, 0));
      builder->CreateStore(llvm::ConstantInt::get(i32Ty, 0), addrPtr32);

      // Bind socket
      auto *bindResult =
          builder->CreateCall(bind_func, {sock, addr, SOCKADDR_SIZE});
      auto *bindFailed =
          builder->CreateICmpSLT(bindResult, llvm::ConstantInt::get(i32Ty, 0));

      auto *bindErrorBB = llvm::BasicBlock::Create(*context, "bind_error", fn);
      auto *listenBB = llvm::BasicBlock::Create(*context, "listen", fn);
      builder->CreateCondBr(bindFailed, bindErrorBB, listenBB);

      builder->SetInsertPoint(bindErrorBB);
      builder->CreateCall(close_func, {sock});
      builder->CreateRet(llvm::ConstantInt::get(i64Ty, -1));

      builder->SetInsertPoint(listenBB);

      // Listen with backlog of 10
      auto *listenResult = builder->CreateCall(
          listen_func, {sock, llvm::ConstantInt::get(i32Ty, 10)});
      auto *listenFailed = builder->CreateICmpSLT(
          listenResult, llvm::ConstantInt::get(i32Ty, 0));

      auto *listenErrorBB =
          llvm::BasicBlock::Create(*context, "listen_error", fn);
      auto *returnBB = llvm::BasicBlock::Create(*context, "return", fn);
      builder->CreateCondBr(listenFailed, listenErrorBB, returnBB);

      builder->SetInsertPoint(listenErrorBB);
      builder->CreateCall(close_func, {sock});
      builder->CreateRet(llvm::ConstantInt::get(i64Ty, -1));

      builder->SetInsertPoint(returnBB);
      auto *sockExt = builder->CreateSExt(sock, i64Ty);
      builder->CreateRet(sockExt);
    }
  }

  // Sockets_accept
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty}, false);
    if (!module->getFunction("Sockets_accept")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_accept", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *sockFd64 = fn->getArg(0);
      auto *sockFd = builder->CreateTrunc(sockFd64, i32Ty);

      // Accept connection (we ignore client address for simplicity)
      auto *clientSock = builder->CreateCall(
          accept_func, {sockFd, llvm::Constant::getNullValue(i8PtrTy),
                        llvm::Constant::getNullValue(i8PtrTy)});

      auto *clientSockExt = builder->CreateSExt(clientSock, i64Ty);
      builder->CreateRet(clientSockExt);
    }
  }

  // Sockets_connect
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i8PtrTy, i64Ty}, false);
    if (!module->getFunction("Sockets_connect")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_connect", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *host = fn->getArg(0);
      auto *port = fn->getArg(1);
      auto *port16 = builder->CreateTrunc(port, i16Ty);

      // Create socket
      auto *sock =
          builder->CreateCall(socket_func, {AF_INET, SOCK_STREAM, IPPROTO_TCP});

      auto *sockFailed =
          builder->CreateICmpSLT(sock, llvm::ConstantInt::get(i32Ty, 0));
      auto *errorBB = llvm::BasicBlock::Create(*context, "error", fn);
      auto *successBB = llvm::BasicBlock::Create(*context, "success", fn);
      builder->CreateCondBr(sockFailed, errorBB, successBB);

      builder->SetInsertPoint(errorBB);
      builder->CreateRet(llvm::ConstantInt::get(i64Ty, -1));

      builder->SetInsertPoint(successBB);

      // Create sockaddr_in
      auto *addr = builder->CreateCall(runtime.mallocFunc,
                                       {llvm::ConstantInt::get(i64Ty, 16)});
      builder->CreateCall(memset_func, {addr, llvm::ConstantInt::get(i32Ty, 0),
                                        llvm::ConstantInt::get(i64Ty, 16)});

      // Set sin_family
      auto *familyPtr =
          builder->CreateBitCast(addr, llvm::PointerType::get(i16Ty, 0));
      builder->CreateStore(builder->CreateTrunc(AF_INET, i16Ty), familyPtr);

      // Set sin_port
      auto *portPtr = builder->CreateGEP(i16Ty, familyPtr,
                                         llvm::ConstantInt::get(i64Ty, 1));
      auto *portNetwork = builder->CreateCall(htons_func, {port16});
      builder->CreateStore(portNetwork, portPtr);

      // Set sin_addr
      auto *ipAddr = builder->CreateCall(inet_addr_func, {host});
      auto *addrPtr =
          builder->CreateGEP(i8Ty, addr, llvm::ConstantInt::get(i64Ty, 4));
      auto *addrPtr32 =
          builder->CreateBitCast(addrPtr, llvm::PointerType::get(i32Ty, 0));
      builder->CreateStore(ipAddr, addrPtr32);

      // Connect
      auto *connectResult =
          builder->CreateCall(connect_func, {sock, addr, SOCKADDR_SIZE});
      auto *connectFailed = builder->CreateICmpSLT(
          connectResult, llvm::ConstantInt::get(i32Ty, 0));

      auto *connectErrorBB =
          llvm::BasicBlock::Create(*context, "connect_error", fn);
      auto *returnBB = llvm::BasicBlock::Create(*context, "return", fn);
      builder->CreateCondBr(connectFailed, connectErrorBB, returnBB);

      builder->SetInsertPoint(connectErrorBB);
      builder->CreateCall(close_func, {sock});
      builder->CreateRet(llvm::ConstantInt::get(i64Ty, -1));

      builder->SetInsertPoint(returnBB);
      auto *sockExt = builder->CreateSExt(sock, i64Ty);
      builder->CreateRet(sockExt);
    }
  }

  // Sockets_send
  {
    auto *fnTy = llvm::FunctionType::get(i64Ty, {i64Ty, i8PtrTy}, false);
    if (!module->getFunction("Sockets_send")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_send", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *sockFd64 = fn->getArg(0);
      auto *data = fn->getArg(1);
      auto *sockFd = builder->CreateTrunc(sockFd64, i32Ty);

      auto *strlen = module->getFunction("strlen");
      auto *len = builder->CreateCall(strlen, {data});

      auto *bytesSent = builder->CreateCall(
          send_func, {sockFd, data, len, llvm::ConstantInt::get(i32Ty, 0)});

      builder->CreateRet(bytesSent);
    }
  }
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i64Ty, i64Ty}, false);
    if (!module->getFunction("Sockets_receive")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_receive", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *sockFd64 = fn->getArg(0);
      auto *maxBytes = fn->getArg(1);
      auto *sockFd = builder->CreateTrunc(sockFd64, i32Ty);

      auto *buffer = builder->CreateCall(
          runtime.mallocFunc,
          {builder->CreateAdd(maxBytes, llvm::ConstantInt::get(i64Ty, 1))});

      auto *bytesRecv =
          builder->CreateCall(recv_func, {sockFd, buffer, maxBytes,
                                          llvm::ConstantInt::get(i32Ty, 0)});

      // Null terminate
      auto *nullPos = builder->CreateGEP(i8Ty, buffer, bytesRecv);
      builder->CreateStore(llvm::ConstantInt::get(i8Ty, 0), nullPos);

      builder->CreateRet(buffer);
    }
  }

  // Sockets_close
  {
    auto *fnTy = llvm::FunctionType::get(voidTy, {i64Ty}, false);
    if (!module->getFunction("Sockets_close")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Sockets_close", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      auto *sockFd64 = fn->getArg(0);
      auto *sockFd = builder->CreateTrunc(sockFd64, i32Ty);

      builder->CreateCall(close_func, {sockFd});
      builder->CreateRetVoid();
    }
  }

  // Simple HTTP GET (using existing socket infrastructure)
  {
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    if (!module->getFunction("Http_get")) {
      auto *fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage,
                                        "Http_get", module.get());
      auto *entry = llvm::BasicBlock::Create(*context, "entry", fn);
      builder->SetInsertPoint(entry);

      // For now, return placeholder
      // Full implementation would parse URL, connect, send HTTP request
      auto *placeholder =
          createString("[HTTP client not yet fully implemented - use libcurl]");
      builder->CreateRet(placeholder);
    }
  }
}

