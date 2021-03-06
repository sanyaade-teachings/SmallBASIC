// This file is part of SmallBASIC
//
// byte-code executor
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//
// Copyright(C) 2000 Nicholas Christopoulos

#define BRUN_MODULE

#include "common/sys.h"
#include "common/panic.h"
#include "common/blib.h"
#include "common/blib_ui.h"
#include "common/str.h"
#include "common/fmt.h"
#include "common/extlib.h"
#include "common/units.h"
#include "common/kw.h"
#include "common/var.h"
#include "common/scan.h"
#include "common/smbas.h"
#include "common/messages.h"
#include "common/device.h"
#include "common/pproc.h"
#include "common/var_uds.h"

int brun_create_task(const char *filename, mem_t preloaded_bc, int libf);
int exec_close_task();
void exec_setup_predefined_variables();
var_t *code_isvar_arridx(var_t *basevar_p);
void code_pop_until(int type);
void code_pop_and_free(stknode_t *node);
stknode_t *code_stackpeek();
void sys_before_comp();
void sys_after_comp();
void sys_before_run();
void sys_after_run();
int sbasic_exec_task(int tid);
int sbasic_recursive_exec(int tid);
void sbasic_exec_prepare(const char *filename);
int sbasic_exec(const char *file);
int sbasic_main(const char *file);
int exec_close(int tid);
int sbasic_exec(const char *file);
void cmd_options(void);
var_t *code_resolve_varptr(var_t* var_p, int until_parens);

static dword evt_check_every;
static char fileName[OS_FILENAME_SIZE + 1];
static int main_tid;
static int exec_tid;

/**
 * returns the next 32bit and moves the instruction pointer to the next instruction
 */
dword code_getnext32() {
  dword v;

  memcpy(&v, prog_source + prog_ip, 4);
  prog_ip += 4;
  return v;
}

#if defined(OS_PREC64)

/**
 * returns the next 64bit and moves the instruction pointer to the next instruction
 */
var_int_t code_getnext64i()
{
  var_int_t v;

  memcpy(&v, prog_source + prog_ip, sizeof(var_int_t));
  prog_ip += sizeof(var_int_t);
  return v;
}
#endif

/**
 * returns the next 64bit and moves the instruction pointer to the next instruction
 */
double code_getnext64f() {
  double v;

  memcpy(&v, prog_source + prog_ip, sizeof(double));
  prog_ip += sizeof(double);
  return v;
}

#if defined(OS_PREC64)

/**
 * returns the next 128bit and moves the instruction pointer to the next instruction
 */
var_num_t code_getnext128f()
{
  var_num_t v;

  memcpy(&v, prog_source + prog_ip, sizeof(var_num_t));
  prog_ip += sizeof(var_num_t);
  return v;
}
#endif

/**
 * jump to label
 */
void code_jump_label(word label_id) {
  prog_ip = tlab[label_id].ip;
}

/**
 * Put the node 'node' in stack (PUSH)
 */
void code_push(stknode_t * node) {
#if defined(_UnixOS) && defined(_CHECK_STACK)
  int i;
#endif
  if (prog_stack_count + 1 >= prog_stack_alloc) {
    err_stackoverflow();
    return;
  }

  prog_stack[prog_stack_count] = *node;
  prog_stack[prog_stack_count].line = prog_line;
#if defined(_UnixOS) && defined(_CHECK_STACK)
  for (i = 0; keyword_table[i].name[0] != '\0'; i++) {
    if (node->type == keyword_table[i].code) {
      printf("%3d: PUSH %s (%d)\n", prog_stack_count,
          keyword_table[i].name, prog_line);
      break;
    }
  }
#endif
  prog_stack_count++;
}

/**
 * Returns and deletes the topmost node from stack (POP)
 */
void code_pop(stknode_t * node) {
#if defined(_UnixOS) && defined(_CHECK_STACK)
  int i;
#endif
  if (prog_stack_count) {
    prog_stack_count--;
    if (node)
      *node = prog_stack[prog_stack_count];
#if defined(_UnixOS) && defined(_CHECK_STACK)
    for (i = 0; keyword_table[i].name[0] != '\0'; i++) {
      if (prog_stack[prog_stack_count].type == keyword_table[i].code) {
        printf("%3d: POP %s (%d)\n", prog_stack_count,
            keyword_table[i].name, prog_line);
        break;
      }
    }
#endif
  } else {
    if (node) {
      err_stackunderflow();
      node->type = 0xFF;
    }
  }
}

/**
 * Returns and deletes the topmost node from stack (POP)
 */
void code_pop_and_free(stknode_t * node) {
#if defined(_UnixOS) && defined(_CHECK_STACK)
  int i;
#endif

  if (prog_stack_count) {
    stknode_t *cur_node;

    prog_stack_count--;
    if (node)
      *node = prog_stack[prog_stack_count];

#if defined(_UnixOS) && defined(_CHECK_STACK)
    for (i = 0; keyword_table[i].name[0] != '\0'; i++) {
      if (prog_stack[prog_stack_count].type == keyword_table[i].code) {
        printf("%3d: POP %s (%d)\n", prog_stack_count,
            keyword_table[i].name, prog_line);
        break;
      }
    }
#endif

    // free node's data
    cur_node = &prog_stack[prog_stack_count];
    switch (cur_node->type) {
    case kwTYPE_CRVAR:
      v_free(tvar[cur_node->x.vdvar.vid]);  // free local variable data
      tmp_free(tvar[cur_node->x.vdvar.vid]);
      tvar[cur_node->x.vdvar.vid] = cur_node->x.vdvar.vptr; // restore ptr
      break;

    case kwBYREF:
      tvar[cur_node->x.vdvar.vid] = cur_node->x.vdvar.vptr;
      break;

    case kwTYPE_VAR:
      if ((cur_node->x.param.vcheck == 1) || (cur_node->x.param.vcheck == 0x81)) {
        v_free(cur_node->x.param.res);
        tmp_free(cur_node->x.param.res);
      }
      break;

    case kwTYPE_RET:
      v_free(cur_node->x.vdvar.vptr); // free ret-var
      tmp_free(cur_node->x.vdvar.vptr);
      break;

    case kwFUNC:
      if (cur_node->x.vcall.rvid != INVALID_ADDR) {
        tvar[cur_node->x.vcall.rvid] = cur_node->x.vcall.retvar;  // restore ptr
      }
      break;

    case kwFOR:
      if (cur_node->x.vfor.subtype == kwIN) {
        if (cur_node->x.vfor.flags & 1) {
          // allocated in for
          v_free(cur_node->x.vfor.arr_ptr);
          tmp_free(cur_node->x.vfor.arr_ptr);
        }
      }
    }
  } else {
    if (node) {
      err_stackunderflow();
      node->type = 0xFF;
    }
  }
}

/**
 * removes nodes from stack until 'type' node found
 */
void code_pop_until(int type) {
  stknode_t node;

  code_pop(&node);
  while (node.type != type) {
    code_pop(&node);
    IF_ERR_RETURN;
  }
}

/**
 * Peek the topmost node of stack
 */
