/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 chys <admin@chys.info>
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

#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

static double g_factor = 0.5;
static time_t g_init_time;
static struct timeval g_init_tv;

static time_t (*real_time)(time_t *);
static int (*real_gettimeofday)(struct timeval *, struct timezone *);

static void initialize(void) __attribute__((constructor(101)));
static void initialize(void) {
	const char *mul_s = getenv("SLOWER_FACTOR");
	if (mul_s) {
		double mul = strtod(mul_s, NULL);
		if ((mul >= 0.01) && (mul <= 100))
			g_factor = 1. / mul;
	}

	real_time = dlsym(RTLD_NEXT, "time");
	real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");

	real_time(&g_init_time);
	real_gettimeofday(&g_init_tv, NULL);
}

time_t time(time_t *pres) {
	time_t now = real_time(NULL);
	now = (time_t)((now - g_init_time) * g_factor);
	if (pres)
		*pres = now;
	return now;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
	int res = real_gettimeofday(tv, tz);
	if (res != 0)
		return res;

	double elapsed = (tv->tv_sec - g_init_tv.tv_sec) + (tv->tv_usec - g_init_tv.tv_usec) * 1e-6;
	elapsed *= g_factor;

	long usec = g_init_tv.tv_usec + (long)((elapsed - floor(elapsed)) * 1e6);
	tv->tv_usec = usec % 1000000;
	tv->tv_sec = g_init_tv.tv_sec + (long)elapsed + usec / 1000000;
	return 0;
}
