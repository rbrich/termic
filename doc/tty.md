TTY Subsystem
=============

Window Resize
-------------

Current window size is kept in kernel. Any program can write or read it
via `TIOCSWINSZ`, setting or getting `struct winsize` with the sizes.
Terminal emulator program is expected to set new sizes whenever
window size changes. Foreground program is then signalled with `SIGWINCH`
(automatically). It may read the new sizes and act on them.

To test this in a terminal emulator, trap WINCH in Bash
and watch the size changes:

    trap 'echo size `stty size`' WINCH


References
----------

* [The TTY demystified][tty-demystified]
* [POSIX terminal interface][posix]
* [termios(4)][termios]

[tty-demystified]: http://linusakesson.net/programming/tty/
[posix]: https://en.wikipedia.org/wiki/POSIX_terminal_interface
[termios]: https://linux.die.net/man/4/termios