stknode_t *code_stackpeek() {
  if (prog_stack_count) {
    return &prog_stack[prog_stack_count - 1];
  }
  return NULL;
}

/**
 * Convertion multi-dim index to one-dim index
 */
addr_t getarrayidx(var_t* array, var_t** var_hash_val) {
  byte code;
  var_t var;
  addr_t idx = 0, lev = 0, m = 0;
  addr_t idim, i;

  do {
    v_init(&var);
    eval(&var);
    IF_ERR_RETURN_0;

    if (var.type == V_STR) {
      // array elemement is a string - convert array to hash
      hash_get_value(array, &var, var_hash_val);

      if (code_peek() == kwTYPE_LEVEL_END) {
        code_skipnext();
      } else {
        err_missing_sep();
      }
      v_free(&var);
      return 0;
    } else {
      idim = v_getint(&var);
      v_free(&var);
      IF_ERR_RETURN_0;

      idim = idim - array->v.a.lbound[lev];

      m = idim;
      for (i = lev + 1; i < array->v.a.maxdim; i++) {
        m = m * (ABS(array->v.a.ubound[i] - array->v.a.lbound[i]) + 1);
      }
      idx += m;

      // skip separator
      code = code_peek();
      if (code == kwTYPE_SEP) {
        code_skipnext();
        if (code_getnext() != ',') {
          err_syntax_error();
        }
      }
      // next
      lev++;
    }
  } while (code_peek() != kwTYPE_LEVEL_END);

  if (!prog_error) {
    if ((int) array->v.a.maxdim != lev) {
      err_missing_sep();
    }
  }
  return idx;
}

/**
 * Used by code_getvarptr() to retrieve an element ptr of an array
 */
var_t *code_getvarptr_arridx(var_t* basevar_p) {
  addr_t array_index;
  var_t *var_p = NULL;

  if (code_peek() != kwTYPE_LEVEL_BEGIN) {
    err_arrmis_lp();
  } else {
    code_skipnext();            // '('
    array_index = getarrayidx(basevar_p, &var_p);

    if (var_p != NULL) {
      // hash map value
      return var_p;
    }

    if (!prog_error) {
      if ((int) array_index < basevar_p->v.a.size && (int) array_index >= 0) {
        var_p = (var_t *)(basevar_p->v.a.ptr + (array_index * sizeof(var_t)));

        if (code_peek() == kwTYPE_LEVEL_END) {
          code_skipnext();      // ')', ')' level
          if (code_peek() == kwTYPE_LEVEL_BEGIN) {
            // there is a second array inside
            if (var_p->type != V_ARRAY) {
              err_varisnotarray();
            } else {
              return code_getvarptr_arridx(var_p);
            }
          }
        } else {
          err_arrmis_rp();
        }
      } else {
        err_arridx(array_index, basevar_p->v.a.size);
      }
    }
  }

  return var_p;
}

/**
 * resolve a composite variable reference, eg: ar.ch(0).foo
 */
var_t* code_resolve_varptr(var_t* var_p, int until_parens) {
  if (var_p) {
    switch (code_peek()) {
    case kwTYPE_LEVEL_BEGIN:
      if (!until_parens) {
        var_p = code_resolve_varptr(code_getvarptr_arridx(var_p), until_parens);
      }
      break;
    case kwTYPE_UDS_EL:
      var_p = code_resolve_varptr(uds_resolve_fields(var_p), until_parens);
      break;
    }
  }
  return var_p;
}

/**
 * returns the varptr of the next variable. if the variable is an array 
 * returns the element ptr
 */
var_t *code_getvarptr() {
  return code_getvarptr_parens(0);
}

/**
 * helper for code_getvarptr
 */
var_t* code_getvarptr_parens(int until_parens) {
  var_t *var_p = NULL;

  switch (code_peek()) {
  case kwTYPE_VAR:
    code_skipnext();
    var_p = tvar[code_getaddr()];
    switch (var_p->type) {
    case V_HASH:
    case V_ARRAY:
      var_p = code_resolve_varptr(var_p, until_parens);
      break;
    default:
      if (!until_parens && code_peek() == kwTYPE_LEVEL_BEGIN) {
        err_varisnotarray();
      }
    }
    break;

  case kwTYPE_UDS:
    code_skipnext();
    var_p = tvar[code_getaddr()];
    var_p = code_resolve_varptr(uds_resolve_fields(var_p), until_parens);
    break;
  }

  if (var_p == NULL && !prog_error) {
    err_notavar();
    return tvar[0];
  }

  return var_p;
}

/**
 * Used by code_isvar() to retrieve an element ptr of an array
 */
var_t *code_isvar_arridx(var_t * basevar_p) {
  addr_t array_index;
  var_t *var_p = NULL;

  if (code_peek() != kwTYPE_LEVEL_BEGIN) {
    return NULL;
  } else {
    code_skipnext();            // '('
    array_index = getarrayidx(basevar_p, &var_p);

    if (var_p != NULL) {
      // hash map value
      return var_p;
    }

    if (!prog_error) {
      if ((int) array_index < basevar_p->v.a.size) {
        var_p = (var_t *)(basevar_p->v.a.ptr + (array_index * sizeof(var_t)));

        if (code_peek() == kwTYPE_LEVEL_END) {
          code_skipnext();      // ')', ')' level
          if (code_peek() == kwTYPE_LEVEL_BEGIN) {
            // there is a second array inside
            if (var_p->type != V_ARRAY) {
              return NULL;
            } else {
              return code_isvar_arridx(var_p);
            }
          }
        } else {
          return NULL;
        }
      } else {
        return NULL;
      }
    }
  }

  return var_p;
}

/**
 * returns true if the next code is a variable.
 * if the following code is an expression (no matter if the first item is a variable),
 * returns false
 */
int code_isvar() {
  var_t *basevar_p, *var_p = NULL;
  addr_t cur_ip;

  cur_ip = prog_ip;             // store IP

  switch (code_peek()) {
  case kwTYPE_VAR:
    code_skipnext();
    var_p = basevar_p = tvar[code_getaddr()];

    switch (basevar_p->type) {
    case V_HASH:
    case V_ARRAY:
      // variable is an array or hash
      var_p = code_resolve_varptr(var_p, 0);
      break;
    default:
      if (code_peek() == kwTYPE_LEVEL_BEGIN) {
        var_p = NULL;
      }
    }
    break;

  case kwTYPE_UDS:
    code_skipnext();
    var_p = tvar[code_getaddr()];
    var_p = code_resolve_varptr(uds_resolve_fields(var_p), 0);
    break;
  }

  if (var_p) {
    if (kw_check_evexit(code_peek()) || code_peek() == kwTYPE_LEVEL_END) {
      prog_ip = cur_ip;         // restore IP
      return 1;
    }
  }

  prog_ip = cur_ip;             // restore IP
  return 0;
}

/**
 * sets the value of an integer system-variable
 */
void setsysvar_int(int index, var_int_t value) {
  int tid;
  int i;

  tid = ctask->tid;
  for (i = 0; i < count_tasks(); i++) {
    activate_task(i);
    if (ctask->has_sysvars) {
      var_t *var_p = tvar[index];

      var_p->type = V_INT;
      var_p->const_flag = 1;
      var_p->v.i = value;
    }
  }
  activate_task(tid);
}

