; ModuleID = 'magolor_program'
source_filename = "magolor_program"

@Calculator.lastResult = global i32 0

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare void @free(ptr)

define i32 @factorial(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %cmp = icmp sle i32 %n1, 1
  %zext = zext i1 %cmp to i32
  %ifcond = icmp ne i32 %zext, 0
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  ret i32 1

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else
  %n2 = load i32, ptr %n, align 4
  %n3 = load i32, ptr %n, align 4
  %sub = sub i32 %n3, 1
  %call = call i32 @factorial(i32 %sub)
  %mul = mul i32 %n2, %call
  ret i32 %mul
}

define i32 @fibonacci(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %n1 = load i32, ptr %n, align 4
  %cmp = icmp sle i32 %n1, 1
  %zext = zext i1 %cmp to i32
  %ifcond = icmp ne i32 %zext, 0
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  %n2 = load i32, ptr %n, align 4
  ret i32 %n2

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else
  %n3 = load i32, ptr %n, align 4
  %sub = sub i32 %n3, 1
  %call = call i32 @fibonacci(i32 %sub)
  %n4 = load i32, ptr %n, align 4
  %sub5 = sub i32 %n4, 2
  %call6 = call i32 @fibonacci(i32 %sub5)
  %add = add i32 %call, %call6
  ret i32 %add
}

define i32 @sum(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %result = alloca i32, align 4
  store i32 0, ptr %result, align 4
  %i = alloca i32, align 4
  store i32 1, ptr %i, align 4
  br label %whilecond

whilecond:                                        ; preds = %whilebody, %entry
  %i1 = load i32, ptr %i, align 4
  %n2 = load i32, ptr %n, align 4
  %cmp = icmp sle i32 %i1, %n2
  %zext = zext i1 %cmp to i32
  %whilecond3 = icmp ne i32 %zext, 0
  br i1 %whilecond3, label %whilebody, label %afterwhile

whilebody:                                        ; preds = %whilecond
  %result4 = load i32, ptr %result, align 4
  %i5 = load i32, ptr %i, align 4
  %add = add i32 %result4, %i5
  store i32 %add, ptr %result, align 4
  %i6 = load i32, ptr %i, align 4
  %add7 = add i32 %i6, 1
  store i32 %add7, ptr %i, align 4
  br label %whilecond

afterwhile:                                       ; preds = %whilecond
  %result8 = load i32, ptr %result, align 4
  ret i32 %result8
}

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 5, ptr %x, align 4
  %fact = alloca i32, align 4
  %x1 = load i32, ptr %x, align 4
  %call = call i32 @factorial(i32 %x1)
  store i32 %call, ptr %fact, align 4
  %fib = alloca i32, align 4
  %call2 = call i32 @fibonacci(i32 10)
  store i32 %call2, ptr %fib, align 4
  %total = alloca i32, align 4
  %call3 = call i32 @sum(i32 100)
  store i32 %call3, ptr %total, align 4
  %fact4 = load i32, ptr %fact, align 4
  ret i32 %fact4
}

define i32 @Calculator.add(i32 %0, i32 %1) {
entry:
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  %b = alloca i32, align 4
  store i32 %1, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %add = add i32 %a1, %b2
  ret i32 %add
}

define i32 @Calculator.multiply(i32 %0, i32 %1) {
entry:
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  %b = alloca i32, align 4
  store i32 %1, ptr %b, align 4
  %result = alloca i32, align 4
  store i32 0, ptr %result, align 4
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %whilecond

whilecond:                                        ; preds = %whilebody, %entry
  %i1 = load i32, ptr %i, align 4
  %b2 = load i32, ptr %b, align 4
  %cmp = icmp slt i32 %i1, %b2
  %zext = zext i1 %cmp to i32
  %whilecond3 = icmp ne i32 %zext, 0
  br i1 %whilecond3, label %whilebody, label %afterwhile

whilebody:                                        ; preds = %whilecond
  %result4 = load i32, ptr %result, align 4
  %a5 = load i32, ptr %a, align 4
  %add = add i32 %result4, %a5
  store i32 %add, ptr %result, align 4
  %i6 = load i32, ptr %i, align 4
  %add7 = add i32 %i6, 1
  store i32 %add7, ptr %i, align 4
  br label %whilecond

afterwhile:                                       ; preds = %whilecond
  %result8 = load i32, ptr %result, align 4
  ret i32 %result8
}
