/**********************************************************************

  ruby/mvm.h -

  $Author$
  created at: Sat May 31 15:17:36 2008

  Copyright (C) 2008 Yukihiro Matsumoto

**********************************************************************/

#ifndef RUBY_MVM_H
#define RUBY_MVM_H 1

#define HAVE_MVM 1

#if defined(_WIN32)
/* Win32 thread */
typedef CRITICAL_SECTION rb_thread_lock_t;
#define RB_THREAD_LOCK_INITIALIZER {}
int ruby_native_thread_lock(rb_thread_lock_t*);
int ruby_native_thread_unlock(rb_thread_lock_t*);
int ruby_native_thread_yield(void);

#elif defined(HAVE_PTHREAD_H)
/* pthread */
#include <pthread.h>
typedef pthread_mutex_t rb_thread_lock_t;
#define RB_THREAD_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define ruby_native_thread_lock(lock) pthread_mutex_lock(lock)
#define ruby_native_thread_unlock(lock) pthread_mutex_unlock(lock)
#define ruby_native_thread_yield() sched_yield()

#endif

typedef struct rb_vm_struct rb_vm_t;
typedef struct rb_thread_struct rb_thread_t;

VALUE *ruby_vm_verbose_ptr(rb_vm_t *);
VALUE *ruby_vm_debug_ptr(rb_vm_t *);

VALUE ruby_vm_get_argv(rb_vm_t*);
void ruby_vm_set_argv(rb_vm_t *, int, char **);
const char *ruby_vm_get_inplace_mode(rb_vm_t *);
void ruby_vm_set_inplace_mode(rb_vm_t *, const char *);
void ruby_vm_prog_init(rb_vm_t *);
VALUE ruby_vm_process_options(rb_vm_t *, int, char **);
VALUE ruby_vm_parse_options(rb_vm_t *, int, char **);

void rb_vm_clear_trace_func(rb_vm_t *);
void rb_vm_thread_terminate_all(rb_vm_t *);

rb_vm_t *ruby_vm_new(void);
int ruby_vm_run(rb_vm_t *, VALUE);

int rb_vm_key_count(void);
int rb_vm_key_create(void);
VALUE *ruby_vm_specific_ptr(rb_vm_t *, int);
VALUE *rb_vm_specific_ptr(int);

char *ruby_thread_getcwd(rb_thread_t *);

#endif /* RUBY_MVM_H */