/**
 * sets the value of a real system-variable
 */
void setsysvar_num(int index, var_num_t value) {
  int tid;
  int i;

  tid = ctask->tid;
  for (i = 0; i < count_tasks(); i++) {
    activate_task(i);
    if (ctask->has_sysvars) {
      var_t *var_p = tvar[index];

      var_p->type = V_NUM;
      var_p->const_flag = 1;
      var_p->v.n = value;
    }
  }
  activate_task(tid);
}

/**
 * sets the value of an string system-variable
 */
void setsysvar_str(int index, const char *value) {
  int tid;
  int i;
  int l = strlen(value) + 1;

  tid = ctask->tid;
  for (i = 0; i < count_tasks(); i++) {
    activate_task(i);
    if (ctask->has_sysvars) {
      var_t *var_p = tvar[index];

      if (var_p->type == V_STR) {
        tmp_free(var_p->v.p.ptr);
      }
      var_p->type = V_STR;
      var_p->const_flag = 1;
      var_p->v.p.ptr = tmp_alloc(l);
      strcpy(var_p->v.p.ptr, value);
      var_p->v.p.size = l;
    }
  }
  activate_task(tid);
}

/**
 * create predefined system variables for this task
 */
void exec_setup_predefined_variables() {
  char homedir[OS_PATHNAME_SIZE + 1];
#if defined(_UnixOS)
  int l;
#endif

  // needed here (otherwise task will not updated)
  ctask->has_sysvars = 1;

  setsysvar_int(SYSVAR_OSVER, os_ver);
  setsysvar_str(SYSVAR_OSNAME, OS_NAME);
  setsysvar_str(SYSVAR_SBVER, SB_STR_VER);
  setsysvar_num(SYSVAR_PI, SB_PI);

  // Change from Haraszti - 30/3/2007 Thanks Atilla :)
  // setsysvar_int(SYSVAR_XMAX, 159);
  // setsysvar_int(SYSVAR_YMAX, 159);
  // setsysvar_int(SYSVAR_BPP, 1);
  setsysvar_int(SYSVAR_XMAX, os_graf_mx - 1);
  setsysvar_int(SYSVAR_YMAX, os_graf_my - 1);
  if (os_graphics) {
    setsysvar_int(SYSVAR_BPP, os_color_depth);
  }
  else {
    setsysvar_int(SYSVAR_BPP, 4);
  }
  // end of change

  setsysvar_int(SYSVAR_TRUE, -1);
  setsysvar_int(SYSVAR_FALSE, 0);
  setsysvar_int(SYSVAR_LINECHART, 1);
  setsysvar_int(SYSVAR_BARCHART, 2);
  setsysvar_str(SYSVAR_PWD, dev_getcwd());
  setsysvar_str(SYSVAR_COMMAND, opt_command);

#if defined(_UnixOS)
  if (dev_getenv("HOME")) {
    strcpy(homedir, dev_getenv("HOME"));
  }
  else {
    strcpy(homedir, "/tmp/");
  }
  l = strlen(homedir);
  if (homedir[l - 1] != OS_DIRSEP) {
    homedir[l] = OS_DIRSEP;
    homedir[l + 1] = '\0';
  }
  setsysvar_str(SYSVAR_HOME, homedir);
#elif defined(_Win32)
  if (dev_getenv("HOME")) {     // this works on cygwin
    strcpy(homedir, dev_getenv("HOME"));
  }
  else {
    char *p;

    GetModuleFileName(NULL, homedir, 1024);
    p = strrchr(homedir, '\\');
    *p = '\0';
    strcat(homedir, "\\");

    if (OS_DIRSEP == '/') {
      p = homedir;
      while (*p) {
        if (*p == '\\')
        *p = '/';
        p++;
      }
    }
  }
  setsysvar_str(SYSVAR_HOME, homedir);  // mingw32

  {
    static char stupid_os_envsblog[1024]; // it must be static at
    // least by default on DOS 
    // or Win32(BCB)
    sprintf(stupid_os_envsblog, "SBLOG=%s%csb.log", homedir, OS_DIRSEP);
    putenv(stupid_os_envsblog);
  }
#else
  setsysvar_str(SYSVAR_HOME, "");
#endif
}

/**
 * BREAK
 */
void brun_stop() {
  prog_error = -3;
}

/**
 * returns the status of executor (runing or stopped)
 */
int brun_status() {
  if (prog_error) {
    return BRUN_STOPPED;
  }
  return BRUN_RUNNING;
}

/**
 * BREAK - display message, too
 */
void brun_break() {
  if (brun_status() == BRUN_RUNNING) {
    inf_break(prog_line);
  }
  brun_stop();
}

/**
 * CHAIN sb-source
 */
void cmd_chain(void) {
  var_t var;
  int success, tid_main, tid_base, tid_prev;
  const char *code = 0;
  char *code_alloc = 0;

  v_init(&var);
  eval(&var);

  if (prog_error) {
    v_free(&var);
    return;
  }

  if (var.type == V_STR) {
    if (access(var.v.p.ptr, R_OK) == 0) {
      // argument is a file name
      FILE *f = fopen(var.v.p.ptr, "r");
      if (!fseek(f, 0, SEEK_END)) {
        int len = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (len) {
          code_alloc = tmp_alloc(len + 1);
          fgets(code_alloc, len, f);
          code = code_alloc;
        }
      }
      fclose(f);
    }
    if (!code) {
      code = var.v.p.ptr;
    }
  } else if (var.type == V_ARRAY) {
    int el;
    int len = 0;
    for (el = 0; el < var.v.a.size; el++) {
      var_t *el_p = (var_t *)(var.v.a.ptr + sizeof(var_t) * el);
      if (el_p->type == V_STR) {
        int str_len = strlen(el_p->v.a.ptr) + 2;
        if (len) {
          code_alloc = tmp_realloc(code_alloc, len + str_len);
          strcat(code_alloc, el_p->v.a.ptr);
        } else {
          code_alloc = tmp_alloc(str_len);
          strcpy(code_alloc, el_p->v.a.ptr);
        }
        strcat(code_alloc, "\n");
        len += str_len + 1;
      }
    }
    if (len) {
      code = code_alloc;
    }
  }

  if (code == 0) {
    v_free(&var);
    err_typemismatch();
    return;
  }

  tid_base = create_task("CH_BASE");
  tid_prev = activate_task(tid_base);

  // compile the buffer
  sys_before_comp();
  success = comp_compile_buffer(code);
  sys_after_comp();

  v_free(&var);
  if (code_alloc) {
    tmp_free(code_alloc);
  }
  if (success == 0) {
    close_task(tid_base);
    activate_task(tid_prev);
    prog_error = 1;
    return;
  }

  tid_main = brun_create_task("CH_MAIN", bytecode_h, 0);
  sys_before_run();

  dev_init(opt_graphics, 0);
  exec_sync_variables(0);

  bc_loop(0);
  success = prog_error;         // save tid_main status

  sys_after_run();
  exec_close_task();            // cleanup task data - tid_main
  close_task(tid_main);         // cleanup task container
  close_task(tid_base);         // cleanup task container
  activate_task(tid_prev);      // resume calling task

  if (success == 0) {
    prog_error = 1;
    return;
  }
}

