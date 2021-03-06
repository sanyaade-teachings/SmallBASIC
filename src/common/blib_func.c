// This file is part of SmallBASIC
//
// SmallBASIC LIBRARY - STANDARD FUNCTIONS
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//
// Copyright(C) 2000 Nicholas Christopoulos

#include "common/sys.h"
#include "common/str.h"
#include "common/kw.h"
#include "common/panic.h"
#include "common/var.h"
#include "common/blib.h"
#include "common/pproc.h"
#include "common/device.h"
#include "common/blib_math.h"
#include "common/fmt.h"
#include "common/geom.h"
#include "common/messages.h"

#if defined(_UnixOS)
#include <unistd.h>
#endif

struct code_array_node_s {
  var_t *v;
  var_int_t col;
  var_int_t row;
};
typedef struct code_array_node_s canode_t;

// relative coordinates (current x/y) from blib_graph
extern int gra_x;
extern int gra_y;

// date
static char *date_wd3_table[] = TABLE_WEEKDAYS_3C;
static char *date_wdN_table[] = TABLE_WEEKDAYS_FULL;
static char *date_m3_table[] = TABLE_MONTH_3C;
static char *date_mN_table[] = TABLE_MONTH_FULL;

/*
 */
var_int_t r2int(var_num_t x, var_int_t l, var_int_t h) {
  var_int_t nx;

  if (x < 0.0) {
    nx = (var_int_t) -floor(-x + .5);
  } else {
    nx = (var_int_t) floor(x + .5);
  }

  if (nx < l) {
    nx = l;
  } else if (nx > h) {
    nx = h;
  }
  return nx;
}

/*
 * PEN function
 */
void cmd_pen() {
  byte code;
  IF_ERR_RETURN;

  code = code_getnext();
  IF_ERR_RETURN;

  if (code == kwOFF) {
    dev_setpenmode(0);
  } else if (code == kwON) {
    dev_setpenmode(1);
  } else {
    err_syntax();
  }
}

/*
 * ARRAY ROUTINES - First element
 * funcCode is the function code, r is the return value of the 
 * function, elem_p is the element
 */
void dar_first(long funcCode, var_t *r, var_t *elem_p) {
  var_num_t n;

  switch (funcCode) {
  case kwMAX:
  case kwMIN:
    v_set(r, elem_p);
    break;
  default:
    r->type = V_NUM;
    n = v_getval(elem_p);

    switch (funcCode) {
    case kwABSMAX:
    case kwABSMIN:
      r->v.n = fabs(n);
    case kwSUM:
    case kwSTATMEAN:
      r->v.n = n;
      break;
    case kwSUMSV:
      r->v.n = n * n;
      break;
    }
  }
}

/*
 * ARRAY ROUTINES - Next (each) element
 * funcCode is the function code, r is the return value of 
 * the function, elem_p is the current element
 */
void dar_next(long funcCode, var_t *r, var_t *elem_p) {
  var_num_t n;

  switch (funcCode) {
  case kwMAX:
    if (v_compare(r, elem_p) < 0) {
      v_set(r, elem_p);
    }
    break;
  case kwMIN:
    if (v_compare(r, elem_p) > 0) {
      v_set(r, elem_p);
    }
    break;
  default:                     // numeric
    n = v_getval(elem_p);

    switch (funcCode) {
    case kwABSMIN:
      n = fabs(n);
      if (r->v.n < n) {
        r->v.n = n;
      }
      break;
    case kwABSMAX:
      n = fabs(n);
      if (r->v.n > n) {
        r->v.n = n;
      }
      break;
    case kwSUM:
    case kwSTATMEAN:
      r->v.n += n;
      break;
    case kwSUMSV:
      r->v.n += (n * n);
      break;
    }                         // sw2
  }                           // sw1
}

/*
 * ARRAY ROUTINES - Last element
 * funcCode is the function code, r is the return value of 
 * the function, elem_p is the element
 */
void dar_final(long funcCode, var_t *r, int count) {
  switch (funcCode) {
  case kwSTATMEAN:
    if (count) {
      r->v.n = r->v.n / count;
    }
    break;
  };
}

/*
 * DATE mm/dd/yy string to ints
 */
void date_str2dmy(char *str, long *d, long *m, long *y) {
  char *p;
  char tmp[8];
  int mode = 0, count = 0;

  p = str;
  while (*p) {
    if (*p == '/') {
      tmp[count] = '\0';
      switch (mode) {
      case 0:                  // day
        *d = xstrtol(tmp);
        if (*d < 1 || *d > 31) {
          rt_raise(ERR_DATE, str);
          return;
        }
        break;
      case 1:                  // month
        *m = xstrtol(tmp);
        if (*m < 1 || *m > 12) {
          rt_raise(ERR_DATE, str);
          return;
        }
        break;
      default:
        rt_raise(ERR_DATE, str);
        return;
      };
      mode++;
      count = 0;
    } else {
      tmp[count] = *p;
      count++;
    }
    p++;
  }

  if (mode != 2) {
    rt_raise(ERR_DATE, str);
    return;
  }

  tmp[count] = '\0';
  *y = xstrtol(tmp);
  if (*y < 100) {
    *y += 2000;
  }
}

/*
 *   TIME hh:mm:ss string to ints
 */
void date_str2hms(char *str, long *h, long *m, long *s) {
  char *p;
  char tmp[8];
  int mode = 0, count = 0;

  p = str;
  while (*p) {
    if (*p == ':') {
      tmp[count] = '\0';
      switch (mode) {
      case 0:                  // hour
        *h = xstrtol(tmp);
        if (*h < 0 || *h > 23) {
          rt_raise(ERR_TIME, str);
          return;
        }
        break;
      case 1:                  // min
        *m = xstrtol(tmp);
        if (*m < 0 || *m > 59) {
          rt_raise(ERR_TIME, str);
          return;
        }
        break;
      default:
        rt_raise(ERR_TIME, str);
        return;
      };
      mode++;
      count = 0;
    } else {
      tmp[count] = *p;
      count++;
    }
    p++;
  }

  if (mode != 2) {
    rt_raise(ERR_TIME, str);
    return;
  }

  tmp[count] = '\0';
  *s = xstrtol(tmp);
  if (*s < 0 || *s > 59) {
    rt_raise(ERR_TIME, str);
  }
}

/*
 *   calc julian date
 */
long date_julian(long d, long m, long y) {
  long j = -1L, t, jp;

  if (y < 0) {
    return j;
  }
  if (m < 1 || m > 12) {
    return j;
  }
  if (d < 1 || d > 31) {
    return j;
  }
  t = (m - 14L) / 12L;
  jp = d - 32075L + (1461L * (y + 4800L + t) / 4L);
  jp = jp + (367L * (m - 2L - t * 12L) / 12L);
  j = jp - (3L * ((y + 4900L + t) / 100) / 4);
  return j;
}

/*
 * date: weekday (0=sun)
 */
int date_weekday(long d, long m, long y) {
  if (y < 0) {
    return -1;
  }
  if (m < 1 || m > 12) {
    return -1;
  }
  if (d < 1 || d > 31) {
    return -1;
  }
  if (y < 100) {
    y += 2000;
  }
  return ((1461 * (y + 4800 + (m - 14) / 12) / 4 + 367 * (m - 2 - 12 * ((m - 14) / 12)) / 12
      - 3 * ((y + 4900 + (m - 14) / 12) / 100) / 4 + d) % 7);
}

/*
 * format date
 */
void date_fmt(char *fmt, char *buf, long d, long m, long y) {
  int dc, mc, yc, wd, l, i;
  char *p, tmp[32];

  buf[0] = '\0';
  dc = 0;
  mc = 0;
  yc = 0;
  p = fmt;
  if (!(*p)) {
    return;
  }
  while (1) {
    if (*p == DATEFMT_DAY_U || *p == DATEFMT_DAY_L) {
      dc++;
    } else if (*p == DATEFMT_MONTH_U || *p == DATEFMT_MONTH_L) {
      mc++;
    } else if (*p == DATEFMT_YEAR_U || *p == DATEFMT_YEAR_L) {
      yc++;
    } else {
      //
      // Separator
      //
      if (dc) {                 // day
        switch (dc) {
        case 1:
          ltostr(d, tmp);
          strcat(buf, tmp);
          break;
        case 2:
          ltostr(d, tmp);
          if (d < 10) {
            strcat(buf, "0");
          }
          strcat(buf, tmp);
          break;
        default:               // weekday
          wd = date_weekday(d, m, y);
          if (wd >= 0 && wd <= 6) {
            if (dc == 3) {
              // 3 letters
              strcat(buf, date_wd3_table[wd]);
            } else {
              // full name
              strcat(buf, date_wdN_table[wd]);
            }
          } else {
            strcat(buf, "***");
          }
        }

        dc = 0;
      }                         // day
      else if (mc) {            // month
        switch (mc) {
        case 1:
          ltostr(m, tmp);
          strcat(buf, tmp);
          break;
        case 2:
          ltostr(m, tmp);
          if (m < 10) {
            strcat(buf, "0");
          }
          strcat(buf, tmp);
          break;
        default:               // month
          if (m >= 1 && m <= 12) {
            if (mc == 3) {
              // 3 letters
              strcat(buf, date_m3_table[m - 1]);
            } else {
              // full name
              strcat(buf, date_mN_table[m - 1]);
            }
          } else
            strcat(buf, "***");
        }

        mc = 0;
      }                         // month
      else if (yc) {            // year
        ltostr(y, tmp);
        l = strlen(tmp);
        if (l < yc) {
          for (i = l; i < yc; i++) {
            strcat(buf, "0");
          }
        } else {
          strcat(buf, tmp + (l - yc));
        }
        yc = 0;
      }                         // year

      // add separator
      tmp[0] = *p;
      tmp[1] = '\0';
      strcat(buf, tmp);
    }

    if (*p == '\0') {
      return;
    }
    p++;                        // next
  }
}

