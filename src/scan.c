/**
*	SmallBASIC, pseudo-compiler: Converts the source to byte-code.
*
*	2000-05-27, Nicholas Christopoulos
*
*	This program is distributed under the terms of the GPL v2.0 or later
*	Download the GNU Public License (GPL) from www.gnu.org
*/

#define SCAN_MODULE
#include "sys.h"
#include "device.h"
#include "kw.h"
#include "bc.h"
#include "scan.h"
#include "smbas.h"
#include "units.h"
#include "extlib.h"

#if defined(_UnixOS)
#include <assert.h>
#endif

#if defined(_WinBCB)
#define	strcasecmp	stricmp
#endif

void	err_wrongproc(const char *name)		SEC(BCSC2);
void	err_wrongproc(const char *name)		{	sc_raise("Wrong procedure/function name: %s", name); }
void	err_comp_missing_q()				SEC(BCSC2);
void	err_comp_missing_q()				{	sc_raise("Expression: Missing (\"), string remains open");	}
void	err_comp_missing_rp()				SEC(BCSC2);
void	err_comp_missing_rp()				{	sc_raise("Expression: Missing \')\'");	}
void	err_comp_missing_lp()				SEC(BCSC2);
void	err_comp_missing_lp()				{	sc_raise("Expression: Missing \'(\'");	}
void	err_comp_label_not_def(const char *name)	SEC(BCSC2);
void	err_comp_label_not_def(const char *name)
											{	sc_raise("Label '%s' is not defined", name); }
void	err_comp_unknown_unit(const char *name)		SEC(BCSC2);
void	err_comp_unknown_unit(const char *name)
											{	sc_raise("Unknown unit '%s'; use IMPORT\n", name); }

void	comp_text_line(char *text) 					SEC(BCSC3);
int		comp_single_line_if(char *text) 				SEC(BCSC3);
addr_t	comp_search_bc(addr_t ip, code_t code) 			SEC(BCSC3);

extern void	expr_parser(bc_t *bc)			 	SEC(BCSC3);

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	GENERIC KEYWORDS (basic bc-types & oldest code)
*
*	This table is limited to 256 elements
*/
struct keyword_s keyword_table[] = {
/* real commands */
{ "LOCAL", 		kwLOCAL	},
{ "SUB", 		kwPROC	},
{ "FUNC", 		kwFUNC	},
{ "DEF", 		kwFUNC	},
{ "BYREF",		kwBYREF	},
{ "DECLARE",	kwDECLARE	},
{ "IMPORT",		kwIMPORT	},
{ "EXPORT",		kwEXPORT	},
{ "UNIT",		kwUNIT	},

{ "LET", 		kwLET	},
{ "CONST", 		kwCONST	},
{ "DIM", 		kwDIM	},
{ "REDIM", 		kwREDIM	},
{ "STOP",		kwSTOP	},
{ "END",		kwEND	},
{ "PRINT",		kwPRINT	},
{ "SPRINT",		kwSPRINT },
{ "INPUT",		kwINPUT	},
{ "SINPUT",		kwSINPUT	},
{ "REM",		kwREM	},
{ "CHAIN",		kwCHAIN	},
{ "ON",			kwON	},

{ "LABEL",		kwLABEL	},
{ "GOTO",		kwGOTO	},
{ "IF",			kwIF	},
{ "THEN",		kwTHEN	},
{ "ELSE",		kwELSE	},
{ "ELIF",		kwELIF	},
{ "ELSEIF",		kwELIF	},
{ "ENDIF",		kwENDIF	},
{ "FI",			kwENDIF	},
{ "FOR",		kwFOR	},
{ "TO",			kwTO	},
{ "STEP",		kwSTEP	},
{ "NEXT",		kwNEXT	},
{ "WHILE",		kwWHILE	},
{ "WEND",		kwWEND	},
{ "REPEAT",		kwREPEAT	},
{ "UNTIL",		kwUNTIL	},
{ "GOSUB",		kwGOSUB	},
{ "RETURN",		kwRETURN	},
{ "READ",		kwREAD	},
{ "DATA",		kwDATA	},
{ "RESTORE",	kwRESTORE	},
{ "EXIT",		kwEXIT	},

{ "ERASE", 		kwERASE	},
{ "USE",		kwUSE	},
{ "USING",		kwUSING		},
{ "USG",		kwUSING		},

{ "LINE",		kwLINE	},
{ "COLOR",		kwCOLOR	},

{ "RUN",		kwRUN	},
{ "EXEC",		kwEXEC	},

{ "OPEN", 		kwOPEN	},
{ "APPEND",		kwAPPEND	},
{ "AS",			kwAS },		// OPEN's args
{ "CLOSE", 		kwCLOSE  },
{ "LINEINPUT", 	kwLINEINPUT },		// The QB's keyword is "LINE INPUT"
{ "LINPUT", 	kwLINEINPUT },		// The QB's keyword is "LINE INPUT"
{ "SEEK", 		kwSEEK	},
{ "WRITE", 		kwFILEWRITE	},
//{ "READ", 	kwFILEREAD	},
//{ "INPUT", 		kwFINPUT },	// not needed

{ "INSERT",		kwINSERT },
{ "DELETE",		kwDELETE },

/* DEBUG */
{ "TRON",		kwTRON	},
{ "TROFF",		kwTROFF	},
{ "OPTION",		kwOPTION },

{ "BG",		kwBACKG },

/* for debug */
/* by using small letters, */
/* the keywords are invisible by compiler */
{ "$i32", 		kwTYPE_INT		},
{ "$r64",		kwTYPE_NUM		},
{ "$str",		kwTYPE_STR		},
{ "$log",		kwTYPE_LOGOPR	},
{ "$cmp",		kwTYPE_CMPOPR	},
{ "$add",		kwTYPE_ADDOPR	},
{ "$mul",		kwTYPE_MULOPR	},
{ "$pow",		kwTYPE_POWOPR	},
{ "$unr",		kwTYPE_UNROPR	},
{ "$var",		kwTYPE_VAR	},
{ "$tln",		kwTYPE_LINE	},
{ "$lpr",		kwTYPE_LEVEL_BEGIN	},
{ "$rpr",		kwTYPE_LEVEL_END	},
{ "$crv",		kwTYPE_CRVAR	},
{ "$sep",		kwTYPE_SEP		},
{ "$biF",		kwTYPE_CALLF	},
{ "$biP",		kwTYPE_CALLP	},
{ "$exF",		kwTYPE_CALLEXTF	},
{ "$exP",		kwTYPE_CALLEXTP	},
{ "$ret",		kwTYPE_RET	},
{ "$udp",		kwTYPE_CALL_UDP	},
{ "$udf",		kwTYPE_CALL_UDF	},

{ "", 0 }
};

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	OPERATORS (not the symbols)
*/
struct opr_keyword_s opr_table[] = {
{ "AND",   kwTYPE_LOGOPR, '&' 		 },
{ "OR",    kwTYPE_LOGOPR, '|' 		 },
{ "BAND",  kwTYPE_LOGOPR, OPLOG_BAND },
{ "BOR",   kwTYPE_LOGOPR, OPLOG_BOR	 },
{ "XOR",   kwTYPE_LOGOPR, '~'        },
{ "NOT",   kwTYPE_UNROPR, '!'        },
{ "MOD",   kwTYPE_MULOPR, OPLOG_MOD	 },
{ "MDL",   kwTYPE_MULOPR, OPLOG_MDL	 },
{ "EQV",   kwTYPE_LOGOPR, OPLOG_EQV	 },
{ "IMP",   kwTYPE_LOGOPR, OPLOG_IMP	 },
{ "NAND",  kwTYPE_LOGOPR, OPLOG_NAND },
{ "NOR",   kwTYPE_LOGOPR, OPLOG_NOR	 },
{ "XNOR",  kwTYPE_LOGOPR, OPLOG_NOR	 },
{ "IN",    kwTYPE_CMPOPR, OPLOG_IN	 },
{ "LIKE",  kwTYPE_CMPOPR, OPLOG_LIKE },

{ "", 0, 0 }
};

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	SPECIAL SEPERATORS
*/
struct spopr_keyword_s spopr_table[] = {
{ "COLOR",		kwCOLOR 	},
{ "FILLED",		kwFILLED	},
{ "FOR",		kwFORSEP	},
{ "INPUT",		kwINPUTSEP	},
{ "OUTPUT",		kwOUTPUTSEP	},
{ "APPEND",		kwAPPENDSEP	},
{ "ACCESS",		kwACCESS	},
{ "USING",		kwUSING		},
{ "USG",		kwUSING		},
{ "SHARED",		kwSHARED	},
{ "AS",			kwAS		},
{ "TO",			kwTO		},
{ "DO",			kwDO		},
{ "STEP",		kwSTEP		},
{ "THEN",		kwTHEN		},
{ "SUB", 		kwPROCSEP	},
{ "FUNC", 		kwFUNCSEP	},
{ "DEF", 		kwFUNCSEP	},
{ "LOOP", 		kwLOOPSEP	},
{ "ON",			kwON		},
{ "OFF",		kwOFF		},
{ "USE",		kwUSE		},
{ "BG",			kwBACKG 	},

{ "", 0 }
};

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	BUILDIN-FUNCTIONS
*/
struct func_keyword_s func_table[] = {
///1234567890123456
{ "ASC",			kwASC },
{ "VAL",			kwVAL },
{ "CHR",			kwCHR },
{ "STR",			kwSTR },
{ "OCT",			kwOCT },
{ "HEX",			kwHEX },
{ "LCASE",			kwLCASE },
{ "LOWER",			kwLCASE },
{ "UCASE",			kwUCASE },
{ "UPPER",			kwUCASE },
{ "LTRIM",			kwLTRIM },
{ "RTRIM",			kwRTRIM },
{ "SPACE",			kwSPACE }, 
{ "SPC",			kwSPACE }, 
{ "TAB",			kwTAB },
{ "CAT",			kwCAT },
{ "ENVIRON",		kwENVIRONF },
{ "ENV",			kwENVIRONF },
{ "TRIM",			kwTRIM },
{ "STRING",			kwSTRING },
{ "SQUEEZE",		kwSQUEEZE },
{ "LEFT",			kwLEFT },
{ "RIGHT",			kwRIGHT },
{ "LEFTOF",			kwLEFTOF },
{ "RIGHTOF",		kwRIGHTOF },
{ "LEFTOFLAST",		kwLEFTOFLAST },
{ "RIGHTOFLAST",	kwRIGHTOFLAST },
{ "MID",			kwMID },
{ "REPLACE",		kwREPLACE },
{ "RUN",			kwRUNF },
{ "INKEY",			kwINKEY },
{ "TIME",			kwTIME },
{ "DATE",			kwDATE },
{ "INSTR",			kwINSTR },
{ "RINSTR",			kwINSTR },
{ "LBOUND",			kwLBOUND },
{ "UBOUND",			kwUBOUND },
{ "LEN",			kwLEN },
{ "EMPTY",			kwEMPTY },
{ "ISARRAY",		kwISARRAY },
{ "ISNUMBER",		kwISNUMBER },
{ "ISSTRING",		kwISSTRING },
{ "ATAN2",			kwATAN2 },
{ "POW",			kwPOW },
{ "ROUND",			kwROUND },
{ "COS",			kwCOS },
{ "SIN",			kwSIN },
{ "TAN",			kwTAN },
{ "COSH",			kwCOSH },
{ "SINH",			kwSINH },
{ "TANH",			kwTANH },
{ "ACOS",			kwACOS },
{ "ASIN",			kwASIN },
{ "ATAN",			kwATAN },
{ "ATN",			kwATAN },
{ "ACOSH",			kwACOSH },
{ "ASINH",	   		kwASINH },
{ "ATANH",			kwATANH },

{ "SEC",			kwSEC },
{ "ASEC",			kwASEC },
{ "SECH",			kwSECH },
{ "ASECH",			kwASECH },

{ "CSC",			kwCSC },
{ "ACSC",			kwACSC },
{ "CSCH",			kwCSCH },
{ "ACSCH",			kwACSCH },

{ "COT",			kwCOT },
{ "ACOT",			kwACOT },
{ "COTH",			kwCOTH },
{ "ACOTH",			kwACOTH },

{ "SQR",	   		kwSQR },
{ "ABS",			kwABS },
{ "EXP",			kwEXP },
{ "LOG",			kwLOG },
{ "LOG10",			kwLOG10 },
{ "FIX",			kwFIX },
{ "INT",			kwINT },
{ "CDBL",			kwCDBL },
{ "CREAL",			kwCDBL },
{ "DEG",			kwDEG },
{ "RAD",			kwRAD },
{ "PEN",			kwPENF },
{ "FLOOR",			kwFLOOR },
{ "CEIL",			kwCEIL },
{ "FRAC",			kwFRAC },
{ "FRE",			kwFRE },
{ "SGN",			kwSGN },
{ "CINT",			kwCINT },
{ "EOF",			kwEOF },
{ "SEEK",			kwSEEKF },
{ "LOF",			kwLOF },
{ "RND",			kwRND },
{ "MAX",			kwMAX },
{ "MIN",			kwMIN },
{ "ABSMAX",			kwABSMAX },
{ "ABSMIN",			kwABSMIN },
{ "SUM",			kwSUM },
{ "SUMSQ",			kwSUMSV },
{ "STATMEAN",		kwSTATMEAN },
{ "STATMEANDEV",	kwSTATMEANDEV },
{ "STATSPREADS",	kwSTATSPREADS },
{ "STATSPREADP",	kwSTATSPREADP },
{ "SEGCOS",			kwSEGCOS },
{ "SEGSIN",			kwSEGSIN },
{ "SEGLEN",			kwSEGLEN },
{ "POLYAREA",		kwPOLYAREA },
{ "POLYCENT",		kwPOLYCENT },
{ "PTDISTSEG",		kwPTDISTSEG },
{ "PTSIGN",	   		kwPTSIGN },
{ "PTDISTLN",		kwPTDISTLN },
{ "POINT",			kwPOINT },
{ "XPOS",			kwXPOS },
{ "YPOS",			kwYPOS },
{ "INPUT",			kwINPUTF },
{ "ARRAY",			kwCODEARRAY }, 
{ "LINEQN",			kwGAUSSJORDAN },
{ "FILES",			kwFILES },
{ "INVERSE",		kwINVERSE },
{ "DETERM",			kwDETERM },
{ "JULIAN",			kwJULIAN },
{ "DATEFMT",		kwDATEFMT },
{ "WEEKDAY",		kwWDAY },
{ "IF",				kwIFF },
{ "IFF",			kwIFF },
{ "FORMAT",			kwFORMAT },
{ "FREEFILE",		kwFREEFILE },
{ "TICKS",			kwTICKS },
{ "TICKSPERSEC",	kwTICKSPERSEC },
{ "TIMER", 			kwTIMER }, 
{ "PROGLINE",		kwPROGLINE },
{ "RUN",			kwRUNF	},
{ "TXTW",			kwTEXTWIDTH },
{ "TXTH",			kwTEXTHEIGHT },
{ "TEXTWIDTH", 		kwTEXTWIDTH },
{ "TEXTHEIGHT",		kwTEXTHEIGHT },
{ "EXIST",			kwEXIST },
{ "ISFILE",			kwISFILE },
{ "ISDIR",			kwISDIR },
{ "ISLINK",			kwISLINK },
{ "ACCESS",			kwACCESSF },
{ "RGB",			kwRGB },
{ "RGBF",			kwRGBF },
{ "BIN",			kwBIN },
{ "ENCLOSE",		kwENCLOSE },
{ "DISCLOSE",		kwDISCLOSE },
{ "TRANSLATE",		kwTRANSLATEF },
{ "CHOP",			kwCHOP },
{ "BGETC",			kwBGETC },
{ "BALLOC",			kwBALLOC },
{ "MALLOC",			kwBALLOC },
{ "PEEK32",			kwPEEK32 },
{ "PEEK16",			kwPEEK16 },
{ "PEEK",			kwPEEK },
{ "VADR",			kwVADDR },

{ "SEQ",			kwSEQ },

{ "CBS",			kwCBS },
{ "BCS",			kwBCS },

{ "LOADLIB",		kwLOADLIB },
{ "CALL",			kwCALLCF },

{ "", 0 }
};

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	BUILD-IN PROCEDURES
*/
struct proc_keyword_s proc_table[] = {
///1234567890123456
{ "CLS",		kwCLS	},
{ "RTE",		kwRTE	},
//kwSHELL,
{ "ENVIRON",	kwENVIRON },
{ "ENV",		kwENVIRON },
{ "LOCATE",		kwLOCATE	},
{ "AT",			kwAT		},
{ "PEN",		kwPEN	},
{ "DATEDMY",	kwDATEDMY },
{ "BEEP",		kwBEEP	},
{ "SOUND",		kwSOUND	},
{ "NOSOUND",	kwNOSOUND	},
{ "PSET",		kwPSET	},
{ "RECT",		kwRECT	},
{ "CIRCLE",		kwCIRCLE	},
{ "RANDOMIZE",	kwRANDOMIZE	},
{ "SPLIT",		kwSPLIT },
{ "WSPLIT",		kwWSPLIT },
{ "JOIN",		kwWJOIN	},
{ "PAUSE",		kwPAUSE	},
{ "DELAY",		kwDELAY	},
{ "ARC",		kwARC	},
{ "DRAW",		kwDRAW	},
{ "PAINT",		kwPAINT	},
{ "PLAY",		kwPLAY	},
{ "SORT",		kwSORT	},
{ "SEARCH",		kwSEARCH },
{ "ROOT",		kwROOT },
{ "DIFFEQN",	kwDIFFEQ },
{ "CHART",		kwCHART	},
{ "WINDOW",		kwWINDOW },
{ "VIEW",		kwVIEW },
{ "DRAWPOLY",	kwDRAWPOLY },
{ "M3IDENT",	kwM3IDENT },
{ "M3ROTATE",	kwM3ROTATE },
{ "M3SCALE", 	kwM3SCALE },
{ "M3TRANS",	kwM3TRANSLATE },
{ "M3APPLY",	kwM3APPLY },
{ "INTERSECT",	kwSEGINTERSECT },
{ "POLYEXT",	kwPOLYEXT },
{ "DERIV",		kwDERIV },
{ "KILL", 		kwKILL	 },
{ "RENAME", 	kwRENAME	},
{ "COPY", 		kwCOPY	},
{ "CHDIR", 		kwCHDIR	},
{ "MKDIR", 		kwMKDIR	},
{ "RMDIR", 		kwRMDIR	},
{ "TLOAD",	 	kwLOADLN },
{ "TSAVE",		kwSAVELN },
{ "LOCK",		kwFLOCK },
{ "CHMOD",		kwCHMOD },
{ "PLOT2",		kwPLOT2 },
{ "PLOT",		kwPLOT },
{ "LOGPRINT",	kwLOGPRINT	},
{ "SWAP",		kwSWAP	},
{ "BUTTON",		kwBUTTON	},
{ "TEXT",		kwTEXT	},
{ "DOFORM",		kwDOFORM	},
{ "DIRWALK",	kwDIRWALK	},
{ "BPUTC",		kwBPUTC	},
{ "POKE32",		kwPOKE32 },
{ "POKE16",		kwPOKE16 },
{ "POKE",		kwPOKE },
{ "BCOPY",		kwBCOPY },
{ "BLOAD",		kwBLOAD },
{ "BSAVE",		kwBSAVE },
{ "USRCALL",	kwCALLADR },
{ "IMGGET",		kwIMGGET },
{ "IMGPUT",		kwIMGPUT },
{ "TIMEHMS",	kwTIMEHMS },
{ "EXPRSEQ",	kwEXPRSEQ },
{ "UNLOADLIB",	kwUNLOADLIB },
{ "CALL",		kwCALLCP },
#if !defined(OS_LIMITED)
{ "STKDUMP",	kwSTKDUMP	},
#endif

{ "", 0 }
};

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	reset the external proc/func lists
*/
void	comp_reset_externals(void)
{
	// reset functions
	if	( comp_extfunctable )
		tmp_free(comp_extfunctable);
	comp_extfunctable = NULL;
	comp_extfunccount = comp_extfuncsize = 0;

	// reset procedures
	if	( comp_extproctable )
		tmp_free(comp_extproctable);
	comp_extproctable = NULL;
	comp_extproccount = comp_extprocsize = 0;
}

