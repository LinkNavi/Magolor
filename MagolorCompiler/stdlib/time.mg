// Std.Time - Time and date operations
// Timestamps, durations, and date manipulation

using Std.Core.Prelude;

// ============================================================================
// Current time
// ============================================================================

// Get current Unix timestamp in seconds
pub fn now() -> int {
    @cpp {
        return static_cast<int>(std::time(nullptr));
    }
}

// Get current Unix timestamp in milliseconds
pub fn nowMillis() -> int {
    @cpp {
        auto now = std::chrono::system_clock::now();
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());
    }
}

// Get current Unix timestamp in nanoseconds (as string to avoid overflow)
pub fn nowNanos() -> string {
    @cpp {
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        return std::to_string(nanos);
    }
}

// High-resolution monotonic clock for timing
pub fn monotonicNanos() -> string {
    @cpp {
        auto now = std::chrono::steady_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        return std::to_string(nanos);
    }
}

// ============================================================================
// Sleeping/waiting
// ============================================================================

pub fn sleep(milliseconds: int) {
    @cpp {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
}

pub fn sleepSec(seconds: int) {
    @cpp {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
}

pub fn sleepMicro(microseconds: int) {
    @cpp {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    }
}

// ============================================================================
// Date/time formatting
// ============================================================================

pub fn formatTimestamp(timestamp: int, format: string) -> string {
    @cpp {
        std::time_t time = static_cast<std::time_t>(timestamp);
        std::tm* tm = std::localtime(&time);
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), format.c_str(), tm);
        return std::string(buffer);
    }
}

pub fn isoDate(timestamp: int) -> string {
    return formatTimestamp(timestamp, "%Y-%m-%d");
}

pub fn isoTime(timestamp: int) -> string {
    return formatTimestamp(timestamp, "%H:%M:%S");
}

pub fn isoDateTime(timestamp: int) -> string {
    return formatTimestamp(timestamp, "%Y-%m-%dT%H:%M:%S");
}

pub fn rfc2822(timestamp: int) -> string {
    return formatTimestamp(timestamp, "%a, %d %b %Y %H:%M:%S %z");
}

// Human-readable "X ago" format
pub fn timeAgo(timestamp: int) -> string {
    let diff = now() - timestamp;
    
    if (diff < 60) { return toString(diff) + " seconds ago"; }
    if (diff < 3600) { return toString(diff / 60) + " minutes ago"; }
    if (diff < 86400) { return toString(diff / 3600) + " hours ago"; }
    if (diff < 604800) { return toString(diff / 86400) + " days ago"; }
    if (diff < 2592000) { return toString(diff / 604800) + " weeks ago"; }
    if (diff < 31536000) { return toString(diff / 2592000) + " months ago"; }
    return toString(diff / 31536000) + " years ago";
}

// ============================================================================
// Date/time parsing
// ============================================================================

pub fn parseTimestamp(dateStr: string, format: string) -> Option<int> {
    @cpp {
        std::tm tm = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tm, format.c_str());
        if (ss.fail()) return std::nullopt;
        return std::make_optional(static_cast<int>(std::mktime(&tm)));
    }
}

pub fn parseIso(dateStr: string) -> Option<int> {
    return parseTimestamp(dateStr, "%Y-%m-%dT%H:%M:%S");
}

// ============================================================================
// Date components
// ============================================================================

pub class DateTime {
    pub year: int;
    pub month: int;
    pub day: int;
    pub hour: int;
    pub minute: int;
    pub second: int;
    pub weekday: int;  // 0 = Sunday
    pub yearday: int;  // 1-366
    pub isDst: bool;
}

pub fn toDateTime(timestamp: int) -> DateTime {
    @cpp {
        std::time_t time = static_cast<std::time_t>(timestamp);
        std::tm* tm = std::localtime(&time);
        DateTime dt;
        dt.year = tm->tm_year + 1900;
        dt.month = tm->tm_mon + 1;
        dt.day = tm->tm_mday;
        dt.hour = tm->tm_hour;
        dt.minute = tm->tm_min;
        dt.second = tm->tm_sec;
        dt.weekday = tm->tm_wday;
        dt.yearday = tm->tm_yday + 1;
        dt.isDst = tm->tm_isdst > 0;
        return dt;
    }
}

pub fn toTimestamp(dt: DateTime) -> int {
    @cpp {
        std::tm tm = {};
        tm.tm_year = dt.year - 1900;
        tm.tm_mon = dt.month - 1;
        tm.tm_mday = dt.day;
        tm.tm_hour = dt.hour;
        tm.tm_min = dt.minute;
        tm.tm_sec = dt.second;
        tm.tm_isdst = dt.isDst ? 1 : 0;
        return static_cast<int>(std::mktime(&tm));
    }
}

pub fn year(timestamp: int) -> int {
    return toDateTime(timestamp).year;
}

pub fn month(timestamp: int) -> int {
    return toDateTime(timestamp).month;
}

pub fn day(timestamp: int) -> int {
    return toDateTime(timestamp).day;
}

pub fn hour(timestamp: int) -> int {
    return toDateTime(timestamp).hour;
}

pub fn minute(timestamp: int) -> int {
    return toDateTime(timestamp).minute;
}

pub fn second(timestamp: int) -> int {
    return toDateTime(timestamp).second;
}

pub fn weekday(timestamp: int) -> int {
    return toDateTime(timestamp).weekday;
}

pub fn weekdayName(day: int) -> string {
    let names = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    return names[day % 7];
}

pub fn monthName(month: int) -> string {
    let names = ["January", "February", "March", "April", "May", "June",
                 "July", "August", "September", "October", "November", "December"];
    return names[(month - 1) % 12];
}

// ============================================================================
// Date arithmetic
// ============================================================================

pub fn addDays(timestamp: int, days: int) -> int {
    return timestamp + (days * 86400);
}

pub fn addHours(timestamp: int, hours: int) -> int {
    return timestamp + (hours * 3600);
}

pub fn addMinutes(timestamp: int, minutes: int) -> int {
    return timestamp + (minutes * 60);
}

pub fn addSeconds(timestamp: int, seconds: int) -> int {
    return timestamp + seconds;
}

pub fn diffDays(a: int, b: int) -> int {
    return (a - b) / 86400;
}

pub fn diffHours(a: int, b: int) -> int {
    return (a - b) / 3600;
}

pub fn diffMinutes(a: int, b: int) -> int {
    return (a - b) / 60;
}

// ============================================================================
// Timers and stopwatch
// ============================================================================

pub class Stopwatch {
    startTime: string;
    pub elapsed: int;
    pub running: bool;
    
    pub fn create() {
        this.startTime = "0";
        this.elapsed = 0;
        this.running = false;
    }
    
    pub fn start() {
        this.startTime = monotonicNanos();
        this.running = true;
    }
    
    pub fn stop() -> int {
        if (this.running) {
            // Simplified - return milliseconds
            this.elapsed = nowMillis();
            this.running = false;
        }
        return this.elapsed;
    }
    
    pub fn reset() {
        this.startTime = "0";
        this.elapsed = 0;
        this.running = false;
    }
}

pub fn newStopwatch() -> Stopwatch {
    return new Stopwatch();
}

// Measure execution time of a function
pub fn measure(f: fn()) -> int {
    let start = nowMillis();
    f();
    return nowMillis() - start;
}
