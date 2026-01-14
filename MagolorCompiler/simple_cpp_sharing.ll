; ModuleID = 'simple_cpp_sharing'
source_filename = "simple_cpp_sharing"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"%s\00"
@.str.1 = private unnamed_addr constant [5 x i8] c"%lld\00"
@.str.2 = private unnamed_addr constant [6 x i8] c"%lld\0A\00"
@.str.3 = private unnamed_addr constant [3 x i8] c"%g\00"
@.str.4 = private unnamed_addr constant [4 x i8] c"%g\0A\00"
@.str.5 = private unnamed_addr constant [5 x i8] c"true\00"
@.str.6 = private unnamed_addr constant [6 x i8] c"false\00"
@.str.7 = private unnamed_addr constant [4 x i8] c"%s\0A\00"
@.str.8 = private unnamed_addr constant [19 x i8] c"Runtime Error: %s\0A\00"
@stdin = external global ptr
@.str.9 = private unnamed_addr constant [28 x i8] c"Called unwrap on None value\00"
@.str.10 = private unnamed_addr constant [33 x i8] c"=== Basic Variable Sharing ===\0A\0A\00"
@.str.11 = private unnamed_addr constant [19 x i8] c"Before C++: count=\00"
@.str.12 = private unnamed_addr constant [10 x i8] c"<unknown>\00"
@.str.13 = private unnamed_addr constant [7 x i8] c", sum=\00"
@.str.14 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.15 = private unnamed_addr constant [64 x i8] c"Warning: @cpp block encountered - requires external compilation\00"
@.str.16 = private unnamed_addr constant [24 x i8] c"Back in Magolor: count=\00"
@.str.17 = private unnamed_addr constant [27 x i8] c"\0A=== C++ Calculation ===\0A\0A\00"
@.str.18 = private unnamed_addr constant [27 x i8] c"Calculating sum from 1 to \00"
@.str.19 = private unnamed_addr constant [5 x i8] c"...\0A\00"
@.str.20 = private unnamed_addr constant [19 x i8] c"Magolor received: \00"
@.str.21 = private unnamed_addr constant [31 x i8] c"\0A=== String Manipulation ===\0A\0A\00"
@.str.22 = private unnamed_addr constant [6 x i8] c"Hello\00"
@.str.23 = private unnamed_addr constant [12 x i8] c"Original: '\00"
@.str.24 = private unnamed_addr constant [3 x i8] c"'\0A\00"
@.str.25 = private unnamed_addr constant [16 x i8] c"Magolor sees: '\00"
@.str.26 = private unnamed_addr constant [28 x i8] c"\0A=== Float Operations ===\0A\0A\00"
@.str.27 = private unnamed_addr constant [14 x i8] c"Temperature: \00"
@.str.28 = private unnamed_addr constant [5 x i8] c"\C2\B0C\0A\00"
@.str.29 = private unnamed_addr constant [9 x i8] c"Result: \00"
@.str.30 = private unnamed_addr constant [7 x i8] c"\C2\B0C = \00"
@.str.31 = private unnamed_addr constant [5 x i8] c"\C2\B0F\0A\00"
@.str.32 = private unnamed_addr constant [116 x i8] c"\E2\95\94\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\97\0A\00"
@.str.33 = private unnamed_addr constant [44 x i8] c"\E2\95\91  Variable Sharing Demo             \E2\95\91\0A\00"
@.str.34 = private unnamed_addr constant [46 x i8] c"\E2\95\91  Magolor \E2\86\94 C++ @cpp blocks         \E2\95\91\0A\00"
@.str.35 = private unnamed_addr constant [117 x i8] c"\E2\95\9A\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\90\E2\95\9D\0A\0A\00"
@.str.36 = private unnamed_addr constant [30 x i8] c"\0A\E2\9C\85 Variable sharing works!\0A\00"
@.str.37 = private unnamed_addr constant [21 x i8] c"\0A\F0\9F\92\A1 How it works:\0A\00"
@.str.38 = private unnamed_addr constant [49 x i8] c"   \E2\80\A2 All Magolor variables accessible in @cpp\0A\00"
@.str.39 = private unnamed_addr constant [43 x i8] c"   \E2\80\A2 Changes in @cpp persist to Magolor\0A\00"
@.str.40 = private unnamed_addr constant [50 x i8] c"   \E2\80\A2 'let mut' = modifiable, 'let' = read-only\0A\00"

declare i32 @printf(ptr, ...)

declare i32 @puts(ptr)

declare i64 @getline(ptr, ptr, ptr)

declare ptr @malloc(i64)

declare void @free(ptr)

declare ptr @memcpy(ptr, ptr, i64)

declare ptr @memset(ptr, i32, i64)