/**
 * RUN "program"
 *
 */
void cmd_run(int retf) {
  var_t var;

  v_init(&var);
  eval(&var);
  IF_ERR_RETURN;

  if (var.type != V_STR) {
    err_typemismatch();
    return;
  } else {
    strcpy(fileName, var.v.p.ptr);
    v_free(&var);
  }

  if (!dev_run(fileName, retf)) {
    err_run_err(fileName);
  }
}

/**
 * OPTION (run-time part) keyword
 */
void cmd_options(void) {
  byte c;
  addr_t data;

  c = code_getnext();
  data = code_getaddr();
  switch (c) {
  case OPTION_BASE:
    opt_base = data;
    break;
  case OPTION_UICS:
    opt_uipos = data;
    break;
  case OPTION_MATCH:
    opt_usepcre = data;
    break;
  };
}

/**
 * execute commands (loop)
 *
 * @param isf if 1, the program must return if found return (by level <= 0);
 * otherwise an RTE will generated
 * if 2; like 1, but increase the proc_level because UDF call it was executed internaly
 */
void bc_loop(int isf) {
  dword now;
  static dword next_check;
  byte code, pops;
  byte trace_flag = 0;
  addr_t next_ip;
  int proc_level = 0;
  pcode_t pcode;
#if !defined(OS_LIMITED)
  int i;
#endif

  /**
   *   DO NOT CREATE FUNCTION-PTR-TABLE
   *   function tables didn't work on multi-segment version
   *
   *   For commands witch changes the IP use
   *
   *   case mycommand:
   *       command();
   *       if  ( prog_error )  break;
   *       continue;
   */

  // printf("task #%d; bc_loop(%d)\n", ctask->tid, isf);
  if (isf == 2) {
    proc_level++;
  }
  while (prog_ip < prog_length) {

#if defined(_Win32)
    now = GetTickCount();
#else
    now = clock();
#endif

    // events
    // every ~50ms, check events (on some drivers that redraws the
    // screen)
    if (now >= next_check) {
      next_check = now + evt_check_every;

      switch (dev_events(0)) {
      case -1:
        break;                  // break event
      case -2:
        prog_error = -2;
        inf_break(prog_line);
        break;
      };
    }
    // proceed to the next command
    if (!prog_error) {
      // next command
      code = prog_source[prog_ip];
      prog_ip++;

      // debug
      /*
       * fprintf(stderr, "\t%d: %d = ", prog_ip, code); for ( i = 0; 
       * keyword_table[i].name[0] != '\0'; i ++) { if ( code ==
       * keyword_table[i].code ) { fprintf(stderr,"%s ",
       * keyword_table[i].name); break; } } fprintf(stderr,"\n"); 
       */

      switch (code) {
      case kwLABEL:
      case kwREM:
      case kwTYPE_EOC:
        continue;
      case kwTYPE_LINE:
        prog_line = code_getaddr();
        if (trace_flag) {
          dev_printf("<%d>", prog_line);
        }
        continue;
      case kwLET:
        cmd_let(0);
        break;
      case kwCONST:
        cmd_let(1);
        break;
      case kwGOTO:
        next_ip = code_getaddr();

        // clear the stack (whatever you can)
        pops = code_getnext();
        while (pops > 0) {
          code_pop_and_free(NULL);
          pops--;
        }

        // jump
        prog_ip = next_ip;
        continue;
      case kwGOSUB:
        cmd_gosub();
        IF_ERR_BREAK;
        continue;
      case kwRETURN:
        cmd_return();
        IF_ERR_BREAK;
        continue;
      case kwONJMP:
        cmd_on_go();
        IF_ERR_BREAK;
        continue;
      case kwPRINT:
        cmd_print(PV_CONSOLE);
        break;
      case kwINPUT:
        cmd_input(PV_CONSOLE);
        break;
      case kwIF:
        cmd_if();
        IF_ERR_BREAK;
        continue;
      case kwELIF:
        cmd_elif();
        IF_ERR_BREAK;
        continue;
      case kwELSE:
        cmd_else();
        IF_ERR_BREAK;
        continue;
      case kwENDIF:
        cmd_endif();
        IF_ERR_BREAK;
        continue;
      case kwFOR:
        cmd_for();
        IF_ERR_BREAK;
        continue;
      case kwNEXT:
        cmd_next();
        IF_ERR_BREAK;
        continue;
      case kwWHILE:
        cmd_while();
        IF_ERR_BREAK;
        continue;
      case kwWEND:
        cmd_wend();
        IF_ERR_BREAK;
        continue;
      case kwREPEAT:
        cmd_repeat();
        IF_ERR_BREAK;
        continue;
      case kwUNTIL:
        cmd_until();
        IF_ERR_BREAK;
        continue;
      case kwSELECT:
        cmd_select();
        IF_ERR_BREAK;
        continue;
      case kwCASE:
        cmd_case();
        IF_ERR_BREAK;
        continue;
      case kwCASE_ELSE:
        cmd_case_else();
        IF_ERR_BREAK;
        continue;
      case kwENDSELECT:
        cmd_end_select();
        IF_ERR_BREAK;
        continue;
      case kwDIM:
        cmd_dim(0);
        break;
      case kwREDIM:
        cmd_redim();
        break;
      case kwAPPEND:
        cmd_ladd();
        break;
      case kwINSERT:
        cmd_lins();
        break;
      case kwDELETE:
        cmd_ldel();
        break;
      case kwERASE:
        cmd_erase();
        break;
      case kwREAD:
        cmd_read();
        break;
      case kwDATA:
        cmd_data();
        break;
      case kwRESTORE:
        cmd_restore();
        break;
      case kwOPTION:
        cmd_options();
        break;

        /*
         * ----------------------------------------- * external
         * procedures 
         */
      case kwTYPE_CALLEXTP:    // [lib][index]
      {
        addr_t lib, idx;

        lib = code_getaddr();
        idx = code_getaddr();
        if (lib & UID_UNIT_BIT) {
          unit_exec(lib & (~UID_UNIT_BIT), idx, NULL);
          if (gsb_last_error) {
            prog_error = gsb_last_error;
          }
          IF_ERR_BREAK;
        } else {
          sblmgr_procexec(lib, prog_symtable[idx].exp_idx);
        }
      }
        break;
        /*
         * ----------------------------------------- * buildin
         * procedures -- BEGIN 
         */
      case kwTYPE_CALLP:
        pcode = code_getaddr();
        switch (pcode) {
        case kwCLS:
          // cdw-s 19/11/2004
          // dev_cls(); called in graph_reset()
          graph_reset();
          break;
        case kwRTE:
          cmd_RTE();
          break;
          // case kwSHELL:
          // break;
        case kwENVIRON:
          cmd_environ();
          break;
        case kwLOCATE:
          cmd_locate();
          break;
        case kwAT:
          cmd_at();
          break;
        case kwPEN:
          cmd_pen();
          break;
        case kwDATEDMY:
          cmd_datedmy();
          break;
        case kwTIMEHMS:
          cmd_timehms();
          break;
        case kwBEEP:
          cmd_beep();
          break;
        case kwSOUND:
          cmd_sound();
          break;
        case kwNOSOUND:
          cmd_nosound();
          break;
        case kwPSET:
          cmd_pset();
          break;
        case kwRECT:
          cmd_rect();
          break;
        case kwCIRCLE:
          cmd_circle();
          break;
        case kwRANDOMIZE:
          cmd_randomize();
          break;
        case kwWSPLIT:
          cmd_wsplit();
          break;
        case kwSPLIT:
          cmd_split();
          break;
        case kwWJOIN:
          cmd_wjoin();
          break;
        case kwPAUSE:
          cmd_pause();
          break;
        case kwDELAY:
          cmd_delay();
          break;
        case kwARC:
          cmd_arc();
          break;
        case kwDRAW:
          cmd_draw();
          break;
        case kwPAINT:
          cmd_paint();
          break;
        case kwPLAY:
          cmd_play();
          break;
        case kwSORT:
          cmd_sort();
          break;
        case kwSEARCH:
          cmd_search();
          break;
        case kwROOT:
          cmd_root();
          break;
        case kwDIFFEQ:
          cmd_diffeq();
          break;
        case kwCHART:
          cmd_chart();
          break;
        case kwWINDOW:
          cmd_window();
          break;
        case kwVIEW:
          cmd_view();
          break;
        case kwDRAWPOLY:
          cmd_drawpoly();
          break;
        case kwM3IDENT:
          cmd_m3ident();
          break;
        case kwM3ROTATE:
          cmd_m3rotate();
          break;
        case kwM3SCALE:
          cmd_m3scale();
          break;
        case kwM3TRANSLATE:
          cmd_m3translate();
          break;
        case kwM3APPLY:
          cmd_m3apply();
          break;
        case kwSEGINTERSECT:
          cmd_intersect();
          break;
        case kwPOLYEXT:
          cmd_polyext();
          break;
        case kwDERIV:
          cmd_deriv();
          break;
        case kwLOADLN:
          cmd_floadln();
          break;
        case kwSAVELN:
          cmd_fsaveln();
          break;
        case kwKILL:
          cmd_fkill();
          break;
        case kwRENAME:
          cmd_filecp(1);
          break;
        case kwCOPY:
          cmd_filecp(0);
          break;
        case kwCHDIR:
          cmd_chdir();
          break;
        case kwMKDIR:
          cmd_mkdir();
          break;
        case kwRMDIR:
          cmd_rmdir();
          break;
        case kwFLOCK:
          cmd_flock();
          break;
        case kwCHMOD:
          cmd_chmod();
          break;
        case kwPLOT2:
          cmd_plot2();
          break;
        case kwPLOT:
          cmd_plot();
          break;
        case kwSWAP:
          cmd_swap();
          break;
        case kwBUTTON:
          cmd_button();
          break;
        case kwTEXT:
          cmd_text();
          break;
        case kwDOFORM:
          cmd_doform();
          break;
        case kwDIRWALK:
          cmd_dirwalk();
          break;
        case kwBPUTC:
          cmd_bputc();
          break;
        case kwBSAVE:
          cmd_bsave();
          break;
        case kwBLOAD:
          cmd_bload();
          break;
        case kwEXPRSEQ:
          cmd_exprseq();
          break;
#if !defined(OS_LIMITED)
        case kwSTKDUMP:
          dev_print("\nSTKDUMP:\n");
          dump_stack();
          prog_error = -1;      // end of program
          break;
#endif
        case kwHTML:
          cmd_html();
          break;
        case kwIMAGE:
          cmd_image();
          break;
        case kwDEFINEKEY:
          cmd_definekey();
          break;
        default:
          err_pcode_err(pcode);
        }
        break;

        /*
         *      buildin procedures -- END
         * ----------------------------------------- */

      case kwTYPE_CALL_UDP:    // user defined procedure
        cmd_udp(kwPROC);
        if (isf) {
          proc_level++;
        }
        IF_ERR_BREAK;
        continue;
      case kwTYPE_CALL_UDF:    // user defined function
        if (isf) {
          cmd_udp(kwFUNC);
          proc_level++;
        } else {
          err_syntax();
        }
        IF_ERR_BREAK;
        continue;
      case kwTYPE_RET:
        cmd_udpret();
        if (isf) {
          proc_level--;
          if (proc_level == 0)
            return;
        }
        IF_ERR_BREAK;
        continue;
      case kwTYPE_CRVAR:
        cmd_crvar();
        break;
      case kwTYPE_PARAM:
        cmd_param();
        break;
      case kwEXIT:
        pops = cmd_exit();
        if (isf && pops) {
          proc_level--;
          if (proc_level == 0)
            return;
        }
        IF_ERR_BREAK;
        continue;

        // //////////////
      case kwLINE:
        cmd_line();
        break;

        // third class
      case kwCOLOR:
        cmd_color();
        break;
        // case kwINTEGRAL:
        // cmd_integral();
        // break;

        // --- at end ---
      case kwOPEN:
        cmd_fopen();
        break;
      case kwCLOSE:
        cmd_fclose();
        break;
      case kwFILEWRITE:
        cmd_fwrite();
        break;
      case kwFILEREAD:
        cmd_fread();
        break;
      case kwLOGPRINT:
        cmd_print(PV_LOG);
        break;
      case kwFILEPRINT:
        cmd_print(PV_FILE);
        break;
      case kwSPRINT:
        cmd_print(PV_STRING);
        break;
      case kwLINEINPUT:
        cmd_flineinput();
        break;
      case kwSINPUT:
        cmd_input(PV_STRING);
        break;
      case kwFILEINPUT:
        cmd_input(PV_FILE);
        break;
      case kwSEEK:
        cmd_fseek();
        break;
      case kwTRON:
        trace_flag = 1;
        continue;
      case kwTROFF:
        trace_flag = 0;
        continue;

      case kwSTOP:
      case kwEND:
        if ((prog_length - 1) > prog_ip) {
          if (code_peek() != kwTYPE_EOC && code_peek() != kwTYPE_LINE) {
            var_t ec;

            v_init(&ec);
            eval(&ec);
            opt_retval = v_igetval(&ec);
            v_free(&ec);
          } else
            opt_retval = 0;
        }
        prog_error = -1;        // end of program
        break;
      case kwCHAIN:
        cmd_chain();
        break;
      case kwRUN:
        cmd_run(1);
        break;
      case kwEXEC:
        cmd_run(0);
        break;

      default:
#if !defined(OS_LIMITED)
        rt_raise("SEG:CODE[%d]=%02x", prog_ip, prog_source[prog_ip]);
        // ////////
        dev_printf("OUT OF ADDRESS SPACE\n");
        for (i = 0; keyword_table[i].name[0] != '\0'; i++) {
          if (prog_source[prog_ip] == keyword_table[i].code) {
            dev_printf("OR ILLEGAL CALL TO '%s'\n", keyword_table[i].name);
            break;
          }
        }
#else
        err_gpf(prog_ip, prog_source[prog_ip]);
#endif
        // ////////
      }
    }
    // too many parameters
    code = prog_source[prog_ip];
    if (code != kwTYPE_EOC && code != kwTYPE_LINE && !prog_error) {
#if !defined(OS_LIMITED)
      if (code == kwTYPE_SEP) {
        rt_raise("COMMAND SEPARATOR '%c' FOUND!", prog_source[prog_ip + 1]);
      } else {
        rt_raise("PARAM COUNT ERROR @%d=%X %d", prog_ip, prog_source[prog_ip], code);
      }
#else
      err_invkw(prog_ip, code);
#endif
    } else {
      prog_ip++;
      if (code == kwTYPE_LINE) {
        prog_line = code_getaddr();
        if (trace_flag) {
          dev_printf("<%d>", prog_line);
        }
      }
    }

    // quit on error
    IF_ERR_BREAK;
  }
}

