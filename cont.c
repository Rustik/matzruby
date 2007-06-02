/**********************************************************************

  cont.c - 

  $Author$
  $Date$
  created at: Thu May 23 09:03:43 2007

  Copyright (C) 2007 Koichi Sasada

**********************************************************************/

#include "ruby.h"
#include "yarvcore.h"
#include "gc.h"
#include "eval_intern.h"

typedef struct rb_context_struct {
    VALUE self;
    VALUE value;
    VALUE prev; /* for fiber */
    VALUE *vm_stack;
    VALUE *machine_stack;
    VALUE *machine_stack_src;
    rb_thread_t saved_thread;
    rb_jmpbuf_t jmpbuf;
    int machine_stack_size;
    int alive;
} rb_context_t;

VALUE rb_cCont;
VALUE rb_cFiber;
VALUE rb_eFiberError;

#define GetContPtr(obj, ptr)  \
  Data_Get_Struct(obj, rb_context_t, ptr)

NOINLINE(static VALUE cont_capture(volatile int *stat));

void rb_thread_mark(rb_thread_t *th);

static void
cont_mark(void *ptr)
{
    MARK_REPORT_ENTER("cont");
    if (ptr) {
	rb_context_t *cont = ptr;
	rb_gc_mark(cont->value);
	rb_gc_mark(cont->prev);

	rb_thread_mark(&cont->saved_thread);

	if (cont->vm_stack) {
	    rb_gc_mark_locations(cont->vm_stack,
				 cont->vm_stack + cont->saved_thread.stack_size);
	}

	if (cont->machine_stack) {
	    rb_gc_mark_locations(cont->machine_stack,
				 cont->machine_stack + cont->machine_stack_size);
	}
    }
    MARK_REPORT_LEAVE("cont");
}

static void
cont_free(void *ptr)
{
    FREE_REPORT_ENTER("cont");
    if (ptr) {
	rb_context_t *cont = ptr;
	FREE_UNLESS_NULL(cont->saved_thread.stack);
	FREE_UNLESS_NULL(cont->machine_stack);
	FREE_UNLESS_NULL(cont->vm_stack);
	ruby_xfree(ptr);
    }
    FREE_REPORT_LEAVE("cont");
}

static void
cont_save_machine_stack(rb_thread_t *th, rb_context_t *cont)
{
    int size;

    rb_gc_set_stack_end(&th->machine_stack_end);
    if (th->machine_stack_start > th->machine_stack_end) {
	size = cont->machine_stack_size = th->machine_stack_start - th->machine_stack_end;
	cont->machine_stack_src = th->machine_stack_end;
    }
    else {
	size = cont->machine_stack_size = th->machine_stack_end - th->machine_stack_start;
	cont->machine_stack_src = th->machine_stack_start;
    }

    if (cont->machine_stack) {
	REALLOC_N(cont->machine_stack, VALUE, size);
    }
    else {
	cont->machine_stack = ALLOC_N(VALUE, size);
    }

    MEMCPY(cont->machine_stack, cont->machine_stack_src, VALUE, size);
}

static rb_context_t *
cont_new(VALUE klass)
{
    rb_context_t *cont;
    volatile VALUE contval;
    rb_thread_t *th = GET_THREAD(), *sth;

    contval = Data_Make_Struct(klass, rb_context_t,
			       cont_mark, cont_free, cont);
    cont->self = contval;
    cont->alive = Qtrue;

    /* save context */
    cont->saved_thread = *th;
    sth = &cont->saved_thread;

    return cont;
}

void th_stack_to_heap(rb_thread_t *th);

static VALUE
cont_capture(volatile int *stat)
{
    rb_context_t *cont;
    rb_thread_t *th;

    th_stack_to_heap(GET_THREAD());
    cont = cont_new(rb_cCont);
    th = &cont->saved_thread;

    cont->vm_stack = ALLOC_N(VALUE, th->stack_size);
    MEMCPY(cont->vm_stack, th->stack, VALUE, th->stack_size);
    th->stack = 0;

    cont_save_machine_stack(th, cont);

    if (ruby_setjmp(cont->jmpbuf)) {
	VALUE value;

	value = cont->value;
	cont->value = Qnil;
	*stat = 1;
	return value;
    }
    else {
	*stat = 0;
	return cont->self;
    }
}

