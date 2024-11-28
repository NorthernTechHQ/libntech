#include <test.h>

#include <set.h>
#include <json.h>
#include <alloc.h>

void test_stringset_from_string(void)
{
    StringSet *s = StringSetFromString("one,two, three four,,", ',');

    assert_true(StringSetContains(s, "one"));
    assert_true(StringSetContains(s, "two"));
    assert_true(StringSetContains(s, " three four"));
    assert_true(StringSetContains(s, ""));

    assert_int_equal(4, StringSetSize(s));

    StringSetDestroy(s);
}

void test_stringset_clear(void)
{
    StringSet *s = StringSetNew();
    StringSetAdd(s, xstrdup("a"));
    StringSetAdd(s, xstrdup("b"));

    assert_int_equal(2, StringSetSize(s));

    StringSetClear(s);

    assert_int_equal(0, StringSetSize(s));

    StringSetDestroy(s);
}

void test_stringset_serialization(void)
{
    {
        StringSet *set = StringSetNew();
        StringSetAdd(set, xstrdup("tag_1"));
        StringSetAdd(set, xstrdup("tag_2"));
        StringSetAdd(set, xstrdup("tag_3"));

        Buffer *buff = StringSetToBuffer(set, ',');

        assert_true(buff);
        assert_string_equal(BufferData(buff), "tag_1,tag_2,tag_3");

        BufferDestroy(buff);
        StringSetDestroy(set);
    }

    {
        StringSet *set = StringSetNew();

        Buffer *buff = StringSetToBuffer(set, ',');

        assert_true(buff);
        assert_string_equal(BufferData(buff), "");

        BufferDestroy(buff);
        StringSetDestroy(set);
    }
}

void test_stringset_join(void)
{
    {
        StringSet *set1 = StringSetNew();
        StringSet *set2 = StringSetNew();
        StringSetJoin(set1, set2, xstrdup);
        StringSetDestroy(set2);

        assert_int_equal(0, StringSetSize(set1));

        Buffer *buff = StringSetToBuffer(set1, ',');

        assert_true(buff);
        assert_string_equal(BufferData(buff), "");

        BufferDestroy(buff);
        StringSetDestroy(set1);
    }

    {
        StringSet *set = StringSetNew();
        StringSetAdd(set, xstrdup("foo"));
        StringSetJoin(set, set, xstrdup);

        assert_int_equal(1, StringSetSize(set));

        Buffer *buff = StringSetToBuffer(set, ',');

        assert_true(buff);
        assert_string_equal(BufferData(buff), "foo");

        StringSetDestroy(set);
        BufferDestroy(buff);
    }

    {
        StringSet *set1 = StringSetNew();
        StringSet *set2 = StringSetNew();
        StringSetAdd(set1, xstrdup("foo"));
        StringSetAdd(set2, xstrdup("bar"));
        StringSetJoin(set1, set2, xstrdup);
        StringSetDestroy(set2);

        assert_int_equal(2, StringSetSize(set1));

        Buffer *buff = StringSetToBuffer(set1, ',');
        StringSetDestroy(set1);

        assert_true(buff);
        assert_string_equal(BufferData(buff), "foo,bar");

        BufferDestroy(buff);
    }
}

void test_json_array_to_stringset(void)
{
    StringSet *expected = StringSetNew();
    StringSetAdd(expected, xstrdup("item1"));
    StringSetAdd(expected, xstrdup("2"));
    StringSetAdd(expected, xstrdup("item2"));

    JsonElement *json;
    const char *str = "[\"item1\", \"2\", \"item2\"]";
    assert_int_equal(JSON_PARSE_OK, JsonParse(&str, &json));
    StringSet *converted = JsonArrayToStringSet(json);
    assert_true(converted != NULL);
    assert_true(StringSetIsEqual(expected, converted));
    StringSetDestroy(converted);
    JsonDestroy(json);

    /* integer primitive should just be converted to string */
    str = "[\"item1\", 2, \"item2\"]";
    assert_int_equal(JSON_PARSE_OK, JsonParse(&str, &json));
    converted = JsonArrayToStringSet(json);
    assert_true(converted != NULL);
    assert_true(StringSetIsEqual(expected, converted));
    StringSetDestroy(converted);
    JsonDestroy(json);

    /* non-primitive child elements not supported */
    str = "[\"item1\", {\"key\": \"value\"}, \"item2\"]";
    assert_int_equal(JSON_PARSE_OK, JsonParse(&str, &json));
    converted = JsonArrayToStringSet(json);
    assert_true(converted == NULL);
    JsonDestroy(json);

    /* non-array JSONs not supported */
    str = "{\"key\": \"value\"}";
    assert_int_equal(JSON_PARSE_OK, JsonParse(&str, &json));
    converted = JsonArrayToStringSet(json);
    assert_true(converted == NULL);
    JsonDestroy(json);

    StringSetDestroy(expected);
}

void test_stringset_add_f(void) {
    StringSet *set = StringSetNew();

    StringSetAddF(set, "Hello %s!", "CFEngine");
    StringSetAdd(set, "Bye CFEngine!");
    StringSetAdd(set, xstrdup(""));

    assert_true(StringSetContains(set, "Hello CFEngine!"));
    assert_true(StringSetContains(set, "Bye CFEngine!"));
    assert_true(StringSetContains(set, ""));

    StringSetDestroy(set);
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_stringset_from_string),
        unit_test(test_stringset_serialization),
        unit_test(test_stringset_clear),
        unit_test(test_stringset_join),
        unit_test(test_json_array_to_stringset),
        unit_test(test_stringset_add_f),
    };

    return run_tests(tests);
}