declare i64 @strlen(ptr)

declare ptr @strcpy(ptr, ptr)

declare ptr @strcat(ptr, ptr)

declare i32 @strcmp(ptr, ptr)

declare ptr @strncpy(ptr, ptr, i64)

declare double @pow(double, double)

declare double @sqrt(double)

declare double @sin(double)

declare double @cos(double)

declare double @tan(double)

declare double @log(double)

declare double @exp(double)

declare double @floor(double)

declare double @ceil(double)

declare double @fabs(double)

declare ptr @fopen(ptr, ptr)

declare i32 @fclose(ptr)

declare i64 @fread(ptr, i64, i64, ptr)

declare i64 @fwrite(ptr, i64, i64, ptr)

declare ptr @fgets(ptr, i32, ptr)

declare i32 @fputs(ptr, ptr)

declare void @exit(i32)

declare i32 @sprintf(ptr, ptr, ...)

declare i32 @atoi(ptr)

declare double @atof(ptr)

declare i64 @strtol(ptr, ptr, i32)

define void @mg_print_str(ptr %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str, ptr %0)
  ret void
}

define void @mg_println_str(ptr %0) {
entry:
  %1 = call i32 @puts(ptr %0)
  ret void
}

define void @mg_print_int(i64 %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str.1, i64 %0)
  ret void
}

define void @mg_println_int(i64 %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str.2, i64 %0)
  ret void
}

define void @mg_print_float(double %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str.3, double %0)
  ret void
}

define void @mg_println_float(double %0) {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str.4, double %0)
  ret void
}

define void @mg_print_bool(i1 %0) {
entry:
  %1 = select i1 %0, ptr @.str.5, ptr @.str.6
  %2 = call i32 (ptr, ...) @printf(ptr @.str, ptr %1)
  ret void
}

define void @mg_println_bool(i1 %0) {
entry:
  %1 = select i1 %0, ptr @.str.5, ptr @.str.6
  %2 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %1)
  ret void
}

define ptr @mg_string_concat(ptr %0, ptr %1) {
entry:
  %2 = call i64 @strlen(ptr %0)
  %3 = call i64 @strlen(ptr %1)
  %4 = add i64 %2, %3
  %5 = add i64 %4, 1
  %6 = call ptr @malloc(i64 %5)
  %7 = call ptr @strcpy(ptr %6, ptr %0)
  %8 = call ptr @strcat(ptr %6, ptr %1)
  ret ptr %6
}

define i64 @mg_string_length(ptr %0) {
entry:
  %1 = call i64 @strlen(ptr %0)
  ret i64 %1
}

define ptr @mg_int_to_string(i64 %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = call i32 (ptr, ptr, ...) @sprintf(ptr %1, ptr @.str.1, i64 %0)
  ret ptr %1
}

define ptr @mg_float_to_string(double %0) {
entry:
  %1 = call ptr @malloc(i64 64)
  %2 = call i32 (ptr, ptr, ...) @sprintf(ptr %1, ptr @.str.3, double %0)
  ret ptr %1
}

define ptr @mg_bool_to_string(i1 %0) {
entry:
  %1 = select i1 %0, ptr @.str.5, ptr @.str.6
  ret ptr %1
}

; Function Attrs: noreturn
define void @mg_throw_error(ptr %0) #0 {
entry:
  %1 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr %0)
  call void @exit(i32 1)
  unreachable
}

define void @IO_println(ptr %0) {
entry:
  call void @mg_println_str(ptr %0)
  ret void
}

define void @IO_print(ptr %0) {
entry:
  call void @mg_print_str(ptr %0)
  ret void
}

define ptr @IO_readLine() {
entry:
  %0 = call ptr @malloc(i64 4096)
  %1 = load ptr, ptr @stdin, align 8
  %2 = call ptr @fgets(ptr %0, i32 4096, ptr %1)
  %3 = call i64 @strlen(ptr %0)
  %4 = sub i64 %3, 1
  %5 = getelementptr i8, ptr %0, i64 %4
  store i8 0, ptr %5, align 1
  ret ptr %0
}

define i64 @Math_abs_int(i64 %0) {
entry:
  %1 = icmp slt i64 %0, 0
  %2 = sub i64 0, %0
  %3 = select i1 %1, i64 %2, i64 %0
  ret i64 %3
}

define double @Math_abs(double %0) {
entry:
  %1 = call double @fabs(double %0)
  ret double %1
}

define double @Math_sqrt(double %0) {
entry:
  %1 = call double @sqrt(double %0)
  ret double %1
}

define double @Math_pow(double %0, double %1) {
entry:
  %2 = call double @pow(double %0, double %1)
  ret double %2
}

define double @Math_sin(double %0) {
entry:
  %1 = call double @sin(double %0)
  ret double %1
}