/**
 * debug info
 * stack dump
 */
#if !defined(OS_LIMITED)        // save some bytes
void dump_stack() {
  stknode_t node;
  int i;

  do {
    code_pop(&node);
    if (node.type != 0xFF) {
      for (i = 0; keyword_table[i].name[0] != '\0'; i++) {
        if (node.type == keyword_table[i].code) {
          dev_printf("\t%s", keyword_table[i].name);
          switch (node.type) {
          case kwGOSUB:
            dev_printf(" RIP: %d", node.x.vgosub.ret_ip);
            if (prog_source[node.x.vgosub.ret_ip] == kwTYPE_LINE) {
              dev_printf(" = LI %d", (*((word *)(prog_source + node.x.vgosub.ret_ip + 1))) - 1);
            }
            break;
          }
          dev_print("\n");
          break;
        }
      }
    } else
      break;
  } while (1);
}
#endif

/*
 * RUN byte-code
 * 
 * ByteCode Structure (executables, not units):
 *
 * [header (bc_head_t)]
 * [label-table (ADDRSZ) * label_count]
 * [import-lib-table (bc_lib_rec_t) * lib_count]
 * [import-symbol-table (bc_symbol_rec_t) * symbol_count]
 * [the bytecode itself]
 *
 * brun_init(source)
 * ...brun_create_task(source)
 *
 * brun()
 *
 * exec_close()
 * ...exec_close_task()
 */