/*
*	add an external procedure to the list
*/
int		comp_add_external_proc(const char *proc_name, int lib_id)
{
	// TODO: scan for conflicts

	if	( comp_extproctable == NULL )	{
		comp_extprocsize   = 16;
		comp_extproctable  = (ext_proc_node_t*) tmp_alloc(sizeof(ext_proc_node_t) * comp_extprocsize);
		}
	else if ( comp_extprocsize <= (comp_extproccount+1) )	{
		comp_extprocsize  += 16;
		comp_extproctable  = (ext_proc_node_t*) tmp_realloc(comp_extproctable, sizeof(ext_proc_node_t) * comp_extprocsize);
		}

	comp_extproctable[comp_extproccount].lib_id = lib_id;
	comp_extproctable[comp_extproccount].symbol_index = comp_impcount;
	strcpy(comp_extproctable[comp_extproccount].name, proc_name);
	strupr(comp_extproctable[comp_extproccount].name);

	// update imports table
	{
	bc_symbol_rec_t		sym;

	strcpy(sym.symbol, proc_name);		// symbol name
	sym.type   = stt_procedure;			// symbol type
	sym.lib_id = lib_id;				// library id
	sym.sym_id = comp_impcount;			// symbol  index

	// store it
	dbt_write(comp_imptable, comp_impcount, &sym, sizeof(bc_symbol_rec_t));
	comp_impcount ++;
	}

	comp_extproccount ++;
	return comp_extproccount-1;
}

/*
*	Add an external function to the list
*/
int		comp_add_external_func(const char *func_name, int lib_id)
{
	// TODO: scan for conflicts
	if	( comp_extfunctable == NULL )	{
		comp_extfuncsize   = 16;
		comp_extfunctable  = (ext_func_node_t*) tmp_alloc(sizeof(ext_func_node_t) * comp_extfuncsize);
		}
	else if ( comp_extfuncsize <= (comp_extfunccount+1) )	{
		comp_extfuncsize  += 16;
		comp_extfunctable  = (ext_func_node_t*) tmp_realloc(comp_extfunctable, sizeof(ext_func_node_t) * comp_extfuncsize);
		}

	comp_extfunctable[comp_extfunccount].lib_id = lib_id;
	comp_extfunctable[comp_extfunccount].symbol_index = comp_impcount;
	strcpy(comp_extfunctable[comp_extfunccount].name, func_name);
	strupr(comp_extfunctable[comp_extfunccount].name);

	// update imports table
	{
	bc_symbol_rec_t		sym;

	strcpy(sym.symbol, func_name);		// symbol name
	sym.type   = stt_function;			// symbol type
	sym.lib_id = lib_id;				// library id
	sym.sym_id = comp_impcount;			// symbol  index

	// store it
	dbt_write(comp_imptable, comp_impcount, &sym, sizeof(bc_symbol_rec_t));
	comp_impcount ++;
	}

	comp_extfunccount ++;
	return comp_extfunccount-1;
}

/*
*	returns the external procedure id
*/
int		comp_is_external_proc(const char *name)
{
	int		i;

	for ( i = 0; i < comp_extproccount; i ++ )	{
		if	( strcmp(comp_extproctable[i].name, name) == 0 )
			return i;
		}
	return -1;
}

/*
*	returns the external function id
*/
int		comp_is_external_func(const char *name)
{
	int		i;

	for ( i = 0; i < comp_extfunccount; i ++ )	{
		if	( strcmp(comp_extfunctable[i].name, name) == 0 )
			return i;
		}
	return -1;
}

/* ----------------------------------------------------------------------------------------------------------------------- */

/*
*	Notes:
*		block_level = the depth of nested block
*		block_id	= unique number of each block (based on stack use)
*
*		Example:
*		? xxx			' level 0, id 0
*		for i=1 to 20	' level 1, id 1
*			? yyy		' level 1, id 1
*			if a=1 		' level 2, id 2 (our IF uses stack)
*				...		' level 2, id 2
*			else		' level 2, id 2	// not 3
*				...		' level 2, id 2
*			fi			' level 2, id 2
*			if a=2		' level 2, id 3
*				...		' level 2, id 3
*			fi			' level 2, id 3
*			? zzz		' level 1, id 1
*		next			' level 1, id 1
*		? ooo			' level 0, id 0
*/

#define	GROWSIZE	128

extern void	sc_raise2(const char *fmt, int line, const char *buff);	// sberr

/*
*	error messages
*/
void	sc_raise(const char *fmt, ...)
{
	char	*buff;
	va_list ap;

	va_start(ap, fmt);

	comp_error = 1;

	buff = tmp_alloc(SB_SOURCELINE_SIZE+1);
	#if defined(_PalmOS)
	StrVPrintF(buff, fmt, ap);
	#elif defined(_DOS)
	vsprintf(buff, fmt, ap);
	#else
	vsnprintf(buff, SB_SOURCELINE_SIZE, fmt, ap);
	#endif
	va_end(ap);

	sc_raise2(comp_bc_sec, comp_line, buff);	// sberr.h
	tmp_free(buff);
}

/*
*	prepare name (keywords, variables, labels, proc/func names)
*/
char*	comp_prepare_name(char *dest, const char *source, int size) SEC(BCSC3);
char*	comp_prepare_name(char *dest, const char *source, int size)
{
	char	*p = (char *) source;

	while ( *p == ' ' )		p ++;
	strncpy(dest, p, size);
	dest[size] = '\0';
	p = dest;
	while ( *p && (is_alpha(*p) || is_digit(*p) || *p == '$' || *p == '/' || *p == '_' || *p == '.') )
		p ++;
	*p = '\0';

	str_alltrim(dest);
	return dest;
}

/*
*	returns the ID of the label. If there is no one, then it creates one
*/
bid_t	comp_label_getID(const char *label_name) SEC(BCSC2);
bid_t	comp_label_getID(const char *label_name)
{
	bid_t	idx = -1, i;
	char	name[SB_KEYWORD_SIZE+1];
	comp_label_t	label;

	comp_prepare_name(name, label_name, SB_KEYWORD_SIZE);

	for ( i = 0; i < comp_labcount; i ++ )	{
		dbt_read(comp_labtable, i, &label, sizeof(comp_label_t));
		if	( strcmp(label.name, name) == 0 )	{
			idx = i;
			break;
			}
		}

	if	( idx == -1 )	{
		#if !defined(OS_LIMITED)
		if	( opt_verbose )	
			dev_printf("%d: new label [%s], index %d\n", comp_line, name, comp_labcount);
		#endif
		strcpy(label.name, name);
		label.ip = INVALID_ADDR;
		label.dp = INVALID_ADDR;
		label.level = comp_block_level;
		label.block_id = comp_block_id;

		dbt_write(comp_labtable, comp_labcount, &label, sizeof(comp_label_t));
		idx = comp_labcount;
		comp_labcount ++;
		}

	return idx;
}

/*
*	set LABEL's position (IP)
*/
void	comp_label_setip(bid_t idx) SEC(BCSC2);
void	comp_label_setip(bid_t idx)
{
	comp_label_t	label;

	dbt_read(comp_labtable, idx, &label, sizeof(comp_label_t));
	label.ip = comp_prog.count;
	label.dp = comp_data.count;
	label.level = comp_block_level;
	label.block_id = comp_block_id;
	dbt_write(comp_labtable, idx, &label, sizeof(comp_label_t));
}

/*
*	returns the full-path UDP/UDF name
*/
void	comp_prepare_udp_name(char *dest, const char *basename)	SEC(BCSC2);
void	comp_prepare_udp_name(char *dest, const char *basename)
{
	char	tmp[SB_SOURCELINE_SIZE+1];

	comp_prepare_name(tmp, baseof(basename, '/'), SB_KEYWORD_SIZE);
	if	( comp_proc_level )	
		sprintf(dest, "%s/%s", comp_bc_proc, tmp);
	else
		strcpy(dest, tmp);
}

/*
*	returns the ID of the UDP/UDF
*/
bid_t	comp_udp_id(const char *proc_name, int scan_tree) SEC(BCSC3);
bid_t	comp_udp_id(const char *proc_name, int scan_tree)
{
	bid_t	i;
	char	*name = comp_bc_temp, *p;
	char	base[SB_KEYWORD_SIZE+1];
	char	*root;
	int		len;

	if	( scan_tree )	{
		comp_prepare_name(base, baseof(proc_name, '/'), SB_KEYWORD_SIZE);
		root = tmp_strdup(comp_bc_proc);
		do	{
			// (nested procs) move root down
			if	( (len=strlen(root)) != 0 )	{
				sprintf(name, "%s/%s", root, base);
				p = strrchr(root, '/');
				if	( p )
					*p = '\0';
				else
					strcpy(root, "");
				}
			else
				strcpy(name, base);

//			printf("get_udp_id: look for %s\n", name);

			// search on local
			for ( i = 0; i < comp_udpcount; i ++ )	{
				if	( strcmp(comp_udptable[i].name, name) == 0 )	{
					tmp_free(root);
					return i;
					}
				}

			} while ( len );

		// not found
		tmp_free(root);
		}
	else	{
		comp_prepare_udp_name(name, proc_name);

		// search on local
		for ( i = 0; i < comp_udpcount; i ++ )	{
			if	( strcmp(comp_udptable[i].name, name) == 0 )	
				return i;
			}
		}

	return -1;
}

/*
*	creates a new UDP/UDF node
*	and returns the new ID
*/
bid_t	comp_add_udp(const char *proc_name) SEC(BCSC3);
bid_t	comp_add_udp(const char *proc_name)
{
	char	*name = comp_bc_temp;
	bid_t	idx = -1, i;

	comp_prepare_udp_name(name, proc_name);

/*
	#if !defined(OS_LIMITED)
	// check variables for conflict
	for ( i = 0; i < comp_varcount; i ++ )	{
		if	( strcmp(comp_vartable[i].name, name) == 0 )	{
			sc_raise("User-defined function/procedure name, '%s', conflicts with variable", name);
			break;
			}
		}
	#endif
*/

	// search
	for ( i = 0; i < comp_udpcount; i ++ )	{
		if	( strcmp(comp_udptable[i].name, name) == 0 )	{
			idx = i;
			break;
			}
		}

	if	( idx == -1 )	{
		if ( comp_udpcount >= comp_udpsize ) {
			comp_udpsize += GROWSIZE;
			comp_udptable = tmp_realloc(comp_udptable, comp_udpsize * sizeof(comp_udp_t));
			}

		if	( !(is_alpha(name[0]) || name[0] == '_') )
			err_wrongproc(name);
		else	{
			#if !defined(OS_LIMITED)
			if	( opt_verbose )	
				dev_printf("%d: new UDP/F [%s], index %d\n", comp_line, name, comp_udpcount);
			#endif
			comp_udptable[comp_udpcount].name = tmp_alloc(strlen(name)+1);
			comp_udptable[comp_udpcount].ip = INVALID_ADDR; //bc_prog.count;
			comp_udptable[comp_udpcount].level = comp_block_level;
			comp_udptable[comp_udpcount].block_id = comp_block_id;
			comp_udptable[comp_udpcount].pline = comp_line;
			strcpy(comp_udptable[comp_udpcount].name, name);
			idx = comp_udpcount;
			comp_udpcount ++;
			}
		}

	return idx;
}

/*
*	sets the IP of the user-defined-procedure (or function)
*/
bid_t	comp_udp_setip(const char *proc_name, addr_t ip) SEC(BCSC2);
bid_t	comp_udp_setip(const char *proc_name, addr_t ip)
{
	bid_t	idx;
	char	*name = comp_bc_temp;

	comp_prepare_udp_name(name, proc_name);
	
	idx = comp_udp_id(name, 0);
	if	( idx != -1 )	{
		comp_udptable[idx].ip = comp_prog.count;
		comp_udptable[idx].level = comp_block_level;
		comp_udptable[idx].block_id = comp_block_id;
		}
	return idx;
}

/*
*	Returns the IP of an UDP/UDF
*/
addr_t	comp_udp_getip(const char *proc_name) SEC(BCSC3);
addr_t	comp_udp_getip(const char *proc_name)
{
	bid_t	idx;
	char	*name = comp_bc_temp;
	
	comp_prepare_udp_name(name, proc_name);

	idx = comp_udp_id(name, 1);
	if	( idx != -1 )	
		return comp_udptable[idx].ip;
	return INVALID_ADDR;
}

/*
*	parameters string-section
*/
char	*get_param_sect(char *text, const char *delim, char *dest) SEC(BCSC3);
char	*get_param_sect(char *text, const char *delim, char *dest)
{
	char	*p = (char *) text;
	char	*d = dest;
	int		quotes = 0, level = 0, skip_ch = 0;

	if	( p == NULL )	{
		*dest = '\0';
		return 0;
		}

	while ( is_space(*p) )	p ++;

	while ( *p )	{
		if	( quotes )	{
			if	( *p == '\"' )
				quotes = 0;
			}
		else	{
			switch ( *p )	{
			case	'\"':
				quotes = 1;
				break;
			case	'(':
				level ++;
				break;
			case	')':
				level --;
				break;
			case	'\n':
			case	'\r':
				skip_ch = 1;
				break;
				};
			}

		// delim check
		if	( delim != NULL && level <= 0 && quotes == 0 )	{
			if	( strchr(delim, *p) != NULL )	
				break;
			}

		// copy
		if	( !skip_ch )	{
			*d = *p;
			d ++;
			}
		else
			skip_ch = 0;

		p ++;
		}

	*d = '\0';
	
	if	( quotes )
		err_comp_missing_q();
	if	( level > 0 )
		err_comp_missing_rp();
	if	( level < 0 )
		err_comp_missing_lp();

	str_alltrim(dest);
	return p;
}

/*
*/
int		comp_geterror()
{
	return comp_error;
}

/*
*	checking for missing labels
*/
int		comp_check_labels(void) SEC(BCSC3);
int		comp_check_labels()
{
	bid_t		i;
	comp_label_t		label;

	for ( i = 0; i < comp_labcount; i ++ )	{
		dbt_read(comp_labtable, i, &label, sizeof(comp_label_t));
		if	( label.ip == INVALID_ADDR )	{
			err_comp_label_not_def(label.name);
			return 0;
			}
		}

	return 1;
}

/*
*/
int		comp_check_class(const char *name)	SEC(BCSC3);
int		comp_check_class(const char *name)
{
	char	tmp[SB_KEYWORD_SIZE+1];
	char	*p;
	int		i;

	strcpy(tmp, name);
	p = strchr(tmp, '.');
	if	( p )	{
		*p = '\0';
		for	( i = 0; i < comp_libcount; i ++ )	{
			bc_lib_rec_t	lib;

			dbt_read(comp_libtable, i, &lib, sizeof(bc_lib_rec_t));
			if	( strcasecmp(lib.lib, tmp) == 0 )	
				return 1;
			}

		err_comp_unknown_unit(tmp);
		}
	return 0;
}

/*
*/
int		comp_create_var(const char *name)		SEC(BCSC2);
int		comp_create_var(const char *name)
{
	int		idx = -1;
	
	if	( !(is_alpha(name[0]) || name[0] == '_') )
		sc_raise("Wrong variable name: %s", name);
	else	{
		// realloc table if it is needed
		if ( comp_varcount >= comp_varsize ) {
			comp_varsize += GROWSIZE;
			comp_vartable = tmp_realloc(comp_vartable, comp_varsize * sizeof(comp_var_t));
			}

		#if !defined(OS_LIMITED)
		if	( opt_verbose )	
			dev_printf("%d: new VAR [%s], index %d\n", comp_line, name, comp_varcount);
		#endif
		comp_vartable[comp_varcount].name = tmp_alloc(strlen(name)+1);
		strcpy(comp_vartable[comp_varcount].name, name);
		comp_vartable[comp_varcount].dolar_sup = 0;
		comp_vartable[comp_varcount].lib_id = -1;
		idx = comp_varcount;
		comp_varcount ++;
		}
	return idx;
}

/*
*/
int		comp_add_external_var(const char *name, int lib_id)
{
	int		idx;

	idx = comp_create_var(name);
	comp_vartable[idx].lib_id = lib_id;

	if	( lib_id & UID_UNIT_BIT )	{
		// update imports table
		bc_symbol_rec_t		sym;

		strcpy(sym.symbol, name);		// symbol name
		sym.type   = stt_variable;		// symbol type
		sym.lib_id = lib_id;	 		// library id
		sym.sym_id = comp_impcount;		// symbol  index
		sym.var_id = idx;				// variable index

		// store it
		dbt_write(comp_imptable, comp_impcount, &sym, sizeof(bc_symbol_rec_t));
		comp_impcount ++;
		}

	return idx;
}

