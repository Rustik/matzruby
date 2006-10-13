/************************************************

  digest.c -

  $Author$
  created at: Fri May 25 08:57:27 JST 2001

  Copyright (C) 1995-2001 Yukihiro Matsumoto
  Copyright (C) 2001-2006 Akinori MUSHA

  $RoughId: digest.c,v 1.16 2001/07/13 15:38:27 knu Exp $
  $Id$

************************************************/

#include "digest.h"

static VALUE mDigest, cDigest_Base;
static ID id_metadata, id_new, id_initialize, id_update, id_digest;

/*
 * Digest::Base
 */

static algo_t *
get_digest_base_metadata(VALUE klass)
{
    VALUE obj;
    algo_t *algo;

    if (rb_ivar_defined(klass, id_metadata) == Qfalse) {
        return NULL;
    }

    obj = rb_ivar_get(klass, id_metadata);

    Data_Get_Struct(obj, algo_t, algo);

    return algo;
}

static VALUE
hexdigest_str_new(VALUE str_digest)
{
    char *digest;
    size_t digest_len;
    int i;
    VALUE str;
    char *p;
    static const char hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };

    StringValue(str_digest);
    digest = RSTRING_PTR(str_digest);
    digest_len = RSTRING_LEN(str_digest);

    if (LONG_MAX / 2 < digest_len) {
        rb_raise(rb_eRuntimeError, "digest string too long");
    }

    str = rb_str_new(0, digest_len * 2);

    for (i = 0, p = RSTRING_PTR(str); i < digest_len; i++) {
        unsigned char byte = digest[i];

        p[i + i]     = hex[byte >> 4];
        p[i + i + 1] = hex[byte & 0x0f];
    }

    return str;
}

static VALUE
bubblebabble_str_new(VALUE str_digest)
{
    char *digest;
    size_t digest_len;
    VALUE str;
    char *p;
    int i, j, seed = 1;
    static const char vowels[] = {
        'a', 'e', 'i', 'o', 'u', 'y'
    };
    static const char consonants[] = {
        'b', 'c', 'd', 'f', 'g', 'h', 'k', 'l', 'm', 'n',
        'p', 'r', 's', 't', 'v', 'z', 'x'
    };

    StringValue(str_digest);
    digest = RSTRING_PTR(str_digest);
    digest_len = RSTRING_LEN(str_digest);

    if ((LONG_MAX - 2) / 3 < (digest_len | 1)) {
	rb_raise(rb_eRuntimeError, "digest string too long");
    }

    str = rb_str_new(0, (digest_len | 1) * 3 + 2);
    p = RSTRING_PTR(str);

    i = j = 0;
    p[j++] = 'x';

    for (;;) {
        unsigned char byte1, byte2;

        if (i >= digest_len) {
            p[j++] = vowels[seed % 6];
            p[j++] = consonants[16];
            p[j++] = vowels[seed / 6];
            break;
        } 

        byte1 = digest[i++];
        p[j++] = vowels[(((byte1 >> 6) & 3) + seed) % 6];
        p[j++] = consonants[(byte1 >> 2) & 15];
        p[j++] = vowels[((byte1 & 3) + (seed / 6)) % 6];

        if (i >= digest_len) {
            break;
        }

        byte2 = digest[i++];
        p[j++] = consonants[(byte2 >> 4) & 15];
        p[j++] = '-';
        p[j++] = consonants[byte2 & 15];

        seed = (seed * 5 + byte1 * 7 + byte2) % 36;
    }

    p[j] = 'x';

    return str;
}

static VALUE
rb_digest_base_alloc(VALUE klass)
{
    algo_t *algo;
    VALUE obj;
    void *pctx;

    if (klass == cDigest_Base) {
	rb_raise(rb_eNotImpError, "Digest::Base is an abstract class");
    }

    algo = get_digest_base_metadata(klass);

    if (algo == NULL) {
        return Data_Wrap_Struct(klass, 0, free, 0);
    }

    pctx = xmalloc(algo->ctx_size);
    algo->init_func(pctx);

    obj = Data_Wrap_Struct(klass, 0, free, pctx);

    return obj;
}

