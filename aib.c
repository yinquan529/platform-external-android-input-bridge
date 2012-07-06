/*
 * Copyright (C) 2012  Romain Perier <romain.perier@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include "common.h"

#define BUFFERING_EVENTS_QUEUE_SIZE EVENTS_QUEUE_SIZE

static uint32_t buffer[BUFFERING_EVENTS_QUEUE_SIZE * 2];

static void usage(char *progname)
{
    fprintf(stderr, "Usage: %s <hostname> <port> <inputdevice>\n", progname);
    exit(1);
}

static int input_device_init(const char *device)
{
    int indev_fd;
    char *devname = NULL;

    indev_fd = openx(device, O_RDONLY);
    devname = malloc(64 * sizeof(char));

    if (devname == NULL)
        return indev_fd;
    ioctlx(indev_fd, EVIOCGNAME(64), devname);

    printf("Capturing events for device %s\n", devname);
    free(devname);

    return indev_fd;
}

static int _connect(const char *hostname, uint16_t port)
{
    struct sockaddr_in aidd_addr;
    int sock_fd;

    sock_fd = socketx(AF_INET, SOCK_STREAM, 0);
    aidd_addr.sin_family = AF_INET;
    inet_aton(hostname, &(aidd_addr.sin_addr));
    aidd_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr *)&aidd_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("aib");
        exit(1);
    }
    return sock_fd;
}

static void send_event(int sock, struct input_event *ev)
{
    int i = 0, j = 1;

    bzero(buffer, sizeof(buffer));
    for (i = 0; i < BUFFERING_EVENTS_QUEUE_SIZE; i++) {
        buffer[j] = ev[i].type;
        buffer[j] = htonl((buffer[j] << 16) | ev[i].code);
        buffer[j+1] = htonl(ev[i].value);

        j += 2;
        if (is_ev_syn(ev + i))
            break;
    }
    buffer[0] = htonl(j - 1);
    send(sock, buffer, sizeof(uint32_t) * j, 0);
}

static void wait_evdev_input(int aibd_sock, int inputdev)
{
    struct input_event *ev;
    int i = 0;

    ev = malloc(sizeof(struct input_event) * BUFFERING_EVENTS_QUEUE_SIZE);
    while (read(inputdev, ev + i, sizeof(struct input_event))) {
        D("[%lu.%04lu] type %u, code %u, value %d\n", ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type, ev[i].code, ev[i].value);

        if (ev[i].type == EV_MSC)
            continue;
        if (is_ev_syn(ev + i)) {
            send_event(aibd_sock, ev);
	    bzero(ev, sizeof(struct input_event) * BUFFERING_EVENTS_QUEUE_SIZE);
	    i = 0;
	    continue;
	}
        i = (i + 1) % BUFFERING_EVENTS_QUEUE_SIZE; 
    }
}

int main (int argc, char *argv[])
{
    uint16_t port;
    char *endptr = NULL;
    int aidd_sock, inputdev;

    if (argc < 4)
        usage(argv[0]);

    port = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0')
        usage(argv[0]);

    inputdev = input_device_init(argv[3]);
    aidd_sock = _connect(argv[1], port);
    wait_evdev_input(aidd_sock, inputdev);

    return 0;
}