/*
*	returns the id of the variable 'name'
*
*	if there is no such variable then creates a new one
*
*	if a new variable must created then if the var_name includes the path then 
*	the new variable created at local space otherwise at globale space
*/
bid_t	comp_var_getID(const char *var_name) SEC(BCSC3);
bid_t	comp_var_getID(const char *var_name)
{
	bid_t	idx = -1, i;
	char	tmp[SB_KEYWORD_SIZE+1];
	char	*name = comp_bc_temp;

	comp_prepare_name(tmp, baseof(var_name, '/'), SB_KEYWORD_SIZE);

	//
	//	check for external
	//	external variables are recognized by the 'class' name
	//
	//	example: my_unit.my_var
	//
	if	( strchr(tmp, '.') )	{
		comp_check_class(tmp);

		if	( !comp_error )	{
			for ( i = 0; i < comp_varcount; i ++ )	{
				if	( strcmp(comp_vartable[i].name, tmp) == 0 )	{
					//printf("VarID RQ (e): [%s], [%s] = %d\n", var_name, tmp, idx);
					return i;
					}
				}

			sc_raise("Unit has no member named '%s'\n", tmp);
			}

		return 0;
		}

	//
	//	search in global name-space
	//
	//	Note: local space is dynamic,
	//		however a global var-ID per var-name is required
	//
	strcpy(name, tmp);

	for ( i = 0; i < comp_varcount; i ++ )	{
		if	( strcmp(comp_vartable[i].name, name) == 0 )	{
			idx = i;
			break;
			}

		if	( comp_vartable[i].dolar_sup )	{
			// system variables must be visible with or without '$' suffix
			char	*dollar_name;

			dollar_name = tmp_alloc(strlen(comp_vartable[i].name)+2);
			strcpy(dollar_name, comp_vartable[i].name);
			strcat(dollar_name, "$");
			if	( strcmp(dollar_name, name) == 0 )	{
				idx = i;
				tmp_free(dollar_name);
				break;
				}

			tmp_free(dollar_name);
			}
		}

	if	( idx == -1 )	// variable not found; create a new one
		idx = comp_create_var(tmp);

	return idx;
}

/*
*	adds a mark in stack at the current code position
*/
void	comp_push(addr_t ip) SEC(BCSC3);
void	comp_push(addr_t ip)
{
	comp_pass_node_t		node;

	strcpy(node.sec, comp_bc_sec);
	node.pos      = ip;
	node.level    = comp_block_level;
	node.block_id = comp_block_id;
	node.line     = comp_line;
	dbt_write(comp_stack, comp_sp, &node, sizeof(comp_pass_node_t));

	comp_sp ++;
}

/*
*	returns the keyword code 
*/
int		comp_is_keyword(const char *name)
{
	int		i, idx;
	byte	dolar_sup = 0;

//	Code to enable the $ but not for keywords (INKEY$=INKEY, PRINT$=PRINT !!!)
//	I don't want to increase the size of keywords table.
	idx = strlen(name) - 1;
	if	( name[idx] == '$' )	{
		*((char *) (name+idx)) = '\0';
		dolar_sup ++;
		}

	for ( i = 0; keyword_table[i].name[0] != '\0'; i ++ )	{
		if	( strcmp(keyword_table[i].name, name) == 0 )
			return keyword_table[i].code;
		}

	if	( dolar_sup )
		*((char *) (name+idx)) = '$';

	return -1;
}

/*
*	returns the keyword code (buildin functions)
*/
fcode_t		comp_is_func(const char *name)
{
	fcode_t		i;
	int			idx;
	byte		dolar_sup = 0;

//	Code to enable the $ but not for keywords (INKEY$=INKEY, PRINT$=PRINT !!!)
//	I don't want to increase the size of keywords table.
	idx = strlen(name) - 1;
	if	( name[idx] == '$' )	{
		*((char *) (name+idx)) = '\0';
		dolar_sup ++;
		}

	for ( i = 0; func_table[i].name[0] != '\0'; i ++ )	{
		if	( strcmp(func_table[i].name, name) == 0 )
			return func_table[i].fcode;
		}

	if	( dolar_sup )
		*((char *) (name+idx)) = '$';

	return -1;
}

/*
*	returns the keyword code (buildin procedures)
*/
pcode_t	comp_is_proc(const char *name)
{
	pcode_t		i;

	for ( i = 0; proc_table[i].name[0] != '\0'; i ++ )	{
		if	( strcmp(proc_table[i].name, name) == 0 )
			return proc_table[i].pcode;
		}

	return -1;
}

/*
*	returns the keyword code (special separators)
*/
int		comp_is_special_operator(const char *name)
{
	int		i;

	for ( i = 0; spopr_table[i].name[0] != '\0'; i ++ )	{
		if	( strcmp(spopr_table[i].name, name) == 0 )
			return spopr_table[i].code;
		}

	return -1;
}

/*
*	returns the keyword code (operators)
*/
long	comp_is_operator(const char *name)
{
	int		i;

	for ( i = 0; opr_table[i].name[0] != '\0'; i ++ )	{
		if	( strcmp(opr_table[i].name, name) == 0 )
			return ((opr_table[i].code << 8) | opr_table[i].opr);
		}

	return -1;
}

/*
*/
char*	comp_next_char(char *source)	SEC(BCSC3);
char*	comp_next_char(char *source)
{
	char	*p = source;

	while ( *p )	{
		if	( *p != ' ' )
			return p;
		p ++;
		}
	return p;
}

/*
*/
char*	comp_prev_char(const char *root, const char *ptr)	SEC(BCSC3);
char*	comp_prev_char(const char *root, const char *ptr)
{
	char	*p = (char *) ptr;
	
	if	( p > root )
		p --;
	else
		return (char *) root;

	while ( p > root )	{
		if	( *p != ' ' )
			return p;
		p --;
		}
	return p;
}

/**
* 	get next word
*	if buffer's len is zero, then the next element is not a word
*
*	@param text the source 
*	@param dest the buffer to store the result
*	@return pointer of text to the next element
*/
const char	*comp_next_word(const char *text, char *dest) SEC(BCSC3);
const char	*comp_next_word(const char *text, char *dest)
{
	const char	*p = text;
	char		*d = dest;

	if	( p == NULL )	{
		*dest = '\0';
		return 0;
		}

	while ( is_space(*p) )	p ++;

	if	( *p == '?' )	{
		strcpy(dest, "PRINT");
		p ++;
		while ( is_space(*p) )	p ++;
		return p;
		}

	if	( *p == '\'' || *p == '#' )	{
		strcpy(dest, "REM");
		p ++;
		while ( is_space(*p) )	p ++;
		return p;
		}

	if	( is_alnum(*p) || *p == '_' )	{	// don't forget the numeric-labels
		while ( is_alnum(*p) || (*p == '_')  || (*p == '.')  )	{
			*d = *p;
			d ++; p ++;
			}
		}

//	Code to kill the $
//	if	( *p == '$' )	
//		p ++;

//	Code to enable the $
	if	( *p == '$' )
		*d ++ = *p ++;

	*d = '\0';
	while ( is_space(*p) )	p ++;
	return p;
}

/*
*	scan expression
*/
void	comp_expression(char *expr, byte no_parser) SEC(BCSC3);
void	comp_expression(char *expr, byte no_parser)
{
	char	*ptr = (char *) expr;
	long	idx;
	int		level = 0, check_udf = 0;
	int		kw_exec_more = 0;
	int		tp;
	addr_t	w, stip, cip;
	long	lv = 0;
	double	dv = 0;
	bc_t	bc;

	comp_use_global_vartable = 0;				// check local-variables first
	str_alltrim(expr);

	if	( *ptr == '\0' )	
		return;

	bc_create(&bc);

	while ( *ptr )	{

		if	( is_digit(*ptr) || *ptr == '.' || (*ptr == '&' && strchr("XHOB", *(ptr+1) ) ) )	{
			/*
			*	A CONSTANT NUMBER
			*/
			ptr = get_numexpr(ptr, comp_bc_name, &tp, &lv, &dv);
			switch ( tp )	{
			case	1:
				bc_add_cint(&bc, lv);
				continue;
			case	2:
				bc_add_creal(&bc, dv);
				continue;
			default:
				sc_raise("Error on numeric expression");
				}
			}
		else if ( *ptr == '\'' /* || *ptr == '#' */ )	{ // remarks
			break;		
			}
		else if	( is_alpha(*ptr) || *ptr == '?' || *ptr == '_' )	{
			/*
			*	A NAME 
			*/
			ptr = (char *) comp_next_word(ptr, comp_bc_name);
			idx = comp_is_func(comp_bc_name);

			// special case for INPUT
			if	( idx == kwINPUTF )	{
				if	( *comp_next_char(ptr) != '(' )
					idx = -1;	// INPUT is SPECIAL SEPARATOR (OPEN...FOR INPUT...)
				}

			if	( idx != -1 )	{
				/*
				*	IS A FUNCTION
				*/
				if	( !kw_noarg_func(idx) )	{
					if	( *comp_next_char(ptr) != '(' )
						sc_raise("buildin function %s: without parameters", comp_bc_name);
					}
				bc_add_fcode(&bc, idx);
				check_udf ++;
				}
			else	{
				/*
				*	CHECK SPECIAL SEPARATORS
				*/
				idx = comp_is_special_operator(comp_bc_name);
				if	( idx != -1 )	{
					if	( idx == kwUSE )	{
						bc_add_code(&bc, idx);
						bc_add_addr(&bc, 0);
						bc_add_addr(&bc, 0);
						comp_use_global_vartable = 1;	// all the next variables are global (needed for X)
						check_udf ++;
						}
					else if	( idx == kwDO )	{
						while ( *ptr == ' ' )	ptr ++;
						if	( strlen(ptr) )	{
							if	( strlen(comp_do_close_cmd) )	{
								kw_exec_more = 1;
								strcpy(comp_bc_tmp2, ptr);
								strcat(comp_bc_tmp2, ":");
								strcat(comp_bc_tmp2, comp_do_close_cmd);
								strcpy(comp_do_close_cmd, "");
								}
							else
								sc_raise("Keyword DO not allowed here");
							}
						break;
						}
					else
						bc_add_code(&bc, idx);
					}
				else	{
					/*
					*	NOT A COMMAND, CHECK OPERATORS
					*/
					idx = comp_is_operator(comp_bc_name);
					if	( idx != -1 )	{
						bc_add_code(&bc, idx >> 8);
						bc_add_code(&bc, idx & 0xFF);
						}
					else	{
						/*
						*	EXTERNAL FUNCTION
						*/
						idx = comp_is_external_func(comp_bc_name);
						if	( idx != -1 )	{
//							if	( comp_extfunctable[idx].lib_id & UID_UNIT_BIT )
//								bc_add_extfcode(&bc, comp_extfunctable[idx].lib_id, comp_extfunctable[idx].symbol_index);
//							else
//								bc_add_extfcode(&bc, comp_extfunctable[idx].lib_id, idx);
							bc_add_extfcode(&bc, comp_extfunctable[idx].lib_id, comp_extfunctable[idx].symbol_index);
							}
						else	{
							idx = comp_is_keyword(comp_bc_name);
							if	( idx == -1 )
								idx = comp_is_proc(comp_bc_name);

							if	( idx != -1 )	{
								sc_raise("Statement %s must be on the left side (the first keyword of the line)", comp_bc_name);
								}
							else	{
								/*
								*	UDF OR VARIABLE
								*/
								int		udf;

								udf = comp_udp_id(comp_bc_name, 1);
								if	( udf != -1 )	{
									// UDF
									bc_add_code(&bc, kwTYPE_CALL_UDF);
									bc_add_addr(&bc, udf);
									bc_add_addr(&bc, 0);
									check_udf ++;
									}
								else	{
									// VARIABLE
									while ( *ptr == ' ' ) ptr ++;
									if	( *ptr == '(' )	{
										if	( *(ptr+1) == ')' )	{
											// null array
											ptr += 2;
											}
										}

									w = comp_var_getID(comp_bc_name);
									bc_add_code(&bc, kwTYPE_VAR);
									bc_add_addr(&bc, w);
									}
								} // kw
							} // extf
						} // opr
					} // sp. sep
				} // check sep
			}	// isalpha
		else if ( *ptr == ',' || *ptr == ';' || *ptr == '#' )	{
			// parameter separator
			bc_add_code(&bc, kwTYPE_SEP);
			bc_add_code(&bc, *ptr);

			ptr ++;
			}
		else if ( *ptr == '\"' )	{
			// string
			ptr = bc_store_string(&bc, ptr);
			}
		else if ( *ptr == '[' )	{	// code-defined array
			ptr ++;
			level ++;
			bc_add_fcode(&bc, kwCODEARRAY);
			bc_add_code(&bc, kwTYPE_LEVEL_BEGIN);
			}
		else if ( *ptr == '(' )	{
			// parenthesis
			level ++;
			bc_add_code(&bc, kwTYPE_LEVEL_BEGIN);

			ptr ++;
			}
		else if ( *ptr == ')' || *ptr == ']' )	{
			// parenthesis
			bc_add_code(&bc, kwTYPE_LEVEL_END);

			level --;
			ptr ++;
			}
		else if ( is_space(*ptr) )	
			// null characters
			ptr ++;
		else	{
			// operators
			if	( *ptr == '+' || *ptr == '-' )	{
				bc_add_code(&bc, kwTYPE_ADDOPR);
				bc_add_code(&bc, *ptr);
				}
			else if	( *ptr == '*' || *ptr == '/' || *ptr == '\\' || *ptr == '%' )	{
				bc_add_code(&bc, kwTYPE_MULOPR);
				bc_add_code(&bc, *ptr);
				}
			else if	( *ptr == '^' )	{
				bc_add_code(&bc, kwTYPE_POWOPR);
				bc_add_code(&bc, *ptr);
				}
			else if	( strncmp(ptr, "<=", 2) == 0 || strncmp(ptr, "=<", 2) == 0 )	{
				bc_add_code(&bc, kwTYPE_CMPOPR);
				bc_add_code(&bc, OPLOG_LE);

				ptr ++;
				}
			else if	( strncmp(ptr, ">=", 2) == 0 || strncmp(ptr, "=>", 2) == 0 )	{
				bc_add_code(&bc, kwTYPE_CMPOPR);
				bc_add_code(&bc, OPLOG_GE);

				ptr ++;
				}
			else if	( strncmp(ptr, "<>", 2) == 0 || strncmp(ptr, "!=", 2) == 0 )	{
				bc_add_code(&bc, kwTYPE_CMPOPR);
				bc_add_code(&bc, OPLOG_NE);
				ptr ++;
				}
			else if	( strncmp(ptr, "<<", 2) == 0 )	{
				ptr += 2;
				while ( *ptr == ' ' )	ptr ++;
				if	( strlen(ptr) )	{
					kw_exec_more = 1;
					strcpy(comp_bc_tmp2, comp_bc_name);
					strcat(comp_bc_tmp2, " << ");
					strcat(comp_bc_tmp2, ptr);
					}
				else
					sc_raise("operator << not allowed here");

				break;
				}
			else if	( *ptr == '=' || *ptr == '>' || *ptr == '<' )	{
				bc_add_code(&bc, kwTYPE_CMPOPR);
				bc_add_code(&bc, *ptr);
				}
			else if	( strncmp(ptr, "&&", 2) == 0 || strncmp(ptr, "||", 2) == 0 )	{
				bc_add_code(&bc, kwTYPE_LOGOPR);
				bc_add_code(&bc, *ptr);
				ptr ++;
				}
			else if	( *ptr == '&' )	{
				bc_add_code(&bc, kwTYPE_LOGOPR);
				bc_add_code(&bc, OPLOG_BAND);
				}
			else if	( *ptr == '|' )	{
				bc_add_code(&bc, kwTYPE_LOGOPR);
				bc_add_code(&bc, OPLOG_BOR);
				}
			else if	( *ptr == '~' )	{
				bc_add_code(&bc, kwTYPE_UNROPR);
				bc_add_code(&bc, OPLOG_INV);
				}
			else if	( *ptr == '!' )	{
				bc_add_code(&bc, kwTYPE_UNROPR);
				bc_add_code(&bc, *ptr);
				}
			else
				sc_raise("Unknown operator: '%c'", *ptr);

			ptr ++;
			}
		};

	///////////////////////////
	if	( level )
		sc_raise("Missing ')'");

	if	( !comp_error )	{
		
		if	( no_parser == 0 )	{
			// optimization
			bc_add_code(&bc, kwTYPE_EOC);
//			printf("=== before:\n");		hex_dump(bc.ptr, bc.count);
			expr_parser(&bc);
//			printf("=== after:\n");			hex_dump(bc.ptr, bc.count);
			}

		if	( bc.count )	{
			stip = comp_prog.count;
			bc_append(&comp_prog, &bc);	// merge code segments

			// update pass2 stack-nodes
			if	( check_udf )	{
				cip = stip;
				while ( (cip = comp_search_bc(cip, kwUSE)) != INVALID_ADDR )	{
					comp_push(cip);
					cip += (1+ADDRSZ+ADDRSZ);
					}

				cip = stip;
				while ( (cip = comp_search_bc(cip, kwTYPE_CALL_UDF)) != INVALID_ADDR )	{
					comp_push(cip);
					cip += (1+ADDRSZ+ADDRSZ);
					}

				}
			}

		bc_eoc(&comp_prog);
		}

	// clean-up
	comp_use_global_vartable = 0;				// check local-variables first
	bc_destroy(&bc);

	// do additional steps
	if	( kw_exec_more )	{
		comp_text_line(comp_bc_tmp2);
//		printf("exec more: [%s]\n", comp_bc_tmp2);
		}
}

/*
*	Converts DATA commands to bytecode
*/
void	comp_data_seg(char *source) SEC(BCSC2);
void	comp_data_seg(char *source)
{
	char	*ptr = source;
	char	*commap;
	long	lv = 0;
	double	dv = 0, sign = 1;
	char	*tmp = comp_bc_temp;
	int		quotes;
	int		tp;

	while ( *ptr )	{
		while ( *ptr == ' ' )	ptr ++;

		if	( *ptr == '\0' )	
			break;
		else if	( *ptr == ',' )	{
			bc_add_code(&comp_data, kwTYPE_EOC);
			ptr ++;
			}
		else	{
			// find the end of the element
			commap = ptr;
			quotes = 0;
			while ( *commap )	{
				if	( *commap == '\"' )
					quotes = !quotes;
				else if ( (*commap == ',') && (quotes == 0) )
					break;

				commap ++;
				}
			if	( *commap == '\0' )
				commap = NULL;

			if	( commap != NULL )
				*commap = '\0';

			if  ((*ptr == '-' || *ptr == '+') && strchr("0123456789.", *(ptr+1)))	{ 
				if	( *ptr == '-' )
					sign = -1;
				ptr ++;
				}
			else
				sign = 1;

			if	( is_digit(*ptr) || *ptr == '.' 
					|| (*ptr == '&' && strchr("XHOB", *(ptr+1))) )	{

				//	number - constant
				ptr = get_numexpr(ptr, tmp, &tp, &lv, &dv);
				switch ( tp )	{
				case	1:
					bc_add_cint(&comp_data, lv * sign);
					break;
				case	2:
					bc_add_creal(&comp_data, dv * sign);
					break;
				default:
					sc_raise("Error on numeric expression");
					}
				}
			else	{
				// add it as string
				if	( *ptr != '\"' )	{
					strcpy(tmp, "\"");
					strcat(tmp,  ptr);
					strcat(tmp, "\"");
					bc_store_string(&comp_data, tmp);
					if	( commap )
						ptr = commap;
					else
						ptr = ptr + strlen(ptr);
					}
				else
					ptr = bc_store_string(&comp_data, ptr);
				}

			if	( commap != NULL )	
				*commap = ',';
			}
		}

	bc_add_code(&comp_data, kwTYPE_EOC);	// no bc_eoc
}

