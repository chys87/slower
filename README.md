slower
======

This utility helps you run a program on Linux with the clock
running more slowly or quickly than usual.


Usage
=====

Build:

    cd slower
    make

You should get file `libslower.so`.

Run a program (take [DOSBox](http://www.dosbox.com/) as the example):

    SLOWER_FACTOR=2 LD_PRELOAD=/path/to/libslower.so dosbox

Then have fun.
You can adjust the value of `SLOWER_FACTOR` to achieve different ratios.
Values between `0.01` and `100` (inclusive) are accepted.
The larger the value, the slower the clock (as seen by the deceived application) runs.

You can also `make install` and use `slower.sh` instead:

    /path/to/slower.sh -f 2 dosbox


HOW IT WORKS & TODO
===================

This program hacks glibc time functions to deceive applications of the real time.

Currently hacked interfaces:

 * [`clock_gettime`](http://linux.die.net/man/2/clock_gettime)
 * [`time`](http://linux.die.net/man/2/time)
 * [`gettimeofday`](http://linux.die.net/man/2/gettimeofday)

TODO:

 * Sleep functions:
   [`sleep`](http://linux.die.net/man/2/sleep),
   [`usleep`](http://linux.die.net/man/2/usleep),
   [`nanosleep`](http://linux.die.net/man/2/nanosleep),
   [`clock_nanosleep`](http://linux.die.net/man/2/clock_nanosleep)
 * File descriptor polling functions:
   [`select`](http://linux.die.net/man/2/select),
   [`pselect`](http://linux.die.net/man/2/select),
   [`poll`](http://linux.die.net/man/2/poll),
   [`ppoll`](http://linux.die.net/man/2/poll),
   [`epoll_wait`](http://linux.die.net/man/2/epoll_wait),
   [`epoll_pwait`](http://linux.die.net/man/2/epoll_wait)
 * Timer functions:
   TBA
 * Timeout related:
   TBA
 * [`syscall`](http://linux.die.net/man/2/syscall)
 * Others?
