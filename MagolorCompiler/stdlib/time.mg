// Std.Time - Time, Date, and Duration Operations Module
// Provides time measurement, date formatting, and time manipulation

// ============================================================================
// Time Constants
// ============================================================================

let MILLISECONDS_PER_SECOND = 1000;
let SECONDS_PER_MINUTE = 60;
let MINUTES_PER_HOUR = 60;
let HOURS_PER_DAY = 24;
let DAYS_PER_WEEK = 7;
let MONTHS_PER_YEAR = 12;

let SECONDS_PER_HOUR = 3600;
let SECONDS_PER_DAY = 86400;
let SECONDS_PER_WEEK = 604800;

// ============================================================================
// Current Time
// ============================================================================

// Get current Unix timestamp (seconds since epoch)
pub fn now() -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get current time in milliseconds
pub fn nowMillis() -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get current time in microseconds
pub fn nowMicros() -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get current time in nanoseconds
pub fn nowNanos() -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// ============================================================================
// Time Conversion
// ============================================================================

// Convert seconds to milliseconds
pub fn secondsToMillis(seconds: Int) -> Int {
    return seconds * MILLISECONDS_PER_SECOND;
}

// Convert milliseconds to seconds
pub fn millisToSeconds(millis: Int) -> Int {
    return millis / MILLISECONDS_PER_SECOND;
}

// Convert minutes to seconds
pub fn minutesToSeconds(minutes: Int) -> Int {
    return minutes * SECONDS_PER_MINUTE;
}

// Convert seconds to minutes
pub fn secondsToMinutes(seconds: Int) -> Int {
    return seconds / SECONDS_PER_MINUTE;
}

// Convert hours to seconds
pub fn hoursToSeconds(hours: Int) -> Int {
    return hours * SECONDS_PER_HOUR;
}

// Convert seconds to hours
pub fn secondsToHours(seconds: Int) -> Int {
    return seconds / SECONDS_PER_HOUR;
}

// Convert days to seconds
pub fn daysToSeconds(days: Int) -> Int {
    return days * SECONDS_PER_DAY;
}

// Convert seconds to days
pub fn secondsToDays(seconds: Int) -> Int {
    return seconds / SECONDS_PER_DAY;
}

// ============================================================================
// Duration Creation
// ============================================================================

// Create duration from milliseconds
pub fn milliseconds(ms: Int) -> Int {
    return ms;
}

// Create duration from seconds
pub fn seconds(s: Int) -> Int {
    return s * MILLISECONDS_PER_SECOND;
}

// Create duration from minutes
pub fn minutes(m: Int) -> Int {
    return m * SECONDS_PER_MINUTE * MILLISECONDS_PER_SECOND;
}

// Create duration from hours
pub fn hours(h: Int) -> Int {
    return h * SECONDS_PER_HOUR * MILLISECONDS_PER_SECOND;
}

// Create duration from days
pub fn days(d: Int) -> Int {
    return d * SECONDS_PER_DAY * MILLISECONDS_PER_SECOND;
}

// ============================================================================
// Date Components
// ============================================================================