/*
*	Scans the 'source' for "names" separated by 'delims' and returns the elements (pointer in source)
*	into args array.
*
*	Returns the number of items
*/
int		comp_getlist(char *source, char_p_t *args, char *delims, int maxarg) SEC(BCSC2);
int		comp_getlist(char *source, char_p_t *args, char *delims, int maxarg)
{
	char	*p, *ps;
	int		count = 0;

	ps = p = source;
	while ( *p )	{
		if	( strchr(delims, *p) ) {
			*p = '\0';
			while (*ps == ' ')	ps ++;
			args[count] = ps;
			count ++;
			if	( count == maxarg )	{
				if	( *ps )
					sc_raise("Paremeters limit: %d", maxarg);
				return count;
				}
			ps = p+1;
			}

		p ++;
		}

	if	( *ps )	{
		while (*ps == ' ')	ps ++;
		if	( *ps )	{
			*p = '\0';
			args[count] = ps;
			count ++;
			}
		}

	return count;
}

/*
*	returns a list of names
*
*	the list is included between sep[0] and sep[1] characters
*	each element is separated by 'delims' characters
*
*	the 'source' is the raw string (null chars will be placed at the end of each name)
*	the 'args' is the names (pointers on the 'source')
*	maxarg is the maximum number of names (actually the size of args)
*	the count is the number of names which are found by this routine.
*
*	returns the next position in 'source' (after the sep[1])
*/
char	*comp_getlist_insep(char *source, char_p_t *args, char *sep, char *delims, int maxarg, int *count) SEC(BCSC2);
char	*comp_getlist_insep(char *source, char_p_t *args, char *sep, char *delims, int maxarg, int *count)
{
	char	*p = source;
	char	*ps;
	int		level = 1;
	
	*count = 0;
	p = strchr(source, sep[0]);

	if	( p )	{
		ps = p+1;
		p ++;

		while ( *p )	{
			if	( *p == sep[1] )	{
				level --;
				if ( level == 0 )
					break;
				}
			else if	( *p == sep[0] )
				level ++;

			p ++;
			}

		if	( *p == sep[1] )	{
			*p = '\0';
			if	( strlen(ps) )	{
				while ( *ps == ' ' || *ps == '\t' ) ps ++;
				if	( strlen(ps) )	
					*count = comp_getlist(ps, args, delims, maxarg);
				else
					sc_raise("'Empty' parameters are not allowed");
				}
			}
		else	
			sc_raise("Missing '%c'", sep[1]);
		}
	else
		p = source;

	return p;
}

/*
*	Single-line IFs
*
*	converts the string from single-line IF to normal IF syntax
*	returns true if there is a single-line IF.
*
*	IF expr THEN ... ---> IF expr THEN (:) .... (:FI)
*	IF expr THEN ... ELSE ... ---> IF expr THEN (:) .... (:ELSE:) ... (:FI)
*/
int		comp_single_line_if(char *text)
{
	char	*p = (char *) text;	// *text points to 'expr'
	char	*pthen, *pelse;
	char	buf[SB_SOURCELINE_SIZE+1];

	if	( comp_error )		return 0;

	pthen = p;
	do	{
		pthen = strstr(pthen+1, " THEN ");
		if	( pthen )	{
			// store the expression
			while ( *p == ' ' )	p ++;
			strcpy(buf, p);
			p = strstr(buf, " THEN ");
			*p = '\0';

			// check for ':'
			p = pthen+6;
			while ( *p == ' ' )	p ++;

			if	( *p != ':' && *p != '\0' )	{
				// store the IF
				comp_block_level ++;	comp_block_id ++;
				comp_push(comp_prog.count);
				bc_add_ctrl(&comp_prog, kwIF, 0, 0);

				comp_expression(buf, 0);
				if	( comp_error )		return 0;
				// store EOC
				bc_add_code(&comp_prog, kwTYPE_EOC);	//bc_eoc();

				// auto-goto 
				p = pthen + 6;
				while ( *p == ' ' )	p ++;
				if	( is_digit(*p) )	{
					// add goto
					strcpy(buf, "GOTO ");
					strcat(buf, p);
					}
				else
					strcpy(buf, p);

				// ELSE command
				// If there are more inline-ifs (nested) the ELSE belongs to the first IF (that's an error)
				pelse = strstr(buf+1, "ELSE");
				if	( pelse )	{
					do	{
						if	( (*(pelse-1) == ' ' || *(pelse-1) == '\t') &&
							  (*(pelse+4) == ' ' || *(pelse+4) == '\t') )	{

							*pelse = '\0';

							// scan the commands before ELSE
							comp_text_line(buf);
							// add EOC
							bc_eoc(&comp_prog);

							// auto-goto
							strcpy(buf, "ELSE:");
							p = pelse + 4;
							while ( *p == ' ' || *p == '\t' )	p ++;
							if	( is_digit(*p) )	{
								// add goto
								strcat(buf, "GOTO ");
								strcat(buf, p);
								}
							else
								strcat(buf, p);
							
							//
							break;
							}
						else
							pelse = strstr(pelse+1, "ELSE");
						} while ( pelse != NULL );
					}

				// scan the rest commands
				comp_text_line(buf);
				// add EOC
				bc_eoc(&comp_prog);

				// add ENDIF
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, kwENDIF, 0, 0);
				comp_block_level --;

				return 1;
				}
			else	// *p == ':'
				return 0;
			}
		else
			break;

		} while ( pthen != NULL );

	return 0;	// false
}

/*
*	array's args 
*/
void	comp_array_params(char *src)	SEC(BCSC2);
void	comp_array_params(char *src)
{
	char	*p = src;
	char	*ss = NULL, *se = NULL;
	int		level = 0;

	while ( *p )	{
		switch ( *p )	{
		case	'(':
			if	( level == 0 )
				ss = p;
			level ++;
			break;
		case	')':
			level --;

			if	( level == 0 )	{
				se = p;
				// store this index
				if	( !ss )	
					sc_raise("Array: syntax error");
				else	{
					*ss = ' ';	*se = '\0';
	
					bc_add_code(&comp_prog, kwTYPE_LEVEL_BEGIN);
					comp_expression(ss, 0);
					bc_store1(&comp_prog, comp_prog.count-1, kwTYPE_LEVEL_END);

					*ss = '(';	*se = ')';
					ss = se = NULL;
					}
				}	// lev = 0
			break;
			};

		p ++;
		}

	//
	if ( level > 0 )	
		sc_raise("Array: Missing ')', (left side of expression)");
	else if ( level < 0 )
		sc_raise("Array: Missing '(', (left side of expression)");
}

/*
*	run-time options
*/
#define	CHKOPT(x)	(strncmp(p, (x), strlen((x))) == 0)
void	comp_cmd_option(char *src)	SEC(BCSC2);
void	comp_cmd_option(char *src)
{
	char	*p = src;

	if	( CHKOPT("UICS ") )	{	
		bc_add_code(&comp_prog, kwOPTION);
		bc_add_code(&comp_prog, OPTION_UICS);

		p += 5;
		while ( is_space(*p) )	p ++;
		if	( CHKOPT("CHARS") )
			bc_add_addr(&comp_prog, OPTION_UICS_CHARS);
		else if	( CHKOPT("PIXELS") )
			bc_add_addr(&comp_prog, OPTION_UICS_PIXELS);
		else
			sc_raise("Syntax error: OPTION UICS {CHARS|PIXELS}");
		}
	else if	( CHKOPT("BASE ") )	{	
		bc_add_code(&comp_prog, kwOPTION);
		bc_add_code(&comp_prog, OPTION_BASE);
		bc_add_addr(&comp_prog, xstrtol(src+5));
		}
	else if	( CHKOPT("MATCH PCRE CASELESS") )	{
		bc_add_code(&comp_prog, kwOPTION);
		bc_add_code(&comp_prog, OPTION_MATCH);
		bc_add_addr(&comp_prog, 2);
		}
	else if	( CHKOPT("MATCH PCRE") )	{
		bc_add_code(&comp_prog, kwOPTION);
		bc_add_code(&comp_prog, OPTION_MATCH);
		bc_add_addr(&comp_prog, 1);
		}
	else if	( CHKOPT("MATCH SIMPLE") )	{
		bc_add_code(&comp_prog, kwOPTION);
		bc_add_code(&comp_prog, OPTION_MATCH);
		bc_add_addr(&comp_prog, 0);
		}
	else if	( CHKOPT("PREDEF ") || CHKOPT("IMPORT ") )
		; /* ignore it */
	else
		sc_raise("OPTION: Unrecognized option '%s'", src);
}
#undef CHKOPT

int		comp_error_if_keyword(const char *name)	SEC(BCSC3);
int		comp_error_if_keyword(const char *name)
{
	// check if keyword
	if	( !comp_error )	{
		if	( (comp_is_func(name) >= 0) ||
			  (comp_is_proc(name) >= 0) ||
			  (comp_is_special_operator(name) >= 0) ||
			  ( comp_is_keyword(name) >= 0 ) ||
			  (comp_is_operator(name) >= 0) )

		sc_raise("%s: is keyword (left side of expression)", name);
		}
	return comp_error;
}

/**
*	stores export symbols (in pass2 will be checked again)
*/
void	bc_store_exports(const char *slist)	SEC(BCSC2);
void	bc_store_exports(const char *slist)
{
	#if defined(OS_LIMITED)
	char_p_t pars[32];
	#else
	char_p_t pars[256];
	#endif
	int		count = 0, i;
	char	*newlist;
	unit_sym_t	sym;

	newlist = (char *) tmp_alloc(strlen(slist)+3);
	strcpy(newlist, "(");
	strcat(newlist, slist);
	strcat(newlist, ")");

	#if defined(OS_LIMITED)
	comp_getlist_insep(newlist, pars, "()", ",", 32, &count);
	#else
	comp_getlist_insep(newlist, pars, "()", ",", 256, &count);
	#endif

	for ( i = 0; i < count; i ++ )	{
		strcpy(sym.symbol, pars[i]);
		dbt_write(comp_exptable, comp_expcount, &sym, sizeof(unit_sym_t));
		comp_expcount ++;
		}

	tmp_free(newlist);
}

