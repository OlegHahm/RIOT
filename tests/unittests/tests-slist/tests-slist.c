/*
 * Copyright (C) 2014 Benjamin Valentin <benpicco@zedat.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief slist (simple list) unittest
 *
 * @author      Benjamin Valentin <benpicco@zedat.fu-berlin.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "embUnit.h"
#include "slist.h"

char foo[] = "Hello World!";
char bar[] = "Hello CUnit!";
char baz[] = "c-c-c-combobreaker";

char print_buffer[128];

struct test_list {
    struct test_list *next;
    char *buffer;
    int value;
};

struct number_list {
    struct number_list *next;
    int value;
};

struct test_list *_get_by_buffer(struct test_list *head, char *buffer)
{
    return simple_list_find(head, buffer);
}

struct test_list *_get_by_value(struct test_list *head, int value)
{
    return simple_list_find(head, value);
}

struct test_list *_add_test_list(struct test_list **head, char *buffer, int value)
{
    struct test_list *node = simple_list_add_tail(head);

    node->buffer = buffer;
    node->value  = value;

    return node;
}

char *_print_result(struct test_list *result)
{
    if (result) {
        snprintf(print_buffer, sizeof print_buffer, "%d, %s\n", result->value,
                 result->buffer);
    }
    else {
        snprintf(print_buffer, sizeof print_buffer, "Not found\n");
    }

    return print_buffer;
}

int _is_equal(struct test_list *node, const int value, const char *buffer)
{
    return node != 0 && node->value == value && !strcmp(node->buffer, buffer);
}

void test_simple_list_fill(struct test_list *_head)
{
    CHECK_TRUE(_is_equal(_get_by_buffer(_head, bar), 42, bar), "%s",
               _print_result(_get_by_buffer(_head, bar)));
    CHECK_TRUE(_is_equal(_get_by_value(_head, 23), 23, foo), "%s",
               _print_result(_get_by_value(_head, 23)));
}

void test_simple_list_remove(struct test_list **__head)
{
    struct test_list *_head;
    simple_list_remove(__head, _get_by_buffer(*__head, foo));
    _head = *__head;

    CHECK_TRUE(_is_equal(_head, 42, bar), "%s", _print_result(_head));
    CHECK_TRUE(_is_equal(_head->next, 1337, baz), "%s", _print_result(_head->next));
}

void test_simple_list_find(struct test_list *_head)
{
    char buffer[sizeof bar];
    memcpy(buffer, bar, sizeof buffer);

    CHECK_TRUE(_is_equal(simple_list_find_memcmp(_head, buffer), 42, bar), "%s",
               _print_result(simple_list_find_memcmp(_head, buffer)));

    CHECK_TRUE(_is_equal(simple_list_find_cmp(_head, buffer,
                         (int ( *)(void *, void *)) strcmp), 42, bar), "%s",
                _print_result(simple_list_find_cmp(_head, buffer,
                              (int ( *)(void *, void *)) strcmp)));
}

void _add_number_list(struct number_list **head, int value)
{
    struct number_list *node = simple_list_add_before(head, value);
    node->value = value;
}

void test_number_list(void)
{
    struct number_list *head = 0;

    _add_number_list(&head, 23);
    _add_number_list(&head, 42);
    _add_number_list(&head, 17);
    _add_number_list(&head, 32);
    _add_number_list(&head, 1);

    int prev = 0;
    struct number_list *node;
    simple_list_for_each(head, node) {
        CHECK_TRUE(node->value >= prev, "%d < %d", node->value, prev);
        prev = node->value;
    }
}

void test_for_each_remove(void)
{
    struct number_list *head = 0;

    int i = 0;
    int max = 11;

    for (i = 1; i < max; ++i) {
        if (i == 2) {
            _add_number_list(&head, 3);
        }
        else {
            _add_number_list(&head, i);
        }
    }

    char skipped;
    struct number_list *node, *prev;
    simple_list_for_each_safe(head, node, prev, skipped) {
        if (node->value % 2) {
            printf("removing %d\n", node->value);
            simple_list_for_each_remove(&head, node, prev);
        }
        else {
            printf("keeping %d\n", node->value);
        }
    }

    i = 0;
    simple_list_for_each(head, node) {
        CHECK_TRUE(node->value % 2 == 0, "%d", node->value);
        ++i;
    }
    CHECK_TRUE(i == max / 2 - 1, "missed an entry");

    simple_list_clear(&head);

    CHECK_TRUE(head == NULL, "list not cleared properly");
}


Test *tests_slist_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_base64_01_encode_string),
        new_TestFixture(test_base64_02_decode_base64),
        new_TestFixture(test_base64_03_single_character),
        new_TestFixture(test_base64_04_free_conversion),
        new_TestFixture(test_base64_05_decode_larger),
        new_TestFixture(test_base64_06_stream_encode),
        new_TestFixture(test_base64_07_stream_decode),
    };

    EMB_UNIT_TESTCALLER(base64_tests, NULL, NULL, fixtures);

    return (Test *)&base64_tests;
}

void tests_slist(void)
{
    TESTS_RUN(tests_slist_tests());
}

int main(void)
{
    struct test_list *_head = 0;

    _add_test_list(&_head, foo, 23);
    _add_test_list(&_head, bar, 42);
    _add_test_list(&_head, baz, 1337);

    struct test_list *node;
    simple_list_for_each(_head, node)
    printf("%s\n", _print_result(node));

    test_simple_list_fill(_head);
    test_simple_list_remove(&_head);
    test_simple_list_find(_head);

    test_number_list();
    test_for_each_remove();

    puts("-----------------");
    printf("End, %d errors\n", fail);

    return fail;
}