/*
 * date julian->d/m/y
 */
void date_jul2dmy(long j, long *d, long *m, long *y) {
  long ta, tb, tc;

  ta = j + 68569L;
  tb = 4L * ta / 146097L;
  ta = ta - (146097L * tb + 3L) / 4L;
  *y = 4000L * (ta + 1L) / 1461001L;
  tc = *y;
  ta = ta - (1461L * tc / 4L) + 31L;
  *m = 80L * ta / 2447L;
  tc = *m;
  *d = ta - (2447L * tc / 80L);
  ta = *m / 11L;
  *m = *m + 2L - (12L * ta);
  *y = 100L * (tb - 49L) + *y + ta;
}

/*
 * timer->hms
 */
void date_tim2hms(long t, long *h, long *m, long *s) {
  *h = t / 3600L;
  *m = (t - (*h * 3600L)) / 60L;
  *s = t - (*h * 3600L + *m * 60L);
}

/*
 * f <- FUNC (f|i)
 */
var_num_t cmd_math1(long funcCode, var_t *arg) {
  var_num_t r = 0.0, x;

  IF_ERR_RETURN;
  x = v_getval(arg);

  switch (funcCode) {
  case kwPENF:
    r = dev_getpen(x);
    break;
  case kwCOS:
    r = cos(x);
    break;
  case kwSIN:
    r = sin(x);
    break;
  case kwTAN:
    r = tan(x);
    break;
  case kwACOS:
    r = acos(x);
    break;
  case kwASIN:
    r = asin(x);
    break;
  case kwATAN:
    r = atan(x);
    break;
  case kwCOSH:
    r = cosh(x);
    break;
  case kwSINH:
    r = sinh(x);
    break;
  case kwTANH:
    r = tanh(x);
    break;
  case kwACOSH:
#if defined(_Win32)
    r = log(x + sqrt(x * x - 1));
#else
    r = acosh(x);
#endif
    break;
  case kwASINH:
#if defined(_Win32)
    r = log(x + sqrt(x * x + 1));
#else
    r = asinh(x);
#endif
    break;
  case kwATANH:
#if defined(_Win32)
    r = log((1 + x) / (1 - x)) / 2;
#else
    r = atanh(x);
#endif
    break;

  case kwSEC:
    r = 1.0 / cos(x);
    break;
  case kwASEC:
    r = atan(sqrt(x * x - 1.0)) + (ZSGN(x) - 1.0) * SB_PI / 2.0;
    break;
  case kwSECH:
    r = 2.0 / (exp(x) + exp(-x));
    break;
  case kwASECH:
    r = log((1.0 + sqrt(1.0 - x * x)) / x);
    break;

  case kwCSC:
    r = 1.0 / sin(x);
    break;
  case kwACSC:
    r = atan(1.0 / sqrt(x * x - 1.0)) + (ZSGN(x) - 1.0) * SB_PI / 2.0;
    break;
  case kwCSCH:
    r = 2.0 / (exp(x) - exp(-x));
    break;
  case kwACSCH:
    r = log((ZSGN(x) * sqrt(x * x + 1.0) + 1.0) / x);
    break;

  case kwCOT:
    r = 1.0 / tan(x);
    break;
  case kwACOT:
    r = SB_PI / 2.0 - atan(x);
    break;
  case kwCOTH:
    r = 2.0 * exp(-x) / (exp(x) - exp(-x)) + 1.0;
    break;
  case kwACOTH:
    r = log((x + 1.0) / (x - 1.0)) / 2.0;
    break;

  case kwSQR:
    r = sqrt(x);
    break;
  case kwABS:
    r = (x > 0.0) ? x : -x;
    break;
  case kwEXP:
    r = exp(x);
    break;
  case kwLOG:
    r = log(x);
    break;
  case kwLOG10:
    r = log10(x);
    break;
  case kwINT:
    r = (x < 0) ? -floor(-x) : floor(x);
    break;
  case kwFIX:
    r = (x < 0) ? -ceil(-x) : ceil(x);
    break;
  case kwCEIL:
    r = ceil(x);
    break;
  case kwFLOOR:
    r = floor(x);
    break;
  case kwFRAC:
    r = (x < 0) ? x + floor(-x) : x - floor(x);
    break;
  case kwDEG:
    r = x * 180.0 / PI;
    break;
  case kwRAD:
    r = x * PI / 180.0;
    break;
  case kwCDBL:
    r = x;
    break;
  case kwXPOS:
    if (os_graphics)
      r = dev_getx() / dev_textwidth("0");
    else
      r = dev_getx();
    r++;
    break;
  case kwYPOS:
    if (os_graphics)
      r = dev_gety() / dev_textheight("0");
    else
      r = dev_gety();
    r++;
    break;
  case kwRND:
    r = ((var_num_t) rand()) / (RAND_MAX + 1.0);
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (2)", funcCode);
  };

  return r;
}

/*
 * i <- FUNC (f|i)
 */
var_int_t cmd_imath1(long funcCode, var_t *arg) {
  var_int_t r = 0, x;

  IF_ERR_RETURN;
  x = v_getint(arg);

  switch (funcCode) {
  case kwTIMER:
    //
    // int <- TIMER // seconds from 00:00
    //
  {
    struct tm tms;
    time_t now;

    time(&now);
    tms = *localtime(&now);
    r = tms.tm_hour * 3600L + tms.tm_min * 60L + tms.tm_sec;
  }
    break;
  case kwTICKS:
    //
    // int <- TICKS // clock()
    //
#if defined(_Win32)
    {
      __int64 start, freq;
      QueryPerformanceFrequency((LARGE_INTEGER *) & freq);
      QueryPerformanceCounter((LARGE_INTEGER *) & start);
      if (freq > 100000)
      r = start / 1000;
      else
      r = start;
    }
#else
    r = clock();
#endif
    break;
  case kwTICKSPERSEC:
    //
    // int <- TICKSPERSEC
    //
#if defined(_Win32)
    {
      __int64 freq;
      QueryPerformanceFrequency((LARGE_INTEGER *) & freq);
      if (freq > 100000)
      r = freq / 1000;
      else
      r = freq;
    }
    //              r = 1; // fake! but the GetTickCount() returns the number of ms from the start
#else
    r = CLOCKS_PER_SEC;
#endif
    break;
  case kwPROGLINE:
    //
    // int <- current program line
    //
    r = prog_line;
    break;
  case kwCINT:
    //
    // int <- CINT(float)
    //
    r = x;
    break;
  case kwEOF:
    //
    // int <- EOF(file-handle)
    //
#if defined(_UnixOS) || defined(_DOS)
    if (x == 0) {
      r = feof(stdin);
      break;
    }
#endif
    r = dev_feof(x);
    break;
  case kwSEEKF:
    //
    // int <- SEEK(file-handle)
    //
    r = dev_ftell(x);
    break;
  case kwLOF:
    //
    // int <- LOF(file-handle)
    //
    r = dev_flength(x);
    break;
  case kwSGN:
    //
    // int <- SGN(n)
    //
    r = v_sign(arg);
    break;
  case kwFRE:
    //
    // QB-standard:
    // int <- FRE(0) // free memory
    // int <- FRE(-1) // largest block of integers
    // int <- FRE(-2) // free stack
    // int <- FRE(-3) // largest free block
    //
    // Our standard (it is optional for now):
    // int <- FRE(-10) // total ram
    // int <- FRE(-11) // used
    // int <- FRE(-12) // free
    //
    // Optional-set #1: memory related info (-1x)
    // int <- FRE(-13) // shared
    // int <- FRE(-14) // buffers
    // int <- FRE(-15) // cached
    // int <- FRE(-16) // total virtual memory size
    // int <- FRE(-17) // used virtual memory
    // int <- FRE(-18) // free virtual memory
    //
    // Optional-set #2: system related info (-2x)
    //
    // Optional-set #3: file-system related info (-3x)
    //
    // Optional-set #4: battery related info (-4x)
    // int <- FRE(-40) // battery voltage * 1000
    // int <- FRE(-41) // battery percent
    // int <- FRE(-42) // battery critical voltage value * 1000
    // int <- FRE(-43) // battery warning voltage value * 1000
    //
  {
#if defined(_UnixOS)
    int memfd;

    memfd = open("/proc/meminfo", O_RDONLY);
    r = 0;
    if (memfd) {
      char tmp[1024];
      char *p, *ps;
      // total, used, free, shared, buffs, cached;
      long long vals[6];
      int i;

      read(memfd, tmp, 1024);
      if ((p = strstr(tmp, "Mem: ")) != NULL) {
        p += 4;

        for (i = 0; i < 6; i++) {
          // word
          while (*p == ' ' || *p == '\t')
          p++;
          ps = p;
          while (is_digit(*p))
          p++;
          *p = '\0';
          p++;

#if defined(__CYGWIN__)
          vals[i] = atol(ps);
#else
          vals[i] = atoll(ps);
#endif
        }

        switch (x) {
          case 0:
          r = (vals[2] + vals[4] + vals[5]);
          break;
          case -1:
          r = (vals[2] + vals[4] + vals[5]) / 4;
          break;
          case -2:
          r = (vals[2] + vals[4] + vals[5]);
          break;
          case -3:
          r = (vals[2] + vals[4] + vals[5]);
          break;
          default:
          if ((x <= -10) && (x >= -15))
          r = vals[(-x) - 10];
          else
          r = 0;
        };
      }
      close(memfd);
    }
#elif defined (_FRANKLIN_EBM)
    switch (x) {
      case 0:
      case -3:
      case -12:
      r = Sigma_GetAvailableMem();
      break;
      case -1:
      r = Sigma_GetAvailableMem() / 4L;
      break;
      case -10:
      r = Sigma_GetPhysicalMemSize();
      break;
      case -11:
      r = Sigma_GetPhysicalMemSize() - Sigma_GetAvailableMem();
      break;
      case -40:
      r = BAT_GetVoltage() * 10;
      break;
      case -41:
      r = BAT_GetPercent();
      break;
      default:
      r = 0;
    }
#elif defined(_DOS)
    switch (x) {
      case 0:                  // free mem
      case -12:
      r = _go32_dpmi_remaining_physical_memory();
      break;
      case -1: {
        __dpmi_free_mem_info mi;
        _go32_dpmi_get_free_memory_information(&mi);
        r = mi.largest_available_free_block_in_bytes / 4L;
      }
      break;
      case -2:                 // stk
      r = stackavail();
      break;
      case -3: {
        __dpmi_free_mem_info mi;
        _go32_dpmi_get_free_memory_information(&mi);
        r = mi.largest_available_free_block_in_bytes;
      }
      break;
    }
#elif defined(_WinBCB)
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&ms);

    switch (x) {
      case 0:                  // free mem
      case -3:// largest block
      case -12:// free mem
      r = ms.dwAvailPhys;
      break;
      case -1:// int
      r = ms.dwAvailPhys / 4L;
      break;
      case -2:// stk
      r = 0x120000;
      break;
    }
#else
    r = 0;
#endif
  }
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (3)", funcCode);
  };

  return r;
}

