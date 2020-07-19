/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 chys <admin@chys.info>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

typedef struct ClockInfo {
	int64_t init_time;
	int64_t fake_init_time;
} ClockInfo;

#define CLOCK_COUNT 12
static ClockInfo g_clocks[CLOCK_COUNT] = {};

static double g_factor = 0.5;

static int (*real_clock_gettime)(clockid_t, struct timespec *);

static inline int64_t ts_to_int64(struct timespec ts) {
	return ts.tv_sec * INT64_C(1000000000) + ts.tv_nsec;
}

static void do_initialize(void) {
	const char *mul_s = getenv("SLOWER_FACTOR");
	if (mul_s) {
		double mul = strtod(mul_s, NULL);
		if ((mul >= 0.01) && (mul <= 100))
			g_factor = 1. / mul;
	}

	real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");

	{
		struct timespec ts;
		real_clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		g_clocks[CLOCK_MONOTONIC_RAW].init_time = ts_to_int64(ts);
		g_clocks[CLOCK_MONOTONIC_RAW].fake_init_time = g_clocks[CLOCK_MONOTONIC_RAW].init_time;
	}
	{
		struct timespec ts;
		real_clock_gettime(CLOCK_REALTIME, &ts);
		g_clocks[CLOCK_REALTIME].init_time = ts_to_int64(ts);
		g_clocks[CLOCK_REALTIME].fake_init_time = g_clocks[CLOCK_REALTIME].init_time;
	}

	const char *time_s = getenv("SLOWER_TIME");
	if (time_s) {
		struct tm tm;
		if (strptime(time_s, "%Y-%m-%d %H:%M:%S", &tm) != NULL) {
			g_clocks[CLOCK_REALTIME].fake_init_time = mktime(&tm) * INT64_C(1000000000);
		}
	}
}

static void initialize(void) {
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	pthread_once(&once, &do_initialize);
}

static time_t hack_time(time_t *);
static int hack_gettimeofday(struct timeval *, struct timezone *);
static int hack_clock_gettime(clockid_t clockid, struct timespec *ts);

static time_t hack_time(time_t *pres) {
	struct timespec ts;
#ifdef CLOCK_REALTIME_COARSE
	hack_clock_gettime(CLOCK_REALTIME_COARSE, &ts);
#else
	hack_clock_gettime(CLOCK_REALTIME, &ts);
#endif
	time_t now = ts.tv_sec;
	if (pres)
		*pres = now;
	return now;
}

static int hack_gettimeofday(struct timeval *tv, struct timezone *tz) {
	// A non-null tz is long deprecated, and we don't support it.
	if (tz != NULL) {
		errno = ENOSYS;
		return -1;
	}

	struct timespec ts;
	int ret = hack_clock_gettime(CLOCK_REALTIME, &ts);
	if (ret != 0) {
		return ret;
	}

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / 1000;
	return 0;
}

// Returns either CLOCK_REALTIME or CLOCK_MONOTONIC_RAW
static clockid_t get_reference_clock(clockid_t clockid) {
	switch (clockid) {
	case CLOCK_REALTIME:
#ifdef CLOCK_REALTIME_COARSE
	case CLOCK_REALTIME_COARSE:
#endif
#ifdef CLOCK_REALTIME_ALARM
	case CLOCK_REALTIME_ALARM:
#endif
#ifdef CLOCK_TAI
	case CLOCK_TAI:
#endif
		return CLOCK_REALTIME;
	default:
		return CLOCK_MONOTONIC_RAW;
	}
}

static int hack_clock_gettime(clockid_t clockid, struct timespec *ts) {
	initialize();

	if (clockid < 0 || clockid >= CLOCK_COUNT) {
		return real_clock_gettime(clockid, ts);
	}

	clockid_t clockid_ref = get_reference_clock(clockid);
	ClockInfo *clock_ref = &g_clocks[clockid_ref];

	ClockInfo *clock = &g_clocks[clockid];
	if (clock->init_time == 0 && clock->fake_init_time == 0) {
		int ret = real_clock_gettime(clockid_ref, ts);
		if (ret != 0) {
			return ret;
		}
		int64_t now_ref = ts_to_int64(*ts);
		int64_t elapsed_ref = now_ref - clock_ref->init_time;

		ret = real_clock_gettime(clockid, ts);
		if (ret != 0) {
			return ret;
		}
		int64_t now = ts_to_int64(*ts);

		clock->init_time = now - elapsed_ref;
		clock->fake_init_time = clock->init_time + (clock_ref->fake_init_time - clock_ref->init_time);
	}

	int ret = real_clock_gettime(clockid, ts);
	if (ret != 0) {
		return ret;
	}

	int64_t now = ts_to_int64(*ts);
	int64_t fake_now = clock->fake_init_time + (int64_t)((now - clock->init_time) * g_factor);
	ts->tv_sec = fake_now / 1000000000;
	ts->tv_nsec = fake_now % 1000000000;
	if (ts->tv_nsec < 0) {
		ts->tv_sec--;
		ts->tv_nsec += 1000000000;
	}
	return 0;
}

time_t time(time_t *) __attribute__((alias("hack_time")));

// The second parameter of gettimeofday is "struct timezone *".
// However, it's long deprecated, and most recent glibc versions declare it as void *.
// For compatibility with older and newer glibc, we have to do this hack.
int GETTIMEOFDAY(struct timeval *, struct timezone *) asm("gettimeofday") __attribute__((alias("hack_gettimeofday")));

int clock_gettime(clockid_t, struct timespec *) __attribute__((alias("hack_clock_gettime")));
