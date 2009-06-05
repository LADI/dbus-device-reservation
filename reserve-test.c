/***
  Copyright 2009 Lennart Poettering

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dbus/dbus.h>

#include "reserve.h"

#define RUN_FOR_SECS 60

static int quit = 0;
static rd_device *device = NULL;

static int request_cb(rd_device *d, int forced) {
	printf("Giving up device. (forced=%i)\n", forced);

	rd_release(d);
	device = NULL;

	quit = 1;
	return 1;
}

int main(int argc, char *argv[]) {
	DBusError error;
	DBusConnection *c;
	int r = 1, e;
	time_t started_at;
	int32_t priority = 0;
	const char *dname;

	dbus_error_init(&error);

	if (argc >= 2)
		priority = atoi(argv[1]);

	if (argc >= 3)
		dname = argv[2];
	else
		dname = "Audio0";

	printf("Using priority %i.\n", priority);

	if (!(c = dbus_bus_get(DBUS_BUS_SESSION, &error))) {
		fprintf(stderr, "Failed to connect to session bus: %s\n", error.message);
		goto finish;
	}

	if ((e = rd_acquire(
		     &device,
		     c,
		     dname,
		     "ReserveTest",
		     priority,
		     request_cb,
		     &error)) < 0) {

		fprintf(stderr, "Failed to acquire device: %s\n", strerror(-e));
		goto finish;
	}

	printf("Successfully acquired device.\n");

	started_at = time(NULL);

	while (!quit) {
		time_t t;

		t = time(NULL);

		if (t >= started_at + RUN_FOR_SECS)
			break;

		if (!dbus_connection_read_write_dispatch(c, (started_at + RUN_FOR_SECS - t) * 1000))
			break;
	}

	r = 0;
	fprintf(stderr, "Exiting cleanly.\n");

finish:

	if (device)
		rd_release(device);

	if (c)
		dbus_connection_unref(c);

	return r;
}