//
//      i|f <- FUNC (str)
//
void cmd_ns1(long funcCode, var_t * arg, var_t * r) {
  IF_ERR_RETURN;
  if (arg->type != V_STR) {
    v_tostr(arg);
    IF_ERR_RETURN;
  }

  switch (funcCode) {
  case kwASC:
    //
    // int <- ASC(s)
    //
    r->type = V_INT;
    r->v.i = *((byte *) arg->v.p.ptr);
    break;
  case kwVAL:
    //
    // float <- VAL(s)
    //
    r->type = V_NUM;
    r->v.n = numexpr_sb_strtof((char *) arg->v.p.ptr);
    break;
  case kwTEXTWIDTH:
    //
    // int <- TXTW(s)
    //
    r->type = V_INT;
    r->v.i = dev_textwidth((char *) arg->v.p.ptr);
    break;
  case kwTEXTHEIGHT:
    //
    // int <- TXTH(s)
    //
    r->type = V_INT;
    r->v.i = dev_textheight((char *) arg->v.p.ptr);
    break;
  case kwEXIST:
    //
    // int <- EXIST(s)
    //
    r->type = V_INT;
    r->v.i = dev_fexists((char *) arg->v.p.ptr);
    break;
  case kwACCESSF:
    //
    // int <- ACCESS(s)
    //
    r->type = V_INT;
    r->v.i = dev_faccess((char *) arg->v.p.ptr);
    break;
  case kwISFILE:
    //
    // int <- ISFILE(s)
    //
    r->type = V_INT;
    r->v.i = dev_fattr((char *) arg->v.p.ptr) & VFS_ATTR_FILE;
    break;
  case kwISDIR:
    //
    // int <- ISDIR(s)
    //
    r->type = V_INT;
    r->v.i = dev_fattr((char *) arg->v.p.ptr) & VFS_ATTR_DIR;
    break;
  case kwISLINK:
    //
    // int <- ISLINK(s)
    //
    r->type = V_INT;
    r->v.i = dev_fattr((char *) arg->v.p.ptr) & VFS_ATTR_LINK;
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (4)", funcCode);
  };
}

//
// str <- FUNC (any)
//
void cmd_str1(long funcCode, var_t * arg, var_t * r) {
  byte *p, *wp;
  byte *tb;
  var_int_t l, i;

  switch (funcCode) {
  case kwBALLOC:
    //
    // ptr <- BALLOC(size)
    //
    l = v_getint(arg);
    if (l > 0) {
      r->v.p.ptr = tmp_alloc(l);
      memset(r->v.p.ptr, 0, l);
      r->v.p.size = l;
    } else {
      rt_raise("BALLOC size %d <= 0", l);
    }
    break;
  case kwCHR:
    //
    // str <- CHR$(n)
    //
    wp = r->v.p.ptr = (byte *) tmp_alloc(2);
    wp[0] = v_getint(arg);
    wp[1] = '\0';
    r->v.p.size = 2;
    break;
  case kwSTR:
    //
    // str <- STR$(n)
    //
    r->v.p.ptr = tmp_alloc(64);
    if (arg->type == V_INT) {
      ltostr(arg->v.i, (char *) r->v.p.ptr);
    } else if (arg->type == V_REAL) {
      ftostr(arg->v.n, (char *) r->v.p.ptr);
    } else if (arg->type == V_STR) {
      tmp_free(r->v.p.ptr);
      r->v.p.ptr = (byte *) tmp_strdup((char *)arg->v.p.ptr);
    }
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwCBS:
    //
    // str <- CBS$(str)
    // convert C-Style string to BASIC-style string
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    r->v.p.ptr = (byte *) cstrdup((char *) arg->v.p.ptr);
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;

  case kwBCS:
    //
    // str <- BCS$(str)
    // convert BASIC-Style string to C-style string
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;

    r->v.p.ptr = (byte *) bstrdup((char *) arg->v.p.ptr);
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;

  case kwOCT:
    //
    // str <- OCT$(n)
    //
    r->v.p.ptr = (byte *) tmp_alloc(64);
    sprintf((char *) r->v.p.ptr, "%lo", (unsigned long) v_getint(arg));
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
    //
    // str <- BIN$(n)
    //
  case kwBIN:
    l = v_getint(arg);
    IF_ERR_RETURN
    ;
    tb = tmp_alloc(33);
    memset(tb, 0, 33);
    for (i = 0; i < 32; i++) {
      if (l & (1 << i))
        tb[31 - i] = '1';
      else
        tb[31 - i] = '0';
    }

    r->v.p.ptr = tb;
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwHEX:
    //
    // str <- HEX$(n)
    //
    r->v.p.ptr = (byte *) tmp_alloc(64);
    sprintf((char *) r->v.p.ptr, "%lX", (unsigned long) v_getint(arg));
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwLCASE:
    //
    // str <- LCASE$(s)
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    r->v.p.ptr = (byte *) tmp_alloc(strlen((char *)arg->v.p.ptr) + 1);
    strcpy((char *) r->v.p.ptr, (char *) arg->v.p.ptr);
    p = r->v.p.ptr;
    while (*p) {
      *p = to_lower(*p);
      p++;
    }
    r->v.p.size = arg->v.p.size;
    break;
  case kwUCASE:
    //
    // str <- UCASE$(s)
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    r->v.p.ptr = (byte *) tmp_alloc(strlen((char *)arg->v.p.ptr) + 1);
    strcpy((char *) r->v.p.ptr, (char *) arg->v.p.ptr);
    p = r->v.p.ptr;
    while (*p) {
      *p = to_upper(*p);
      p++;
    }
    r->v.p.size = arg->v.p.size;
    break;
  case kwLTRIM:
    //
    // str <- LTRIM$(s)
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    p = arg->v.p.ptr;
    while (is_wspace(*p))
      p++;
    r->v.p.ptr = (byte *) tmp_alloc(strlen((char *)p) + 1);
    strcpy((char *) r->v.p.ptr, (char *) p);
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwTRIM:
    //
    // str <- LTRIM(RTRIM(s))
    //
  case kwRTRIM:
    //
    // str <- RTRIM$(s)
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    p = arg->v.p.ptr;
    if (*p != '\0') {           // ndc: 20/2/2001
      while (*p)
        p++;
      p--;
      while (p >= arg->v.p.ptr && (is_wspace(*p)))
        p--;
      p++;
      *p = '\0';
    }
    r->v.p.ptr = (byte *) tmp_alloc(strlen((char *)arg->v.p.ptr) + 1);
    strcpy((char *) r->v.p.ptr, (char *) arg->v.p.ptr);
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;

    // alltrim
    if (funcCode == kwTRIM) {
      byte *tmp_p;

      tmp_p = p = r->v.p.ptr;
      while (is_wspace(*p))
        p++;
      r->v.p.ptr = (byte *) tmp_alloc(strlen((char *)p) + 1);
      strcpy((char *) r->v.p.ptr, (char *) p);
      r->v.p.size = strlen((char *) r->v.p.ptr) + 1;

      tmp_free(tmp_p);
    }
    break;
  case kwCAT:
    // we can add color codes
    r->v.p.ptr = tmp_alloc(8);
    strcpy(r->v.p.ptr, "");
    l = v_getint(arg);
    switch (l) {
    case 0:                    // reset
      strcpy(r->v.p.ptr, "\033[0m");
      break;
    case 1:                    // bold on
      strcpy(r->v.p.ptr, "\033[1m");
      break;
    case -1:                   // bold off
      strcpy(r->v.p.ptr, "\033[21m");
      break;
    case 2:                    // underline on
      strcpy(r->v.p.ptr, "\033[4m");
      break;
    case -2:                   // underline off
      strcpy(r->v.p.ptr, "\033[24m");
      break;
    case 3:                    // reverse on
      strcpy(r->v.p.ptr, "\033[7m");
      break;
    case -3:                   // reverse off
      strcpy(r->v.p.ptr, "\033[27m");
      break;
    case 80:                   // select system font
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
      sprintf((char *) r->v.p.ptr, "\033[8%dm", (int) l - 80);
      break;
    case 90:                   // select custom font
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
    case 98:
    case 99:
      if (os_charset == 0)
        sprintf((char *) r->v.p.ptr, "\033[9%dm", (int) l - 90);
      break;
    }
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwTAB:
    // TODO: use calctab
    l = v_igetval(arg);
    r->v.p.ptr = tmp_alloc(16);
    *r->v.p.ptr = '\0';
    r->v.p.size = 16;

#if defined (_FRANKLIN_EBM) || defined (_FLTK)
    // based on quick basic documentation for tab()
    // move to the n/80th pixel of screen
    sprintf(r->v.p.ptr, "\033[%dT", (int)l);
#else
    sprintf((char *) r->v.p.ptr, "\033[%dG", (int) l);
#endif
    break;
  case kwSPACE:
    //
    // str <- SPACE$(n)
    //
    l = v_getint(arg);
    wp = r->v.p.ptr = tmp_alloc(l + 1);
    for (i = 0; i < l; i++)
      wp[i] = ' ';
    wp[l] = '\0';
    r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    break;
  case kwENVIRONF:
    //
    // str <- ENVIRON$(str)
    //
    v_tostr(arg);
    IF_ERR_RETURN
    ;
    if (*arg->v.p.ptr != '\0') {
      // return the variable
      char *v;
      int l;

      v = dev_getenv((char *) arg->v.p.ptr);
      if (v) {
        l = strlen(v) + 1;
        r->v.p.ptr = tmp_alloc(l);
        strcpy(r->v.p.ptr, v);
        r->v.p.size = l;
      } else {
        r->v.p.ptr = tmp_alloc(2);
        *r->v.p.ptr = '\0';
        r->v.p.size = 1;
      }
    } else {
      // return all
      int count, i;
      var_t *elem_p;

      count = dev_env_count();
      r->type = V_INT;
      if (count) {
        v_toarray1(r, count);
        for (i = 0; i < count; i++) {
          elem_p = (var_t *) (r->v.a.ptr + (sizeof(var_t) * i));

          elem_p->type = V_STR;
          elem_p->v.p.ptr = (byte *) tmp_strdup((char *)dev_getenv_n(i));
          elem_p->v.p.size = strlen((char *) elem_p->v.p.ptr) + 1;
        }
      } else
        // no vars found
        v_toarray1(r, 0);
    }
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (5)", funcCode);
  };
}

