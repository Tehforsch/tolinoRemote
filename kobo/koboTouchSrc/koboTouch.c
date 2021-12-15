#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

const char *fname = "/dev/input/event1";

struct input_event make_event(__u16 type, __u16 code, __s32 value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = type;
    event.code = code;
    event.value = value;
    return event;
}

void send_event(int *fd, struct input_event *event) {
    write(*fd, event, sizeof(*event));
    usleep(1000);
}

int openDevice() {
    int fd = open(fname, O_RDWR);
    if (fd < 0) {
        printf("Error opening mouse event file:%s\n", strerror(errno));
        return -1;
    }
    return fd;
}

void touch_at(int x, int y) {
    int fd = openDevice();
    struct input_event event_x = make_event(EV_ABS, ABS_X, x);
    struct input_event event_y = make_event(EV_ABS, ABS_Y, y);
    struct input_event event_pressure_101 = make_event(EV_ABS, ABS_PRESSURE, 101);
    struct input_event event_pressure_100 = make_event(EV_ABS, ABS_PRESSURE, 100);
    struct input_event event_pressure_0 = make_event(EV_ABS, ABS_PRESSURE, 0);
    struct input_event event_touch = make_event(EV_KEY, BTN_TOUCH, 1);
    struct input_event event_release = make_event(EV_KEY, BTN_TOUCH, 0);
    struct input_event event_end = make_event(EV_SYN, SYN_REPORT, 0);
    // If we don't reset the X and Y coordinates, the EV_ABS: ABS_X/ABS_Y events
    // will not be received when executing this multiple times in a row. I am not 
    // sure why, but this means that the touch doesn't work (even though the EV_KEY
    // and EV_ABS/ABS_PRESSURE events still take place.
    // Here, we simply touch at slightly offset coordinates after each touch
    struct input_event event_slightly_move_x = make_event(EV_ABS, ABS_X, x-1);
    struct input_event event_slightly_move_y = make_event(EV_ABS, ABS_Y, y-1);

    send_event(&fd, &event_x);
    send_event(&fd, &event_y);
    send_event(&fd, &event_pressure_101);
    send_event(&fd, &event_touch);
    send_event(&fd, &event_end);
    usleep(10000);
    send_event(&fd, &event_slightly_move_x);
    send_event(&fd, &event_slightly_move_y);
    send_event(&fd, &event_pressure_100);
    send_event(&fd, &event_end);
    usleep(100000);
    send_event(&fd, &event_pressure_0);
    send_event(&fd, &event_release);
    send_event(&fd, &event_end);
    usleep(100000);

    close(fd);
}

int main(int argc, char **argv) {
    int i, parameter = 0;
    if (argc == 3) {
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        printf("touch at %i %i\n", x, y);
        touch_at(x, y);
    }
    else {
        printf("Need x and y coordinates as arguments");
    }
    return 0;
}
