#include <inttypes.h>

#include "cixl/cx.h"
#include "cixl/box.h"
#include "cixl/error.h"
#include "cixl/scope.h"
#include "cixl/types/fimp.h"
#include "cixl/types/func.h"
#include "cixl/types/rat.h"

static uint64_t gcd(uint64_t a, uint64_t b) {
  while (b) {
    uint64_t t = b;
    b = a % b;
    a = t;
  }

  return a;
}

static void simplify(struct cx_rat *rat) {
  uint64_t m = gcd(rat->num, rat->den);
  rat->num /= m;
  rat->den /= m;
}

struct cx_rat *cx_rat_init(struct cx_rat *rat, uint64_t num, uint64_t den, bool neg) {
  rat->num = num;
  rat->den = den;
  rat->neg = neg;
  return rat;
}

int64_t cx_rat_int(struct cx_rat *rat) {
  int64_t n = rat->num / rat->den;
  return rat->neg ? -n : n;
}

void cx_rat_add(struct cx_rat *dst, struct cx_rat *src) {
  uint64_t
    den = gcd(dst->den, src->den),
    dst_num = dst->num * den / dst->den,
    src_num = src->num * den / src->den;

  if (dst->neg == src->neg) {
    dst->num = dst_num + src_num;
  } else {
    if (src_num > dst_num) {
      dst->num = src_num - dst_num;
      dst->neg = src->neg;
    } else {
      dst->num = dst_num - src_num;
    }
  }
  
  dst->den = den;
  simplify(dst);
}

void cx_rat_mul(struct cx_rat *dst, struct cx_rat *src) {
  dst->num = dst->num*src->num;
  dst->den = dst->den*src->den;
  dst->neg ^= src->neg;
  simplify(dst);
}

enum cx_cmp cx_cmp_rat(const void *x, const void *y) {
  const struct cx_rat *xv = x, *yv = y;

  uint64_t
    den = gcd(xv->den, yv->den),
    xn = xv->num * den / xv->den,
    yn = xv->num * den / xv->den;

  if (xn < yn) { return CX_CMP_LT; }
  return (xn > yn) ? CX_CMP_GT : CX_CMP_EQ;
}

static bool lt_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    x = *cx_test(cx_pop(scope, false));

  cx_box_init(cx_push(scope),
	      scope->cx->bool_type)->as_bool = cx_cmp_rat(&x.as_rat,
							  &y.as_rat) == CX_CMP_LT;
  
  return true;
}

static bool gt_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    x = *cx_test(cx_pop(scope, false));

  cx_box_init(cx_push(scope),
	      scope->cx->bool_type)->as_bool = cx_cmp_rat(&x.as_rat,
							  &y.as_rat) == CX_CMP_GT;
  
  return true;
}

static bool int_imp(struct cx_scope *scope) {
  struct cx_box v = *cx_test(cx_pop(scope, false));
  cx_box_init(cx_push(scope), scope->cx->int_type)->as_int = cx_rat_int(&v.as_rat);
  return true;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  struct cx_rat *xr = &x->as_rat, *yr = &y->as_rat;
  return xr->num == yr->num && xr->den == yr->den;
}

static bool ok_imp(struct cx_box *v) {
  struct cx_rat *r = &v->as_rat;
  return r->num != 0;
}

static void fprint_imp(struct cx_box *v, FILE *out) {
  struct cx_rat *r = &v->as_rat;
  fprintf(out, "%s%" PRIu64 "/%" PRIu64, r->neg ? "-" : "", r->num, r->den);
}

struct cx_type *cx_init_rat_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Rat", cx->num_type);
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->fprint = fprint_imp;

  cx_add_func(cx, "<", cx_arg(t), cx_arg(t))->ptr = lt_imp;
  cx_add_func(cx, ">", cx_arg(t), cx_arg(t))->ptr = gt_imp;
  
  cx_add_func(cx, "int", cx_arg(t))->ptr = int_imp;
  
  return t;
}
