Shell
=====

List available user shells:

    cat /etc/shells

Users can set one of these shells as their login shell.
Only root can add a shell to this list or use unlisted shell.

Default shell for each user is recorded in `/etc/passwd` file.
Edit (and view) this information with `chpass` tool.

C API: `getpwent(3)`, eg. `getpwuid(getuid())->pw_shell`