//
//      str <- FUNC (void)
//
void cmd_str0(long funcCode, var_t * r) {
  word ch;
  char tmp[3];
  struct tm tms;
  time_t now;

  IF_ERR_RETURN;
  switch (funcCode) {
  case kwINKEY:
    //
    // str <- INKEY$
    //
    if (!dev_kbhit()) {
      dev_events(2);
    }
    if (dev_kbhit()) {
      ch = dev_getch();         // MultiByte - dev_getchr() must return the
      // extended code (2
      // bytes char)
      if ((ch & 0xFF00) == 0xFF00) {  // extra code - hardware keys
        tmp[0] = '\033';
        tmp[1] = ch & 0xFF;
        tmp[2] = '\0';
      } else {
        switch (os_charset) {
        case enc_sjis:         // Japan
          if (IsJISFont(ch)) {
            tmp[0] = ch >> 8;
            tmp[1] = ch & 0xFF;
            tmp[2] = '\0';
          } else {
            tmp[0] = ch;
            tmp[1] = '\0';
          }
          break;

        case enc_big5:         // China
          if (IsBig5Font(ch)) {
            tmp[0] = ch >> 8;
            tmp[1] = ch & 0xFF;
            tmp[2] = '\0';
          } else {
            tmp[0] = ch;
            tmp[1] = '\0';
          }
          break;

        case enc_gmb:          // Generic multibyte
          if (IsGMBFont(ch)) {
            tmp[0] = ch >> 8;
            tmp[1] = ch & 0xFF;
            tmp[2] = '\0';
          } else {
            tmp[0] = ch;
            tmp[1] = '\0';
          }
          break;

        case enc_unicode:      // Unicode
          tmp[0] = ch >> 8;
          tmp[1] = ch & 0xFF;
          tmp[2] = '\0';
          break;

        default:               // Europe 8bit
          tmp[0] = ch;
          tmp[1] = '\0';
        };
      }

      v_createstr(r, tmp);
    } else
      v_createstr(r, "");
    break;
  case kwDATE:
    //
    // str <- DATE$
    //
    time(&now);
    tms = *localtime(&now);
    r->type = V_STR;
    r->v.p.ptr = tmp_alloc(32);
    r->v.p.size = 32;
    sprintf((char *) r->v.p.ptr, "%02d/%02d/%04d", tms.tm_mday, tms.tm_mon + 1, tms.tm_year + 1900);
    break;
  case kwTIME:
    //
    // str <- TIME$
    //
    time(&now);
    tms = *localtime(&now);
    r->type = V_STR;
    r->v.p.ptr = tmp_alloc(32);
    r->v.p.size = 32;
    sprintf((char *) r->v.p.ptr, "%02d:%02d:%02d", tms.tm_hour, tms.tm_min, tms.tm_sec);
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (6)", funcCode);
  }
}