static VALUE
rb_digest_base_s_digest(VALUE klass, VALUE str)
{
    algo_t *algo = get_digest_base_metadata(klass);
    void *pctx;
    volatile VALUE obj;

    if (algo == NULL) {
        VALUE obj = rb_funcall(klass, id_new, 0);
        rb_funcall(obj, id_update, 1, str);
        return rb_funcall(obj, id_digest, 0);
    }

    obj = rb_digest_base_alloc(klass);
    Data_Get_Struct(obj, void, pctx);

    StringValue(str);
    algo->update_func(pctx, RSTRING_PTR(str), RSTRING_LEN(str));

    str = rb_str_new(0, algo->digest_len);
    algo->finish_func(pctx, RSTRING_PTR(str));

    return str;
}

static VALUE
rb_digest_base_s_hexdigest(VALUE klass, VALUE str)
{
    return hexdigest_str_new(rb_funcall(klass, id_digest, 1, str));
}

static VALUE
rb_digest_base_s_bubblebabble(VALUE klass, VALUE str)
{
    return bubblebabble_str_new(rb_funcall(klass, id_digest, 1, str));
}

static VALUE
rb_digest_base_copy(VALUE copy, VALUE obj)
{
    algo_t *algo;
    void *pctx1, *pctx2;

    if (copy == obj) return copy;
    rb_check_frozen(copy);
    algo = get_digest_base_metadata(rb_obj_class(copy));

    if (algo == NULL) {
        /* initialize_copy() is undefined or something */
        rb_notimplement();
    }

    /* get_digest_base_metadata() may return a NULL */
    if (algo != get_digest_base_metadata(rb_obj_class(obj))) {
	rb_raise(rb_eTypeError, "wrong argument class");
    }
    Data_Get_Struct(obj, void, pctx1);
    Data_Get_Struct(copy, void, pctx2);
    memcpy(pctx2, pctx1, algo->ctx_size);

    return copy;
}

static VALUE
rb_digest_base_reset(VALUE self)
{
    algo_t *algo;
    void *pctx;

    algo = get_digest_base_metadata(rb_obj_class(self));

    if (algo == NULL) {
        rb_funcall(self, id_initialize, 0);

        return self;
    }

    Data_Get_Struct(self, void, pctx);

    memset(pctx, 0, algo->ctx_size);
    algo->init_func(pctx);

    return self;
}

static VALUE
rb_digest_base_update(VALUE self, VALUE str)
{
    algo_t *algo;
    void *pctx;

    algo = get_digest_base_metadata(rb_obj_class(self));

    if (algo == NULL) {
        /* subclasses must define update() */
        rb_notimplement();
    }

    Data_Get_Struct(self, void, pctx);

    StringValue(str);
    algo->update_func(pctx, RSTRING_PTR(str), RSTRING_LEN(str));

    return self;
}

static VALUE
rb_digest_base_lshift(VALUE self, VALUE str)
{
    algo_t *algo;
    void *pctx;

    algo = get_digest_base_metadata(rb_obj_class(self));

    if (algo == NULL) {
        /* subclasses just need to define update(), not << */
        rb_funcall(self, id_update, 1, str);

        return self;
    }

    Data_Get_Struct(self, void, pctx);

    StringValue(str);
    algo->update_func(pctx, RSTRING_PTR(str), RSTRING_LEN(str));

    return self;
}

static VALUE
rb_digest_base_init(int argc, VALUE *argv, VALUE self)
{
    VALUE arg;

    rb_scan_args(argc, argv, "01", &arg);

    if (!NIL_P(arg)) rb_digest_base_update(self, arg);

    return self;
}

static VALUE
rb_digest_base_digest(VALUE self)
{
    algo_t *algo;
    void *pctx1, *pctx2;
    size_t ctx_size;
    VALUE str;

    algo = get_digest_base_metadata(rb_obj_class(self));

    if (algo == NULL) {
        /* subclasses must define update() */
        rb_notimplement();
    }

    Data_Get_Struct(self, void, pctx1);

    ctx_size = algo->ctx_size;
    pctx2 = xmalloc(ctx_size);
    memcpy(pctx2, pctx1, ctx_size);

    str = rb_str_new(0, algo->digest_len);
    algo->finish_func(pctx2, RSTRING_PTR(str));
    free(pctx2);

    return str;
}

static VALUE
rb_digest_base_hexdigest(VALUE self)
{
    return hexdigest_str_new(rb_funcall(self, id_digest, 0));
}