int brun_create_task(const char *filename, mem_t preloaded_bc, int libf) {
  unit_file_t uft;
  bc_head_t hdr;
  byte *cp;
  int tid, i, h;
  byte *source;
  char fname[OS_PATHNAME_SIZE + 1];mem_t
  bc_h = 0;

  if (preloaded_bc) {
    // I have already BC
    bc_h = preloaded_bc;
    strcpy(fname, filename);
  } else {
    // prepare filename
    if (!libf) {
      char *p;
      strcpy(fname, filename);
      p = strrchr(fname, '.');
      if (p) {
        *p = '\0';
      }
      strcat(fname, ".sbx");
    } else {
      find_unit(filename, fname);
    }
    if (access(fname, R_OK)) {
      panic("File '%s' not found", filename);
    }
    // look if it is already loaded
    if (search_task(fname) != -1) {
      return search_task(fname);
    }
    // open & load
    h = open(fname, O_RDWR | O_BINARY, 0660);
    if (h == -1) {
      panic("File '%s' not found", fname);
    }
    // load it
    if (libf) {
      read(h, &uft, sizeof(unit_file_t));
      lseek(h, sizeof(unit_sym_t) * uft.sym_count, SEEK_CUR);
    }
    read(h, &hdr, sizeof(bc_head_t));

    bc_h = mem_alloc(hdr.size + 4);
    source = (byte *)mem_lock(bc_h);

    lseek(h, 0, SEEK_SET);
    read(h, source, hdr.size);
    close(h);

    mem_unlock(bc_h);
  }

  // create task

  tid = create_task(fname);     // create a task
  activate_task(tid);           // make it active
  bytecode_h = bc_h;
  source = mem_lock(bc_h);

  cp = source;
  if (memcmp(source, "SBUn", 4) == 0) { // load a unit
    memcpy(&uft, cp, sizeof(unit_file_t));
    cp += sizeof(unit_file_t);
    prog_expcount = uft.sym_count;

    // copy export-symbols from BC
    if (prog_expcount) {
      prog_exptable = (unit_sym_t *)tmp_alloc(prog_expcount * sizeof(unit_sym_t));
      for (i = 0; i < prog_expcount; i++) {
        memcpy(&prog_exptable[i], cp, sizeof(unit_sym_t));
        cp += sizeof(unit_sym_t);
      }
    }
  } else if (memcmp(source, "SBEx", 4) == 0) {  // load an executable
    ;
  } else {                        // signature error
    panic("Wrong bytecode signature");
  }

  /*
   *      ---------------------
   *      build executor's task
   *      ---------------------
   */

  memcpy(&hdr, cp, sizeof(bc_head_t));
  cp += sizeof(bc_head_t);

  prog_varcount = hdr.var_count;
  prog_labcount = hdr.lab_count;
  prog_libcount = hdr.lib_count;
  prog_symcount = hdr.sym_count;

  // create variable-table
  if (prog_varcount == 0) {
    prog_varcount++;
  }
  tvar = tmp_alloc(sizeof(var_t *) * prog_varcount);
  for (i = 0; i < prog_varcount; i++) {
    tvar[i] = v_new();
  }
  // create label-table
  if (prog_labcount) {
    tlab = tmp_alloc(sizeof(lab_t) * prog_labcount);
    for (i = 0; i < prog_labcount; i++) {
      // copy labels from BC
      memcpy(&tlab[i].ip, cp, ADDRSZ);
      cp += ADDRSZ;
    }
  }
  // build import-lib table
  if (prog_libcount) {
    prog_libtable = (bc_lib_rec_t *)tmp_alloc(prog_libcount * sizeof(bc_lib_rec_t));
    for (i = 0; i < prog_libcount; i++) {
      memcpy(&prog_libtable[i], cp, sizeof(bc_lib_rec_t));
      cp += sizeof(bc_lib_rec_t);
    }
  }
  // build import-symbol table
  if (prog_symcount) {
    prog_symtable = (bc_symbol_rec_t *)tmp_alloc(prog_symcount * sizeof(bc_symbol_rec_t));
    for (i = 0; i < prog_symcount; i++) {
      memcpy(&prog_symtable[i], cp, sizeof(bc_symbol_rec_t));
      cp += sizeof(bc_symbol_rec_t);
    }
  }
  // create system stack
  prog_stack_alloc = SB_EXEC_STACK_SIZE;
  prog_stack = tmp_alloc(sizeof(stknode_t) * prog_stack_alloc);
  prog_stack_count = 0;

  // create eval's stack
  eval_size = 64;
  eval_stk = tmp_alloc(sizeof(var_t) * eval_size);
  eval_sp = 0;

  // initialize the rest tasks globals
  prog_error = 0;
  prog_line = 0;

  prog_dp = data_org = hdr.data_ip;
  prog_length = hdr.bc_count;
  prog_source = cp;
  prog_ip = 0;

  // 
  exec_setup_predefined_variables();

  // init the keyboard map
  keymap_init();

  /*
   *      --------------
   *      load libraries
   *      --------------
   *
   *      each library is loaded on new task
   */
  if (prog_libcount) {
    int lib_tid, j, k;

    // reset symbol mapping
    for (i = 0; i < prog_symcount; i++) {
      prog_symtable[i].task_id = prog_symtable[i].exp_idx = -1;
    }
    // for each library
    for (i = 0; i < prog_libcount; i++) {
      if (prog_libtable[i].type == 1) {
        // === SB Unit ===

        // create task
        lib_tid = brun_create_task(prog_libtable[i].lib, 0, 1);
        activate_task(tid);

        // update lib-table's task-id field (in this code; not in
        // lib's code)
        prog_libtable[i].tid = lib_tid;

        // update lib-symbols's task-id field (in this code; not
        // in lib's code)
        for (j = 0; j < prog_symcount; j++) {
          char *pname;

          pname = strrchr(prog_symtable[j].symbol, '.') + 1;
          // the name without the 'class'
          if ((prog_symtable[j].lib_id & (~UID_UNIT_BIT)) == prog_libtable[i].id) {

            // find symbol by name (for sure) and update it
            // this is required because lib may be newer than
            // parent
            for (k = 0; k < taskinfo(lib_tid)->sbe.exec.expcount; k++) {
              if (strcmp(pname, taskinfo(lib_tid)->sbe.exec.exptable[k].symbol) == 0) {
                prog_symtable[j].exp_idx = k;
                // adjust sid (sid is <-> exp_idx in lib)
                prog_symtable[j].task_id = lib_tid;
                // connect the library
                break;
              }
            }                   // k
          }
        }                       // j
      }                         // if type = 1
      else {
        // === C Module ===
        // update lib-table's task-id field (in this code; not in
        // lib's code)
        prog_libtable[i].tid = -1;  // not a task

        // update lib-symbols's task-id field (in this code; not
        // in lib's code)
        for (j = 0; j < prog_symcount; j++) {
          prog_symtable[j].exp_idx = slib_get_kid(prog_symtable[j].symbol);
          // adjust sid (sid is <-> exp_idx in lib)
          prog_symtable[j].task_id = -1;  // connect the library
        }                       // j
      }

      // return
      activate_task(tid);
    }                           // i

    // check symbol mapping
    // if ( !opt_decomp ) {
    // for ( i = 0; i < prog_symcount; i ++ ) {
    // if ( prog_symtable[i].task_id == -1 && prog_libtable[i].type == 1 )
    // panic("Symbol (unit) '%s' missing\n", prog_symtable[i].symbol);
    // if ( prog_symtable[i].task_id == -1 && prog_libtable[i].type == 0 ) { 
    // if ( prog_symtable[j].exp_idx == -1 )
    // panic("Symbol (module) '%s' missing\n", prog_symtable[i].symbol);
    // }
    // }
    // }
  }
  // 
  return tid;
}