//
// str <- FUNC (...)
//
void cmd_strN(long funcCode, var_t * r) {
  var_t arg1;
  var_int_t i, count, lsrc, len, start, pc;
  char tmp[2], *tmp_p;
  char *s1 = NULL, *s2 = NULL, *s3 = NULL;

  v_init(&arg1);
  //      r->type = V_STR;

  IF_ERR_RETURN;
  switch (funcCode) {
  case kwTRANSLATEF:
    //
    // s <- TRANSLATE(source, what [, with])
    //
    par_massget("SSs", &s1, &s2, &s3);
    if (!prog_error) {
      if (s3)
        r->v.p.ptr = (byte *) transdup(s1, s2, s3);
      else
        r->v.p.ptr = (byte *) transdup(s1, s2, "");

      r->type = V_STR;
      r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    }
    break;
  case kwCHOP:
    //
    // s <- CHOP(s)
    //
    par_massget("S", &s1);
    if (!prog_error) {
      if (strlen(s1)) {
        r->v.p.ptr = (byte *) tmp_strdup(s1);
        r->v.p.ptr[strlen((char *) r->v.p.ptr) - 1] = '\0';
        r->type = V_STR;
        r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
      } else
        v_zerostr(r);
    }
    break;
  case kwSTRING:
    //
    // str <- STRING$ ( int, int | str )
    //
    start = -1;                 // ascii code
    pc = par_massget("Iis", &count, &start, &s1);
    if (!prog_error) {
      if (s1) {
        len = strlen(s1);
        tmp_p = s1;
      } else {
        if (start == -1) {
          err_argerr();
          break;
        }
        tmp[0] = start;
        tmp[1] = '\0';
        tmp_p = tmp;
        len = 1;
      }

      if (len == 0 || count == 0 || pc == 3) {
        err_argerr();
        r->type = V_INT;        // dont try to free
      } else {
        r->v.p.ptr = tmp_alloc(count * len + 1);
        *((char *) (r->v.p.ptr)) = '\0';
        for (i = 0; i < count; i++)
          strcat((char *) r->v.p.ptr, tmp_p);
        r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
      }
    }
    break;

    //
    // str <- SQUEEZE(str)
    //
  case kwSQUEEZE:
    par_massget("S", &s1);
    if (!prog_error) {
      r->type = V_STR;
      r->v.p.ptr = (byte *) sqzdup(s1);
      r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    }
    break;
    //
    // str <- ENCLOSE(str[, pairs])
    //
  case kwENCLOSE:
    par_massget("Ss", &s1, &s2);
    if (!prog_error) {
      r->type = V_STR;
      if (s2)
        r->v.p.ptr = (byte *) encldup(s1, s2);
      else
        r->v.p.ptr = (byte *) encldup(s1, "\"\"");
      r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    }
    break;
    //
    // str <- DISCLOSE(str[, pairs [, ignore-pairs]])
    //
  case kwDISCLOSE:
    par_massget("Sss", &s1, &s2, &s3);
    if (!prog_error) {
      r->type = V_STR;
      if (s2) {
        if (s3)
          r->v.p.ptr = (byte *) discldup(s1, s2, s3);
        else
          r->v.p.ptr = (byte *) discldup(s1, s2, "");
      } else {
        // auto-mode
        char *p = s1;

        while (is_wspace(*p))
          p++;
        switch (*p) {
        case '\"':
          r->v.p.ptr = (byte *) discldup(s1, "\"\"", "''");
          break;
        case '\'':
          r->v.p.ptr = (byte *) discldup(s1, "''", "\"\"");
          break;
        case '(':
          r->v.p.ptr = (byte *) discldup(s1, "()", "\"\"''");
          break;
        case '[':
          r->v.p.ptr = (byte *) discldup(s1, "[]", "\"\"''");
          break;
        case '{':
          r->v.p.ptr = (byte *) discldup(s1, "{}", "\"\"''");
          break;
        case '<':
          r->v.p.ptr = (byte *) discldup(s1, "<>", "\"\"''");
          break;
        default:
          r->v.p.ptr = (byte *) discldup(s1, "\"\"", "''");
        }
      }

      r->v.p.size = strlen((char *) r->v.p.ptr) + 1;
    }
    break;

  case kwRUNF:
    //
    // str <- RUN(command)
    // Win32: use & at the end of the command to run-it in background
    //
    par_getstr(&arg1);

    if (!prog_error) {
#if defined(RUN_UNSUP)
      err_unsup();
      r->type = V_STR;
      r->v.p.ptr = tmp_alloc(2);
      r->v.p.ptr[0] = '\0';
#else
      FILE *fin;
#if defined(_Win32)
      char *buf;
#else
      int bytes = 0, total = 0;
      char buf[256];
#endif

#if defined(_Win32)
      if ((buf = pw_shell(arg1.v.p.ptr)) != NULL) {
        r->type = V_STR;
        r->v.p.ptr = buf;
        r->v.p.size = strlen(buf) + 1;
      }
#else
      /**
       *   default popen/pclose
       */
      r->type = V_STR;
      r->v.p.ptr = tmp_alloc(256);
      *r->v.p.ptr = '\0';
      r->v.p.size = 256;

      fin = popen((char *) arg1.v.p.ptr, "r");
      if (fin) {

        while (!feof(fin)) {
          bytes = fread(buf, 1, 255, fin);
          total += bytes;
          buf[bytes] = '\0';
          strcat((char *) r->v.p.ptr, buf);
          if (total + 256 >= r->v.p.size) {
            r->v.p.size += 256;
            r->v.p.ptr = tmp_realloc(r->v.p.ptr, r->v.p.size);
          }
        }
        pclose(fin);
      }
#endif // bcb or not
      else {
        v_zerostr(r);
        rt_raise(ERR_RUNFUNC_FILE, arg1.v.p.ptr);
      }
#endif // non palmos
    }
    break;

  case kwLEFT:
    //
    // str <- LEFT$ ( str [, int] )
    //
    count = 1;
    par_massget("Si", &s1, &count);
    if (!prog_error) {
      len = strlen(s1);
      if (count > len) {
        count = len;
      }
      if (count < 0) {
        err_stridx(count);
      } else {
        r->v.p.ptr = tmp_alloc(count + 1);
        if (count) {
          memcpy(r->v.p.ptr, s1, count);
        }
        r->v.p.ptr[count] = '\0';
        r->v.p.size = count + 1;
      }
    }
    break;

  case kwLEFTOF:
    //
    // str <- LEFTOF$(str, strof)
    //
    par_massget("SS", &s1, &s2);
    if (!prog_error) {
      char *p, lc;
      int l;

      if ((p = strstr(s1, s2)) != NULL) {
        lc = *p;
        *p = '\0';
        l = strlen(s1) + 1;
        r->v.p.ptr = tmp_alloc(l);
        strcpy(r->v.p.ptr, s1);
        r->v.p.size = l;
        *p = lc;
      } else
        v_zerostr(r);
    }
    break;

  case kwRIGHT:
    //
    // str <- RIGHT$ ( str [, int] )
    //
    count = 1;
    par_massget("Si", &s1, &count);
    if (!prog_error) {
      len = strlen(s1);
      if (count > len) {
        count = len;
      }
      if (count < 0) {
        err_stridx(count);
      } else {
        r->v.p.ptr = tmp_alloc(count + 1);
        if (count) {
          memcpy(r->v.p.ptr, s1 + (len - count), count + 1);
        }
        r->v.p.ptr[count] = '\0';
        r->v.p.size = count + 1;
      }
    }
    break;

  case kwRIGHTOF:
    //
    // str <- RIGHTOF$(str, strof)
    //
    par_massget("SS", &s1, &s2);
    if (!prog_error) {
      int l;
      char *p;

      if ((p = strstr(s1, s2)) != NULL) {
        p += strlen(s2);
        l = strlen(p) + 1;
        r->v.p.ptr = tmp_alloc(l);
        memcpy(r->v.p.ptr, p, l);
        r->v.p.size = l;
      } else {
        v_zerostr(r);
      }
    }
    break;

  case kwLEFTOFLAST:
    //
    // str <- LEFTOFLAST$(str, strof)
    //
    par_massget("SS", &s1, &s2);
    if (!prog_error) {
      char *p, *lp;
      char lc;
      int l, l2;

      lp = s1;
      l2 = strlen(s2);
      p = NULL;
      while ((lp = strstr(lp, s2)) != NULL) {
        p = lp;
        lp += l2;
      };

      if (p) {
        lc = *p;
        *p = '\0';
        l = strlen(s1) + 1;
        r->v.p.ptr = tmp_alloc(l);
        memcpy(r->v.p.ptr, s1, l);
        r->v.p.size = l;
        *p = lc;
      } else {
        v_zerostr(r);
      }
    }
    break;

  case kwRIGHTOFLAST:
    //
    // str <- RIGHTOFLAST$(str, strof)
    //
    par_massget("SS", &s1, &s2);
    if (!prog_error) {
      char *p, *lp;
      int l, l2;

      lp = s1;
      l2 = strlen(s2);
      p = NULL;
      while ((lp = strstr(lp, s2)) != NULL) {
        p = lp;
        lp += l2;
      };

      if (p) {
        p += l2;
        l = strlen(p) + 1;
        r->v.p.ptr = tmp_alloc(l);
        memcpy(r->v.p.ptr, p, l);
        r->v.p.size = l;
      } else {
        v_zerostr(r);
      }
    }
    break;

  case kwREPLACE:
    //
    // str <- REPLACE$ ( source, pos, str [, len] )
    //
    count = -1;
    par_massget("SISi", &s1, &start, &s2, &count);
    if (!prog_error) {
      int ls1, ls2;

      ls1 = strlen(s1);
      ls2 = strlen(s2);

      start--;
      if (start < 0 || start > ls1) {
        err_stridx(start);
        break;
      }

      r->v.p.ptr = tmp_alloc(ls1 + ls2 + 1);

      // copy the left-part
      if (start > 0) {
        memcpy(r->v.p.ptr, s1, start);
        r->v.p.ptr[start] = '\0';
      } else
        *r->v.p.ptr = '\0';

      // insert the string
      strcat((char *) r->v.p.ptr, s2);

      // add the right-part
      if (count == -1) {
        count = ls2;
      }
      if (start + count < ls1) {
        strcat((char *) r->v.p.ptr, (char *) (s1 + start + count));
      }
      r->v.p.ptr[ls1 + ls2] = '\0';
      r->v.p.size = ls1 + ls2 + 1;
    }

    break;

  case kwMID:
    //
    // str <- MID$ ( str, start [, len] )
    //
    len = -1;
    par_massget("SIi", &s1, &start, &len);
    if (!prog_error) {
      lsrc = strlen(s1);
      if (start <= 0 || start > (int) strlen(s1)) {
        err_stridx(start);
      } else {
        start--;
        if (len < 0 || len + start >= lsrc) {
          len = lsrc - start;
        }
        r->v.p.ptr = tmp_alloc(len + 1);
        memcpy(r->v.p.ptr, s1 + start, len);
        r->v.p.ptr[len] = '\0';
        r->v.p.size = len + 1;
      }
    }

    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (7)", funcCode);
  }

  v_free(&arg1);
  pfree3(s1, s2, s3);
}