NORETURN(static void cont_restore_1(rb_context_t *));

static void
cont_restore_1(rb_context_t *cont)
{
    rb_thread_t *th = GET_THREAD(), *sth = &cont->saved_thread;

    /* restore thread context */
    if (sth->stack) {
	/* fiber */
	th->stack = sth->stack;
	th->stack_size = sth->stack_size;
	th->fiber = cont->self;
    }
    else {
	/* continuation */
	MEMCPY(th->stack, cont->vm_stack, VALUE, sth->stack_size);
	th->fiber = sth->fiber;
    }

    th->cfp = sth->cfp;
    th->safe_level = sth->safe_level;
    th->raised_flag = sth->raised_flag;
    th->state = sth->state;
    th->status = sth->status;
    th->tag = sth->tag;
    th->trap_tag = sth->trap_tag;
    th->errinfo = sth->errinfo;
    th->first_proc = sth->first_proc;

    /* restore machine stack */
    if (cont->machine_stack_src) {
	MEMCPY(cont->machine_stack_src, cont->machine_stack,
	       VALUE, cont->machine_stack_size);
    }

    ruby_longjmp(cont->jmpbuf, 1);
}

NORETURN(NOINLINE(static void cont_restore_0(rb_context_t *, VALUE *)));

static void
cont_restore_0(rb_context_t *cont, VALUE *addr_in_prev_frame)
{
    if (cont->machine_stack_src) {
#define STACK_PAD_SIZE 1024
	VALUE space[STACK_PAD_SIZE];

#if STACK_GROW_DIRECTION < 0 /* downward */
	if (addr_in_prev_frame > cont->machine_stack_src) {
	    cont_restore_0(cont, &space[0]);
	}
#elif STACK_GROW_DIRECTION > 0 /* upward */
	if (addr_in_prev_frame < cont->machine_stack_src + cont->machine_stack_size) {
	    cont_restore_0(cont, &space[STACK_PAD_SIZE-1]);
	}
#else
	if (addr_in_prev_frame > &space[0]) {
	    /* Stack grows downward */
	    if (addr_in_prev_frame > cont->saved_thread.machine_stack_src) {
		cont_restore_0(cont, &space[0]);
	    }
	}
	else {
	    /* Stack grows upward */
	    if (addr_in_prev_frame < cont->machine_stack_src + cont->machine_stack_size) {
		cont_restore_0(cont, &space[STACK_PAD_SIZE-1]);
	    }
	}
#endif
    }
    cont_restore_1(cont);
}

/*
 *  Document-class: Continuation
 *
 *  Continuation objects are generated by
 *  <code>Kernel#callcc</code>. They hold a return address and execution
 *  context, allowing a nonlocal return to the end of the
 *  <code>callcc</code> block from anywhere within a program.
 *  Continuations are somewhat analogous to a structured version of C's
 *  <code>setjmp/longjmp</code> (although they contain more state, so
 *  you might consider them closer to threads).
 *     
 *  For instance:
 *     
 *     arr = [ "Freddie", "Herbie", "Ron", "Max", "Ringo" ]
 *     callcc{|$cc|}
 *     puts(message = arr.shift)
 *     $cc.call unless message =~ /Max/
 *     
 *  <em>produces:</em>
 *     
 *     Freddie
 *     Herbie
 *     Ron
 *     Max
 *     
 *  This (somewhat contrived) example allows the inner loop to abandon
 *  processing early:
 *     
 *     callcc {|cont|
 *       for i in 0..4
 *         print "\n#{i}: "
 *         for j in i*5...(i+1)*5
 *           cont.call() if j == 17
 *           printf "%3d", j
 *         end
 *       end
 *     }
 *     print "\n"
 *     
 *  <em>produces:</em>
 *     
 *     0:   0  1  2  3  4
 *     1:   5  6  7  8  9
 *     2:  10 11 12 13 14
 *     3:  15 16
 */

/*
 *  call-seq:
 *     callcc {|cont| block }   =>  obj
 *  
 *  Generates a <code>Continuation</code> object, which it passes to the
 *  associated block. Performing a <em>cont</em><code>.call</code> will
 *  cause the <code>callcc</code> to return (as will falling through the
 *  end of the block). The value returned by the <code>callcc</code> is
 *  the value of the block, or the value passed to
 *  <em>cont</em><code>.call</code>. See class <code>Continuation</code>
 *  for more details. Also see <code>Kernel::throw</code> for
 *  an alternative mechanism for unwinding a call stack.
 */

