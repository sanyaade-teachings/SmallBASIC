/**
*	SmallBASIC, main() console versions
*
*	This program is distributed under the terms of the GPL v2.0 or later
*	Download the GNU Public License (GPL) from www.gnu.org
*
*	Nicholas Christopoulos
*/

#include "sbapp.h"
#include "help_subsys.h"
#include "interactive_mode.h"


#if defined(_UnixOS) || defined(_DOS)
// global the filename (its needed for CTRL+C signal - delete temporary)
char	g_file[1025];

// global command-line args
char	**g_argv;
int		g_argc;
						  
/*
*	remove temporary files
*
*	its called by atexit() only if the
*	source file it had been created in /tmp
*/
void	sbunx_remove_temp(void)
{
	unlink(g_file);
}
#endif

/**
*	main()
*/
#if defined(_Win32) 
int	console_main(int argc, char *argv[])
#else
int	main(int argc, char *argv[])
#endif
{
	char	prev_cwd[1025];
	char	cwd[1025], *slash;
	int		i, j, use_stdin = 0, c;
	int		opt_ihavename = 0, opt_nomore = 0;
	FILE	*fp;
  
	strcpy(g_file, "");
	opt_graphics = 0;
	opt_quite = 0;
	opt_ide = 0;
	opt_command[0] = '\0';

	opt_pref_width = opt_pref_height = opt_pref_bpp = 0;

	/*
	*	command-line parameters
	*/
	for ( i = 1; i < argc; i ++ )	{
		if	( argv[i][0] == '-' )	{
			if	( opt_nomore )	{
				if	( strlen(opt_command) )
					strcat(opt_command, " ");
				strcat(opt_command, argv[i]);
				}
			else	{
				switch ( argv[i][1] )	{
				case '-':
					// the following parameters are going to script (COMMAND$)
					opt_nomore = 1;
					break;
				case 's':
					// decompile
					opt_decomp ++;
					break;
				case 'c':
					// syntax check
					opt_syntaxcheck ++;
					break;
				case 'v':
					// verbose check
					opt_verbose ++;
					break;
				case 'i':
					opt_ide = IDE_EXTERNAL;
					break;
				case 'g':
					// run in graphics mode
					opt_graphics = 2;
					if	( (argv[i][2] >= '1') && (argv[i][2] <= '9') )	{
						// setup graphics mode
						slash = &argv[i][2];
						sprintf(cwd, "SBGRAF=%s", slash);
						dev_putenv(cwd);
						comp_preproc_grmode(slash);
						opt_graphics = 2;
						}
					break;
				case 'p':
					if	( strcmp(argv[i]+1, "pkw") == 0 )	{
						printf("SmallBASIC keywords table\n");
						printf("::':#:rem:\"\n");	// ted's format
						printf("$$$-remarks\n");
						printf("'\n");
						printf("REM\n");
						// operators
						printf("$$$-operators\n");
						printf("() \"\"\n");
						printf("%s\n", "+ - * / \\ % ^");
						printf("%s\n", "= <= =< >= => <> != !");
						printf("%s\n", "&& & || | ~");
						for ( j = 0; opr_table[j].name[0] != '\0'; j ++ )	
							printf("%s\n", opr_table[j].name);

						// print keywords
						printf("$$$-keywords\n");
						for ( j = 0; keyword_table[j].name[0] != '\0'; j ++ )	{
							if	( keyword_table[j].name[0] != '$' )
								printf("%s\n", keyword_table[j].name);
							}

						// special separators
						for ( j = 0; spopr_table[j].name[0] != '\0'; j ++ )	
							printf("%s\n", spopr_table[j].name);

						// functions
						printf("$$$-functions\n");
						for ( j = 0; func_table[j].name[0] != '\0'; j ++ )	
							printf("%s\n", func_table[j].name);

						// procedures
						printf("$$$-procedures\n");
						for ( j = 0; proc_table[j].name[0] != '\0'; j ++ )	
							printf("%s\n", proc_table[j].name);

						// external procedures
						// ....
						exit(1);
						}
					break;
				case 'm':
					// load run-time modules (linux only, shared libraries)
					#if defined(__linux__)
					opt_loadmod = 1;
					strcpy(opt_modlist, argv[i]+2);
					#else
					printf("\n\a* Modules are supported only on Linux platform\n\n");
					#endif
					break;
				case 'q':
					// shutup
					opt_quite = 1;
					break;
				case 'h':
					// print command-line parameters
					fprintf(stderr, "SmallBASIC version %s - kw:%d, pc:%d, fc:%d, ae:%d\n", SB_STR_VER, kwNULL, (kwNULLPROC-kwCLS)+1, (kwNULLFUNC-kwASC)+1, (int) (65536 / sizeof(var_t)));
					fprintf(stderr, "http://smallbasic.sourceforge.net\n\n");

					if	( argv[i][2] == '-' || argv[i][2] == 'x' )	{
						/*
						*	search for command, or print all doc
						*/
						char	*command = argv[i]+3;

						if ( argv[i][2] == '-' ) 
							help_printinfo(command);
						else if ( argv[i][2] == 'x' )	{
							// print all
							//printf("%s\n", help_text);
							;
							}
						}
					else {
						/*
						*	Generic help-page
						*/
//						printf("%s\n", about_text);
						printf("usage: sbasic [options] source [--] [program parameters]\n");
						printf("-c		syntax check (compile only)\n");
						printf("-s		decompiler\n");
						printf("-g		enable graphics\n");
						printf("-g[<width>x<height>[x<bpp>]] enable graphics & setup the graphics mode (depented on driver)\n");
						printf("-m[mod1,mod2,...] load all or the specified modules\n");
						printf("-q		quite\n");
						printf("-v		verbose\n");
						printf("-pkw	prints all keywords (useful to create color-syntax macros for editors)\n");
						// -i for ide
/*
						fprintf(stderr, "\ncharset (default: utf8)\n");
						fprintf(stderr, "-j		enable sjis\n");
						fprintf(stderr, "-b		enable big5\n");
						fprintf(stderr, "-m		enable generic multibyte\n");
						fprintf(stderr, "-u		enable unicode!\n");
*/
						printf("-h[-command] help pages\n");
						printf("\nExamples:\n\tsbasic -h | less\n\tsbasic -h-input\n");
						}
					return 255;
				default:
					fprintf(stderr, "unknown option: %s\n", argv[i]);
					return 255;
					};
				}
			}
		else	{
			// no - switch
			// this is the filename or script-parameters
			if	( opt_ihavename == 0 )	{
				strcpy(g_file, argv[i]);
				if	( access(g_file, F_OK) )	{
					strcat(g_file, ".bas");
					if	( access(g_file, F_OK) )		{
						perror("sbasic");
						return 255;
						}
					}
				if	( access(g_file, R_OK) )	{
					perror("sbasic");
					return 255;
					}
				opt_ihavename = 1;
				}
			else	{
				if	( strlen(opt_command) )
					strcat(opt_command, " ");
				strcat(opt_command, argv[i]);
				}
			}
		}

	/*
	*	initialization
	*/

	getcwd(prev_cwd, 1024);
	strcpy(cwd, prev_cwd);

	if	( strlen(g_file) == 0 )	{
		/*
		*	stdin
		*/
		use_stdin ++;
		if ( isatty (STDIN_FILENO) )	{	/*  check if it is a terminal. */
			use_stdin ++;
			opt_interactive = 1;
			}

		#if defined(_Win32) || defined(_DOS)
		slash = strchr(argv[0], OS_DIRSEP);
		if	( slash )	{
			strcpy(g_file, argv[0]);
			slash = strrchr(g_file, OS_DIRSEP);
			*slash = OS_DIRSEP;
			*(slash+1) = '\0';
			strcat(g_file, "sbasic.tmp");
			}
		else
			sprintf(g_file, "sbasic.tmp");
		#else
		sprintf(g_file, "%ctmp%csb%d.bas", OS_DIRSEP, OS_DIRSEP, getpid());
		#endif

		// its a temporary and it must be deleted
		atexit(sbunx_remove_temp);

		if	( opt_interactive )	{
			// get it from console
			memmgr_init();

			interactive_mode(g_file);
			}
		else	{
			// get it from stdin
			fp = fopen(g_file, "wb");
			if	( fp )	{
				while ( (c = fgetc(stdin)) != -1 )	
					fputc(c, fp);
				fclose(fp);
				}
			else
				perror("sbasic");
			}
		}
	else	{
		/*
		*	file
		*/
		if	( !opt_quite )	
			printf("SmallBASIC version %s, use -h for help\n", SB_STR_VER);

		if	( g_file[0] == OS_DIRSEP )
			cwd[0] = '\0';
		#if defined(_Win32) || defined(_DOS)
		if	( strlen(g_file) > 2 )	{
			if ( g_file[1] == ':' )	
				cwd[0] = '\0';
			}
		#endif
		slash = strrchr(g_file, OS_DIRSEP);
		if	( slash )	{
			char	tmp[1024];
			char	sep[2];
			char	*final_dir;

			*slash = '\0';
			
			sep[0] = OS_DIRSEP;
			sep[1] = '\0';
			strcat(cwd, sep);
			strcat(cwd, g_file);
			strcpy(tmp, slash+1);
			strcpy(g_file, tmp);

//			printf("Current directory changed to [%s]\n", cwd);
//			printf("Source file changed to: [%s]\n", g_file);
			#if defined(_CygWin)
			if	( strncmp(cwd, "//", 2) == 0 )
				final_dir = cwd + 1;
			else
			#endif
				final_dir = cwd;
			if	( chdir(final_dir) != 0 )	{
				printf("Can't change directory to '%s'! (cygwin?)\n", final_dir);
				exit(1);
				}
			}
		}

	/*
	*	run it
	*/
	if	( !opt_interactive )	{
		memmgr_init();
		sbasic_main(g_file);
		}

	/*
	*	cleanup
	*/
	if	( use_stdin )
		remove(g_file);
	chdir(prev_cwd);

	if	( gsb_last_error )
		return 1;
	return opt_retval;
}