//
// int <- FUNC (...)
//
void cmd_intN(long funcCode, var_t * r) {
  int pc;
  char *s1 = NULL, *s2 = NULL, *s3 = NULL;
  var_int_t start;

  var_t arg1;
  int l;
  char *p;
  var_t *var_p = NULL;

  r->type = V_INT;
  v_init(&arg1);
  IF_ERR_RETURN;

  switch (funcCode) {
  case kwINSTR:
  case kwRINSTR:
    //
    // int <- INSTR ( [start,] str1, str2 )
    // int <- RINSTR ( [start,] str1, str2 )
    //
    r->v.i = 0;
    start = 1;
    pc = par_massget("iSS", &start, &s1, &s2);
    if (!prog_error) {
      l = strlen(s1);
      if (l) {
        start--;
        if (start >= l || start < 0) {
          err_stridx(start);
        } else {
          p = s1 + start;
          l = strlen(s2);

          while (*p) {
            if (strncmp(p, s2, l) == 0) {
              r->v.i = (p - (char *) s1) + 1;
              if (funcCode == kwINSTR) {
                break;
              }
            }
            p++;
          }
        }                       // start
      }                         // l
    }                           // error
    break;
  case kwISARRAY:
    //
    // bool <- ISARRAY(v)
    //
    if (code_isvar()) {
      var_p = code_getvarptr();
    } else {
      eval(&arg1);
      var_p = &arg1;
    }

    if (!prog_error) {
      r->v.i = (var_p->type == V_ARRAY);
    }
    break;
  case kwISSTRING:
    //
    // bool <- ISSTRING(v)
    //
  case kwISNUMBER:
    //
    // bool <- ISNUMBER(v)
    //
    if (code_isvar()) {
      var_p = code_getvarptr();
    } else {
      eval(&arg1);
      var_p = &arg1;
    }

    r->v.i = 0;

    if (!prog_error) {
      if (var_p->type == V_STR) {
        char buf[64], *np;
        int type;
        var_int_t lv = 0;
        var_num_t dv = 0;

        np = get_numexpr((char *) var_p->v.p.ptr, buf, &type, &lv, &dv);

        if (type == 1 && *np == '\0') {
          r->v.i = (funcCode == kwISSTRING) ? 0 : 1;
        } else if (type == 2 && *np == '\0') {
          r->v.i = (funcCode == kwISSTRING) ? 0 : 1;
        } else {
          r->v.i = (funcCode == kwISSTRING) ? 1 : 0;
        }
      } else {
        if (var_p->type == V_NUM || var_p->type == V_INT) {
          r->v.i = (funcCode == kwISSTRING) ? 0 : 1;
        }
      }
    }

    break;
  case kwLEN:
    //
    // int <- LEN(v)
    //
    if (code_isvar()) {
      var_p = code_getvarptr();
    } else {
      eval(&arg1);
      var_p = &arg1;
    }

    if (!prog_error) {
      r->v.i = v_length(var_p);
    } else {
      r->v.i = 1;
    }
    break;

  case kwEMPTY:
    //
    // bool <- EMPTY(x)
    //
    if (code_isvar()) {
      var_p = code_getvarptr();
    } else {
      eval(&arg1);
      var_p = &arg1;
    }

    if (!prog_error) {
      r->v.i = v_isempty(var_p);
    } else {
      r->v.i = 1;
    }
    break;
  case kwLBOUND:
    //
    // int <- LBOUND(array [, dim])
    //
    if (code_peek() == kwTYPE_VAR) {
      var_p = code_getvarptr();
      if (var_p->type == V_ARRAY) {

        l = 1;
        if (code_peek() == kwTYPE_SEP) {
          par_getcomma();
          if (!prog_error) {
            eval(&arg1);
            if (!prog_error) {
              l = v_getint(&arg1);
            }
            v_free(&arg1);
          }
        }

        if (!prog_error) {
          l--;
          if (l >= 0 && l < var_p->v.a.maxdim) {
            r->v.i = var_p->v.a.lbound[l];
          } else {
            rt_raise(ERR_BOUND_DIM, var_p->v.a.maxdim, l);
          }
        }
      } else {
        rt_raise(ERR_BOUND_VAR);
      }
    } else {
      err_typemismatch();
    }
    break;

  case kwUBOUND:
    //
    // int <- UBOUND(array [, dim])
    //
    if (code_peek() == kwTYPE_VAR) {
      code_skipnext();
      var_p = tvar[code_getaddr()];
      if (var_p->type == V_ARRAY) {

        l = 1;
        if (code_peek() == kwTYPE_SEP) {
          par_getcomma();
          if (!prog_error) {
            eval(&arg1);
            if (!prog_error) {
              l = v_getint(&arg1);
            }
            v_free(&arg1);
          }
        }

        if (!prog_error) {
          l--;
          if (l >= 0 && l < var_p->v.a.maxdim) {
            r->v.i = var_p->v.a.ubound[l];
          } else {
            rt_raise(ERR_BOUND_DIM, var_p->v.a.maxdim);
          }
        }
      } else {
        rt_raise(ERR_BOUND_VAR);
      }
    } else {
      err_typemismatch();
    }
    break;

    // i <- RGB(r,g,b)
    // i <- RGBF(r,g,b)
  case kwRGB:
  case kwRGBF: {
    var_num_t rc, gc, bc;
    int code;

    pc = par_massget("FFF", &rc, &gc, &bc);
    IF_ERR_RETURN;
    code = 0;
    if (funcCode == kwRGBF) {
      if ((rc >= 0 && rc <= 1) && (gc >= 0 && gc <= 1) && (bc >= 0 && bc <= 1)) {
        code = 1;
      }
    } else {
      if ((rc >= 0 && rc <= 255) && (gc >= 0 && gc <= 255) && (bc >= 0 && bc <= 255)) {
        code = 2;
      }
    }

    // ////
    switch (code) {
    case 1:
      r->v.i = (r2int(rc * 255.0, 0, 255) << 16) | (r2int(gc * 255.0, 0, 255) << 8)
          | r2int(bc * 255.0, 0, 255);
      break;
    case 2:
      r->v.i = ((dword) rc << 16) | ((dword) gc << 8) | (dword) bc;
      break;
    default:
      err_argerr();
    }

    r->v.i = -r->v.i;
  }
    break;

  case kwIMGW: {
    // image width
    int h, i;
    par_getsharp();
    IF_ERR_RETURN;

    h = par_getint();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    i = par_getint();
    IF_ERR_RETURN;
    r->v.i = dev_image_width(h, i);
  }
    break;

  case kwIMGH: {
    // image height
    int h, i;
    par_getsharp();
    IF_ERR_RETURN;

    h = par_getint();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    i = par_getint();
    IF_ERR_RETURN;

    r->v.i = dev_image_height(h, i);
  }
    break;

  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (9)", funcCode);
  }

  v_free(&arg1);
  pfree3(s1, s2, s3);
}

/*
 * fp <- FUNC (...)
 */
void cmd_numN(long funcCode, var_t *r) {
  var_num_t x, y, m;
  int pw = 0;

  r->type = V_NUM;

  IF_ERR_RETURN;
  switch (funcCode) {
  case kwATAN2:
    // fp <- ATAN2(x,y)
    x = par_getnum();
    if (!prog_error) {
      par_getcomma();
      if (!prog_error) {
        y = par_getnum();
        r->v.n = atan2(x, y);
      }
    }
    break;

  case kwPOW:
    // fp <- POW(x,y)
    x = par_getnum();
    if (!prog_error) {
      par_getcomma();
      if (!prog_error) {
        y = par_getnum();
        r->v.n = pow(x, y);
      }
    }
    break;

  case kwROUND:
    // fp <- ROUND(x [,decs])
    x = par_getnum();
    if (!prog_error) {
      if (code_peek() == kwTYPE_SEP) {
        par_getcomma();
        if (!prog_error) {
          pw = par_getint();
        }
      } else {
        pw = 0;
      }
      if (!prog_error) {
        // round
        m = floor(pow(10.0, pw));
        if (SGN(x) < 0.0) {
          r->v.n = -floor((-x * m) + .5) / m;
        } else {
          r->v.n = floor((x * m) + .5) / m;
        }
      }
    }
    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (10)", funcCode);
  }
}

/*
 * any <- FUNC (...)
 */
