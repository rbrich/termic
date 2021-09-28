Development
===========

## Multi-terminal implementation

N (Shell + Pty)
- N PTY devices, N Shell processes

1 (EventLoop + Thread)
- PTYs registered in loop for Read/Write

N (Terminal)
- one widget per shell
- one foreground terminal, others inactive
- events have to be processed for all terminals

Input events flow from shell through PTY, EventLoop (Read event),
read operation, into Terminal (decode_input).


## Bracketed paste mode

- https://cirw.in/blog/bracketed-paste

## CSI u

- [iTerm2 doc](https://iterm2.com/documentation-csiu.html)
- [libtickit](http://www.leonerd.org.uk/code/libtickit/)


## Other terminals

- [xterm](https://invisible-island.net/xterm/) - emulates almost everything
- [Terminology](https://github.com/billiob/terminology) - fancy features
- [Alacritty](https://github.com/alacritty/alacritty) - it's fast