define double @Math_cos(double %0) {
entry:
  %1 = call double @cos(double %0)
  ret double %1
}

define double @Math_tan(double %0) {
entry:
  %1 = call double @tan(double %0)
  ret double %1
}

define double @Math_log(double %0) {
entry:
  %1 = call double @log(double %0)
  ret double %1
}

define double @Math_exp(double %0) {
entry:
  %1 = call double @exp(double %0)
  ret double %1
}

define double @Math_floor(double %0) {
entry:
  %1 = call double @floor(double %0)
  ret double %1
}

define double @Math_ceil(double %0) {
entry:
  %1 = call double @ceil(double %0)
  ret double %1
}

define i64 @Math_min_int(i64 %0, i64 %1) {
entry:
  %2 = icmp slt i64 %0, %1
  %3 = select i1 %2, i64 %0, i64 %1
  ret i64 %3
}

define i64 @Math_max_int(i64 %0, i64 %1) {
entry:
  %2 = icmp sgt i64 %0, %1
  %3 = select i1 %2, i64 %0, i64 %1
  ret i64 %3
}

define i64 @String_length(ptr %0) {
entry:
  %1 = call i64 @mg_string_length(ptr %0)
  ret i64 %1
}

define i1 @String_isEmpty(ptr %0) {
entry:
  %1 = call i64 @mg_string_length(ptr %0)
  %2 = icmp eq i64 %1, 0
  ret i1 %2
}

define ptr @String_concat(ptr %0, ptr %1) {
entry:
  %2 = call ptr @mg_string_concat(ptr %0, ptr %1)
  ret ptr %2
}

define i64 @Array_length(ptr %0) {
entry:
  %1 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 1
  %2 = load i64, ptr %1, align 4
  ret i64 %2
}

define void @Array_push(ptr %0, i64 %1) {
entry:
  %2 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 1
  %3 = load i64, ptr %2, align 4
  %4 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 2
  %5 = load i64, ptr %4, align 4
  %6 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 0
  %7 = load ptr, ptr %6, align 8
  %8 = icmp sge i64 %3, %5
  br i1 %8, label %resize, label %store

resize:                                           ; preds = %entry
  %9 = mul i64 %5, 2
  %10 = mul i64 %9, 8
  %11 = call ptr @malloc(i64 %10)
  %12 = mul i64 %3, 8
  %13 = call ptr @memcpy(ptr %11, ptr %7, i64 %12)
  call void @free(ptr %7)
  store ptr %11, ptr %6, align 8
  store i64 %9, ptr %4, align 4
  br label %store

store:                                            ; preds = %resize, %entry
  %14 = load ptr, ptr %6, align 8
  %15 = mul i64 %3, 8
  %16 = getelementptr i8, ptr %14, i64 %15
  store i64 %1, ptr %16, align 4
  %17 = add i64 %3, 1
  store i64 %17, ptr %2, align 4
  ret void
}

define i64 @Array_pop(ptr %0) {
entry:
  %1 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 1
  %2 = load i64, ptr %1, align 4
  %3 = getelementptr inbounds nuw { ptr, i64, i64 }, ptr %0, i32 0, i32 0
  %4 = load ptr, ptr %3, align 8
  %5 = sub i64 %2, 1
  %6 = mul i64 %5, 8
  %7 = getelementptr i8, ptr %4, i64 %6
  %8 = load i64, ptr %7, align 4
  store i64 %5, ptr %1, align 4
  ret i64 %8
}

define i1 @Option_isSome({ i1, i64 } %0) {
entry:
  %1 = extractvalue { i1, i64 } %0, 0
  ret i1 %1
}

define i1 @Option_isNone({ i1, i64 } %0) {
entry:
  %1 = extractvalue { i1, i64 } %0, 0
  %2 = xor i1 %1, true
  ret i1 %2
}

define i64 @Option_unwrap({ i1, i64 } %0) {
entry:
  %1 = extractvalue { i1, i64 } %0, 0
  br i1 %1, label %ok, label %error

error:                                            ; preds = %entry
  call void @mg_throw_error(ptr @.str.9)
  unreachable

ok:                                               ; preds = %entry
  %2 = extractvalue { i1, i64 } %0, 1
  ret i64 %2
}

define i64 @Option_unwrapOr({ i1, i64 } %0, i64 %1) {
entry:
  %2 = extractvalue { i1, i64 } %0, 0
  %3 = extractvalue { i1, i64 } %0, 1
  %4 = select i1 %2, i64 %3, i64 %1
  ret i64 %4
}