static VALUE
rb_callcc(VALUE self)
{
    volatile int called;
    volatile VALUE val = cont_capture(&called);

    if (called) {
	return val;
    }
    else {
	return rb_yield(val);
    }
}

static VALUE
make_passing_arg(int argc, VALUE *argv)
{
    switch(argc) {
      case 0:
	return Qnil;
      case 1:
	return argv[0];
      default:
	return rb_ary_new4(argc, argv);
    }
}

/*
 *  call-seq:
 *     cont.call(args, ...)
 *     cont[args, ...]
 *  
 *  Invokes the continuation. The program continues from the end of the
 *  <code>callcc</code> block. If no arguments are given, the original
 *  <code>callcc</code> returns <code>nil</code>. If one argument is
 *  given, <code>callcc</code> returns it. Otherwise, an array
 *  containing <i>args</i> is returned.
 *     
 *     callcc {|cont|  cont.call }           #=> nil
 *     callcc {|cont|  cont.call 1 }         #=> 1
 *     callcc {|cont|  cont.call 1, 2, 3 }   #=> [1, 2, 3]
 */

static VALUE
rb_cont_call(int argc, VALUE *argv, VALUE contval)
{
    rb_context_t *cont;
    rb_thread_t *th = GET_THREAD();
    GetContPtr(contval, cont);

    if (cont->saved_thread.self != th->self) {
	rb_raise(rb_eRuntimeError, "continuation called across threads");
    }
    if (cont->saved_thread.trap_tag != th->trap_tag) {
	rb_raise(rb_eRuntimeError, "continuation called across trap");
    }

    cont->value = make_passing_arg(argc, argv);

    cont_restore_0(cont, (VALUE *)&cont);
    return Qnil; /* unreachable */
}

/*********/
/* fiber */
/*********/

#define FIBER_STACK_SIZE (4 * 1024)

static VALUE
rb_fiber_s_new(VALUE self)
{
    rb_context_t *cont = cont_new(self);
    rb_thread_t *th = &cont->saved_thread;

    /* initialize */
    cont->prev = Qnil;
    cont->vm_stack = 0;

    th->stack = 0;
    th->stack_size = FIBER_STACK_SIZE;
    th->stack = ALLOC_N(VALUE, th->stack_size);
    th->cfp = (void *)(th->stack + th->stack_size);
    th->cfp--;
    th->cfp->pc = 0;
    th->cfp->sp = th->stack + 1;
    th->cfp->bp = 0;
    th->cfp->lfp = th->stack;
    *th->cfp->lfp = 0;
    th->cfp->dfp = th->stack;
    th->cfp->self = Qnil;
    th->cfp->magic = 0;
    th->cfp->iseq = 0;
    th->cfp->proc = 0;
    th->cfp->block_iseq = 0;

    th->first_proc = rb_block_proc();

    MEMCPY(&cont->jmpbuf, &th->root_jmpbuf, rb_jmpbuf_t, 1);

    return cont->self;
}

static VALUE rb_fiber_yield(int argc, VALUE *args, VALUE fval);

static void
rb_fiber_terminate(rb_context_t *cont)
{
    rb_context_t *prev_cont;
    VALUE value = cont->value;

    GetContPtr(cont->prev, prev_cont);

    cont->alive = Qfalse;


    if (prev_cont->alive == Qfalse) {
	rb_fiber_yield(1, &value, GET_THREAD()->root_fiber);
    }
    else {
	rb_fiber_yield(1, &value, cont->prev);
    }
}

void
rb_fiber_start(void)
{
    rb_thread_t *th = GET_THREAD();
    rb_context_t *cont;
    rb_proc_t *proc;
    VALUE args;
    int state;

    TH_PUSH_TAG(th);
    if ((state = EXEC_TAG()) == 0) {
	GetContPtr(th->fiber, cont);
	GetProcPtr(cont->saved_thread.first_proc, proc);
	args = cont->value;
	cont->value = Qnil;
	th->errinfo = Qnil;
	th->local_lfp = proc->block.lfp;
	th->local_svar = Qnil;

	cont->value = th_invoke_proc(th, proc, proc->block.self, 1, &args);
    }
    TH_POP_TAG();

    if (state) {
	th->thrown_errinfo = th->errinfo;
	th->interrupt_flag = 1;
    }

    rb_fiber_terminate(cont);
    rb_bug("rb_fiber_start: unreachable");
}