void cmd_genfunc(long funcCode, var_t * r) {
  byte code, ready, first;
  int count, tcount, handle, i, len, ch;
  addr_t ofs;
  var_t *basevar_p, *elem_p;
  var_t arg, arg2;
  char tmp[3];
  var_num_t *dar;

  IF_ERR_RETURN;
  v_init(r);

  switch (funcCode) {
  //
  // val = IF(cond,true,false)
  //
  case kwIFF:
    v_init(&arg);
    eval(&arg);                 // condition
    if (!prog_error) {

      par_getcomma();
      IF_ERR_RETURN;
      ch = v_is_nonzero(&arg);
      v_free(&arg);

      if (ch) {
        eval(&arg);             // true-value
        if (!prog_error) {
          v_set(r, &arg);       // set the true value
          v_free(&arg);
        }
      } else {
        par_skip();
      }
      IF_ERR_RETURN;
      par_getcomma();
      IF_ERR_RETURN;
      if (!ch) {
        eval(&arg);             // false-value (there is no jump-optimization,
        // so we must
        // execute that)
        if (!prog_error) {
          v_set(r, &arg);       // set the false value
          v_free(&arg);
        }
      } else
        par_skip();
    }
    break;
    //
    // str = FORMAT$(fmt, n | s, ...)
    //
  case kwFORMAT:
    v_init(&arg);
    eval(&arg);                 // condition
    if (!prog_error) {
      par_getcomma();
      IF_ERR_RETURN;
      if (arg.type != V_STR) {
        rt_raise(ERR_FORMAT_INVALID_FORMAT);
        v_free(&arg);
      } else {
        char *buf;

        buf = tmp_alloc(1024);
        v_init(&arg2);
        eval(&arg2);
        if (!prog_error) {
          switch (arg2.type) {
          case V_STR:
            format_str(buf, (char *) arg.v.p.ptr, (char *) arg2.v.p.ptr);
          case V_INT:
            if (arg2.type == V_INT
              )
              format_num(buf, (char *) arg.v.p.ptr, arg2.v.i);
          case V_NUM:
            if (arg2.type == V_NUM
              )
              format_num(buf, (char *) arg.v.p.ptr, arg2.v.n);

            r->type = V_STR;
            r->v.p.size = strlen(buf) + 1;
            r->v.p.ptr = tmp_alloc(r->v.p.size);
            strcpy((char *) r->v.p.ptr, buf);
            break;
          default:
            err_typemismatch();
          }
        }

        v_free(&arg);
        v_free(&arg2);
        tmp_free(buf);
      }                         // arg.type = V_STR
    }                           // !prog_error
    break;
    //
    // int <- JULIAN(y, m, d) || JULIAN("[d]d/[m]m/[yy]yy")
    //
  case kwJULIAN: {
    long d, m, y;

    r->type = V_INT;

    v_init(&arg);
    eval(&arg);
    IF_ERR_RETURN;

    if (arg.type == V_STR) {
      date_str2dmy((char *) arg.v.p.ptr, &d, &m, &y);
      v_free(&arg);
    } else {
      d = v_igetval(&arg);
      v_free(&arg);
      par_getcomma();
      IF_ERR_RETURN;

      m = par_getint();
      IF_ERR_RETURN;

      par_getcomma();
      IF_ERR_RETURN;

      y = par_getint();
      IF_ERR_RETURN;
    }

    r->v.i = date_julian(d, m, y);
  }
    break;

    //
    // str <- DATEFMT(format, date$ || julian || d [, m, y])
    //

    //
    // int <- WEEKDAY(date$ | d,m,y | julian)
    //
  case kwDATEFMT:
  case kwWDAY: {
    long d, m, y;

    r->type = V_INT;
    r->v.i = 0;

    if (funcCode == kwDATEFMT) {  // format
      v_init(&arg);
      eval(&arg);
      IF_ERR_RETURN;

      par_getcomma();
      IF_ERR_RETURN;
    }

    v_init(&arg2);
    eval(&arg2);
    if (arg2.type == V_STR) {
      date_str2dmy((char *) arg2.v.p.ptr, &d, &m, &y);
      v_free(&arg2);
    } else {
      d = v_igetval(&arg2);
      v_free(&arg2);
      if (code_peek() == kwTYPE_SEP) {
        par_getcomma();
        if (prog_error) {
          v_free(&arg);
          return;
        }
        m = par_getint();
        if (prog_error) {
          v_free(&arg);
          return;
        }
        par_getcomma();
        if (prog_error) {
          v_free(&arg);
          return;
        }
        y = par_getint();
        if (prog_error) {
          v_free(&arg);
          return;
        }
      } else {
        // julian
        date_jul2dmy(d, &d, &m, &y);
      }
    }

    if (funcCode == kwDATEFMT) {  // format
      r->type = V_STR;
      r->v.p.size = 96;
      r->v.p.ptr = tmp_alloc(r->v.p.size);
      date_fmt((char *) arg.v.p.ptr, (char *) r->v.p.ptr, d, m, y);
      v_free(&arg);
    } else {                    // weekday
      r->v.i = date_weekday(d, m, y);
    }
  }
    break;

    //
    // STR <- INPUT$(len [, file])
    //
  case kwINPUTF:
    count = par_getint();
    IF_ERR_RETURN
    ;

    if (code_peek() == kwTYPE_SEP) {
      par_getcomma();
      IF_ERR_RETURN;

      handle = par_getint();
      IF_ERR_RETURN;
    } else {
      handle = -1;
    }
    if (handle == -1) {
      // keyboard
      r->type = V_STR;
      r->v.p.ptr = tmp_alloc((count << 1) + 1);
      r->v.p.ptr[0] = '\0';
      len = 0;
      for (i = 0; i < count; i++) {
        ch = dev_getch();       // MultiByte - dev_getchr() must return the
        // extended
        // code (2 bytes char)
        if (ch == 0xFFFF || prog_error
          )
          break;
        if ((ch & 0xFF00) == 0xFF00) {  // extra code - hardware keys
          tmp[0] = '\033';
          tmp[1] = ch & 0xFF;
          tmp[2] = '\0';
          len += 2;
        } else if (ch & 0xFF00) { // multibyte languages
          tmp[0] = ch >> 8;
          tmp[1] = ch & 0xFF;
          tmp[2] = '\0';
          len += 2;
        } else {                  // simple 8-bit character
          tmp[0] = ch;
          tmp[1] = '\0';
          len++;
        }

        strcat((char *) r->v.p.ptr, tmp);
      }

      r->v.p.size = len + 1;
      r->v.p.ptr[len] = '\0';
    } else {
      // file
      r->type = V_STR;
      r->v.p.ptr = tmp_alloc(count + 1);
      r->v.p.size = count + 1;
      dev_fread(handle, r->v.p.ptr, count);
      r->v.p.ptr[count] = '\0';
    }

    break;
    //
    // INT <- BGETC(file)
    //
  case kwBGETC:
    handle = par_getint();
    IF_ERR_RETURN
    ;

    // file
    dev_fread(handle, &code, 1);
    r->type = V_INT;
    r->v.i = (int) code;
    break;
    //
    // n<-POLYAREA(poly)
    //
  case kwPOLYAREA: {
    int i, count;
    pt_t *poly = NULL;

    r->type = V_NUM;

    count = par_getpoly(&poly);
    IF_ERR_RETURN;

    r->v.n = 0.0;
    for (i = 0; i < count - 1; i++) {
      r->v.n = r->v.n + (poly[i].x - poly[i + 1].x) * (poly[i].y + poly[i + 1].y);
    }

    // hmm.... closed ?
    tmp_free(poly);
  }
    break;

    //
    // [x,y]<-POLYCENT(poly)
    //
  case kwPOLYCENT: {
    pt_t *poly = NULL;
    int err, count;
    var_num_t x, y, area;

    r->type = V_NUM;

    count = par_getpoly(&poly);
    IF_ERR_RETURN;

    err = geo_polycentroid(poly, count, &x, &y, &area);
    v_toarray1(r, 2);
    v_setreal(v_getelemptr(r, 0), x);
    v_setreal(v_getelemptr(r, 1), y);
    if (err == 2 && area == 0) {
      rt_raise(ERR_CENTROID);
    } else {
      rt_raise(ERR_WRONG_POLY);
    }

    // hmm.... closed ?
    tmp_free(poly);
  }
    break;

    //
    // CX <- POINT(0)
    // CY <- POINT(1)
    // color <- POINT(x,y)
    //
  case kwPOINT: {
    int x = -1, y = -1;

    if (code_isvar()) {
      var_t *v;
      v = code_getvarptr();
      if (v->type == V_ARRAY) {
        if (v->v.a.size != 2)
          err_argerr();
        else {
          x = v_getint(v_elem(v, 0));
          y = v_getint(v_elem(v, 0));
        }
      } else {
        x = v_getint(v);
        if (code_peek() == kwTYPE_SEP) {
          par_getcomma();
          IF_ERR_RETURN;
          y = par_getint();
          IF_ERR_RETURN;
        }
      }
    } else {
      x = par_getint();
      IF_ERR_RETURN;
      if (code_peek() == kwTYPE_SEP) {
        par_getcomma();
        IF_ERR_RETURN;

        y = par_getint();
        IF_ERR_RETURN;
      }
    }

    //
    r->type = V_INT;
    r->v.i = 0;
    IF_ERR_RETURN;

    if (y == -1) {
      switch (x) {
      case 0:
        r->v.i = gra_x;
        break;
      case 1:
        r->v.i = gra_y;
        break;
      default:
        rt_raise(ERR_POINT);
      }
    } else {
      r->v.i = dev_getpixel(x, y);
    }
  }
    break;
    //
    // ? <- SEGLEN(Ax,Ay,Bx,By)
    //
  case kwSEGLEN: {
    pt_t A, B;
    var_num_t dx, dy;

    A = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    B = par_getpt();
    IF_ERR_RETURN;

    dx = B.x - A.x;
    dy = B.y - A.y;

    r->type = V_NUM;
    r->v.n = sqrt(dx * dx + dy * dy);
  }
    break;
    //
    // ? <- PTSIGN(Ax,Ay,Bx,By,Qx,Qy)
    //
  case kwPTSIGN: {
    pt_t A, B, Q;

    A = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    B = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    Q = par_getpt();
    IF_ERR_RETURN;

    r->type = V_INT;
    r->v.i = PTSIGN(A.x, A.y, B.x, B.y, Q.x, Q.y);
  }
    break;
    //
    // ? <- PTDISTSEG(Bx,By,Cx,Cy,Ax,Ay)
    // ? <- PTDISTLN(Bx,By,Cx,Cy,Ax,Ay)
    //
  case kwPTDISTSEG:
  case kwPTDISTLN: {
    pt_t A, B, C;

    B = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    C = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    A = par_getpt();
    IF_ERR_RETURN;

    r->type = V_NUM;

    if (funcCode == kwPTDISTLN)
      r->v.n = geo_distfromline(B.x, B.y, C.x, C.y, A.x, A.y);
    else
      r->v.n = geo_distfromseg(B.x, B.y, C.x, C.y, A.x, A.y);
  }
    break;
    //
    // ? <- SEGCOS(Ax,Ay,Bx,By,Cx,Cy,Dx,Dy)
    // ? <- SEGSIN(Ax,Ay,Bx,By,Cx,Cy,Dx,Dy)
    //
  case kwSEGCOS:
  case kwSEGSIN: {
    var_num_t Adx, Ady, Bdx, Bdy;
    pt_t A, B;

    A = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    B = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    Adx = B.x - A.x;
    Ady = B.y - A.y;

    A = par_getpt();
    IF_ERR_RETURN;

    par_getcomma();
    IF_ERR_RETURN;

    B = par_getpt();
    IF_ERR_RETURN;

    Bdx = B.x - A.x;
    Bdy = B.y - A.y;

    r->type = V_NUM;
    r->v.n = geo_segangle(funcCode, Adx, Ady, Bdx, Bdy);
  }
    break;
    //
    // ? <- MAX/MIN(...)
    //
  case kwMAX:
  case kwMIN:
  case kwABSMAX:
  case kwABSMIN:
  case kwSUM:
  case kwSUMSV:
  case kwSTATMEAN:
    ready = 0;
    first = 1;
    tcount = 0;
    r->type = V_NUM;

    do {
      code = code_peek();
      switch (code) {
      case kwTYPE_SEP:         // separator
        code_skipsep();
        break;
      case kwTYPE_LEVEL_END:   // ) -- end of parameters
        ready = 1;
        break;
      case kwTYPE_VAR:         // variable
        ofs = prog_ip;
        if (code_isvar()) {
          basevar_p = code_getvarptr();
          if (basevar_p->type == V_ARRAY) {
            count = basevar_p->v.a.size;  // the number of the elements
            for (i = 0; i < count; i++) {
              elem_p = v_getelemptr(basevar_p, i);

              if (!prog_error) {
                if (first) {
                  dar_first(funcCode, r, elem_p);
                  first = 0;
                } else
                  dar_next(funcCode, r, elem_p);

                tcount++;
              } else
                return;
            }
            break;
          }
        }
        prog_ip = ofs;
        // no 'break' here
      default:
        // default --- expression
        eval(&arg);
        if (!prog_error) {
          if (first) {
            dar_first(funcCode, r, &arg);
            first = 0;
          } else
            dar_next(funcCode, r, &arg);

          tcount++;
        } else
          return;
        v_free(&arg);
      }

      //
    } while (!ready);

    // final
    if (!prog_error)
      dar_final(funcCode, r, tcount);

    break;
    //
    //
    //
  case kwSTATMEANDEV:
  case kwSTATSPREADS:
  case kwSTATSPREADP:

    ready = 0;
    tcount = 0;
    len = 64;
    dar = (var_num_t*) tmp_alloc(sizeof(var_num_t) * len);

    do {
      code = code_peek();
      switch (code) {
      case kwTYPE_SEP:         // separator
        code_skipsep();
        break;
      case kwTYPE_LEVEL_END:   // ) -- end of parameters
        ready = 1;
        break;
      case kwTYPE_VAR:         // variable
        ofs = prog_ip;
        if (code_isvar()) {
          basevar_p = code_getvarptr();
          if (basevar_p->type == V_ARRAY) {
            count = basevar_p->v.a.size;  // the number of the elements
            for (i = 0; i < count; i++) {
              elem_p = v_getelemptr(basevar_p, i);
              if (!prog_error) {
                if (tcount >= len) {
                  len += 64;
                  dar = (var_num_t*) tmp_realloc(dar, sizeof(var_num_t) * len);
                }

                dar[tcount] = v_getval(elem_p);
                tcount++;
              } else {
                tmp_free(dar);
                return;
              }
            }

            break;
          }
        }
        prog_ip = ofs;
        // no 'break' here
      default:
        // default --- expression
        eval(&arg);
        if (!prog_error) {
          if (tcount >= len) {
            len += 64;
            dar = (var_num_t*) tmp_realloc(dar, sizeof(var_num_t) * len);
          }

          dar[tcount] = v_getval(&arg);
          tcount++;
        } else {
          tmp_free(dar);
          return;
        }
        v_free(&arg);
      }

      //
    } while (!ready);

    // final
    if (!prog_error) {
      r->type = V_NUM;
      switch (funcCode) {
      case kwSTATMEANDEV:
        r->v.n = statmeandev(dar, tcount);
        break;
      case kwSTATSPREADS:
        r->v.n = statspreads(dar, tcount);
        break;
      case kwSTATSPREADP:
        r->v.n = statspreadp(dar, tcount);
        break;
      }

      tmp_free(dar);
    }

    break;
    //
    // X <- LINEQGJ(A, B [, toler])
    // linear eq solve
    //
  case kwGAUSSJORDAN: {
    var_num_t toler = 0.0;
    var_num_t *m1;
    var_num_t *m2 = NULL;
    int32 n, rows, cols;
    var_t *a, *b;

    v_init(r);
    a = par_getvarray();
    IF_ERR_RETURN;
    m1 = mat_toc(a, &rows, &cols);
    if (rows != cols || cols < 2) {
      if (m1) {
        tmp_free(m1);
      }
      rt_raise(ERR_LINEEQN_ADIM, rows, cols);
    } else {
      n = rows;
      par_getcomma();
      if (prog_error) {
        if (m1) {
          tmp_free(m1);
        }
        return;
      }

      b = par_getvarray();
      if (prog_error) {
        if (m1) {
          tmp_free(m1);
        }
        return;
      }
      m2 = mat_toc(b, &rows, &cols);

      if (rows != n || cols != 1) {
        if (m1) {
          tmp_free(m1);
        }
        if (m2) {
          tmp_free(m2);
        }
        rt_raise(ERR_LINEEQN_BDIM, rows, cols);
        return;
      }

      if (code_peek() == kwTYPE_SEP) {
        code_skipsep();
        toler = par_getnum();
      }

      if (!prog_error) {
        mat_gauss_jordan(m1, m2, n, toler);
        mat_tov(r, m2, n, 1, 1);
      }

      if (m1)
        tmp_free(m1);
      if (m2)
        tmp_free(m2);
    }
  }
    break;
    //
    // array <- INVERSE(A)
    //
  case kwINVERSE: {
    var_num_t *m1;
    int32 n, rows, cols;
    var_t *a;

    v_init(r);
    a = par_getvarray();
    IF_ERR_RETURN;

    m1 = mat_toc(a, &rows, &cols);
    if (rows != cols || cols < 2) {
      if (m1) {
        tmp_free(m1);
      }
      rt_raise(ERR_WRONG_MAT, rows, cols);
    } else {
      n = rows;
      mat_inverse(m1, n);
      mat_tov(r, m1, n, n, 1);
      tmp_free(m1);
    }
  }
    break;
    //
    // n <- DETERM(A)
    //
  case kwDETERM: {
    var_num_t *m1 = NULL, toler = 0;
    int32 n, rows, cols;
    var_t *a;

    v_init(r);
    a = par_getvarray();
    IF_ERR_RETURN;

    if (code_peek() == kwTYPE_SEP) {
      code_skipsep();
      toler = par_getnum();
    }

    m1 = mat_toc(a, &rows, &cols);
    if (rows != cols || cols < 2) {
      if (m1) {
        tmp_free(m1);
      }
      rt_raise(ERR_WRONG_MAT, rows, cols);
    } else {
      n = rows;
      r->type = V_NUM;
      r->v.n = mat_determ(m1, n, toler);
      tmp_free(m1);
    }
  }
    break;
    //
    // array <- CODEARRAY(x1,y1...[;x2,y2...])
    // its used to create dynamic arrays.
    // its not a function, its the [ ] operators
    //
  case kwCODEARRAY: {
    var_int_t curcol, ready, count, pos;
    var_t *e;
    int32 rows, cols;
    tmplist_t lst;
    canode_t tnode, *tp;
    tmpnode_t *cur;

    rows = 0;
    cols = 0;
    curcol = 0;
    ready = 0;
    count = 0;
    tmplist_init(&lst);

    do {
      code = code_peek();
      switch (code) {
      case kwTYPE_SEP:       // separator
        code_skipnext();
        if (code_peek() == ';') { // next row
          rows++;
          curcol = 0;
        } else {                // next col
          curcol++;
          if (curcol > cols) {
            cols = curcol;
          }
        }
        code_skipnext();
        break;
      case kwTYPE_LEVEL_END: // ) -- end of parameters
        ready = 1;
        break;
      default:
        tnode.col = curcol;
        tnode.row = rows;
        tnode.v = v_new();
        eval(tnode.v);
        if (!prog_error) {
          tmplist_add(&lst, &tnode, sizeof(canode_t));
          count++;
        }
      }

      //
    } while (!ready);

    //
    // create the array
    //
    rows++;
    cols++;
    if (rows > 1) {
      v_tomatrix(r, rows, cols);
    } else {
      v_toarray1(r, cols);
    }
    cur = lst.head;
    while (cur) {
      tp = (canode_t *) cur->data;
      pos = tp->row * cols + tp->col;
      e = (var_t *) (r->v.a.ptr + (sizeof(var_t) * pos));
      v_set(e, tp->v);
      v_free(tp->v);
      tmp_free(tp->v);
      cur = cur->next;
    }

    tmplist_clear(&lst);
  }

    break;
    //
    // array <- FILES([wildcards])
    //
  case kwFILES: {
    int count, i;
    char_p_t *list;
    var_t arg, *elem_p;
    char *wc = NULL;

    v_init(&arg);
    if (code_peek() != kwTYPE_LEVEL_END) {
      par_getstr(&arg);
      wc = (char *) arg.v.p.ptr;
    }

    if (!prog_error) {
      // get the files
      list = dev_create_file_list(wc, &count);

      // create the array
      if (count) {
        v_toarray1(r, count);

        // add the entries
        for (i = 0; i < count; i++) {
          elem_p = (var_t *) (r->v.a.ptr + (sizeof(var_t) * i));

          elem_p->type = V_STR;
          elem_p->v.p.size = strlen(list[i]) + 1;
          elem_p->v.p.ptr = tmp_alloc(elem_p->v.p.size);
          strcpy(elem_p->v.p.ptr, list[i]);
        }
      } else {
        v_toarray1(r, 0);
      }
      // cleanup
      if (list) {
        dev_destroy_file_list(list, count);
      }
      v_free(&arg);
    }
  }

    break;
    //
    // array <- SEQ(min, max, count)
    //
  case kwSEQ: {
    var_t *elem_p;
    var_int_t count;
    var_num_t xmin, xmax, x, dx;

    par_massget("FFI", &xmin, &xmax, &count);

    if (!prog_error) {
      // create the array
      if (count > 1) {
        v_toarray1(r, count);
        dx = (xmax - xmin) / (count - 1);

        // add the entries
        for (i = 0, x = xmin; i < count; i++, x += dx) {
          elem_p = (var_t *) (r->v.a.ptr + (sizeof(var_t) * i));

          elem_p->type = V_NUM;
          elem_p->v.n = x;
        }
      } else {
        v_toarray1(r, 0);
      }
    } else {
      v_toarray1(r, 0);
    }
  }

    break;
  default:
    rt_raise("Unsupported built-in function call %ld, please report this bug (11)", funcCode);
  };
}
