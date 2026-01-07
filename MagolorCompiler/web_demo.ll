; ModuleID = 'web_demo'
source_filename = "web_demo"

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

define void @main() {
entry:
  ret void
}
