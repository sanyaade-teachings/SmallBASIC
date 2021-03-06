// This file is part of SmallBASIC
//
// SmallBASIC SB-Task manager
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//
// Copyright(C) 2000 Nicholas Christopoulos

#if !defined(__sb_tasks_h)
#define __sb_tasks_h

#include "common/smbas.h"
#include "common/bc.h"
#include "common/keymap.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 */
typedef enum {
  tsk_free, tsk_ready, tsk_nil
} task_status_t;

/**
 *   @ingroup sys
 *
 *   @typedef task_t
 *   Task data
 */
typedef struct {
  int status;
  int tid;
  int parent;

  /*
   * --- common ---
   */
  int line; /**< The current source code line               */
  int error; /**< The last RTL error code (its work as flag) */
  char errmsg[SB_ERRMSG_SIZE + 1];char file[OS_PATHNAME_SIZE + 1];
  /**< The program file name (task name)                           */
  mem_t bytecode_h; /**< BC's memory handle                         */
  int bc_type; /**< BC type (1=executable, 2=unit)             */

  int has_sysvars; /**< true if the task has system-variables      */

  /*
   * --- compiler ---
   */
  union {
    struct {
      ext_proc_node_t *extproctable; /**< external procedure table       */
      int extprocsize; /**< ext-proc table allocated size  */
      int extproccount; /**< ext-proc table count           */

      ext_func_node_t *extfunctable; /**< external function table        */
      int extfuncsize; /**< ext-func table allocated size  */
      int extfunccount; /**< ext-func table count           */

      char file_name[OS_PATHNAME_SIZE + 1];
      char unit_name[OS_PATHNAME_SIZE + 1];
      int unit_flag;

      dbt_t libtable;
      int libcount;
      dbt_t imptable;
      int impcount;
      dbt_t exptable;
      int expcount;

      int block_level;        // block level (FOR-NEXT,IF-FI,etc)
      int block_id;// unique ID for blocks (FOR-NEXT,IF-FI,etc)

      addr_t first_data_ip;

      // buffers... needed for devices with limited memory
      char *bc_name;
      char *bc_parm;
      char *bc_temp;// primary all-purpose buffer
      char *bc_tmp2;// secondary all-purpose buffer
      char *bc_proc;
      char *bc_sec;// used by sberr.c

      int proc_level;
      // flag - uses global variable-table for the next commands
      byte use_global_vartable;

      bc_t bc_prog;
      bc_t bc_data;

      char do_close_cmd[64];

      // variable table
      comp_var_t *vartable;
      bid_t varcount;
      bid_t varsize;

      // label table
      dbt_t labtable;
      bid_t labcount;

      // user defined proc/func table
      comp_udp_t *udptable;
      bid_t udpcount;
      bid_t udpsize;

      // user defined structures
      dbt_t udstable;
      bid_t udscount;
      int next_field_id;

      // pass2 stack
      dbt_t stack;
      bid_t stack_count;

    }comp;

    /*
     * --- executor ---
     */
    struct {
      addr_t length; /**< The byte-code length (program length in bytes) */
      addr_t ip; /**< Register IP; the instruction pointer           */
      byte *bytecode; /**< The byte-code itself                           */

      addr_t dp; /**< Register DP; READ/DATA current position        */
      addr_t org; /**< READ/DATA beginning position                   */

      stknode_t *stack; /**< The program stack                            */
      dword stack_alloc; /**< The stack size                               */
      dword sp; /**< Register SP; The stack pointer               */

      var_t *eval_stk; /**< eval's stack                                 */
      word eval_stk_size; /**< eval's stack size                            */
      word eval_sp; /**< Register ESP; eval's stack pointer           */

      /*
       * Register R; no need
       */
      /*
       * Register L; no need
       */

      dword varcount; /**< number of global-variables                     */
      dword labcount; /**< number of labels                               */
      dword libcount; /**< number of linked libraries                     */
      dword symcount; /**< number of linked symbols                       */
      dword expcount; /**< number of exported symbols                     */

      var_t **vartable; /**< The table of variables                         */
      lab_t *labtable; /**< The table of labels                            */
      bc_lib_rec_t *libtable; /**< import-libraries table                         */
      bc_symbol_rec_t *symtable; /**< import-symbols table                        */
      unit_sym_t *exptable; /**< export-symbols table                      */

#if defined(_PalmOS)
      DmOpenRef bytecode_fp;
      word bytecode_recidx;
#elif defined (_FRANKLIN_EBM)
      int bytecode_fp;
#endif

    }exec;

  }sbe;

} task_t;

EXTERN task_t *ctask; /**< current task pointer  */

/**
 *   @ingroup sys
 *
 *   return the number of the tasks
 *
 *   @return the number of the tasks
 */
int count_tasks() SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   return the active task-id
 *
 *   @return the active task-id
 */
int current_tid() SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   return the active task-structure
 *
 *   @return the active task-structure
 */
task_t *current_task() SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   initialize tasks manager
 */
int init_tasks() SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   destroys tasks and closes task manager
 */
void destroy_tasks() SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   create an empty new task
 *
 *   @param name is the task name
 *   @return the task-id
 */
int create_task(const char *name) SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   closes a task and activate the next
 */
void close_task(int tid) SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   set active task
 *
 *   @param tid the task-id
 *   @return the previous task-id
 */
int activate_task(int tid) SEC(BEXEC);

/**
 *   @ingroup sys
 *
 *   return the nth task-structure
 *
 *   @return the nth task-structure
 */
task_t *taskinfo(int n);

/**
 *   @ingroup sys
 *
 *   search for a task
 *
 *   @param task_name the name of the task
 *   @return the task-id; or -1 on error
 */
int search_task(const char *task_name) SEC(BEXEC);

#if defined(__cplusplus)
}
#endif
#endif
