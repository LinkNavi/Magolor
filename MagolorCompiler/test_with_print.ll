; ModuleID = 'magolor_program'
source_filename = "magolor_program"

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare void @free(ptr)

declare void @print_int(i32)

declare void @print_str(ptr)

declare void @print_f32(float)

declare void @print_f64(double)

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

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 5, ptr %x, align 4
  %fact = alloca i32, align 4
  %x1 = load i32, ptr %x, align 4
  %call = call i32 @factorial(i32 %x1)
  store i32 %call, ptr %fact, align 4
  %fact2 = load i32, ptr %fact, align 4
  call void @print_int(i32 %fact2)
  ret i32 0
}