static VALUE
rb_digest_base_bubblebabble(VALUE self)
{
    return bubblebabble_str_new(rb_funcall(self, id_digest, 0));
}

static VALUE
rb_digest_base_inspect(VALUE self)
{
    algo_t *algo;
    VALUE klass, str;
    size_t digest_len = 32;	/* no need to be just the right size */
    char *cname;

    klass = rb_obj_class(self);
    algo = get_digest_base_metadata(klass);

    if (algo != NULL)
        digest_len = algo->digest_len;

    cname = rb_obj_classname(self);

    /* #<Digest::Alg: xxxxx...xxxx> */
    str = rb_str_buf_new(2 + strlen(cname) + 2 + digest_len * 2 + 1);
    rb_str_buf_cat2(str, "#<");
    rb_str_buf_cat2(str, cname);
    rb_str_buf_cat2(str, ": ");
    rb_str_buf_append(str, rb_digest_base_hexdigest(self));
    rb_str_buf_cat2(str, ">");
    return str;
}

static VALUE
rb_digest_base_equal(VALUE self, VALUE other)
{
    algo_t *algo;
    VALUE klass;
    VALUE str1, str2;

    klass = rb_obj_class(self);

    if (rb_obj_class(other) == klass) {
        str1 = rb_funcall(self, id_digest, 0);
        str2 = rb_funcall(other, id_digest, 0);
    } else {
        StringValue(other);
        str2 = other;

        algo = get_digest_base_metadata(klass);

        if (RSTRING_LEN(str2) == algo->digest_len)
            str1 = rb_funcall(self, id_digest, 0);
        else
            str1 = rb_digest_base_hexdigest(self);
    }

    if (RSTRING_LEN(str1) == RSTRING_LEN(str2)
	&& rb_str_cmp(str1, str2) == 0)
	return Qtrue;

    return Qfalse;
}

/*
 * This module provides an interface to the following hash algorithms:
 *
 *   - the MD5 Message-Digest Algorithm by the RSA Data Security,
 *     Inc., described in RFC 1321
 *
 *   - the SHA-1 Secure Hash Algorithm by NIST (the US' National
 *     Institute of Standards and Technology), described in FIPS PUB
 *     180-1.
 *
 *   - the SHA-256/384/512 Secure Hash Algorithm by NIST (the US'
 *     National Institute of Standards and Technology), described in
 *     FIPS PUB 180-2.
 *
 *   - the RIPEMD-160 cryptographic hash function, designed by Hans
 *     Dobbertin, Antoon Bosselaers, and Bart Preneel.
 */

void
Init_digest(void)
{
    mDigest = rb_define_module("Digest");

    cDigest_Base = rb_define_class_under(mDigest, "Base", rb_cObject);

    rb_define_alloc_func(cDigest_Base, rb_digest_base_alloc);
    rb_define_singleton_method(cDigest_Base, "digest", rb_digest_base_s_digest, 1);
    rb_define_singleton_method(cDigest_Base, "hexdigest", rb_digest_base_s_hexdigest, 1);
    rb_define_singleton_method(cDigest_Base, "bubblebabble", rb_digest_base_s_bubblebabble, 1);

    rb_define_method(cDigest_Base, "initialize", rb_digest_base_init, -1);
    rb_define_method(cDigest_Base, "initialize_copy",  rb_digest_base_copy, 1);
    rb_define_method(cDigest_Base, "reset", rb_digest_base_reset, 0);
    rb_define_method(cDigest_Base, "update", rb_digest_base_update, 1);
    rb_define_method(cDigest_Base, "<<", rb_digest_base_lshift, 1);
    rb_define_method(cDigest_Base, "digest", rb_digest_base_digest, 0);
    rb_define_method(cDigest_Base, "hexdigest", rb_digest_base_hexdigest, 0);
    rb_define_method(cDigest_Base, "bubblebabble", rb_digest_base_bubblebabble, 0);
    rb_define_method(cDigest_Base, "to_s", rb_digest_base_hexdigest, 0);
    rb_define_method(cDigest_Base, "inspect", rb_digest_base_inspect, 0);
    rb_define_method(cDigest_Base, "==", rb_digest_base_equal, 1);

    id_metadata = rb_intern("metadata");
    id_new = rb_intern("new");
    id_initialize = rb_intern("initialize");
    id_update = rb_intern("update");
    id_digest = rb_intern("digest");
}
