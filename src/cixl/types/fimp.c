#include <inttypes.h>

#include "cixl/bin.h"
#include "cixl/box.h"
#include "cixl/cx.h"
#include "cixl/error.h"
#include "cixl/eval.h"
#include "cixl/scope.h"
#include "cixl/tok.h"
#include "cixl/types/fimp.h"
#include "cixl/types/func.h"

struct cx_fimp *cx_fimp_init(struct cx_fimp *imp,
			     struct cx_func *func,
			     char *id,
			     size_t i) {
  imp->func = func;
  imp->id = id;
  imp->i = i;
  imp->ptr = NULL;
  imp->bin = NULL;
  cx_vec_init(&imp->args, sizeof(struct cx_func_arg));
  cx_vec_init(&imp->toks, sizeof(struct cx_tok));
  return imp;
}

struct cx_fimp *cx_fimp_deinit(struct cx_fimp *imp) {
  free(imp->id);
       
  cx_do_vec(&imp->args, struct cx_func_arg, a) { cx_func_arg_deinit(a); }
  cx_vec_deinit(&imp->args);
  
  cx_do_vec(&imp->toks, struct cx_tok, t) { cx_tok_deinit(t); }
  cx_vec_deinit(&imp->toks);

  if (imp->bin) { cx_bin_unref(imp->bin); }
  return imp;
}

bool cx_fimp_match(struct cx_fimp *imp, struct cx_vec *stack) {
  if (stack->count < imp->args.count) { return false; }
  if (!imp->args.count) { return true; }
  
  struct cx_func_arg *i = (struct cx_func_arg *)cx_vec_end(&imp->args)-1;
  struct cx_box *j = (struct cx_box *)cx_vec_end(stack)-1;
  
  for (; i >= (struct cx_func_arg *)imp->args.items &&
	 j >= (struct cx_box *)stack->items;
       i--, j--) {    
    struct cx_type *t = i->type;

    if (!t && i->narg != -1) {
      t = ((struct cx_box *)cx_vec_get(stack,
				       stack->count-imp->args.count+i->narg))->type;
    }

    if (t) {
      if (!cx_is(j->type, t)) { return false; }
    } else if (!j->undef) {
      struct cx *cx = imp->func->cx;
      struct cx_scope *s = cx_scope(cx, 0);
      cx_copy(cx_push(s), &i->value);
      cx_copy(cx_push(s), j);
      if (!cx_funcall(cx, "=")) { return false; }
      struct cx_box *ok = cx_test(cx_pop(s, false));
      if (!ok->as_bool) { return false; }
    }
  }

  return true;
}

bool cx_fimp_eval(struct cx_fimp *imp, struct cx_scope *scope) {
  struct cx *cx = scope->cx;
  struct cx_fimp *prev = cx->fimp;
  cx->fimp = imp;
  struct cx_bin_func *bin = cx_bin_get_func(cx->bin, imp);
  bool ok = false;

  if (bin) {
    ok = cx_eval(cx, cx->bin, cx_vec_get(&cx->bin->ops, bin->start_op));
  } else {
    if (!imp->bin) {
      imp->bin = cx_bin_new();
      
      if (!cx_compile(cx,
		      cx_vec_start(&imp->toks),
		      cx_vec_end(&imp->toks),
		      imp->bin)) {
	cx_error(cx, cx->row, cx->col, "Failed compiling func imp");
      }
    }
    
    ok = cx_eval(cx, imp->bin, NULL);
  }
  
  cx->fimp = prev;
  return ok;
}

bool cx_fimp_call(struct cx_fimp *imp, struct cx_scope *scope) {
  if (imp->ptr) {
    imp->ptr(scope);
    return true;
  }
  
  cx_begin(scope->cx, false);
  bool ok = cx_fimp_eval(imp, scope);
  cx_end(scope->cx);
  return ok;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return x->as_ptr == y->as_ptr;
}

static bool call_imp(struct cx_box *value, struct cx_scope *scope) {
  struct cx_fimp *imp = value->as_ptr;
  
  if (!cx_fimp_match(imp, &scope->stack)) {
    struct cx *cx = scope->cx;
    cx_error(cx, cx->row, cx->col, "Func not applicable: '%s'", imp->func->id);
    return -1;
  }

  return cx_fimp_call(imp, scope);
}

static void fprint_imp(struct cx_box *value, FILE *out) {
  struct cx_fimp *imp = value->as_ptr;
  fprintf(out, "Fimp(%s %s)", imp->func->id, imp->id);
}

struct cx_type *cx_init_fimp_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Fimp", cx->any_type);
  t->equid = equid_imp;
  t->call = call_imp;
  t->fprint = fprint_imp;
  return t;
}
