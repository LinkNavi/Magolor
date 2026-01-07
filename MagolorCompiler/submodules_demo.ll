; ModuleID = 'submodules_demo'
source_filename = "submodules_demo"

@.str = private constant [31 x i8] c"<script>alert('XSS');</script>\00"
@.str.1 = private constant [14 x i8] c"Hello \22World\22\00"
@.str.2 = private constant [21 x i8] c"Path: C:\\Users\\Admin\00"
@.str.3 = private constant [13 x i8] c"New\0ALine\09Tab\00"
@.str.4 = private constant [11 x i8] c"/users/:id\00"
@.str.5 = private constant [11 x i8] c"/users/123\00"
@.str.6 = private constant [29 x i8] c"/users/:userId/posts/:postId\00"
@.str.7 = private constant [21 x i8] c"/users/456/posts/789\00"
@.str.8 = private constant [20 x i8] c"/api/:version/users\00"
@.str.9 = private constant [8 x i8] c"/api/v1\00"
@.str.10 = private constant [12 x i8] c"Hello World\00"
@.str.11 = private constant [17 x i8] c"user@example.com\00"
@.str.12 = private constant [43 x i8] c"name=John&age=30&city=New+York&active=true\00"
@.str.13 = private constant [11 x i8] c"session_id\00"
@.str.14 = private constant [13 x i8] c"abc123xyz789\00"
@.str.15 = private constant [2 x i8] c"/\00"
@.str.16 = private constant [10 x i8] c"user_pref\00"
@.str.17 = private constant [10 x i8] c"dark_mode\00"
@.str.18 = private constant [7 x i8] c"Strict\00"

declare i8* @malloc(i64)

declare void @free(i8*)

declare i8* @mg_alloc_simple(i64)

declare void @mg_free_simple(i8*)

declare void @mg_incref(i8*)

declare void @mg_decref(i8*)

declare i8* @mg_string_create(i8*)

declare i8* @mg_string_create_len(i8*, i32)

declare void @mg_string_destroy(i8*)

declare i8* @mg_string_concat(i8*, i8*)

declare i32 @mg_string_length(i8*)

declare i32 @mg_string_compare(i8*, i8*)

declare i1 @mg_string_equals(i8*, i8*)

declare i8* @mg_string_substring(i8*, i32, i32)

declare i32 @mg_string_index_of(i8*, i8*)

declare i1 @mg_string_starts_with(i8*, i8*)

declare i1 @mg_string_ends_with(i8*, i8*)

declare i1 @mg_string_contains(i8*, i8*)

declare i8* @mg_string_trim(i8*)

declare i8* @mg_string_to_lower(i8*)

declare i8* @mg_string_to_upper(i8*)

declare i8* @mg_string_replace(i8*, i8*, i8*)

declare i8* @mg_string_repeat(i8*, i32)

declare i8 @mg_string_char_at(i8*, i32)

declare i8* @mg_string_split(i8*, i8)

declare i8* @mg_string_join(i8*, i8*)

declare i8* @mg_array_create(i32, i32)

declare void @mg_array_destroy(i8*)

declare i32 @mg_array_length(i8*)

declare void @mg_array_push(i8**, i8*)

declare i8* @mg_array_pop(i8*)

declare i8* @mg_array_get(i8*, i32)

declare void @mg_array_set(i8*, i32, i8*)

declare void @mg_array_clear(i8*)

declare void @mg_array_reverse(i8*)

declare i8* @mg_option_some(i8*, i32)

declare i8* @mg_option_none()

declare void @mg_option_destroy(i8*)

declare i1 @mg_option_is_some(i8*)

declare i1 @mg_option_is_none(i8*)

declare i8* @mg_option_unwrap(i8*)

declare i8* @mg_option_unwrap_or(i8*, i8*)

declare void @mg_print(i8*)

declare void @mg_println(i8*)

declare void @mg_print_int(i32)

declare void @mg_println_int(i32)

declare void @mg_print_float(double)

declare void @mg_println_float(double)

declare void @mg_print_bool(i1)

declare void @mg_println_bool(i1)

declare void @mg_eprint(i8*)

declare void @mg_eprintln(i8*)

declare i8* @mg_readLine()

declare i8 @mg_readChar()

declare i8* @mg_int_to_string(i32)

declare i8* @mg_float_to_string(double)

declare i8* @mg_bool_to_string(i1)

declare i8* @mg_char_to_string(i8)

declare i32 @mg_string_to_int(i8*)

declare double @mg_string_to_float(i8*)

declare i1 @mg_string_to_bool(i8*)

declare i8* @mg_parse_int(i8*)

declare i8* @mg_parse_float(i8*)

declare i32 @mg_abs_int(i32)

declare double @mg_abs_float(double)

declare double @mg_sqrt(double)

declare double @mg_cbrt(double)

declare double @mg_pow(double, double)

declare double @mg_exp(double)

declare double @mg_log(double)

declare double @mg_log10(double)

declare double @mg_log2(double)

declare double @mg_sin(double)

declare double @mg_cos(double)

declare double @mg_tan(double)

declare double @mg_asin(double)

declare double @mg_acos(double)

declare double @mg_atan(double)

declare double @mg_atan2(double, double)

declare double @mg_floor(double)

declare double @mg_ceil(double)

declare double @mg_round(double)

declare double @mg_fmod(double, double)

declare i32 @mg_min_int(i32, i32)