define void @demonstrateBasicSharing() {
entry:
  %multiplier = alloca i64, align 8
  %sum = alloca i64, align 8
  %count = alloca i64, align 8
  call void @IO_print(ptr @.str.10)
  store i64 0, ptr %count, align 4
  store i64 0, ptr %sum, align 4
  store i64 2, ptr %multiplier, align 4
  %0 = load i64, ptr %count, align 4
  %1 = load i64, ptr %sum, align 4
  %2 = call ptr @mg_string_concat(ptr @.str.11, ptr @.str.12)
  %3 = call ptr @mg_string_concat(ptr %2, ptr @.str.13)
  %4 = call ptr @mg_string_concat(ptr %3, ptr @.str.12)
  %5 = call ptr @mg_string_concat(ptr %4, ptr @.str.14)
  call void @IO_print(ptr %5)
  call void @mg_println_str(ptr @.str.15)
  %6 = load i64, ptr %count, align 4
  %7 = load i64, ptr %sum, align 4
  %8 = call ptr @mg_string_concat(ptr @.str.16, ptr @.str.12)
  %9 = call ptr @mg_string_concat(ptr %8, ptr @.str.13)
  %10 = call ptr @mg_string_concat(ptr %9, ptr @.str.12)
  %11 = call ptr @mg_string_concat(ptr %10, ptr @.str.14)
  call void @IO_print(ptr %11)
  ret void
}

define void @demonstrateCalculation() {
entry:
  %limit = alloca i64, align 8
  %result = alloca i64, align 8
  call void @IO_print(ptr @.str.17)
  store i64 0, ptr %result, align 4
  store i64 10, ptr %limit, align 4
  %0 = load i64, ptr %limit, align 4
  %1 = call ptr @mg_string_concat(ptr @.str.18, ptr @.str.12)
  %2 = call ptr @mg_string_concat(ptr %1, ptr @.str.19)
  call void @IO_print(ptr %2)
  call void @mg_println_str(ptr @.str.15)
  %3 = load i64, ptr %result, align 4
  %4 = call ptr @mg_string_concat(ptr @.str.20, ptr @.str.12)
  %5 = call ptr @mg_string_concat(ptr %4, ptr @.str.14)
  call void @IO_print(ptr %5)
  ret void
}

define void @demonstrateStrings() {
entry:
  %message = alloca ptr, align 8
  call void @IO_print(ptr @.str.21)
  store ptr @.str.22, ptr %message, align 8
  %0 = load ptr, ptr %message, align 8
  %1 = call ptr @mg_string_concat(ptr @.str.23, ptr @.str.12)
  %2 = call ptr @mg_string_concat(ptr %1, ptr @.str.24)
  call void @IO_print(ptr %2)
  call void @mg_println_str(ptr @.str.15)
  %3 = load ptr, ptr %message, align 8
  %4 = call ptr @mg_string_concat(ptr @.str.25, ptr @.str.12)
  %5 = call ptr @mg_string_concat(ptr %4, ptr @.str.24)
  call void @IO_print(ptr %5)
  ret void
}

define void @demonstrateFloats() {
entry:
  %temperature_f = alloca double, align 8
  %temperature_c = alloca double, align 8
  call void @IO_print(ptr @.str.26)
  store double 2.500000e+01, ptr %temperature_c, align 8
  store double 0.000000e+00, ptr %temperature_f, align 8
  %0 = load double, ptr %temperature_c, align 8
  %1 = call ptr @mg_string_concat(ptr @.str.27, ptr @.str.12)
  %2 = call ptr @mg_string_concat(ptr %1, ptr @.str.28)
  call void @IO_print(ptr %2)
  call void @mg_println_str(ptr @.str.15)
  %3 = load double, ptr %temperature_c, align 8
  %4 = load double, ptr %temperature_f, align 8
  %5 = call ptr @mg_string_concat(ptr @.str.29, ptr @.str.12)
  %6 = call ptr @mg_string_concat(ptr %5, ptr @.str.30)
  %7 = call ptr @mg_string_concat(ptr %6, ptr @.str.12)
  %8 = call ptr @mg_string_concat(ptr %7, ptr @.str.31)
  call void @IO_print(ptr %8)
  ret void
}

define i32 @main() {
entry:
  call void @IO_print(ptr @.str.32)
  call void @IO_print(ptr @.str.33)
  call void @IO_print(ptr @.str.34)
  call void @IO_print(ptr @.str.35)
  call void @demonstrateBasicSharing()
  call void @demonstrateCalculation()
  call void @demonstrateStrings()
  call void @demonstrateFloats()
  call void @IO_print(ptr @.str.36)
  call void @IO_print(ptr @.str.37)
  call void @IO_print(ptr @.str.38)
  call void @IO_print(ptr @.str.39)
  call void @IO_print(ptr @.str.40)
  ret i32 0
}

attributes #0 = { noreturn }