static VALUE
rb_fiber_current(rb_thread_t *th)
{
    if (th->fiber == 0) {
	/* save root */
	th->root_fiber = th->fiber = cont_new(rb_cFiber)->self;
    }
    return th->fiber;
}

static VALUE
cont_store(rb_context_t *next_cont)
{
    rb_thread_t *th = GET_THREAD();
    rb_context_t *cont;

    if (th->fiber) {
	GetContPtr(th->fiber, cont);
	cont->saved_thread = *th;
    }
    else {
	/* create current fiber */
	cont = cont_new(rb_cFiber); /* no need to allocate vm stack */
	th->root_fiber = th->fiber = cont->self;
    }

    if (cont->alive) {
	next_cont->prev = cont->self;
    }
    cont_save_machine_stack(th, cont);

    if (ruby_setjmp(cont->jmpbuf)) {
	/* restored */
	GetContPtr(th->fiber, cont);
	return cont->value;
    }
    else {
	return Qundef;
    }
}

static VALUE
rb_fiber_yield(int argc, VALUE *argv, VALUE fval)
{
    VALUE value;
    rb_context_t *cont;
    rb_thread_t *th = GET_THREAD();

    GetContPtr(fval, cont);

    if (cont->saved_thread.self != th->self) {
	rb_raise(rb_eFiberError, "fiber called across threads");
    }
    if (cont->saved_thread.trap_tag != th->trap_tag) {
	rb_raise(rb_eFiberError, "fiber called across trap");
    }
    if (!cont->alive) {
	rb_raise(rb_eFiberError, "dead fiber called");
    }

    cont->value = make_passing_arg(argc, argv);

    if ((value = cont_store(cont)) == Qundef) {
	cont_restore_0(cont, (VALUE *)&cont);
	rb_bug("rb_fiber_yield: unreachable");
    }

    return value;
}

static VALUE
rb_fiber_prev(VALUE fval)
{
    rb_context_t *cont;
    GetContPtr(fval, cont);
    return cont->prev;
}

static VALUE
rb_fiber_alive_p(VALUE fval)
{
    rb_context_t *cont;
    GetContPtr(fval, cont);
    return cont->alive;
}

static VALUE
rb_fiber_s_current(VALUE klass)
{
    return rb_fiber_current(GET_THREAD());
}

static VALUE
rb_fiber_s_prev(VALUE klass)
{
    return rb_fiber_prev(rb_fiber_s_current(Qnil));
}

static VALUE
rb_fiber_s_yield(int argc, VALUE *argv, VALUE fval)
{
    return rb_fiber_yield(argc, argv, rb_fiber_s_prev(Qnil));
}

void
Init_Cont(void)
{
    rb_cCont = rb_define_class("Continuation", rb_cObject);
    rb_undef_alloc_func(rb_cCont);
    rb_undef_method(CLASS_OF(rb_cCont), "new");
    rb_define_method(rb_cCont, "call", rb_cont_call, -1);
    rb_define_method(rb_cCont, "[]", rb_cont_call, -1);
    rb_define_global_function("callcc", rb_callcc, 0);

    rb_cFiber = rb_define_class("Fiber", rb_cObject);
    rb_undef_alloc_func(rb_cFiber);
    rb_define_method(rb_cFiber, "yield", rb_fiber_yield, -1);
    rb_define_method(rb_cFiber, "prev", rb_fiber_prev, 0);
    rb_define_method(rb_cFiber, "alive?", rb_fiber_alive_p, 0);

    rb_define_singleton_method(rb_cFiber, "current", rb_fiber_s_current, 0);
    rb_define_singleton_method(rb_cFiber, "prev", rb_fiber_s_prev, 0);
    rb_define_singleton_method(rb_cFiber, "yield", rb_fiber_s_yield, -1);
    rb_define_singleton_method(rb_cFiber, "new", rb_fiber_s_new, 0);

    rb_eFiberError = rb_define_class("FiberError", rb_eStandardError);
}

