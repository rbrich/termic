Multi-terminal implementation
-----------------------------

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