// Get year from Unix timestamp
pub fn getYear(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get month from Unix timestamp (1-12)
pub fn getMonth(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get day of month from Unix timestamp (1-31)
pub fn getDay(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get hour from Unix timestamp (0-23)
pub fn getHour(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get minute from Unix timestamp (0-59)
pub fn getMinute(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get second from Unix timestamp (0-59)
pub fn getSecond(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get day of week from Unix timestamp (0=Sunday, 6=Saturday)
pub fn getDayOfWeek(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Get day of year from Unix timestamp (1-366)
pub fn getDayOfYear(timestamp: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// ============================================================================
// Date Formatting
// ============================================================================

// Format timestamp as ISO 8601 string (YYYY-MM-DD HH:MM:SS)
pub fn formatISO(timestamp: Int) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Format timestamp as date (YYYY-MM-DD)
pub fn formatDate(timestamp: Int) -> String {
    let year = getYear(timestamp);
    let month = getMonth(timestamp);
    let day = getDay(timestamp);
    return concat(concat(concat(concat(toString(year), "-"), padStartWith(toString(month), 2, "0")), "-"), padStartWith(toString(day), 2, "0"));
}

// Format timestamp as time (HH:MM:SS)
pub fn formatTime(timestamp: Int) -> String {
    let hour = getHour(timestamp);
    let minute = getMinute(timestamp);
    let second = getSecond(timestamp);
    return concat(concat(concat(concat(padStartWith(toString(hour), 2, "0"), ":"), padStartWith(toString(minute), 2, "0")), ":"), padStartWith(toString(second), 2, "0"));
}

// Format timestamp as human-readable string
pub fn formatDateTime(timestamp: Int) -> String {
    return concat(concat(formatDate(timestamp), " "), formatTime(timestamp));
}

// Format timestamp with custom format string
pub fn format(timestamp: Int, formatStr: String) -> String {
    // Implementation provided by LLVM runtime
    // Supports: YYYY, MM, DD, HH, mm, ss
    return "";
}

// ============================================================================
// Date Parsing
// ============================================================================

// Parse ISO 8601 string to timestamp
pub fn parseISO(dateStr: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Parse date string (YYYY-MM-DD) to timestamp
pub fn parseDate(dateStr: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Parse custom format string to timestamp
pub fn parse(dateStr: String, formatStr: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// ============================================================================
// Date Construction
// ============================================================================

// Create timestamp from date components
pub fn makeDate(year: Int, month: Int, day: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Create timestamp from date and time components
pub fn makeDateTime(year: Int, month: Int, day: Int, hour: Int, minute: Int, second: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// ============================================================================
// Date Arithmetic
// ============================================================================

// Add seconds to timestamp
pub fn addSeconds(timestamp: Int, seconds: Int) -> Int {
    return timestamp + seconds;
}

// Add minutes to timestamp
pub fn addMinutes(timestamp: Int, minutes: Int) -> Int {
    return timestamp + (minutes * SECONDS_PER_MINUTE);
}

// Add hours to timestamp
pub fn addHours(timestamp: Int, hours: Int) -> Int {
    return timestamp + (hours * SECONDS_PER_HOUR);
}

// Add days to timestamp
pub fn addDays(timestamp: Int, days: Int) -> Int {
    return timestamp + (days * SECONDS_PER_DAY);
}

// Add weeks to timestamp
pub fn addWeeks(timestamp: Int, weeks: Int) -> Int {
    return timestamp + (weeks * SECONDS_PER_WEEK);
}

// Add months to timestamp
pub fn addMonths(timestamp: Int, months: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Add years to timestamp
pub fn addYears(timestamp: Int, years: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Subtract two timestamps to get difference in seconds
pub fn diff(timestamp1: Int, timestamp2: Int) -> Int {
    return timestamp1 - timestamp2;
}

// Get difference in days
pub fn diffDays(timestamp1: Int, timestamp2: Int) -> Int {
    return diff(timestamp1, timestamp2) / SECONDS_PER_DAY;
}

// Get difference in hours
pub fn diffHours(timestamp1: Int, timestamp2: Int) -> Int {
    return diff(timestamp1, timestamp2) / SECONDS_PER_HOUR;
}

// Get difference in minutes
pub fn diffMinutes(timestamp1: Int, timestamp2: Int) -> Int {
    return diff(timestamp1, timestamp2) / SECONDS_PER_MINUTE;
}

// ============================================================================
// Date Comparison
// ============================================================================

// Check if timestamp1 is before timestamp2
pub fn isBefore(timestamp1: Int, timestamp2: Int) -> Bool {
    return timestamp1 < timestamp2;
}

// Check if timestamp1 is after timestamp2
pub fn isAfter(timestamp1: Int, timestamp2: Int) -> Bool {
    return timestamp1 > timestamp2;
}

// Check if two timestamps are on the same day
pub fn isSameDay(timestamp1: Int, timestamp2: Int) -> Bool {
    return formatDate(timestamp1) == formatDate(timestamp2);
}

// Check if timestamp is in the past
pub fn isPast(timestamp: Int) -> Bool {
    return timestamp < now();
}

// Check if timestamp is in the future
pub fn isFuture(timestamp: Int) -> Bool {
    return timestamp > now();
}

// Check if timestamp is today
pub fn isToday(timestamp: Int) -> Bool {
    return isSameDay(timestamp, now());
}

// ============================================================================
// Relative Time
// ============================================================================

// Get time until timestamp (in seconds)
pub fn timeUntil(timestamp: Int) -> Int {
    return timestamp - now();
}

// Get time since timestamp (in seconds)
pub fn timeSince(timestamp: Int) -> Int {
    return now() - timestamp;
}

// Format relative time (e.g., "2 hours ago", "in 3 days")
pub fn formatRelative(timestamp: Int) -> String {
    let diffSec = now() - timestamp;
    let absDiff = absInt(diffSec);
    
    if absDiff < SECONDS_PER_MINUTE {
        return "just now";
    }
    
    if absDiff < SECONDS_PER_HOUR {
        let mins = absDiff / SECONDS_PER_MINUTE;
        if diffSec < 0 {
            return concat(concat("in ", toString(mins)), " minutes");
        }
        return concat(concat(toString(mins), " minutes"), " ago");
    }
    
    if absDiff < SECONDS_PER_DAY {
        let hrs = absDiff / SECONDS_PER_HOUR;
        if diffSec < 0 {
            return concat(concat("in ", toString(hrs)), " hours");
        }
        return concat(concat(toString(hrs), " hours"), " ago");
    }
    
    let dys = absDiff / SECONDS_PER_DAY;
    if diffSec < 0 {
        return concat(concat("in ", toString(dys)), " days");
    }
    return concat(concat(toString(dys), " days"), " ago");
}

// ============================================================================
// Date Validation
// ============================================================================

// Check if year is a leap year
pub fn isLeapYear(year: Int) -> Bool {
    if year % 4 != 0 {
        return false;
    }
    if year % 100 != 0 {
        return true;
    }
    return year % 400 == 0;
}

// Get number of days in month
pub fn daysInMonth(year: Int, month: Int) -> Int {
    if month == 2 {
        if isLeapYear(year) {
            return 29;
        }
        return 28;
    }
    if month == 4 || month == 6 || month == 9 || month == 11 {
        return 30;
    }
    return 31;
}

// Validate date components
pub fn isValidDate(year: Int, month: Int, day: Int) -> Bool {
    if month < 1 || month > 12 {
        return false;
    }
    if day < 1 || day > daysInMonth(year, month) {
        return false;
    }
    return true;
}

// ============================================================================
// Sleep and Timing
// ============================================================================

// Sleep for specified milliseconds
pub fn sleep(millis: Int) {
    // Implementation provided by LLVM runtime
}

// Sleep for specified seconds
pub fn sleepSeconds(seconds: Int) {
    sleep(seconds * MILLISECONDS_PER_SECOND);
}

// ============================================================================
// Benchmarking
// ============================================================================

// Measure execution time of a function (returns duration in milliseconds)
pub fn measure(pub fn: () -> Void) -> Int {
    let start = nowMillis();
    pub fn();
    return nowMillis() - start;
}

// ============================================================================
// Month Names
// ============================================================================

// Get month name from month number (1-12)
pub fn getMonthName(month: Int) -> String {
    if month == 1 { return "January"; }
    if month == 2 { return "February"; }
    if month == 3 { return "March"; }
    if month == 4 { return "April"; }
    if month == 5 { return "May"; }
    if month == 6 { return "June"; }
    if month == 7 { return "July"; }
    if month == 8 { return "August"; }
    if month == 9 { return "September"; }
    if month == 10 { return "October"; }
    if month == 11 { return "November"; }
    if month == 12 { return "December"; }
    return "Unknown";
}

// Get abbreviated month name
pub fn getMonthNameShort(month: Int) -> String {
    return left(getMonthName(month), 3);
}

// Get day of week name (0=Sunday, 6=Saturday)
pub fn getDayName(day: Int) -> String {
    if day == 0 { return "Sunday"; }
    if day == 1 { return "Monday"; }
    if day == 2 { return "Tuesday"; }
    if day == 3 { return "Wednesday"; }
    if day == 4 { return "Thursday"; }
    if day == 5 { return "Friday"; }
    if day == 6 { return "Saturday"; }
    return "Unknown";
}

// Get abbreviated day name
pub fn getDayNameShort(day: Int) -> String {
    return left(getDayName(day), 3);
}