/*
*	PASS1: scan source line
*/
void	comp_text_line(char *text)
{
	char	*p;
	char	*lb_end;
	char	*last_cmd;
	#if defined(OS_ADDR16)
	int		idx;
	#else
	long	idx;
	#endif
	int		sharp, ladd, linc, ldec, decl = 0, vattr;
	int		leqop;
	char	pname[SB_KEYWORD_SIZE+1], vname[SB_KEYWORD_SIZE+1];

	if	( comp_error )		return;

	str_alltrim(text);
	p = text;

	// EOL
	if	( *p == ':' )	{	
		p ++;
		comp_text_line(p);
		return;
		}

	// remark
	if	( *p == '\'' || *p == '#' )	return;

	// empty line
	if	( *p == '\0' )		return;

	//
	lb_end = p = (char *) comp_next_word(text, comp_bc_name);
	last_cmd = p;
	p = get_param_sect(p, ":", comp_bc_parm);
//	printf("\n%d: [%s] [%s]\n", comp_line, comp_bc_name, comp_bc_parm);

	/*
	*	check old style labels 
	*/
	if	( is_all_digits(comp_bc_name) )	{
		str_alltrim(comp_bc_name);
		idx = comp_label_getID(comp_bc_name);
		comp_label_setip(idx);
		if	( comp_error )		return;

		// continue
		last_cmd = p = (char *) comp_next_word(lb_end, comp_bc_name);
		if	( strlen(comp_bc_name) == 0 )	{
			if	( !p )	return;
			if	( *p == '\0' ) return;
			}
		p = get_param_sect(p, ":", comp_bc_parm);
		}

	/* what's this ? */
	idx = comp_is_keyword(comp_bc_name);
	if	( idx == kwREM )	return;		// remarks... return
	if	( idx == -1 )	{
		idx = comp_is_proc(comp_bc_name);
		if	( idx != -1 )	{
			//	simple buildin procedure
			//
			//	there is no need to check it more...
			//	save it and return (go to next)
			bc_add_pcode(&comp_prog, idx);
			comp_expression(comp_bc_parm, 0);

			if	( *p == ':' )	{	// command separator
				bc_eoc(&comp_prog);
				p ++;
				comp_text_line(p);
				}

			return;
			}
		}

	if	( idx == kwLET )	{	// old-style keyword LET
		char	*p;
		idx = -1;
		p = (char *) comp_next_word(comp_bc_parm, comp_bc_name);
		strcpy(comp_bc_parm, p);
		}
	else if ( idx == kwDECLARE )	{ // declaration
		char	*p;

		decl = 1;
		p = (char *) comp_next_word(comp_bc_parm, comp_bc_name);
		idx = comp_is_keyword(comp_bc_name);
		if	( idx == -1 )	idx = comp_is_proc(comp_bc_name);
		strcpy(comp_bc_parm, p);
		if	( idx != kwPROC && idx != kwFUNC )	{
			sc_raise("Use DECLARE with SUB or FUNC keyword");
			return;
			}
		}

	//////////////////////////////////////////
	if	( idx == kwREM )	return;
	sharp = (comp_bc_parm[0] == '#');			// if #  -> file commands
	ladd  = (strncmp(comp_bc_parm,"<<",2)==0);	// if << -> array, append
	linc  = (strncmp(comp_bc_parm,"++",2)==0);	// 
	ldec  = (strncmp(comp_bc_parm,"--",2)==0);	// 
	if	( comp_bc_parm[1] == '=' && strchr("-+/\\*^%&|", comp_bc_parm[0]) )	
		leqop = comp_bc_parm[0];
	else
		leqop = 0;

	if	( (comp_bc_parm[0] == '=' || 
			ladd || linc || ldec || leqop
			) && (idx != -1) )	{

		sc_raise("%s: is keyword (left side of expression)", comp_bc_name);
		return;
		}
	else if	( (idx == kwCONST)
			|| (
				(comp_bc_parm[0] == '=' || comp_bc_parm[0] == '(' ||
				ladd || linc || ldec || leqop
				) 
				&& (idx == -1) 
				)
			)	{
		//
		//	LET/CONST commands
		//
		char	*parms = comp_bc_parm;

		if	( idx == kwCONST )	{
			p = (char *) comp_next_word(comp_bc_parm, comp_bc_name);
			p = get_param_sect(p, ":", comp_bc_parm);
			parms = comp_bc_parm;
			bc_add_code(&comp_prog, kwCONST);
			}
		else if	( ladd )	{
			bc_add_code(&comp_prog, kwAPPEND);
			parms += 2;
			}
		else if ( linc )	{
			bc_add_code(&comp_prog, kwLET);
			strcpy(comp_bc_parm, "=");
			strcat(comp_bc_parm, comp_bc_name);
			strcat(comp_bc_parm, "+1");
			}
		else if ( ldec )	{
			bc_add_code(&comp_prog, kwLET);
			strcpy(comp_bc_parm, "=");
			strcat(comp_bc_parm, comp_bc_name);
			strcat(comp_bc_parm, "-1");
			}
		else if ( leqop )	{
			char	*buf;
			int		l;

			bc_add_code(&comp_prog, kwLET);
			l = strlen(comp_bc_parm)+strlen(comp_bc_name)+1;
			buf = tmp_alloc(l);
			memset(buf, 0, l);
			strcpy(buf, "=");
			strcat(buf, comp_bc_name);
			buf[strlen(buf)] = leqop;
			strcat(buf, comp_bc_parm+2);

			strcpy(comp_bc_parm, buf);
			tmp_free(buf);
			}
		else
			bc_add_code(&comp_prog, kwLET);


		comp_error_if_keyword(comp_bc_name);

		//
		bc_add_code(&comp_prog, kwTYPE_VAR);
		bc_add_addr(&comp_prog, comp_var_getID(comp_bc_name));

		if	( !comp_error )	{
			if	( parms[0] == '(' )	{
				char	*p = strchr(parms, '=');
			
				if	( !p )	
					sc_raise("LET/CONST/APPEND: Missing '='");
				else	{
					if	( *comp_next_char(parms+1) == ')' )	{
						// its the variable's name only
						comp_expression(p, 0);
						}
					else	{
						// ARRAY (LEFT)
						*p = '\0';
						comp_array_params(parms);

						*p = '=';
						if	( !comp_error )	{
							bc_add_code(&comp_prog, kwTYPE_CMPOPR);
							bc_add_code(&comp_prog, '=');
							comp_expression(p+1, 0);
							}
						}
					}
				}
			else	{
				bc_add_code(&comp_prog, kwTYPE_CMPOPR);
				bc_add_code(&comp_prog, '=');
				comp_expression(parms+1, 0);
				}
			}
		}
	else {
		// add generic command
		if	( idx != -1 )	{
			if	( idx == kwLABEL )	{
				str_alltrim(comp_bc_parm);
				idx = comp_label_getID(comp_bc_parm);
				comp_label_setip(idx);
				}

			else if	( idx == kwEXIT )	{
				bc_add_code(&comp_prog, idx);
				str_alltrim(comp_bc_parm);
				if	( strlen(comp_bc_parm) && comp_bc_parm[0] != '\'' )	{
					idx = comp_is_special_operator(comp_bc_parm);
					if	( idx == kwFORSEP || idx == kwLOOPSEP || idx == kwPROCSEP || idx == kwFUNCSEP )	
						bc_add_code(&comp_prog, idx);
					else
						sc_raise("Use EXIT [FOR|LOOP|SUB|FUNC]");
					}
				else
					bc_add_code(&comp_prog, 0);
				}

			else if	( idx == kwDECLARE )	{
				}

			else if	( idx == kwPROC || idx == kwFUNC )	{
				//
				// 	USER-DEFINED PROCEDURES/FUNCTIONS
				//
				#if defined(OS_LIMITED)
				char_p_t pars[32];
				#else
				char_p_t pars[256];
				#endif
				char	*lpar_ptr, *eq_ptr;
				int		i, count, pidx;

				// single-line function (DEF FN)
				if	( (eq_ptr   = strchr(comp_bc_parm, '=')) )		*eq_ptr = '\0';

				// parameters start
				if	( (lpar_ptr = strchr(comp_bc_parm, '(')) )		*lpar_ptr = '\0';

				comp_prepare_name(pname, baseof(comp_bc_parm, '/'), SB_KEYWORD_SIZE);
				comp_error_if_keyword(baseof(comp_bc_parm, '/'));

				if	( decl )	{
					// its only a declaration (DECLARE)
					if	( comp_udp_getip(pname) == INVALID_ADDR )
						comp_add_udp(pname);
					}
				else	{
					// func/sub
					if	( comp_udp_getip(pname) != INVALID_ADDR )
						sc_raise("The SUB/FUNC %s is already defined", pname);
					else {

						// setup routine's address (and get an id)
						if	( (pidx = comp_udp_setip(pname, comp_prog.count)) == -1 )	{
							pidx = comp_add_udp(pname);
							comp_udp_setip(pname, comp_prog.count);
							}

						// put JMP to the next command after the END 
						// (now we just keep the rq space, pass2 will update that)
						bc_add_code(&comp_prog, kwGOTO);
						bc_add_addr(&comp_prog, 0);
						bc_add_code(&comp_prog, 0);

						comp_block_level ++; comp_block_id ++;
						comp_push(comp_prog.count);						// keep it in stack for 'pass2'
						bc_add_code(&comp_prog, idx);		// store (FUNC/PROC) code

						// func/proc name (also, update comp_bc_proc)
						if	( comp_proc_level )	{
							strcat(comp_bc_proc, "/");
							strcat(comp_bc_proc, baseof(pname, '/'));
//							printf("comp_bc_proc=%s ... %s\n", comp_bc_proc, pname);
							}
						else	{
							strcpy(comp_bc_proc, pname);
//							printf("comp_bc_proc=%s\n", comp_bc_proc);
							}

						if	( !comp_error )	{
							comp_proc_level ++;

							// if its a function,
							// setup the code for the return-value (vid={F}/{F})
							if	( idx == kwFUNC )	{
								strcpy(comp_bc_tmp2, baseof(pname, '/'));
								comp_udptable[pidx].vid = comp_var_getID(comp_bc_tmp2);
								}
							else
								// procedure, no return value here
								comp_udptable[pidx].vid = INVALID_ADDR;	

							// parameters
							if	( lpar_ptr )	{
								*lpar_ptr = '(';
								#if defined(OS_LIMITED)
								comp_getlist_insep(comp_bc_parm, pars, "()", ",", 32, &count);
								#else
								comp_getlist_insep(comp_bc_parm, pars, "()", ",", 256, &count);
								#endif
								bc_add_code(&comp_prog, kwTYPE_PARAM);
								bc_add_code(&comp_prog, count);

								for ( i = 0; i < count; i ++ )	{
									if	( 
										( strncmp(pars[i], "BYREF ", 6) == 0 ) ||
										( pars[i][0] == '@' )	)	{

										if	( pars[i][0] == '@' )
											comp_prepare_name(vname, pars[i]+1, SB_KEYWORD_SIZE);
										else
											comp_prepare_name(vname, pars[i]+6, SB_KEYWORD_SIZE);

										vattr = 0x80;
										}
									else	{
										comp_prepare_name(vname, pars[i], SB_KEYWORD_SIZE);
										vattr = 0;
										}
										
									if	( strchr(pars[i], '(') )	{
										vattr |= 1;
										}

									bc_add_code(&comp_prog, vattr);
									bc_add_addr(&comp_prog, comp_var_getID(vname));
									}
								}
							else	{
								// no parameters
								bc_add_code(&comp_prog, kwTYPE_PARAM);	// params
								bc_add_code(&comp_prog, 0);				// pcount = 0
								}

							bc_eoc(&comp_prog); // EOC

							// -----------------------------------------------
							// scan for single-line function (DEF FN format)
							if	( eq_ptr && idx == kwFUNC )	{
								eq_ptr ++;	// *eq_ptr was '\0'
								while ( *eq_ptr == ' ' || *eq_ptr == '\t' )	eq_ptr ++;

								if	( strlen(eq_ptr) )	{
									char	*macro;

									macro = tmp_alloc(SB_SOURCELINE_SIZE+1);
									sprintf(macro, "%s=%s:END", pname, eq_ptr);

									//	run comp_text_line again
									comp_text_line(macro);
									tmp_free(macro);
									}
								else
									sc_raise("FUNC/DEF it has an '=' on declaration, but I didn't found the body!");
								}
							}
						}
					}
				}
			else if	( idx == kwLOCAL )	{	// local variables
				#if defined(OS_LIMITED)
				char_p_t pars[32];
				#else
				char_p_t pars[256];
				#endif
				int		i, count;

				#if defined(OS_LIMITED)
				count = comp_getlist(comp_bc_parm, pars, ",", 32);
				#else
				count = comp_getlist(comp_bc_parm, pars, ",", 256);
				#endif
				bc_add_code(&comp_prog, kwTYPE_CRVAR);
				bc_add_code(&comp_prog, count);
				for ( i = 0; i < count; i ++ )	{
					comp_prepare_name(vname, pars[i], SB_KEYWORD_SIZE);
					bc_add_addr(&comp_prog, comp_var_getID(vname));
					}
				}
			else if ( idx == kwREM )	
				return;
			else if ( idx == kwEXPORT )	{ // export
				if	( comp_unit_flag )
					bc_store_exports(comp_bc_parm);
				else
					sc_raise("EXPORT: Unit name is not defined");
				}
			else if ( idx == kwOPTION )		
				comp_cmd_option(comp_bc_parm);
			else if ( idx == kwGOTO )	{
				str_alltrim(comp_bc_parm);
				comp_push(comp_prog.count);
				bc_add_code(&comp_prog, idx);
				bc_add_addr(&comp_prog, comp_label_getID(comp_bc_parm));
				bc_add_code(&comp_prog, comp_block_level);
				}
			else if ( idx == kwGOSUB )	{
				str_alltrim(comp_bc_parm);
				bc_add_code(&comp_prog, idx);
				bc_add_addr(&comp_prog, comp_label_getID(comp_bc_parm));
				}
			//
			//	IF
			//
			else if ( idx == kwIF )	{
				strcpy(comp_do_close_cmd, "ENDIF");

				// from here, we can scan for inline IF
				if	( comp_single_line_if(last_cmd) )	{
					// inline-IFs
					return;
					}
				else	{
					comp_block_level ++; comp_block_id ++;
					comp_push(comp_prog.count);	
					bc_add_ctrl(&comp_prog, idx, 0, 0);
					comp_expression(comp_bc_parm, 0);
					bc_add_code(&comp_prog, kwTYPE_EOC);	//bc_eoc();
					}
				}
			//
			//	ON x GOTO|GOSUB ...
			//
			else if ( idx == kwON )	{
				char	*p;
				int		keep_ip, count;
				#if defined(OS_LIMITED)
				char_p_t pars[32];
				#else
				char_p_t pars[256];
				#endif
				int		i;
			
				idx = kwONJMP;		// WARNING!

				comp_push(comp_prog.count);
				bc_add_ctrl(&comp_prog, idx, 0, 0);

				if	( (p = strstr(comp_bc_parm, " GOTO ")) != NULL )	{
					bc_add_code(&comp_prog, kwGOTO);	// the command
					*p = '\0'; p += 6;
					keep_ip = comp_prog.count;
					bc_add_code(&comp_prog, 0);			// the counter

					//count = bc_scan_label_list(p);
					#if defined(OS_LIMITED)
					count = comp_getlist(p, pars, ",", 32);
					#else
					count = comp_getlist(p, pars, ",", 256);
					#endif
					for ( i = 0; i < count; i ++ )	
						bc_add_addr(&comp_prog, comp_label_getID(pars[i]));	// IDs

					if	( count == 0 )
						sc_raise("ON x GOTO WHERE?");
					else	
						comp_prog.ptr[keep_ip] = count;

					comp_expression(comp_bc_parm, 0);	// the expression
					bc_eoc(&comp_prog);
					}
				else if	( (p = strstr(comp_bc_parm, " GOSUB ")) != NULL )	{
					bc_add_code(&comp_prog, kwGOSUB);	// the command
					*p = '\0';	p += 7;
					keep_ip = comp_prog.count;
					bc_add_code(&comp_prog, 0);			// the counter

					//count = bc_scan_label_list(p);
					#if defined(OS_LIMITED)
					count = comp_getlist(p, pars, ",", 32);
					#else
					count = comp_getlist(p, pars, ",", 256);
					#endif
					for ( i = 0; i < count; i ++ )	
						bc_add_addr(&comp_prog, comp_label_getID(pars[i]));

					if	( count == 0 )
						sc_raise("ON x GOSUB WHERE?");
					else	
						comp_prog.ptr[keep_ip] = count;

					comp_expression(comp_bc_parm, 0);	// the expression
					bc_eoc(&comp_prog);
					}
				else
					sc_raise("ON WHAT?");
				}
			//
			//	FOR
			//
			else if ( idx == kwFOR )	{
				char	*p = strchr(comp_bc_parm, '=');
				char	*p_do = strstr(comp_bc_parm, " DO ");
				char	*p_lev;
				char	*n;

				// fix DO bug
				if	( p_do )	{
					if	( p > p_do )
						p = NULL;
					}

				//
				strcpy(comp_do_close_cmd, "NEXT");

				comp_block_level ++; comp_block_id ++;
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, kwFOR, 0, 0);

				if	( !p )	{
					// FOR [EACH] X IN Y
					if	( (p = strstr(comp_bc_parm, " IN ")) == NULL )
						sc_raise("FOR: Missing '=' OR 'IN'");
					else	{
						*p = '\0';
						n = p;
						strcpy(comp_bc_name, comp_bc_parm);
						str_alltrim(comp_bc_name);
						if	( !is_alpha(*comp_bc_name) )
							sc_raise("FOR: %s is not a variable", comp_bc_name);
						else	{
							p_lev = comp_bc_name;
							while ( is_alnum(*p_lev) || *p_lev == ' ' )	p_lev ++;
							if	( *p_lev == '(' )	
								sc_raise("FOR: %s is an array. Arrays are not allowed", comp_bc_name);
							else	{
								if	( !comp_error_if_keyword(comp_bc_name) )	{
									bc_add_code(&comp_prog, kwTYPE_VAR);
									bc_add_addr(&comp_prog, comp_var_getID(comp_bc_name));

									*n = ' ';
									bc_add_code(&comp_prog, kwIN);
									comp_expression(n+4, 0);
									}
								}
							}
						}
					}
				else	{
					// FOR X=Y TO Z [STEP L]
					*p = '\0';
					n = p;

					strcpy(comp_bc_name, comp_bc_parm);
					str_alltrim(comp_bc_name);
					if	( !is_alpha(*comp_bc_name) )
						sc_raise("FOR: %s is not a variable", comp_bc_name);
					else	{
						p_lev = comp_bc_name;
						while ( is_alnum(*p_lev) || *p_lev == ' ' )	p_lev ++;
						if	( *p_lev == '(' )	
							sc_raise("FOR: %s is an array. Arrays are not allowed", comp_bc_name);
						else	{
							if	( !comp_error_if_keyword(comp_bc_name) )	{
								bc_add_code(&comp_prog, kwTYPE_VAR);
								bc_add_addr(&comp_prog, comp_var_getID(comp_bc_name));

								*n = '=';
								comp_expression(n+1, 0);
								}
							}
						}
					}
				}
			//
			//	WHILE - REPEAT
			//
			else if ( idx == kwWHILE )	{
				strcpy(comp_do_close_cmd, "WEND");

				comp_block_level ++; comp_block_id ++;
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, idx, 0, 0);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwREPEAT )	{
				// WHILE & REPEAT DOES NOT USE STACK 
				comp_block_level ++; comp_block_id ++;
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, idx, 0, 0);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwELSE || idx == kwELIF ) {
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, idx, 0, 0);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwENDIF || idx == kwNEXT 
					|| (idx == kwEND && strncmp(comp_bc_parm, "IF", 2) == 0) )	{
				if	(idx == kwEND )		idx = kwENDIF;
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, idx, 0, 0);
				comp_block_level --;
				}
			else if ( idx == kwWEND || idx == kwUNTIL )	{
				comp_push(comp_prog.count);	
				bc_add_ctrl(&comp_prog, idx, 0, 0);
				comp_block_level --;
				comp_expression(comp_bc_parm, 0);
				}
			else if (  idx == kwSTEP || idx == kwTO || idx == kwIN || idx == kwTHEN 
					|| idx == kwCOS  || idx == kwSIN || idx == kwLEN || idx == kwLOOP  )		// functions...
				{
				sc_raise("%s: Wrong position", comp_bc_name);
				}
			else if ( idx == kwRESTORE )	{
				comp_push(comp_prog.count);
				bc_add_code(&comp_prog, idx);
				bc_add_addr(&comp_prog, comp_label_getID(comp_bc_parm));
				}
			else if ( idx == kwEND )	{
				if	( comp_proc_level )	{
					char	*dol;

					// UDP/F RETURN
					dol = strrchr(comp_bc_proc, '/');
					if	( dol )
						*dol = '\0';
					else
						*comp_bc_proc = '\0';

					comp_push(comp_prog.count);
					bc_add_code(&comp_prog, kwTYPE_RET);

					comp_proc_level --;
					comp_block_level --;
					comp_block_id ++;
					}
				else	{
					// END OF PROG
					bc_add_code(&comp_prog, idx);
					}
				}
			else if ( idx == kwDATA )	
				comp_data_seg(comp_bc_parm);
			else if ( idx == kwREAD && sharp )	{
				bc_add_code(&comp_prog, kwFILEREAD);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwINPUT && sharp )	{
				bc_add_code(&comp_prog, kwFILEINPUT);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwPRINT && sharp )	{
				bc_add_code(&comp_prog, kwFILEPRINT);
				comp_expression(comp_bc_parm, 0);
				}
			else if ( idx == kwLINE  && strncmp(comp_bc_parm, "INPUT ", 6) == 0  )	{
				bc_add_code(&comp_prog, kwLINEINPUT);
				comp_expression(comp_bc_parm+6, 0);
				}
			else	{
				// something else
				bc_add_code(&comp_prog, idx);
				comp_expression(comp_bc_parm, 0);
				}
			}
		else	{
			/*
			*	EXTERNAL OR USER-DEFINED PROCEDURE
			*/
			int		udp;

			udp = comp_is_external_proc(comp_bc_name);
			if	( udp > -1 )	{
//				if	( comp_extproctable[udp].lib_id & UID_UNIT_BIT )	// store call to symbol-index
//					bc_add_extpcode(&comp_prog, comp_extproctable[udp].lib_id, comp_extproctable[udp].symbol_index);
//				else													// store call to procedure-index
//					bc_add_extpcode(&comp_prog, comp_extproctable[udp].lib_id, udp);
				bc_add_extpcode(&comp_prog, comp_extproctable[udp].lib_id, comp_extproctable[udp].symbol_index);

				bc_add_code(&comp_prog, kwTYPE_LEVEL_BEGIN);
				comp_expression(comp_bc_parm, 0);
				bc_add_code(&comp_prog, kwTYPE_LEVEL_END);
				}
			else	{
				udp = comp_udp_id(comp_bc_name, 1);
				if	( udp == -1 )
					udp = comp_add_udp(comp_bc_name);

				comp_push(comp_prog.count);
				bc_add_ctrl(&comp_prog, kwTYPE_CALL_UDP, udp, 0);
				bc_add_code(&comp_prog, kwTYPE_LEVEL_BEGIN);
				comp_expression(comp_bc_parm, 0);
				bc_add_code(&comp_prog, kwTYPE_LEVEL_END);
				}
			}
		}

	///////////////////////////////////////
	if	( *p == ':' )	{
		// command separator
		bc_eoc(&comp_prog);
		p ++;
		comp_text_line(p);
		}
}

/*
*	skip command bytes
*/
addr_t	comp_next_bc_cmd(addr_t ip) SEC(BCSC3);
addr_t	comp_next_bc_cmd(addr_t ip)
{
	code_t	code;
	#if	defined(ADDR16)
	word	len;
	#else
	dword	len;
	#endif

	code = comp_prog.ptr[ip];
	ip ++;

	switch ( code )	{
	case	kwTYPE_INT:				// integer
		ip += OS_INTSZ;
		break;
	case	kwTYPE_NUM:				// number
		ip += OS_REALSZ;
		break;
	case	kwTYPE_STR:				// string: [2/4B-len][data]
		memcpy(&len, comp_prog.ptr+ip, OS_STRLEN);
		len += OS_STRLEN;
		ip  += len;
		break;
	case	kwTYPE_CALLF:
	case	kwTYPE_CALLP:			// [fcode_t]
		ip += CODESZ;		
		break;
	case	kwTYPE_CALLEXTF:
	case	kwTYPE_CALLEXTP:		// [lib][index]
		ip += (ADDRSZ * 2);
		break;
	case	kwEXIT:
	case	kwTYPE_SEP:
	case	kwTYPE_LOGOPR:
	case	kwTYPE_CMPOPR:
	case	kwTYPE_ADDOPR:
	case	kwTYPE_MULOPR:
	case	kwTYPE_POWOPR:
	case	kwTYPE_UNROPR:			// [1B data]
		ip ++;				
		break;
	case	kwRESTORE:
	case	kwGOSUB:
	case	kwTYPE_LINE:
	case	kwTYPE_VAR:				// [addr|id]
		ip += ADDRSZ;		
		break;
	case	kwTYPE_CALL_UDP:
	case	kwTYPE_CALL_UDF:		// [true-ip][false-ip]
		ip += BC_CTRLSZ;	
		break;
	case	kwGOTO:					// [addr][pop-count]
		ip += (ADDRSZ+1);	
		break;
	case	kwTYPE_CRVAR:			// [1B count][addr1][addr2]...
		len = comp_prog.ptr[ip];	
		ip += ((len * ADDRSZ) + 1);
		break;
	case	kwTYPE_PARAM:			// [1B count] {[1B-pattr][addr1]} ...
		len = comp_prog.ptr[ip];
		ip += ((len * (ADDRSZ+1)) + 1);
		break;
	case	kwONJMP:				// [true-ip][false-ip] [GOTO|GOSUB] [count] [addr1]...
		ip += (BC_CTRLSZ+1);
		ip += (comp_prog.ptr[ip] * ADDRSZ);
		break;
	case	kwOPTION:				// [1B-optcode][addr-data]
		ip += (ADDRSZ+1);	
		break;
	case	kwIF:		case	kwFOR: 		case	kwWHILE:	case	kwREPEAT:
	case	kwELSE:		case	kwELIF:
	case	kwENDIF:	case	kwNEXT:		case	kwWEND:		case	kwUNTIL:
	case	kwUSE:
		ip += BC_CTRLSZ;
		break;
		};
	return ip;
}