/**
 * clean up the current task's (executor's) data
 */
int exec_close_task() {
  word i;
  stknode_t node;

  if (bytecode_h) {
    // clean up - format list
    free_format();

    // clean up - eval stack
    tmp_free(eval_stk);
    eval_size = 0;
    eval_sp = 0;

    // clean up - prog stack
    while (prog_stack_count > 0) {
      code_pop_and_free(&node);
    }tmp_free(prog_stack);
    // clean up - variables
    for (i = 0; i < (int) prog_varcount; i++) {
      int j, shared;
      // do not free imported variables
      shared = -1;
      for (j = 0; j < prog_symcount; j++) {
        if (prog_symtable[j].type == stt_variable) {
          if (prog_symtable[j].var_id == i) {
            shared = i;
            break;
          }
        }
      }

      // free this variable
      if (shared == -1) {
        v_free(tvar[i]);
        tmp_free(tvar[i]);
      }
    }

    tmp_free(tvar);
    ctask->has_sysvars = 0;

    // clean up - rest tables
    if (prog_expcount) {
      tmp_free(prog_exptable);
    }
    if (prog_libcount) {
      tmp_free(prog_libtable);
    }
    if (prog_symcount) {
      tmp_free(prog_symtable);
    }
    if (prog_labcount) {
      tmp_free(tlab);
    }
    // clean up - the rest
    mem_unlock(bytecode_h);mem_free(bytecode_h);
    bytecode_h = 0;

    // cleanup the keyboard map
    keymap_free();
  }

  if (prog_error != -1 && prog_error != 0) {
    return 1;
  }
  return 0;
}

/**
 * close the executor
 */
int exec_close(int tid) {
  int prev_tid, i;

  prev_tid = activate_task(tid);

  for (i = 0; i < count_tasks(); i++) {
    activate_task(i);

    if (ctask->status == tsk_ready && ctask->parent == tid) {
      exec_close(ctask->tid);
    }
  }

  activate_task(tid);
  exec_close_task();
  close_task(tid);
  activate_task(prev_tid);
  return 1;
}

/**
 * update common variables
 *
 * if dir = 0, source is the unit, dir = 1 source is this program
 *
 * actually dir has no meaning, but it is more logical (dir=0 always is also correct)
 */
void exec_sync_variables(int dir) {
  int i, tid;
  unit_sym_t *us;               // unit's symbol data
  bc_symbol_rec_t *ps;          // program's symbol data
  var_t *vp;                    // variable for swap

  tid = ctask->tid;

  for (i = 0; i < prog_symcount; i++) {
    if (prog_symtable[i].type == stt_variable) {
      ps = &prog_symtable[i];
      us = &(taskinfo(ps->task_id)->sbe.exec.exptable[ps->exp_idx]);

      if (dir == 0) {
        activate_task(ps->task_id);
        vp = tvar[us->vid];

        activate_task(tid);
        tvar[ps->var_id] = vp;  // pointer assignment (shared
        // var_t pointer)
      } else {
        activate_task(tid);
        vp = tvar[ps->var_id];

        activate_task(ps->task_id);
        tvar[us->vid] = vp;

        activate_task(tid);
      }
    }
  }
}

/**
 * system specific things - before compilation
 */
void sys_before_comp() {
  // opt_nosave is a user controlled variable (chris 4/7/04)
  // #if defined(OS_LIMITED)
  // opt_nosave = 0; // create .sbx;
  // #else
  // opt_nosave = 1; // don't create .sbx; however it will create any
  // unit file (.sbu)
  // #endif

  // setup prefered screen mode variables
  if (dev_getenv("SBGRAF")) {
    if (dev_getenv("SBGRAF"))
      comp_preproc_grmode(dev_getenv("SBGRAF"));
    opt_graphics = 2;
  }
}

/**
 * system specific things - after compilation
 */
void sys_after_comp() {
}

/**
 * system specific things - before execution
 */
void sys_before_run() {
}

/**
 * system specific things - after execution
 */
void sys_after_run() {
}

/**
 * execute the code on this task
 */
int sbasic_exec_task(int tid) {
  int prev_tid, success;

  prev_tid = activate_task(tid);

  bc_loop(0);                   // natural the value -1 is end of program
  success = (prog_error == 0 || prog_error == -1);
  if (success) {
    prog_error = 0;
  }
  activate_task(prev_tid);
  return success;
}

