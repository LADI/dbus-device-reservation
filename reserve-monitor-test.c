/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-*/

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

#include "reserve-monitor.h"

static DBusConnection *connection = NULL;

static void change_cb(rm_monitor *m) {
	int e;

	if ((e = rm_busy(m)) < 0) {
		fprintf(stderr, "Failed to query device status: %s\n", strerror(-e));
		dbus_connection_close(connection);
		return;
	}

	printf("Device is busy: %s\n", e ? "yes" : "no");
}

int main(int argc, char *argv[]) {
	DBusError error;
	int r = 1, e;
	rm_monitor *monitor = NULL;
	const char *dname = NULL;

	dbus_error_init(&error);

	if (argc >= 2)
		dname = argv[1];
	else
		dname = "Audio0";

	if (!(connection = dbus_bus_get_private(DBUS_BUS_SESSION, &error))) {
		fprintf(stderr, "Failed to connect to session bus: %s\n", error.message);
		goto finish;
	}

	if ((e = rm_watch(
		    &monitor,
		    connection,
		    dname,
		    change_cb,
		    &error)) < 0) {

		fprintf(stderr, "Failed to watch device: %s\n", strerror(-e));
		goto finish;
	}

	if ((e = rm_busy(monitor)) < 0) {
		fprintf(stderr, "Failed to query device status: %s\n", strerror(-e));
		goto finish;
	}

	printf("Device is busy: %s\n", e ? "yes" : "no");

	while (dbus_connection_read_write_dispatch(connection, -1))
                ;

	r = 0;
	fprintf(stderr, "Exiting cleanly.\n");

finish:
	if (monitor)
		rm_release(monitor);

	if (connection) {
		if (dbus_connection_get_is_connected(connection))
			dbus_connection_close(connection);

		dbus_connection_unref(connection);
	}

	return r;
}