/*
*	search for command (in byte-code)
*/
addr_t	comp_search_bc(addr_t ip, code_t code)
{
	addr_t	i = ip;

	do	{
		if	( code == comp_prog.ptr[i] )
			return i;

		i = comp_next_bc_cmd(i);
		} while ( i < comp_prog.count );
	return INVALID_ADDR;
}

/*
*	search for End-Of-Command mark
*/
addr_t	comp_search_bc_eoc(addr_t ip) SEC(BCSC3);
addr_t	comp_search_bc_eoc(addr_t ip)
{
	addr_t	i = ip;
	code_t	code;

	do	{
		code = comp_prog.ptr[i];
		if	( code == kwTYPE_EOC || code == kwTYPE_LINE )
			return i;

		i = comp_next_bc_cmd(i);
		} while ( i < comp_prog.count );
	return comp_prog.count;
}

/*
*	search stack
*/
addr_t	comp_search_bc_stack(addr_t start, code_t code, int level) SEC(BCSC3);
addr_t	comp_search_bc_stack(addr_t start, code_t code, int level)
{
	addr_t		i;
	comp_pass_node_t	node;

	for ( i = start; i < comp_sp; i ++ )	{
		dbt_read(comp_stack, i, &node, sizeof(comp_pass_node_t));

		if	( comp_prog.ptr[node.pos] == code )	{
			if	( node.level == level )	
				return node.pos;
			}
		}
	return INVALID_ADDR;
}

/*
*	search stack backward
*/
addr_t	comp_search_bc_stack_backward(addr_t start, code_t code, int level) SEC(BCSC3);
addr_t	comp_search_bc_stack_backward(addr_t start, code_t code, int level)
{
	addr_t		i = start;
	comp_pass_node_t	node;

	for ( ; i < comp_sp; i -- )	{	// WARNING: ITS UNSIGNED, SO WE'LL SEARCH IN RANGE [0..STK_COUNT]
		dbt_read(comp_stack, i, &node, sizeof(comp_pass_node_t));
		if	( comp_prog.ptr[node.pos] == code )	{
			if	( node.level == level )	
				return node.pos;
			}
		}
	return INVALID_ADDR;
}

/*
*	Advanced error messages:
*	Analyze LOOP-END errors
*/
void	print_pass2_stack(addr_t pos, code_t lcode, int level) SEC(BCSC2);
void	print_pass2_stack(addr_t pos, code_t lcode, int level)
{
	#if !defined(OS_LIMITED)
	addr_t	ip;
	#endif
	addr_t	i;
	int		j, cs_idx;
	char	cmd[16], cmd2[16];
	comp_pass_node_t	node;
	code_t	ccode[256], code;
	int		csum[256];
	int		cs_count;
	code_t	start_code[] =
	{ kwWHILE, kwREPEAT, kwIF,    kwFOR,  kwFUNC, 0 };
	code_t	end_code[] =
	{ kwWEND,  kwUNTIL,  kwENDIF, kwNEXT, kwTYPE_RET, 0 };
#if !defined(OS_LIMITED)
	int		details = 1;
#endif

	code = lcode;

	kw_getcmdname(code, cmd);

	/*
	*	search for closest keyword (forward)
	*/	
#if !defined(OS_LIMITED)
	fprintf(stderr, "Detailed report (y/N) ?");
	details = (fgetc(stdin) == 'y');
	if	( details )	{
		ip = comp_search_bc_stack(pos+1, code, level - 1);
		if	( ip == INVALID_ADDR )	{
			ip = comp_search_bc_stack(pos+1, code, level + 1);
			if	( ip == INVALID_ADDR )	{
				int		cnt = 0;

				for ( i = pos+1; i < comp_sp; i ++ )	{
					dbt_read(comp_stack, i, &node, sizeof(comp_pass_node_t));

					if	( comp_prog.ptr[node.pos] == code )	{
						fprintf(stderr, "\n%s found on level %d (@%d) instead of %d (@%d+)\n", cmd, node.level, node.pos, level, pos);
						cnt ++;
						if	( cnt > 3 )
							break;
						}
					}
				}
			else
				fprintf(stderr, "\n%s found on level %d (@%d) instead of %d (@%d+)\n", cmd, level+1, node.pos, level, pos);
			}
		else
			fprintf(stderr, "\n%s found on level %d (@%d) instead of %d (@%d+)\n", cmd, level-1, node.pos, level, pos);
		}
#endif

	/*
	*	print stack
	*/
	cs_count = 0;
#if !defined(OS_LIMITED)
	if	( details )	{
		fprintf(stderr, "\n");
		fprintf(stderr, "--- Pass 2 - stack ------------------------------------------------------\n");
		fprintf(stderr, "%s%4s  %16s %16s %6s %6s %5s %5s %5s\n", "  ", "   i", "Command", "Section", "Addr", "Line", "Level", "BlkID", "Count");
		fprintf(stderr, "-------------------------------------------------------------------------\n");
		}
#endif
	for ( i = 0; i < comp_sp; i ++ )	{
		dbt_read(comp_stack, i, &node, sizeof(comp_pass_node_t));

		code = comp_prog.ptr[node.pos];
		if	( node.pos != INVALID_ADDR )	
			kw_getcmdname(code, cmd);
		else
			strcpy(cmd, "---");

		// sum
		cs_idx = -1;
		for ( j = 0; j < cs_count; j ++ )	{
			if	( ccode[j] == code )	{
				cs_idx = j;
				csum[cs_idx] ++;
				break;
				}
			}
		if	( cs_idx == -1 )	{
			cs_idx = cs_count;
			cs_count ++;
			ccode[cs_idx] = code;
			csum[cs_idx] = 1;
			}

#if !defined(OS_LIMITED)
		if	( details )	{
			// info
			fprintf(stderr, "%s%4d: %16s %16s %6d %6d %5d %5d %5d\n",
				((i==pos)?">>":"  "), i, cmd, node.sec, node.pos, node.line, node.level, node.block_id, csum[cs_idx]);
			}
#endif
		}

	/*
	*	sum
	*/
#if !defined(OS_LIMITED)
	if	( details )	{
		fprintf(stderr, "\n");
		fprintf(stderr, "--- Sum -----------------------------------------------------------------\n");
		for ( i = 0; i < cs_count; i ++ )	{
			code = ccode[i];
			if	( !kw_getcmdname(code, cmd) )	sprintf(cmd, "(%d)", code);
			fprintf(stderr, "%16s - %5d\n", cmd, csum[i]);
			}
		}
#endif

	/*
	*	decide
	*/
#if !defined(OS_LIMITED)
	fprintf(stderr, "\n");
#else
	dev_printf("\n");
#endif
	for ( i = 0; start_code[i] != 0; i ++ )	{
		int		sa, sb;
		code_t	ca, cb;

		ca = start_code[i];
		cb = end_code[i];

		sa = 0;
		for ( j = 0; j < cs_count; j ++ )	{
			if	( ccode[j] == ca )	
				sa = csum[j];
			if	( ca == kwFUNC )	{
				if	( ccode[j] == kwPROC )	
					sa += csum[j];
				}
			}

		sb = 0;
		for ( j = 0; j < cs_count; j ++ )	{
			if	( ccode[j] == cb )	{
				sb = csum[j];
				break;
				}
			}

		if	( sa - sb != 0 )	{
			kw_getcmdname(ca, cmd);
			kw_getcmdname(cb, cmd2);
			if	( sa > sb )	
#if !defined(OS_LIMITED)
				fprintf(stderr, "Hint: Missing %d %s or there is/are %d more %s\n", sa - sb, cmd2, sa - sb, cmd);
#else
				dev_printf("Hint: Missing %d %s or there is/are %d more %s\n", sa - sb, cmd2, sa - sb, cmd);
#endif
			else	
#if !defined(OS_LIMITED)
				fprintf(stderr, "Hint: There is/are %d more %s or missing %d %s\n", sb - sa, cmd2, sb-sa, cmd);
#else
				dev_printf("Hint: There is/are %d more %s or missing %d %s\n", sb - sa, cmd2, sb-sa, cmd);
#endif
			}
		}
	
#if !defined(OS_LIMITED)
	fprintf(stderr, "\n");
#else
	dev_printf("\n\n");
#endif
}

/*
*	PASS 2 (write jumps for IF,FOR,WHILE,REPEAT,etc)
*/
void	comp_pass2_scan(void) SEC(BCSC3);
void	comp_pass2_scan()
{
	addr_t	i = 0, j, true_ip, false_ip, label_id, w;
	addr_t	a_ip, b_ip, c_ip, count;
	code_t	code;
	byte	level;
	comp_pass_node_t	node;
	comp_label_t	label;

	if	( !opt_quite && !opt_interactive )	{
		#if defined(_UnixOS)
		if ( isatty (STDOUT_FILENO) ) 
		#endif
		dev_printf("\rPASS2: Node %d/%d", i, comp_sp);
		}

	// for each node in stack
	for ( i = 0; i < comp_sp; i ++ )	{

		if	( !opt_quite && !opt_interactive )	{
			#if defined(_UnixOS)
			if ( isatty (STDOUT_FILENO) ) 
			#endif
			if	( (i % SB_KEYWORD_SIZE) == 0 )
				dev_printf("\rPASS2: Node %d/%d", i, comp_sp);
			}

		dbt_read(comp_stack, i, &node, sizeof(comp_pass_node_t));

		comp_line = node.line;
		strcpy(comp_bc_sec, node.sec);
		code = comp_prog.ptr[node.pos];

		if	( code == kwTYPE_EOC || code == kwTYPE_LINE )
			continue;

//		debug (node.pos = the address of the error)
//
//		if	( node.pos == 360 || node.pos == 361 )
//			printf("=== stack code %d\n", code);

		if	(  code != kwGOTO && code != kwRESTORE && code != kwONJMP 
		    && code != kwTYPE_CALL_UDP && code != kwTYPE_CALL_UDF 
			&& code != kwPROC && code != kwFUNC && code != kwTYPE_RET )	{

			// default - calculate true-ip
			true_ip = comp_search_bc_eoc(node.pos+(BC_CTRLSZ+1));
			memcpy(comp_prog.ptr+node.pos+1, &true_ip, ADDRSZ);
			}

		switch ( code )	{
		case kwPROC:
		case kwFUNC:
			// update start's GOTO
			true_ip = comp_search_bc_stack(i+1, kwTYPE_RET, node.level) + 1;
			if	( true_ip == INVALID_ADDR )	{
				sc_raise("SUB/FUNC: Missing END on the same level");
				print_pass2_stack(i, kwTYPE_RET, node.level);
				return;
				}
			memcpy(comp_prog.ptr+node.pos-(ADDRSZ+1), &true_ip, ADDRSZ);
			break;

		case kwRESTORE:
			// replace the label ID with the real IP
			memcpy(&label_id, comp_prog.ptr+node.pos+1, ADDRSZ);
			dbt_read(comp_labtable, label_id, &label, sizeof(comp_label_t));
			count = comp_first_data_ip + label.dp;
			memcpy(comp_prog.ptr+node.pos+1, &count, ADDRSZ);	// change LABEL-ID with DataPointer
			break;

		case kwTYPE_CALL_UDP:
		case kwTYPE_CALL_UDF:
			// update real IP
			memcpy(&label_id, comp_prog.ptr+node.pos+1, ADDRSZ);
			true_ip = comp_udptable[label_id].ip + (ADDRSZ+3);
			memcpy(comp_prog.ptr+node.pos+1, &true_ip, ADDRSZ);	

			// update return-var ID
			true_ip = comp_udptable[label_id].vid;
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &true_ip, ADDRSZ);	
			break;

		case kwONJMP:
			// kwONJMP:1 trueip:2 falseip:2 command:1 count:1 label1:2 label2:2 ...
			count = comp_prog.ptr[node.pos+(ADDRSZ+ADDRSZ+2)];

			true_ip = comp_search_bc_eoc(node.pos+BC_CTRLSZ+(count*ADDRSZ)+3);
			memcpy(comp_prog.ptr+node.pos+1, &true_ip, ADDRSZ);

			// change label IDs with the real IPs
			for ( j = 0; j < count; j ++ )	{
				memcpy(&label_id, comp_prog.ptr+node.pos+(j*ADDRSZ)+(ADDRSZ+ADDRSZ+3), ADDRSZ);
				dbt_read(comp_labtable, label_id, &label, sizeof(comp_label_t));
				w = label.ip;
				memcpy(comp_prog.ptr+node.pos+(j*ADDRSZ)+(ADDRSZ+ADDRSZ+3), &w, ADDRSZ);
				}
			break;

		case kwGOTO:	// LONG JUMPS
			memcpy(&label_id, comp_prog.ptr+node.pos+1, ADDRSZ);
			dbt_read(comp_labtable, label_id, &label, sizeof(comp_label_t));
			w = label.ip;
			memcpy(comp_prog.ptr+node.pos+1, &w, ADDRSZ);	// change LABEL-ID with IP
			level = comp_prog.ptr[node.pos+(ADDRSZ+1)];
			comp_prog.ptr[node.pos+(ADDRSZ+1)] = 0; // number of POPs ?????????

			if	( level >= label.level )
				comp_prog.ptr[node.pos+(ADDRSZ+1)] = level - label.level; // number of POPs
			else	
				comp_prog.ptr[node.pos+(ADDRSZ+1)] = 0; // number of POPs ?????????

			break;
		case kwFOR:
			a_ip = comp_search_bc(node.pos+(ADDRSZ+ADDRSZ+1), kwTO);
			b_ip = comp_search_bc(node.pos+(ADDRSZ+ADDRSZ+1), kwIN);
			if	( a_ip < b_ip )
				b_ip = INVALID_ADDR;
			else if	( a_ip > b_ip )	
				a_ip = b_ip;

			false_ip = comp_search_bc_stack(i+1, kwNEXT, node.level);

			if	( false_ip == INVALID_ADDR )	{
				sc_raise("FOR: Missing NEXT on the same level");
				print_pass2_stack(i, kwNEXT, node.level);
				return;
				}
			if	( a_ip > false_ip || a_ip == INVALID_ADDR )	{
				if	( b_ip != INVALID_ADDR )
					sc_raise("FOR: Missing IN");
				else
					sc_raise("FOR: Missing TO");
				return;
				}

			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;
		case kwWHILE:
			false_ip = comp_search_bc_stack(i+1, kwWEND, node.level);

			if	( false_ip == INVALID_ADDR )	{
				sc_raise("WHILE: Missing WEND on the same level");
				print_pass2_stack(i, kwWEND, node.level);
				return;
				}

			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;

		case kwREPEAT:
			false_ip = comp_search_bc_stack(i+1, kwUNTIL, node.level);

			if	( false_ip == INVALID_ADDR )	{
				sc_raise("REPEAT: Missing UNTIL on the same level");
				print_pass2_stack(i, kwUNTIL, node.level);
				return;
				}

			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;

		case kwUSE:
			true_ip = node.pos+(ADDRSZ+ADDRSZ+1);
			false_ip = comp_search_bc_eoc(true_ip);
			memcpy(comp_prog.ptr+node.pos+1, &true_ip, ADDRSZ);
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;

		case kwIF:
		case kwELIF:
			a_ip = comp_search_bc_stack(i+1, kwENDIF, node.level);
			b_ip = comp_search_bc_stack(i+1, kwELSE,  node.level);
			c_ip = comp_search_bc_stack(i+1, kwELIF,  node.level);

			false_ip = a_ip;
			if	( b_ip != INVALID_ADDR && b_ip < false_ip ) false_ip = b_ip;
			if	( c_ip != INVALID_ADDR && c_ip < false_ip ) false_ip = c_ip;

			if	( false_ip == INVALID_ADDR )	{
				sc_raise("IF: Missing ELIF/ELSE/ENDIF on the same level");
				print_pass2_stack(i, kwENDIF, node.level);
				return;
				}

			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;

		case kwELSE:
			false_ip = comp_search_bc_stack(i+1, kwENDIF, node.level);

			if	( false_ip == INVALID_ADDR )	{
				sc_raise("(ELSE) IF: Missing ENDIF on the same level");
				print_pass2_stack(i, kwENDIF, node.level);
				return;
				}

			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;

/////
/////
/////

		case kwTYPE_RET:
			break;

		case kwWEND:
			false_ip = comp_search_bc_stack_backward(i-1, kwWHILE, node.level);
			if	( false_ip == INVALID_ADDR )	{
				sc_raise("WEND: Missing WHILE on the same level");
				print_pass2_stack(i, kwWHILE, node.level);
				return;
				}
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;
		case kwUNTIL:
			false_ip = comp_search_bc_stack_backward(i-1, kwREPEAT, node.level);
			if	( false_ip == INVALID_ADDR )	{
				sc_raise("UNTIL: Missing REPEAT on the same level");
				print_pass2_stack(i, kwREPEAT, node.level);
				return;
				}
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;
		case kwNEXT:
			false_ip = comp_search_bc_stack_backward(i-1, kwFOR, node.level);
			if	( false_ip == INVALID_ADDR )	{
				sc_raise("NEXT: Missing FOR on the same level");
				print_pass2_stack(i, kwFOR, node.level);
				return;
				}
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;
		case kwENDIF:
			false_ip = comp_search_bc_stack_backward(i-1, kwIF, node.level);
			if	( false_ip == INVALID_ADDR )	{
				sc_raise("ENDIF: Missing IF on the same level");
				print_pass2_stack(i, kwIF, node.level);
				return;
				}
			memcpy(comp_prog.ptr+node.pos+(ADDRSZ+1), &false_ip, ADDRSZ);
			break;
			};
		}

	if	( !opt_quite && !opt_interactive )
		dev_printf("\rPASS2: Node %d/%d\n", comp_sp, comp_sp);
}

