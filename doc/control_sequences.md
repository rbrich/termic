Termic Control Sequences
========================

This is documentation of control sequences recognized by Termic.

`SGR 38 ; 2 ; <r> ; <g> ; <b> m`\
set foreground [24-bit RGB color][truecolor] (each element 0..255)

`SGR 38 ; 5 ; <index> m`\
set foreground indexed color (index 0..255)

References:
* [ANSI escape code][ansi]
* [ECMA-48][ecma-48]
* [terminfo(5)][terminfo]
* [vttest][vttest] - tests compatibility with VT100 terminal

... TODO ...

[ansi]: https://en.wikipedia.org/wiki/ANSI_escape_code
[c0c1]: https://en.wikipedia.org/wiki/C0_and_C1_control_codes
[vt510]: https://vt100.net/docs/vt510-rm/contents.html
[xterm]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
[dtterm]: http://www2.phys.canterbury.ac.nz/dept/docs/manuals/unix/DEC_4.0e_Docs/HTML/MAN/MAN5/0036____.HTM
[screen]: https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html
[vttest]: http://invisible-island.net/vttest/vttest.html
[ecma-48]: https://www.ecma-international.org/wp-content/uploads/ECMA-48_5th_edition_june_1991.pdf
[itu-t416]: https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-T.416-199303-I!!PDF-E&type=items
[truecolor]: https://gist.github.com/XVilka/8346728
[windows]: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
[terminfo]: https://linux.die.net/man/5/terminfo
