/**
*	@file str.h
*	Strings
*
*	@author Nicholas Christopoulos
*	@date 5/5/2000
*/

/**
*	@defgroup str Strings
*/

#if !defined(_sb_str_h)
#define _sb_str_h

#include "sys.h"

#if defined(__cplusplus)
extern "C" {
#endif

// SJIS
//Begin: Modified by Araiguma
#define IsJIS1Font(x)           (((x) >= 0x81 && (x) <= 0x9f) || ((x) >= 0xe0 && (x) <= 0xef))		/**< SJIS charset, true if the first character is belongs to sjis encode @ingroup str */
#define IsJIS2Font(x)           ((x) >= 0x40 && (x) <= 0xfc)										/**< SJIS charset, true if the second character is belongs to sjis encode @ingroup str */
#define IsJISFont(x)            (IsJIS1Font((x >> 8) & 0xff) && IsJIS2Font(x & 0xff))				/**< SJIS charset, true if the character is belongs to sjis encode @ingroup str */
// End :  Modified by Araiguma

// Big5
#define IsBig51Font(x)           ((x) >= 0x81 && (x) <= 0xfe)										/**< BIG5 charset, true if the first character is belongs to big5 encode @ingroup str */
#define IsBig52Font(x)           (((x) >= 0x40 && (x) <= 0x7e) || ((x) >= 0xa1 && (x) <= 0xfe))		/**< BIG5 charset, true if the second character is belongs to big5 encode @ingroup str */
#define IsBig5Font(x)            (IsBig51Font((x >> 8) & 0xff) && IsBig52Font(x & 0xff))			/**< BIG5 charset, true if the character is belongs to big5 encode @ingroup str */

// generic multibyte
#define IsGMB1Font(x)           ((x) >= 0x81 && (x) <= 0xfe)										/**< generic-multibyte charset, true if the character is belongs to gmb encode @ingroup str */
#define IsGMB2Font(x)           ((x) >= 0x21 && (x) <= 0xff)										/**< generic-multibyte charset, true if the second character is belongs to gmb encode @ingroup str */
#define IsGMBFont(x)            (IsGMB1Font((x >> 8) & 0xff) && IsGMB2Font(x & 0xff))				/**< generic-multibyte charset, true if the character is belongs to gmb encode @ingroup str */

#define	is_digit(c)	((c) >= 48 && (c) <= 57)												/**< true if the character is a digit @ingroup str */
#define	is_upper(c)	((c) >= 65 && (c) <= 90)												/**< true if the character is upper-case @ingroup str */
#define	is_lower(c)	((c) >= 97 && (c) <= 122)												/**< true if the character is lower-case @ingroup str */
#define	to_upper(c)	(((c) >= 97 && (c) <= 122) ? (c) - 32 : (c))							/**< returns the upper-case character @ingroup str */
#define	to_lower(c)	(((c) >= 65 && (c) <= 90) ? (c) + 32 : (c))								/**< returns the lower-case character @ingroup str */
#define	is_hexdigit(c)	(is_digit((c)) || (to_upper((c)) >= 65 && to_upper((c)) <= 70))		/**< true if the character is a hexadecimal digit @ingroup str */
#define	is_octdigit(c)	((c) >= '0' && (c) <= '7')											/**< true if the character is an octadecimal digit @ingroup str */
#define	to_hexdigit(c)	( ( ((c) & 0xF) > 9)? ((c)-10)+'A' : (c)+'0' )								/**< returns the hex-digit of the 4-bit number c @ingroup str */

/**
*	@ingroup str
*
*	returns true if the character is an alphabet symbol
*
*	@param ch the character
*	@return non-zero if the character is an alphabet symbol
*/
int		is_alpha(int ch)				SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if the character is an alphabet symbol or a digit
*
*	@param ch the character
*	@return non-zero if the character is an alphanumeric symbol
*/
int		is_alnum(int ch)				SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if the character is a 'space'
*
*	@param ch the character
*	@return non-zero if the character is a 'space'
*/
int		is_space(int ch)				SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if the character is a white-space character
*
*	@param ch the character
*	@return non-zero if the character is a white-space character
*/
int		is_wspace(int c)				SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if all the characters of the string are digits
*
*	@param text the text
*	@return true if all the characters of the string are digits
*/
int		is_all_digits(const char *text)	SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if the string 'name' can be a keyword
*
*	@param name the string
*	@return true if the string 'name' can be a keyword
*/
int		is_keyword(const char *name)	SEC(BCSCAN);

/**
*	@ingroup str
*
*	returns true if the string is a number
*
*	@param name the string
*	@return true if the string is a number
*/
int		is_number(const char *str)		SEC(BCSCAN);

#if !defined(_Win32)
/**
*	@ingroup str
*
*	converts a string to upper-case
*
*	@param str the string
*	@return the str
*/
char	*strupr(char *str)				SEC(BIO3);

/**
*	@ingroup str
*
*	converts a string to lower-case
*
*	@param str the string
*	@return the str
*/
char	*strlwr(char *str)				SEC(BIO3);
#endif

/**
*	@ingroup str
*
*	stores the next keyword of text in dest and returns a pointer to the next position
*
*	@param text the source string
*	@param dest the buffer to store the keyword
*	@return a pointer in 'text' that points to the next position
*/
char	*get_keyword(char *text, char *dest)	SEC(BCSCAN);

/**
*	@ingroup str
*
*	Returns the number of a complex constant numeric expression
*
*	type  <=0 = error
*		    1 = int32
*           2 = double
*
*	Warning: octals are different from C (QB compatibility: 009 = 9)
*
*	@param text the source text, the text does not need to contains only the expression
*	@param dest buffer to return the string of the expression
*	@param type the type (1=integer-number, 2=real-number, otherwise error)
*	@param lv the integer value
*	@param dv the real value
*	@return a pointer in 'text' that points to the next position
*/
char	*get_numexpr(char *text, char *dest, int *type, long *lv, double *dv) SEC(BCSCAN);

/**
*	@ingroup str
*
*	removes all the leading/trailing white-spaces
*
*	<b>Example</b>
*	@code
*	before "   Hello   World  " -> after "Hello   World"
*	@endcode
*
*	@param str the string
*/
void	str_alltrim(char *str)			SEC(BIO3);

/**
*	@ingroup str
*
*	returns the value of a string that contains an integer number in binary form
*
*	@param str the string
*	@return the number
*/
long	bintol(const char *str)			SEC(BIO3);

/**
*	@ingroup str
*
*	returns the value of a string that contains an integer number in octal form
*
*	@param str the string
*	@return the number
*/
long	octtol(const char *str)			SEC(BIO3);

/**
*	@ingroup str
*
*	returns the value of a string that contains an integer number in hexadecimal form
*
*	@param str the string
*	@return the number
*/
long	hextol(const char *str)			SEC(BIO3);

/**
*	@ingroup str
*
*	returns the value of a string that contains a real number in decimal form
*
*	@param str the string
*	@return the number
*/
double	sb_strtof(const char *str)			SEC(BIO3);
#define	xsb_strtof(s)	sb_strtof((s))

/**
*	@ingroup str
*
*	returns the value of a string that contains an integer number in decimal form
*
*	@param str the string
*	@return the number
*/
long	xstrtol(const char *str)		SEC(BIO3);

/**
*	@ingroup str
*
*	converts a real number to string
*
*	@param num the number
*	@param dest is the buffer to store the string
*	@return a pointer to dest
*/
char	*ftostr(double num, char *dest)	SEC(BIO3);

/**
*	@ingroup str
*
*	converts an integer number to string
*
*	@param num the number
*	@param dest is the buffer to store the string
*	@return a pointer to dest
*/
char	*ltostr(long num, char *dest)	SEC(BIO3);

/**
*	@ingroup str
*
*	locate the substring 's2' into 's1' and return a pointer of the
*	first occurence (in 's1') or NULL if not found.
*
*	the difference with strstr() is that the stristr do the search ignoring the case
*
*	@param s1 the text
*	@param s2 the substring
*	@return on success a pointer to 's1' in the place which the 's2' is starting; otherwise NULL
*/
char	*stristr(const char *s1, const char *s2)							SEC(BIO3);

/**
*	@ingroup str
*
*	locate the substring 's2' into 's1' and return a pointer of the
*	first occurence (in 's1') or NULL if not found.
*
*	the difference with strstr() is that the q_strstr do the search ignoring everything
*	that included in the specified pairs.
*
*	example
*	@code
*	q_strstr("Hello (world of) of", "of", "()");
*	@endcode
*
*	@param s1 the text
*	@param s2 the substring
*	@param pairs ignore the string that is included in any of that pairs
*	@return on success a pointer to 's1' in the place which the 's2' is starting; otherwise NULL
*/
char	*q_strstr(const char *s1, const char *s2, const char *pairs)		SEC(BIO3);

#if defined(_PalmOS)
/**
*	@ingroup str
*
*	searches for the last occurence of the character 'ch' in the string 'source'
*
*	@param source the string
*	@param ch the character
*	@return on success a pointer in 'source' of the last occurence of 'ch'; otherwise returns NULL
*/
char	*strrchr(const char *source, int ch) SEC(BIO3);
#endif

/**
*	@ingroup str
* 
*	returns the real-number of a complex-constant numeric expression
*
*	@param source is the text
*	@returns the real-number
*/
double	numexpr_sb_strtof(char *source) SEC(BIO3);

/**
*	@ingroup str
* 
*	returns the integer-number of a complex-constant numeric expression
*
*	@param source is the text
*	@returns the real-number
*/
long	numexpr_strtol(char *source) SEC(BIO3);

/**
*	@ingroup str
* 
*	returns the string 'source' enclosed by pairs
*
*	@param source the source-string
*	@param pairs the pair
*	@return a newly allocated string of the enclosed result
*/
char	*encldup(const char *source, const char *pairs)						SEC(BIO3);

/**
*	@ingroup str
* 
*	returns the string 'source' that is located inside the of some pair of 'pairs'
*
*	@param source the source-string
*	@param pairs the pairs
*	@param ignpairs ignore the string that is included in any of that pairs
*	@return a newly allocated string of the disclosed result
*/
char	*discldup(const char *source, const char *pairs, const char *ignpairs)	SEC(BIO3);

/**
*	@ingroup str
*
*	The squeeze function removes all duplicated white-spaces from the string, include all the leading/trailing white-spaces.
*
*	<b>Example</b>
*	@code
*	source="   Hello   World  " -> result="Hello World"
*	@endcode
*
*	@param source is the source string
*	@return a newly created string
*/
char	*sqzdup(const char *source)		SEC(BIO3);

/**
*	@ingroup str
*
*	translates the 'what' with the 'with'
*
*	@param src is the source string
*	@param what is what to search
*	@param with is what to replace with
*	@return a newly created string
*/
char	*transdup(const char *src, const char *what, const char *with)	SEC(BIO3);

/**
*	@ingroup str
*
*	removes all the leading/trailing white-spaces
*
*	<b>Example</b>
*	@code
*	source="   Hello   World  " -> result="Hello   World"
*	@endcode
*
*	@param source is the source string
*	@return a newly created string
*/
char	*trimdup(const char *str)		SEC(BIO3);

/**
*	@ingroup str
*
*	with few words,	the filename's playground.
*
*	@param dest is the buffer to store the result
*	@param source is the filename
*	@param newdir if not NULL, the directory to replace the old-one (note the newdir must ends with directory-separator character)
*	@param prefix if not NULL, the prefix to be used on the basename
*	@param new_ext if not NULL, the new extension of the file
*	@param suffix if not NULL, the suffix to be used on the basename (before the extension)
*	@return a pointer to dest
*/
char    *chgfilename(char *dest, char *source, char *newdir, char *prefix, char *new_ext, char *suffix) SEC(BIO3);

/**
*	@ingroup str
*
*	returns the basename of source. basename means the name without the directory.
*	on error the dest will be empty.
*
*	@param dest is the buffer to store the name
*	@param source is the filename
*	@return a pointer to dest
*/
char    *xbasename(char *dest, const char *source) SEC(BIO3);

/**
*	@ingroup str
*
*	Converts a string which contains c-style special characters
*	to normal text.
*
*	@param source the string
*	@return a newly created string
*/
char	*cstrdup(const char *source) SEC(BIO3);

/**
*	@ingroup str
*
*	Converts a string to c-style string
*
*	@param source the string
*	@return a newly created string
*/
char	*bstrdup(const char *source) SEC(BIO3);

/**
*	@ingroup str
*
*	returns a pointer of 'source' on the base-name
*
*	@param source the string
*	@param delim the delimiter
*	@return pointer to base
*/
const char *baseof(const char *source, int delim)	SEC(BIO3);

#if defined(_WinBCB)
#define	strncasecmp(a,b,n)	strnicmp(a,b,n)
#define	strcasecmp(a,b)		stricmp(a,b)
#endif

#if defined(__cplusplus)
}
#endif

#endif