/*
*	initialize compiler
*/
void	comp_init()
{
	comp_bc_sec  = tmp_alloc(SB_KEYWORD_SIZE+1);
	memset(comp_bc_sec, 0, SB_KEYWORD_SIZE+1);
	comp_bc_name = tmp_alloc(SB_SOURCELINE_SIZE+1);
	comp_bc_parm = tmp_alloc(SB_SOURCELINE_SIZE+1);
	comp_bc_temp = tmp_alloc(SB_SOURCELINE_SIZE+1);
	comp_bc_tmp2 = tmp_alloc(SB_SOURCELINE_SIZE+1);
	comp_bc_proc = tmp_alloc(SB_SOURCELINE_SIZE+1);

	comp_line = 0;
	comp_error = 0;
	comp_labcount = 0;
	comp_expcount = comp_impcount = comp_libcount = 0;
	comp_varcount = 0;
	comp_sp = 0;
	comp_udpcount = 0;
	comp_block_level = 0;
	comp_block_id = 0;
	comp_unit_flag = 0;
	comp_first_data_ip = INVALID_ADDR;
	comp_proc_level = 0;
	comp_bc_proc[0] = '\0';

	comp_vartable  = (comp_var_t*)       tmp_alloc(GROWSIZE * sizeof(comp_var_t));
	

	comp_udptable  = (comp_udp_t*)      tmp_alloc(GROWSIZE * sizeof(comp_udp_t));

	sprintf(comp_bc_temp, "SBI-LBL%d", ctask->tid);	comp_labtable = dbt_create(comp_bc_temp, 0);
	sprintf(comp_bc_temp, "SBI-STK%d", ctask->tid);	comp_stack    = dbt_create(comp_bc_temp, 0);
	sprintf(comp_bc_temp, "SBI-ILB%d", ctask->tid);	comp_libtable = dbt_create(comp_bc_temp, 0);
	sprintf(comp_bc_temp, "SBI-IMP%d", ctask->tid);	comp_imptable = dbt_create(comp_bc_temp, 0);
	sprintf(comp_bc_temp, "SBI-EXP%d", ctask->tid);	comp_exptable = dbt_create(comp_bc_temp, 0);

	comp_varsize   = comp_udpsize  = GROWSIZE;
	comp_varcount  = comp_labcount = comp_sp = comp_udpcount = 0;

	bc_create(&comp_prog);
	bc_create(&comp_data);

	#if	!defined(_UnixOS)
	if	(	!comp_vartable || 
			comp_imptable == -1 || 
			comp_libtable == -1 || 
			comp_exptable == -1 || 
			comp_labtable == - 1 || 
			!comp_udptable || 
			comp_stack == -1 )	
		panic("comp_init(): OUT OF MEMORY");
	#else
	assert(comp_vartable != 0); 
	assert(comp_imptable != -1);
	assert(comp_libtable != -1);
	assert(comp_exptable != -1);
	assert(comp_labtable != -1);
	assert(comp_udptable != 0);
	assert(comp_stack    != -1);	
	#endif

	dbt_prealloc(comp_labtable, os_cclabs1, sizeof(comp_label_t));
	dbt_prealloc(comp_stack, os_ccpass2, sizeof(comp_pass_node_t));

	/*
	*	create system variables
	*/
	comp_var_getID("OSVER");
	comp_vartable[comp_var_getID("OSNAME")].dolar_sup = 1;
	comp_var_getID("SBVER");
	comp_var_getID("PI");
	comp_var_getID("XMAX");
	comp_var_getID("YMAX");
	comp_var_getID("BPP");
	comp_var_getID("TRUE");
	comp_var_getID("FALSE");
	comp_var_getID("LINECHART");
	comp_var_getID("BARCHART");
	comp_vartable[comp_var_getID("CWD")].dolar_sup = 1;
	comp_vartable[comp_var_getID("HOME")].dolar_sup = 1;
	comp_vartable[comp_var_getID("COMMAND")].dolar_sup = 1;
	comp_var_getID("X");	// USE keyword
	comp_var_getID("Y");	// USE keyword
	comp_var_getID("Z");	// USE keyword
	comp_var_getID("VIDADR"); 
}

/*
*	clean up
*/
void	comp_close()
{
	int		i;

	bc_destroy(&comp_prog);
	bc_destroy(&comp_data);

	for ( i = 0; i < comp_varcount; i ++ )
		tmp_free(comp_vartable[i].name);
	for ( i = 0; i < comp_udpcount; i ++ )
		tmp_free(comp_udptable[i].name);

	tmp_free(comp_vartable);
	dbt_close(comp_labtable);
	dbt_close(comp_exptable);
	dbt_close(comp_imptable);
	dbt_close(comp_libtable);
	tmp_free(comp_udptable);
	dbt_close(comp_stack);

	comp_varcount = comp_labcount = comp_sp = comp_udpcount = 0;
	comp_libcount = comp_impcount = comp_expcount = 0;

	tmp_free(comp_bc_proc);
	tmp_free(comp_bc_tmp2);
	tmp_free(comp_bc_temp);
	tmp_free(comp_bc_parm);
	tmp_free(comp_bc_name);
	tmp_free(comp_bc_sec);
	comp_reset_externals();
}

/*
*	returns true if the 'fileName' exists
*/
int		comp_bas_exist(const char *basfile)
{
	int		check = 0;
	char	*p, *fileName;
	#if defined(_PalmOS)
	LocalID	lid;
	#endif

	fileName = tmp_alloc(strlen(basfile)+5);
	strcpy(fileName, basfile);

	p = strchr(fileName, '.');
	if	( !p )	
		strcat(fileName, ".bas");

	#if defined(_PalmOS)
	lid = DmFindDatabase(0, fileName);
	check = (lid != 0);
	#elif defined(_VTOS)
	{
	FILE *fp;
	check=FALSE;
	fp=fopen(fileName,"rb");
	if (fp) {
		fclose(fp);
		check=TRUE;
		}
	}
	#else
		#if !defined(_UnixOS)
		check = (access(fileName, 0) == 0);
		#else
		check = (access(fileName, R_OK) == 0);
		#endif
	#endif

	tmp_free(fileName);
	return check;
}

/*
old bc_load(), palmos section (I'll need it for convertion)

#if defined(_PalmOS)
	DmOpenRef	fp;
	LocalID		lid;
	int			l, i;
	VoidPtr		rec_p = NULL;
	VoidHand	rec_h = NULL;

	lid = DmFindDatabase(0, (char *) file_name);
	fp = DmOpenDatabase(0, lid, dmModeReadWrite);
	if	( !fp )	{
		panic("LOAD: CAN'T OPEN FILE %s", fileName);
		return;
		}
	l = DmNumRecords(fp) - 1;
	if	( l <= 0 )	{
		panic("LOAD: BAD FILE STRUCTURE %s", fileName);
		return;
		}

	for ( i = 0; i < l; i ++ )	{
		rec_h = DmGetRecord(fp, i+1);
		if	( !rec_h )
			panic("LOAD: CAN'T GET RECORD %s", fileName);
		rec_p = mem_lock(rec_h);
		if	( !rec_p )
			panic("LOAD: CAN'T LOCK RECORD %s", fileName);

		if	( i == 0 && strlen(rec_p+6) == 0 )
			bc_scan("Main", rec_p+70);	// + sizeof(sec_t);
		else
			bc_scan(rec_p+6, rec_p+70);	// + sizeof(sec_t);

		mem_unlock(rec_h);
		DmReleaseRecord(fp, i+1, 0);

		if	( comp_geterror() )
			break;
		}

	DmCloseDatabase(fp);
*/

/*
*	load a source file
*/
char	*comp_load(const char *file_name)
{
	int		h;
	char	*buf = NULL;;

	strcpy(comp_file_name, file_name);
	h = open(comp_file_name, O_BINARY | O_RDWR);

	if	( h == -1 )	{
		#if defined(_CygWin)
		char	temp[1024];
		getcwd(temp, 1024);
		panic("Can't open '%s' at '%s'\n", comp_file_name, temp);
		#else
		panic("Can't open '%s'\n", comp_file_name);
		#endif
		}
	else	{
		int		size;

		size = lseek(h, 0, SEEK_END);
		lseek(h, 0, SEEK_SET);

		buf = (char *) tmp_alloc(size+1);
		read(h, buf, size);
		buf[size] = '\0';
		close(h);
		}

	return buf;
}

/**
*	format source-code text
*
*		* space-chars is the only the space
*		* CR/LF are fixed
*		* control chars are out
*		* remove remarks (')
*
*	TODO: join-lines character (&)
*
*	returns a newly created string
*/
char	*comp_format_text(const char *source)	SEC(BCSC2);
char	*comp_format_text(const char *source)
{
	const char	*p;
	char	*ps;
	int		quotes = 0;
	char	*new_text;
	int		sl, last_ch = 0, i;
	char	*last_nonsp_ptr;
	int		adj_line_num = 0;
	
	sl = strlen(source);
	new_text = tmp_alloc(sl+2);
//	memset(new_text, 0, sl+2);

	comp_line = 0;
	p = source;
	last_nonsp_ptr = ps = new_text;
	while ( *p )	{
		if	( !quotes )	{
			switch ( *p )	{
			case	'\n': 				// new line
				if	( *last_nonsp_ptr == '&' )	{	// join lines
					p ++;
					*last_nonsp_ptr = ' ';
					if	( *(last_nonsp_ptr-1) == ' ' )
						ps = last_nonsp_ptr;
					else
						ps = last_nonsp_ptr + 1;

					adj_line_num ++;
					}
				else	{
					for ( i = 0; i <= adj_line_num; i ++ )
						*ps ++ = '\n';	// at least one nl
					adj_line_num = 0;
					p ++;
					}

				// skip spaces
				while ( *p == ' ' || *p == '\t' )	p ++;

				comp_line ++;
				last_ch = '\n';
				last_nonsp_ptr = ps-1;
				break;

			case	'\'': 				// remarks
				// skip the rest line
				while ( *p )	{
					if	( *p == '\n' )	break;
					p ++;
					}
				break;

			case	' ':				// spaces
			case	'\t':
				if	( last_ch == ' ' || last_ch == '\n' )	
					p ++;
				else	{
					*ps ++ = ' ';
					p ++;
					last_ch = ' ';
					}
				break;

			case	'\"':				// quotes
				quotes = !quotes;
				last_nonsp_ptr = ps;
				*ps ++ = last_ch = *p ++;
				break;

			default:
				if	( 
					(strncasecmp(p, ":rem ", 5) == 0) ||
					(strncasecmp(p, ":rem\t", 5) == 0) ||
					(strncasecmp(p, "rem ", 4) == 0 && last_ch == '\n') ||
					(strncasecmp(p, "rem\n", 4) == 0 && last_ch == '\n')
					)	{

					// skip the rest line
					while ( *p )	{
						if	( *p == '\n' )	break;
						p ++;
						}
					break;
					}
				else	{
					if	( (*p > ' ') || (*p & 0x80) )	{	// simple code-character
						last_nonsp_ptr = ps;
						*ps ++ = last_ch = to_upper(*p);
						p ++;
						}
					else
						p ++;
					// else ignore it
					}
				}

			}
		else	{	// in quotes
			if	( *p == '\"' )	
				quotes = !quotes;
			*ps ++ = *p ++;
			}
		}

	// close
	*ps ++ = '\n';
	*ps = '\0';

	// debug
	//printf("%s", new_text);

	return new_text;
}

/**
*	scans prefered graphics mode paramaters
*
*	syntax: XXXXxYYYY[xBB]
*/
void	err_grmode()	SEC(BCSC2);
void	err_grmode()
{
	// dev_printf() instead of sc_raise()... it is just a warning...
	#if !defined(OS_LIMITED)
	dev_printf("GRMODE, usage:<width>x<height>[x<bits-per-pixel>]\nExample: OPTION PREDEF GRMODE=640x480x4\n");
	#endif
}

void	comp_preproc_grmode(const char *source)
{
	char	*p, *v;
	int		x, y, b;

	// prepare the string (copy it to comp_bc_tmp2)
	// we use second buffer because we want to place some '\0' characters into the buffer
	// in a non-SB code, there must be a dynamic allocation
	strncpy(comp_bc_tmp2, source, 32);
	comp_bc_tmp2[31] = '\0';		// safe paranoia
	p = comp_bc_tmp2;

	// searching the end of the string
	while ( *p )	{				// while *p is not '\0'
		if	( *p == '\n' || *p == ':' )	{	// yeap, we must close the string here (enter or command-seperator) 
									// it is supposed that remarks had already removed from source
			*p = '\0';				// terminate the string
			break;
			}

		p ++;						// next
		}

	// get parameters
	p = comp_bc_tmp2;
	while ( *p == ' ' )	p ++;		// skip spaces (tabs also had been removed from comp_format_text)

	// the width
	v = p;							// 'v' points to first letter of 'width', (1024x768)
									// ........................................^ <- p, v
	p = strchr(v, 'X');				// search for the end of 'width' parameter (1024x768). Remeber that the string is in upper-case
									// .............................................^ <- p
	if	( !p )	{					// we don't accept one parameter, the width must followed by the height
									// so, if 'X' delimiter is omitted, there is no height parameter
		err_grmode();
		return;
		}
	*p = '\0';						// we close the string at X position (example: "1024x768" it will be "1024\0768")
	x = xstrtol(v);					// now the v points to a string-of-digits, we can perform atoi() (xstrtol()=atoi())
	p ++; v = p;					// v points to first letter of 'height' (1024x768x24)
									// ...........................................^ <- v
			
	// the height
	p = strchr(v, 'X');				// searching for the end of 'height' (1024x768x24)
									// ...........................................^ <- p
	if	( p )	{					// if there is a 'X' delimiter, then the 'bpp' is followed, so, we need different path
		*p = '\0';					// we close the string at second's X position
		y = xstrtol(v);				// now the v points to a string-of-digits, we can perform atoi() (xstrtol()=atoi())

		p ++; v = p;				// v points to first letter of 'bpp' (1024x768x24)
									// ............................................^ <- v

		// the bits-per-pixel
		if	( strlen(v) )			// if *v != '\0', there is actually a string
			b = xstrtol(v);			// integer value of (v). v points to a string-of-digits...
									// btw, if the user pass some wrong characters (like a-z), the xstrtol will return a value of zero
		else
			b = 0;					// undefined = 0, user deserves a compile-time error becase v is empty, but we forgive him :)
									// remember that, the source, except of upper-case, is also trimmed
		}
	else	{						// there was no 'X' delimiter after the 'height', so, bpp is undefined
		y = xstrtol(v);				// now the v points to a string-of-digits, we can perform atoi() (xstrtol()=atoi())
		b = 0;						// bpp is undefined (value 0)
		}

	// setup the globals
	opt_pref_width = x;
	opt_pref_height = y;
	opt_pref_bpp = b;
//	fprintf(stderr, "PREF: %dx%dx%d\n", opt_pref_width, opt_pref_height, opt_pref_bpp);	// just to be sure :)
}

/**
*	imports units
*/
void	comp_preproc_import(const char *slist)	SEC(BCSC2);
void	comp_preproc_import(const char *slist)
{
	const char	*p;
	char	*d;
	char	buf[OS_PATHNAME_SIZE+1];
	int		uid;
	bc_lib_rec_t	imlib;

	p = slist;
	
	// skip spaces
	while ( *p == ' ' || *p == '\t' )	p ++;

	while ( is_alpha(*p) )	{
		// get name
		d = buf;
		while ( is_alnum(*p) || *p == '_' )	
			*d ++ = *p ++;
		*d = '\0';

		// import name
		strlwr(buf);
		if	( (uid = slib_get_module_id(buf)) != -1 )	{	// C module
			// store lib-record
			strcpy(imlib.lib, buf);
			imlib.id = uid;
			imlib.type = 0;	// C module

			slib_setup_comp(uid);
			dbt_write(comp_libtable, comp_libcount, &imlib, sizeof(bc_lib_rec_t));
			comp_libcount ++;
			}
		else	{ 										// SB unit
			uid = open_unit(buf);
			if	( uid < 0 )	{
				sc_raise("Unit %s.sbu not found", buf);
				return;
				}

			if	( import_unit(uid) < 0 )	{
				sc_raise("Unit %s.sbu, import failed", buf);
				close_unit(uid);
				return;
				}

			// store lib-record
			strcpy(imlib.lib, buf);
			imlib.id = uid;
			imlib.type = 1;	// unit

			dbt_write(comp_libtable, comp_libcount, &imlib, sizeof(bc_lib_rec_t));
			comp_libcount ++;

			// clean up
			close_unit(uid);
			}

		// skip spaces and commas
		while ( *p == ' ' || *p == '\t' || *p == ',' )	p ++;
		}
}

/**
*	makes the current line full of spaces
*/
void	comp_preproc_remove_line(char *s, int cmd_sep_allowed)		SEC(BCSC2);
void	comp_preproc_remove_line(char *s, int cmd_sep_allowed)
{
	char	*p = s;

	if	( cmd_sep_allowed )	{
		while ( *p != '\n' && *p != ':' )	{
			*p = ' ';
			p ++;
			}
		}
	else	{
		while ( *p != '\n' )	{
			*p = ' ';
			p ++;
			}
		}
}

/**
*	prepare compiler for UNIT-source
*/
void	comp_preproc_unit(char *name)	SEC(BCSC2);
void	comp_preproc_unit(char *name)
{
	char	*p = name;
	char	*d;

	d = comp_unit_name;
	while ( *p == ' ' || *p == '\t' )	p ++;
	if	( !is_alpha(*p) )	
		sc_raise("Invalid unit name");

	while ( is_alpha(*p) || *p == '_' )	
		*d ++ = *p ++;
	*d = '\0';
	comp_unit_flag = 1;

	while ( *p == ' ' || *p == '\t' )	p ++;

	if	( *p != '\n' && *p != ':' )
		sc_raise("Unit name alread defined");
}