/**
 * run libraries and main-code
 */
int sbasic_recursive_exec(int tid) {
  int i, success = 1;
  int prev_tid;

  prev_tid = activate_task(tid);

  for (i = 0; i < count_tasks(); i++) {
    if (taskinfo(i)->parent == tid) {
      // do the same for the childs
      activate_task(i);
      success = sbasic_recursive_exec(i);
      if (!success) {
        break;
      }
    }
  }

  if (success) {
    activate_task(tid);
    exec_sync_variables(0);

    // run
    if (!(opt_quiet || opt_interactive)) {
      dev_printf("Initializing #%d (%s) ...\n", ctask->tid, ctask->file);
    }
    success = sbasic_exec_task(ctask->tid);
  }

  activate_task(prev_tid);
  return success;
}

#if !defined(OS_LIMITED)

/**
 * dump-taskinfo
 */
void sbasic_dump_taskinfo(FILE * output) {
  int i;
  int prev_tid = 0;

  fprintf(output, "\n* task list:\n");
  for (i = 0; i < count_tasks(); i++) {
    prev_tid = activate_task(i);
    fprintf(output, "  id %d, child of %d, file %s, status %d\n", ctask->tid, ctask->parent, ctask->file,
        ctask->status);
  }
  activate_task(prev_tid);
}

/**
 * dump-bytecode
 */
void sbasic_dump_bytecode(int tid, FILE * output) {
  int i;
  int prev_tid;

  prev_tid = activate_task(tid);

  for (i = 0; i < count_tasks(); i++) {
    activate_task(i);
    if (ctask->parent == tid) {
      // do the same for the childs
      sbasic_dump_bytecode(ctask->tid, output);
    }
  }

  activate_task(tid);
  fprintf(output, "\n* task: %d/%d (%s)\n", ctask->tid, count_tasks(), prog_file);
  // run the code
  dump_bytecode(output);
  activate_task(prev_tid);
}
#endif

/**
 * TODO add comment
 */
int sbasic_compile(const char *file) {
  int comp_rq = 0;              // compilation required = 0
  int success = 1;

  if (strstr(file, ".sbx") == file + strlen(file) - 4) {
    return success;             // file is an executable
  }

  if (opt_nosave) {
    comp_rq = 1;
  } else {
    char exename[OS_PATHNAME_SIZE + 1];
    char *p;

    // executable name
    strcpy(exename, file);
    p = strrchr(exename, '.');
    if (p) {
      *p = '\0';
    }
    strcat(exename, ".sbx");

    if ((access(exename, R_OK) == 0)) {
      time_t bin_date = 0, src_date = 0;

      // compare dates
      if ((bin_date = sys_filetime(exename)) == 0L) {
        comp_rq = 1;
      }
      else if ((src_date = sys_filetime(file)) == 0L) {
        comp_rq = 1;
      }
      if (bin_date >= src_date) {
        // TODO: check binary version
        ;
      }
      else {
        comp_rq = 1;
      }
    }
    else {
      comp_rq = 1;
    }
  }

  // compile it
  if (comp_rq) {
    sys_before_comp();  // system specific preparations for compilation
    success = comp_compile(file);
    sys_after_comp();   // system specific things; after compilation
  }
  return success;
}

/**
 * initialize executor and run a binary
 */
void sbasic_exec_prepare(const char *filename) {
  // load source
  if (opt_nosave)
    exec_tid = brun_create_task(filename, bytecode_h, 0);
  else
    exec_tid = brun_create_task(filename, 0, 0);

  // reset system
  cmd_play_reset();
  ui_reset();
}

/**
 * this is the main 'execute' routine; its work depended on opt_xxx flags
 * use it instead of sbasic_main if managers are already initialized
 *
 * @param file the source file
 * @return true on success
 */
int sbasic_exec(const char *file) {
  int success = 0;
  int exec_rq = 1;

  // setup some default options
  opt_pref_width = opt_pref_height = opt_pref_bpp = 0;
  if (opt_decomp) {
    opt_nosave = 1;
  }
  // setup global values
  gsb_last_line = gsb_last_error = 0;
  strcpy(gsb_last_file, file);
  strcpy(gsb_last_errmsg, "");

  // compile it - if opt_nosave, bytecode_h is a 
  // memory handle of BC; otherwise you must run the file
  success = sbasic_compile(file); 

  if (opt_syntaxcheck)          // this is a command-line flag to
    // syntax-check only
    exec_rq = 0;
#if !defined(OS_LIMITED)
  else if (opt_decomp && success) {
    sbasic_exec_prepare(file);  // load everything
    sbasic_dump_taskinfo(stdout);
    sbasic_dump_bytecode(exec_tid, stdout);
    exec_close(exec_tid);       // clean up executor's garbages
    exec_rq = 0;
  }
#endif
  else if (!success) {          // there was some errors; do not continue
    exec_rq = 0;
    gsb_last_error = 1;
  }

  if (exec_rq) {                // we will run it
    // load everything
    sbasic_exec_prepare(file);

    sys_before_run();           // system specific things; before run
    dev_init(opt_graphics, 0);  // initialize output device for graphics
    evt_check_every = (50 * CLOCKS_PER_SEC) / 1000; // setup event checker time = 50ms
    srand(clock());             // randomize

    // run 
    sbasic_recursive_exec(exec_tid);

    // normal exit
    if (!opt_quiet) {
      inf_done();
    }

    exec_close(exec_tid);       // clean up executor's garbages
    dev_restore();              // restore device

    sys_after_run();            // system specific things; after run
  }
#if !defined(OS_LIMITED)
  // update IDE when it used as external
  if (opt_ide == IDE_EXTERNAL) {
    char fn[OS_PATHNAME_SIZE + 1];FILE *fp;

#  if defined(_UnixOS)
    sprintf(fn, "%s/.sbide/.lastmsg", getenv("HOME"));
#  else
    strcpy(fn, "sbide.out");
#  endif
    fp = fopen(fn, "w");
    if (fp) {
      fprintf(fp, "Error: %d\nLine: %d\nFile: %s\nMessage: %s\n",
          gsb_last_error, gsb_last_line, gsb_last_file, gsb_last_errmsg);
      fclose(fp);
    }
  }
#endif

  // cdw-s 22/11/2004 return as failure for compilation errors
  return !success ? 0 : !gsb_last_error;
}

/**
 * this is the main routine; its work depended on opt_xxx flags
 *
 * @param file the source file
 * @return true on success
 */
int sbasic_main(const char *file) {
  int success;

  // initialize task manager
  main_tid = init_tasks();

  // initialize SB's units manager
  unit_mgr_init();

  // modules load
  if (opt_loadmod) {
    sblmgr_init(1, opt_modlist);  // initialize (load) Linux's C
    // units
  } else {
    sblmgr_init(0, NULL);
  }
  // go...
  success = sbasic_exec(file);

  unit_mgr_close();             // shutdown SB's unit manager
  sblmgr_close();               // shutdown C-coded modules
  destroy_tasks();              // closes all remaining tasks

  return success;
}

