char *help_page_i=
">> Console\n"
"_______________________________________________________________________\n"
"\n"
"Supported console codes\n"
"Note: \\e = CHR(27)\n"
"\n"
"\\t		    tab (32 pixels)\n"
"\\a		    beep\n"
"\\r\\n	    new line (cr/lf)\n"
"\\xC		    clear screen\n"
"\\e[K	    clear to EOL\n"
"\\e[nG	    moves cursor to specified column\n"
"\\e[0m	    reset all attributes to their defaults\n"
"\\e[1m	    set bold on\n"
"\\e[4m	    set underline on\n"
"\\e[7m	    reverse video\n"
"\\e[21m	    set bold off\n"
"\\e[24m	    set underline off\n"
"\\e[27m	    set reverse off\n"
"\\e[3nm      set foreground\n"
"            color. where n:\n"
"\n"
"            0 black\n"
"            1 red\n"
"            2 green\n"
"            3 brown\n"
"            4 blue\n"
"            5 magenta\n"
"            6 cyan\n"
"            7 white\n"
"\n"
"\\e[4nm      set background color.\n"
"            (see set foreground)\n"
"\n"
"PalmOS only:\n"
"\n"
"\\e[8nm	    (n=0..7) select system font\n"
"\\e[9nm	    (n=0..3) select buildin font\n"
"\n"
"eBookMan only:\n"
"\\e[50m      select 9pt font\n"
"\\e[51m      select 12pt font\n"
"\\e[52m      select 16pt font\n"
"\\e[nT       move to n/80th screen character position\n"
"\n"
"_______________________________________________________________________\n"
"\n"
"${key:print}\n"
"PRINT [USING [format];] [expr|str [{,|;} [expr|str]] ...\n"
"\n"
"Displays a text or the value of an expression.\n"
"\n"
"\n"
"PRINT SEPARATORS\n"
"________________\n"
"\n"
"TAB(n)  Moves cursor position to the nth column.\n"
"SPC(n)  Prints a number of spaces specified by n.\n"
";       Carriage return/line feed suppressed after printing.\n"
",       Carriage return/line feed suppressed after printing.\n"
"        A TAB character is placed.\n"
"\n"
"\n"
"The PRINT USING\n"
"_______________\n"
"\n"
"Print USING, is using the FORMAT() to display numbers and strings.\n"
"Unlike the FORMAT, this one can include literals, too.\n"
"\n"
"_       Print next character as a literal. The combination _#, for \n"
"        example, allows you to include a number sign as a literal \n"
"        in your numeric format.\n"
"\n"
"[other] Characters other than the foregoing may be included as\n"
"        literals in the format string. \n"
"\n"
"Notes:\n"
"    * When a PRINT USING command is executed, the format will remains\n"
"      on the memory until a new format is passed. \n"
"      Calling a PRINT USING without a new format specified, the PRINT\n"
"      will use the format of previous call.\n"
"\n"
"Examples:\n"
"\n"
"PRINT USING \"##: #,###,##0.00\";\n"
"FOR i=0 TO 20\n"
"    PRINT USING; i+1, A(i)\n"
"NEXT\n"
"\n"
"PRINT USING \"Total ###,##0 of \\ \\\"; number, \"bytes\"\n"
"\n"
"Notes:\n"
"The symbol ? can be used instead of keyword PRINT\n"
"You can use 'USG' instead of 'USING'\n"
"_______________________________________________________________________\n"
"\n"
"${key:cat}\n"
"CAT(x)\n"
"\n"
"Returns console codes\n"
"\n"
"0  - reset\n"
"1  - bold on\n"
"-1 - bold off\n"
"2  - underline on\n"
"-2 - underline off\n"
"3  - reverse on\n"
"-3 - reverse off\n"
"\n"
"PalmOS only:\n"
"80..87 - select system font\n"
"90..93 - select custom font\n"
"\n"
"Example:\n"
"? cat(1);\"Bold\";cat(0)\n"
"_______________________________________________________________________\n"
"\n"
"${key:input}\n"
"INPUT [prompt {,|;}] var[, var [, ...]]\n"
"\n"
"Reads from \"keyboard\" a text and store it to variable.\n"
"_______________________________________________________________________\n"
"\n"
"LINE INPUT var\n"
"or\n"
"LINEINPUT var\n"
"\n"
"Reads a whole text line from console.\n"
"_______________________________________________________________________\n"
"\n"
"INPUT(len[,fileN])\n"
"\n"
"This function is similar to INPUT.\n"
"\n"
"This function is a low-level function. That means does not convert the\n"
"data, and does not remove the spaces.\n"
"_______________________________________________________________________\n"
"\n"
"${key:inkey}\n"
"INKEY\n"
"\n"
"This function returns the last key-code in keyboard buffer, or\n"
"an empty string if there are no keys.\n"
"\n"
"Special key-codes like the function-keys (PC) or the hardware-buttons\n"
"(PalmOS) are returned as 2-byte string.\n"
"\n"
"Example:\n"
"\n"
"k=INKEY\n"
"IF LEN(k)\n"
"  IF LEN(k)=2\n"
"    ? \"H/W #\"+ASC(RIGHT(k,1))\n"
"  ELSE\n"
"    ? k; \" \"; ASC(k)\n"
"  FI\n"
"ELSE\n"
"  ? \"keyboard buffer is empty\"\n"
"FI\n"
"_______________________________________________________________________\n"
"\n"
"${key:cls}\n"
"CLS\n"
"\n"
"Clears the screen.\n"
"_______________________________________________________________________\n"
"\n"
"${key:at}\n"
"AT x,y (in pixels)\n"
"\n"
"Moves the console cursor to the specified position.\n"
"\n"
"x,y are in pixels\n"
"_______________________________________________________________________\n"
"\n"
"${key:locate}\n"
"LOCATE y,x\n"
"\n"
"Moves the console cursor to the specified position.\n"
"\n"
"x,y are in character cells.\n"
"_______________________________________________________________________\n"
;