declare i32 @mg_max_int(i32, i32)

declare double @mg_min_float(double, double)

declare double @mg_max_float(double, double)

declare i32 @mg_clamp_int(i32, i32, i32)

declare double @mg_clamp_float(double, double, double)

declare void @mg_random_init()

declare i32 @mg_random_int(i32, i32)

declare double @mg_random_float()

declare double @mg_random_float_range(double, double)

declare i1 @mg_random_bool()

declare i1 @mg_file_exists(i8*)

declare i1 @mg_file_is_file(i8*)

declare i1 @mg_file_is_directory(i8*)

declare i8* @mg_file_read(i8*)

declare i1 @mg_file_write(i8*, i8*)

declare i1 @mg_file_append(i8*, i8*)

declare i1 @mg_file_delete(i8*)

declare i1 @mg_dir_create(i8*)

declare i1 @mg_dir_delete(i8*)

declare i64 @mg_file_size(i8*)

declare i64 @mg_time_now()

declare i64 @mg_time_now_millis()

declare void @mg_time_sleep(i32)

declare i8* @mg_time_format(i64, i8*)

declare i8* @mg_env_get(i8*)

declare i1 @mg_env_set(i8*, i8*)

declare i32 @mg_system_exec(i8*)

declare void @mg_exit(i32)

declare i8* @mg_map_create(i32)

declare void @mg_map_destroy(i8*)

declare void @mg_map_set(i8*, i8*, i8*)

declare i8* @mg_map_get(i8*, i8*)

declare i1 @mg_map_has(i8*, i8*)

declare i1 @mg_map_remove(i8*, i8*)

declare i32 @mg_map_size(i8*)

declare void @mg_map_clear(i8*)

declare void @mg_runtime_init()

declare void @mg_runtime_shutdown()

declare void @mg_panic(i8*)

declare void @mg_assert(i1, i8*)

define void @testHTTPMethods() {
entry:
  ret void

forcond:                                          ; No predecessors!

forloop:                                          ; No predecessors!

afterfor:                                         ; No predecessors!
}

define void @testSecurity() {
entry:
  %count = alloca i32, align 4
  %dangerousInput = alloca i8*, align 8
  store i8* getelementptr inbounds ([31 x i8], [31 x i8]* @.str, i32 0, i32 0), i8** %dangerousInput, align 8
  store i32 0, i32* %count, align 4
  br label %whilecond

whilecond:                                        ; preds = %whileloop, %entry
  %count1 = load i32, i32* %count, align 4
  %cmptmp = icmp slt i32 %count1, 7
  br i1 %cmptmp, label %whileloop, label %afterloop

whileloop:                                        ; preds = %whilecond
  %count2 = load i32, i32* %count, align 4
  %addtmp = add i32 %count2, 1
  store i32 %addtmp, i32* %count, align 4
  br label %whilecond

afterloop:                                        ; preds = %whilecond
  ret void
}

define void @testJSON() {
entry:
  %str3 = alloca i8*, align 8
  %str2 = alloca i8*, align 8
  %str1 = alloca i8*, align 8
  store i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str.1, i32 0, i32 0), i8** %str1, align 8
  store i8* getelementptr inbounds ([21 x i8], [21 x i8]* @.str.2, i32 0, i32 0), i8** %str2, align 8
  store i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str.3, i32 0, i32 0), i8** %str3, align 8
  ret void
}

define void @testRouting() {
entry:
  %path3 = alloca i8*, align 8
  %pattern3 = alloca i8*, align 8
  %path2 = alloca i8*, align 8
  %pattern2 = alloca i8*, align 8
  %path1 = alloca i8*, align 8
  %pattern1 = alloca i8*, align 8
  store i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i32 0, i32 0), i8** %pattern1, align 8
  store i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.5, i32 0, i32 0), i8** %path1, align 8
  store i8* getelementptr inbounds ([29 x i8], [29 x i8]* @.str.6, i32 0, i32 0), i8** %pattern2, align 8
  store i8* getelementptr inbounds ([21 x i8], [21 x i8]* @.str.7, i32 0, i32 0), i8** %path2, align 8
  store i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.8, i32 0, i32 0), i8** %pattern3, align 8
  store i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.9, i32 0, i32 0), i8** %path3, align 8
  ret void
}

define void @testURLUtils() {
entry:
  %query = alloca i8*, align 8
  %original2 = alloca i8*, align 8
  %original1 = alloca i8*, align 8
  store i8* getelementptr inbounds ([12 x i8], [12 x i8]* @.str.10, i32 0, i32 0), i8** %original1, align 8
  store i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.11, i32 0, i32 0), i8** %original2, align 8
  store i8* getelementptr inbounds ([43 x i8], [43 x i8]* @.str.12, i32 0, i32 0), i8** %query, align 8
  ret void
}

define void @testStatusCodes() {
entry:
  ret void
}

define void @testResponseHelpers() {
entry:
  ret void
}

define void @testCookies() {
entry:
  ret void
}

define void @printBanner() {
entry:
  ret void
}

define void @printSummary() {
entry:
  ret void
}

define void @main() {
entry:
  call void @printBanner()
  call void @testHTTPMethods()
  call void @testSecurity()
  call void @testJSON()
  call void @testRouting()
  call void @testURLUtils()
  call void @testStatusCodes()
  call void @testResponseHelpers()
  call void @testCookies()
  call void @printSummary()
  ret void
}