/**
*	PASS 1
*/
int		comp_pass1(const char *section, const char *text)
{
	char	*ps, *p, lc = 0;
	int		i;
	char	pname[SB_KEYWORD_SIZE+1];
	char	*code_line;
	char	*new_text;

	code_line = tmp_alloc(SB_SOURCELINE_SIZE+1);
	memset(comp_bc_sec, 0, SB_KEYWORD_SIZE+1);
	if	( section )
		strncpy(comp_bc_sec, section, SB_KEYWORD_SIZE);
	else
		strncpy(comp_bc_sec, "Main", SB_KEYWORD_SIZE);

	new_text = comp_format_text(text);

	/*
	*	second (we can change it to support preprocessor)
	*
	*	Check for:
	*	include (#inc:)
	*	units-dir (#unit-path:)
	*	IMPORT
	*	UDF and UDP declarations
	*	PREDEF OPTIONS
	*/
	p = ps = new_text;
	comp_proc_level = 0;
	*comp_bc_proc = '\0';

	while ( *p )	{

		/*
		*	OPTION environment parameters
		*/
		if	( strncmp("OPTION", p, 6) == 0 )	{
			p += 6;
			while ( *p == ' ' )	p ++;

			if	( strncmp("PREDEF", p, 6) == 0 )	{
				p += 6;
				while ( *p == ' ' )	p ++;
				if	( strncmp("QUITE", p, 5) == 0 )
					opt_quite = 1;
				else if	( strncmp("GRMODE", p, 6) == 0 )	{
					p += 6;
					comp_preproc_grmode(p);
					opt_graphics = 1;
					}
				else if	( strncmp("TEXTMODE", p, 8) == 0 )
					opt_graphics = 0;
				else if	( strncmp("CSTR", p, 4) == 0 )
					opt_cstr = 1;
				else if	( strncmp("COMMAND", p, 7) == 0 )	{
					char	*pe;

					p += 7;
					while ( *p == ' ' )	p ++;

					pe = p;
					while ( *pe != '\0' && *pe != '\n' )	pe ++;

					lc = *pe;	*pe = '\0';
					if	( strlen(p) < OPT_CMD_SZ )
						strcpy(opt_command, p);
					else	{
						memcpy(opt_command, p, OPT_CMD_SZ-1);
						opt_command[OPT_CMD_SZ-1] = '\0';
						}
						
					*pe = lc;
					}
				else
					sc_raise("OPTION PREDEF: Unrecognized option '%s'", p);
				}
			}

		/*
		*	IMPORT units
		*/
		else if	( strncmp("IMPORT ", p, 7) == 0 )	{
			comp_preproc_import(p+7);
			comp_preproc_remove_line(p, 1);
			}
		/*
		*	UNIT name
		*/
		else if	( strncmp("UNIT ", p, 5) == 0 )	{
			if	( comp_unit_flag )
				sc_raise("Use 'Unit' keyword only once");
			else
				comp_preproc_unit(p+5);
			comp_preproc_remove_line(p, 1);
			}
		/*
		*	UNIT-PATH name
		*/
		else if	( strncmp("#UNIT-PATH:", p, 11) == 0 )	{
			#if defined(_UnixOS) || defined(_DOS) || defined(_Win32)
			char	upath[SB_SOURCELINE_SIZE+1], *up;
			char	*ps;

			ps = p;
			p += 11;
			while ( *p == ' ' )	p ++;

			if	( *p == '\"' )
				p ++;

			up = upath;
			while ( *p != '\n' && *p != '\"' )	
				*up ++ = *p ++;
			*up = '\0';

			sprintf(comp_bc_temp, "SB_UNIT_PATH=%s", upath);
			putenv(strdup(comp_bc_temp));
			p = ps;
			comp_preproc_remove_line(p, 0);
			#else	// supported OSes
			comp_preproc_remove_line(p, 0);
			#endif	
			}
		else	{

			/*
			*	INCLUDE FILE
			*	this is not a normal way but needs less memory
			*/
			if	( strncmp("#INC:", p, 5) == 0 )	{
				char	*crp  = NULL;

				p += 5;
				if	( *p == '\"' )	{
					p ++;

					crp = p;
					while ( *crp != '\0' && *crp != '\"' )
						crp ++;

					if	( *crp == '\0' )	{
						sc_raise("#INC: Missing \"");
						break;
						}

					(lc = *crp, *crp = '\0');
					}
				else	{
					crp = strchr(p, '\n');
					*crp = '\0';
					lc = '\n';
					}

				strcpy(code_line, p);
				*crp = lc;
				str_alltrim(code_line);
				#if defined(_PalmOS)
				{
				#else
				if	( !comp_bas_exist(code_line) )	
					sc_raise("File %s: File %s does not exist", comp_file_name, code_line);
				else	{
				#endif
					#if defined(_PalmOS) 
					char	fileName[65];
					char	sec[64];
					#else
					char	fileName[1024];
					char	sec[SB_KEYWORD_SIZE+1];
					#endif
				
					strcpy(sec, comp_bc_sec);
					strcpy(fileName, comp_file_name);
					if	( strchr(code_line, '.') == NULL )
						strcat(code_line, ".bas");
					comp_load(code_line); 
					strcpy(comp_file_name, fileName);
					strcpy(comp_bc_sec, sec);
					}
				}

			/*
			*	SUB/FUNC/DEF - Automatic declaration - BEGIN
			*/
			if	( (strncmp("SUB ", p, 4) == 0) || (strncmp("FUNC ", p, 5) == 0) || (strncmp("DEF ", p, 4) == 0) )	{
				char	*dp;
				int		single_line_f = 0;

				if	( *p == 'S' || *p == 'D' )
					p += 4;
				else
					p += 5;

				// skip spaces
				while ( *p == ' ' )		p ++;

				// copy proc/func name
				dp = pname;
				while ( is_alnum(*p) || *p == '_' )
					*dp ++ = *p ++;
				*dp = '\0';

				// search for '='
				while ( *p != '\n' && *p != '=' )	p ++;
				if	( *p == '=' )	{
					single_line_f = 1;
					while ( *p != '\n' )	p ++;
					}

				// add declaration
				if	( comp_udp_getip(pname) == INVALID_ADDR )
					comp_add_udp(pname);
				else
					sc_raise("SUB/FUNC %s already defined", pname);

				// func/proc name (also, update comp_bc_proc)
				if	( comp_proc_level )	{
					strcat(comp_bc_proc, "/");
					strcat(comp_bc_proc, baseof(pname, '/'));
//					printf("comp_bc_proc=%s ... %s\n", comp_bc_proc, pname);
					}
				else	{
					strcpy(comp_bc_proc, pname);
//					printf("comp_bc_proc=%s\n", comp_bc_proc);
					}

				//
				if	( !single_line_f )
					comp_proc_level ++;
				else	{
					// inline (DEF FN)
					char	*dol;

					dol = strrchr(comp_bc_proc, '/');
					if	( dol )
						*dol = '\0';
					else
						*comp_bc_proc = '\0';
					}
				}

			/*
			*	SUB/FUNC/DEF - Automatic declaration - END
			*/
			else if ( comp_proc_level )	{
				if	( strncmp("END ", p, 4) == 0 || strncmp("END\n", p, 4) == 0 )	{
					char	*dol;

					dol = strrchr(comp_bc_proc, '/');
					if	( dol )
						*dol = '\0';
					else
						*comp_bc_proc = '\0';

					comp_proc_level --;
					}
				}
			}	/* OPTION */

		/*
		*	skip text line
		*/
		while ( *p != '\0' && *p != '\n' )
			p ++;

		if	( *p )
			p ++;
		}

	if	( comp_proc_level )
		sc_raise("File %s: SUB/FUNC: Missing END (possibly in %s)", comp_file_name, comp_bc_proc);

	comp_proc_level = 0;
	*comp_bc_proc = '\0';

	if	( !opt_quite && !opt_interactive )	{
		#if defined(_UnixOS)
		if ( !isatty (STDOUT_FILENO) ) 
			fprintf(stdout, "File: %s\n", comp_file_name);
		else
			dev_printf("File: \033[1m%s\033[0m\n", comp_file_name);
		#elif defined(_PalmOS)	// if (code-sections)
		dev_printf("File: \033[1m%s\033[0m\n\033[80mSection: \033[1m%s\033[0m\033[80m\n", comp_file_name, comp_bc_sec);
		#else
		dev_printf("File: \033[1m%s\033[0m\n", comp_file_name);
		#endif
		}

	/*
	*	Start
	*/
	if	( !comp_error )	{
		comp_line = 0;	
		if	( !opt_quite && !opt_interactive )	{
			#if defined(_UnixOS)
			if ( !isatty (STDOUT_FILENO) ) 
				fprintf(stdout, "Pass1...\n");
			else	{
			#endif

			dev_printf("PASS1: Line %d", comp_line+1);

			#if defined(_UnixOS)
				}
			#endif
			}

		ps = p = new_text;
		while ( *p )	{
			if	( *p == '\n' )	{
				//	proceed
				*p = '\0';

				comp_line ++;
				if	( !opt_quite && !opt_interactive )	{
					#if defined(_UnixOS)
					if ( isatty (STDOUT_FILENO) ) {
					#endif
							
					#if defined(_PalmOS)
					if	( (comp_line % 16) == 0 )	{
						if	( (comp_line % 64) == 0 )	
							dev_printf("\rPASS1: Line %d", comp_line);
						if	( dev_events(0) < 0 )	{
							dev_print("\n\n\a*** interrupted ***\n");
							comp_error = -1;
							}
						}
					#else
					if	( (comp_line % 256) == 0 )	
						dev_printf("\rPASS1: Line %d", comp_line);
					#endif

					#if defined(_UnixOS)
						}
					#endif
					}

				// add debug info: line-number
				bc_add_code(&comp_prog, kwTYPE_LINE);
				bc_add_addr(&comp_prog, comp_line);

				strcpy(code_line, ps);
				comp_text_line(code_line);

				if	( comp_error )
					break;

				ps = p+1;
				}


			if	( comp_error )
				break;

			p ++;
			}
		}

	tmp_free(code_line);
	tmp_free(new_text);

	// undefined keywords... by default are UDP, but if there is no UDP-boddy then ring the bell
	if	( !comp_error )	{
		for ( i = 0; i < comp_udpcount; i ++ )	{
			if	( comp_udptable[i].ip == INVALID_ADDR )	{
				comp_line = comp_udptable[i].pline;
				sc_raise("Undefined SUB/FUNC code: %s", comp_udptable[i].name);
				}
			}
		}

	bc_eoc(&comp_prog);
	bc_resize(&comp_prog, comp_prog.count);
	if	( !comp_error )	{
		if	( !opt_quite && !opt_interactive )	{
			dev_printf("\rPASS1: Line %d; finished\n", comp_line+1);
			#if !defined(_PalmOS)
			#if	!defined(MALLOC_LIMITED)
			dev_printf("\rSB-MemMgr: Maximum use of memory: %dKB\n", (memmgr_getmaxalloc()+512) / 1024);
			#endif
			#endif
			dev_printf("\n");
			}
		}

	return (comp_error == 0);
}

/**
*	setup export table
*/
int		comp_pass2_exports(void)				SEC(BCSC2);
int		comp_pass2_exports()
{
	int		i, j;

	for ( i = 0; i < comp_expcount; i ++ )	{
		unit_sym_t	sym;
		bid_t		pid;

		dbt_read(comp_exptable, i, &sym, sizeof(unit_sym_t));

		// look on procedures/functions
		if	( (pid = comp_udp_id(sym.symbol, 0)) != -1 )	{
			if	( comp_udptable[pid].vid == INVALID_ADDR )
				sym.type = stt_procedure;
			else
				sym.type = stt_function;
			sym.address = comp_udptable[pid].ip;
			sym.vid = comp_udptable[pid].vid;
			}
		else	{
			// look on variables 
			pid = -1;
			for ( j = 0; j < comp_varcount; j ++ )	{
				if	( strcmp(comp_vartable[j].name, sym.symbol) == 0 )	{
					pid = j;
					break;
					}
				}

			if	( pid != -1 )	{
				sym.type = stt_variable;
				sym.address = 0;
				sym.vid = j;
				}
			else	{
				sc_raise("Export symbol '%s' not found", sym.symbol);
				return 0;
				}
			}

		dbt_write(comp_exptable, i, &sym, sizeof(unit_sym_t));
		}

	return (comp_error == 0);
}

/*
*	PASS 2
*/
int		comp_pass2()
{
	if	( !opt_quite && !opt_interactive )	{
			#if defined(_UnixOS)
			if ( !isatty (STDOUT_FILENO) ) 
				fprintf(stdout, "Pass2...\n");
			else	{
			#endif

			dev_printf("PASS2...");

			#if defined(_UnixOS)
				}
			#endif
			}

	if	( comp_proc_level )	
		sc_raise("SUB/FUNC: Missing END");
	else	{
		bc_add_code(&comp_prog, kwSTOP);
		comp_first_data_ip = comp_prog.count;

		comp_pass2_scan();
		}

	if	( comp_block_level && (comp_error==0) )	
		sc_raise("%d loop(s) remains open", comp_block_level);
	if	( comp_data.count )	
		bc_append(&comp_prog, &comp_data);

	if	( comp_expcount )
		comp_pass2_exports();

	return (comp_error == 0);
}

/*
*	final, create bytecode
*/
mem_t	comp_create_bin()
{
	int			i;
	mem_t		buff_h;
	byte		*buff, *cp;
	bc_head_t	hdr;
	dword		size;
	unit_file_t		uft;
	unit_sym_t		sym;


	if	( !opt_quite && !opt_interactive )	{
		if	( comp_unit_flag )
			dev_printf("\nCreating Unit %s...\n", comp_unit_name);
		else
			dev_printf("Creating byte-code...\n");
		}

	//
	memcpy(&hdr.sign, "SBEx", 4);
	hdr.ver       = 2;
	hdr.sbver	  = SB_DWORD_VER;
	#if defined(CPU_BIGENDIAN)
	hdr.flags     = 1;
	#else
	hdr.flags     = 0;
	#endif
	#if defined(OS_ADDR16)
	hdr.flags     |= 2;
	#elif defined(OS_ADDR32)
	hdr.flags     |= 4;
	#endif

	// executable header
	hdr.bc_count  = comp_prog.count;
	hdr.var_count = comp_varcount;
	hdr.lab_count = comp_labcount;
	hdr.data_ip   = comp_first_data_ip;

	hdr.size      = sizeof(bc_head_t) 
					+ comp_prog.count
					+ (comp_labcount * ADDRSZ)
					+ sizeof(unit_sym_t) * comp_expcount
					+ sizeof(bc_lib_rec_t) * comp_libcount
					+ sizeof(bc_symbol_rec_t) * comp_impcount;

	if	( comp_unit_flag )
		hdr.size += sizeof(unit_file_t);

	hdr.lib_count = comp_libcount;
	hdr.sym_count = comp_impcount;
	
	if	( comp_unit_flag )	{
		// it is a unit... add more info
		buff_h = mem_alloc(hdr.size+4); 	// +4
		buff = mem_lock(buff_h);

		// unit header
		memcpy(&uft.sign, "SBUn", 4);
		uft.version = 1;
		strcpy(uft.base, comp_unit_name);
		uft.sym_count = comp_expcount;
		
		cp = buff;
		memcpy(cp, &uft, sizeof(unit_file_t));
		cp += sizeof(unit_file_t);

		// unit symbol table (export)
		for ( i = 0; i < uft.sym_count; i ++ )	{
			dbt_read(comp_exptable, i, &sym, sizeof(unit_sym_t));
			memcpy(cp, &sym, sizeof(unit_sym_t));
			cp += sizeof(unit_sym_t);
			}
		
		// normal file
		memcpy(cp, &hdr, sizeof(bc_head_t));
		cp += sizeof(bc_head_t);
		}
	else	{
		// simple executable
		buff_h = mem_alloc(hdr.size+4); 	// +4
		buff = mem_lock(buff_h);

		cp = buff;
		memcpy(cp, &hdr, sizeof(bc_head_t));
		cp += sizeof(bc_head_t);
		}

	// append label table
	for ( i = 0; i < comp_labcount; i ++ )	{
		comp_label_t		label;

		dbt_read(comp_labtable, i, &label, sizeof(comp_label_t));
		memcpy(cp, &label.ip, ADDRSZ);
		cp += ADDRSZ;
		}

	// append library table
	for ( i = 0; i < comp_libcount; i ++ )	{
		bc_lib_rec_t	lib;

		dbt_read(comp_libtable, i, &lib, sizeof(bc_lib_rec_t));
		memcpy(cp, &lib, sizeof(bc_lib_rec_t));
		cp += sizeof(bc_lib_rec_t);
		}

	// append symbol table
	for ( i = 0; i < comp_impcount; i ++ )	{
		bc_symbol_rec_t	sym;

		dbt_read(comp_imptable, i, &sym, sizeof(bc_symbol_rec_t));
		memcpy(cp, &sym, sizeof(bc_symbol_rec_t));
		cp += sizeof(bc_symbol_rec_t);
		}

	size = cp - buff;

	// the program itself
	memcpy(cp, comp_prog.ptr, comp_prog.count);
	mem_unlock(buff_h);

	size += comp_prog.count;

	// print statistics
	if	( !opt_quite && !opt_interactive )	{
		dev_printf("\n");
		dev_printf("Number of variables %d (%d)\n", comp_varcount, comp_varcount - 18 /* - system variables */);
		dev_printf("Number of labels    %d\n", comp_labcount);
		dev_printf("Number of UDFs/UDPs %d\n", comp_udpcount);
		dev_printf("Code size           %d\n", comp_prog.count);
		dev_printf("\n");
		dev_printf("Imported libraries  %d\n", comp_libcount);
		dev_printf("Imported symbols    %d\n", comp_impcount);
		dev_printf("Exported symbols    %d\n", comp_expcount);
		dev_printf("\n");
		dev_printf("Final size          %d\n", size);
		dev_printf("\n");
		}

	return buff_h;
}

/**
*	save binary
*
*	@param h_bc is the memory-handle of the bytecode (created by create_bin)
*	@return non-zero on success
*/
int		comp_save_bin(mem_t h_bc)		SEC(BCSC2);
int		comp_save_bin(mem_t h_bc)
{
	int		h;
	char	fname[OS_FILENAME_SIZE+1];
	char	*buf;

	if	( (opt_nosave && !comp_unit_flag) || opt_syntaxcheck )
		return 1;

	if	( comp_unit_flag )	{
		strcpy(fname, comp_unit_name);
		strlwr(fname);

		if	( !opt_quite && !opt_interactive  )	{
			#if defined(_Win32) || defined(_DOS)
			if	( strncasecmp(fname, comp_file_name, strlen(fname) ) != 0 )
			#else
			if	( strncmp(fname, comp_file_name, strlen(fname) ) != 0 )
			#endif
				dev_printf("Warning: unit's file name is different than source\n");
			}

		strcat(fname, ".sbu");	// add ext
		}
	else	{
		char	*p;

		strcpy(fname, comp_file_name);
		p = strrchr(fname, '.');
		if	( p )	
			*p = '\0';
		strcat(fname, ".sbx");
		}

	h = open(fname, O_BINARY | O_RDWR | O_TRUNC | O_CREAT, 0660);
	if	( h != -1 )	{
		buf = (char *) mem_lock(h_bc);
		write(h, buf, mem_handle_size(h_bc));
		close(h);
		mem_unlock(h_bc);
 
		if	( !opt_quite && !opt_interactive )
			dev_printf("BC file '%s' created!\n", fname);
		}
	else
		panic("Can't create binary file\n");


	return 1;
}

/**
*	compiler - main
*
*	@param sb_file_name the source file-name
*	@return non-zero on success
*/
int		comp_compile(const char *sb_file_name)
{
	char	*source;
	int		tid, prev_tid;
	int		success = 0;
	mem_t	h_bc = 0;

	tid = create_task(sb_file_name);
	prev_tid = activate_task(tid);

	comp_reset_externals();
	comp_init();						// initialize compiler

	source = comp_load(sb_file_name);	// load file and run pre-processor
	if	( source )	{
		success = comp_pass1(NULL, source);				// PASS1
		tmp_free(source);
		if	( success )	success = comp_pass2();			// PASS2
		if	( success )	success = comp_check_labels();
		if	( success )	success = ((h_bc = comp_create_bin()) != 0);
		if	( success ) success = comp_save_bin(h_bc);
		}

	comp_close();

	close_task(tid);
	activate_task(prev_tid);

	if	( opt_nosave )
		bytecode_h = h_bc;	// update task's bytecode
	else	
		mem_free(h_bc);

	return success;
}