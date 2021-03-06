/*
 * Copyright (c) 2016 Nicholas Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "benchmark.h"
#include "cmp.h"
#include "buffer.h"

static object_t* root_object;

// cmp doesn't have built-in support for writing to a growable buffer.
static size_t buffer_cmp_writer(cmp_ctx_t* ctx, const void* data, size_t count) {
    return buffer_write((buffer_t*)ctx->buf, (const char*)data, count) ? count : 0;
}

static bool write_object(cmp_ctx_t* cmp, object_t* object) {
    switch (object->type) {
        case type_bool:   return cmp_write_bool(cmp, object->b);
        case type_nil:    return cmp_write_nil(cmp);
        case type_int:    return cmp_write_sint(cmp, object->i);
        case type_uint:   return cmp_write_uint(cmp, object->u);
        case type_double: return cmp_write_double(cmp, object->d);
        case type_str:    return cmp_write_str(cmp, object->str, object->l);

        case type_array:
            if (!cmp_write_array(cmp, object->l))
                return false;
            for (size_t i = 0; i < object->l; ++i)
                if (!write_object(cmp, object->children + i))
                    return false;
            return true;

        case type_map:
            if (!cmp_write_map(cmp, object->l))
                return false;
            for (size_t i = 0; i < object->l; ++i) {

                // we expect keys to be short strings
                object_t* key = object->children + i * 2;
                assert(key->type == type_str);
                if (!cmp_write_str(cmp, key->str, key->l))
                    return false;

                if (!write_object(cmp, object->children + i * 2 + 1))
                    return false;
            }
            return true;

        default:
            assert(0);
            break;
    }
    return false;
}

bool run_test(uint32_t* hash_out) {
    buffer_t buffer;
    buffer_init(&buffer);
    cmp_ctx_t cmp;
    cmp_init(&cmp, &buffer, NULL, buffer_cmp_writer);

    if (!write_object(&cmp, root_object)) {
        buffer_destroy(&buffer);
        return false;
    }

    *hash_out = hash_str(*hash_out, buffer.data, buffer.count);
    buffer_destroy(&buffer);
    return true;
}

bool setup_test(size_t object_size) {
    root_object = benchmark_object_create(object_size);
    return true;
}

void teardown_test(void) {
    object_destroy(root_object);
}

bool is_benchmark(void) {
    return true;
}

const char* test_version(void) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "v%u", cmp_version());
    return buf;
}

const char* test_language(void) {
    return BENCHMARK_LANGUAGE_C;
}

const char* test_format(void) {
    return "MessagePack";
}

const char* test_filename(void) {
    return __FILE__;
}
