#include "Headers/StringTests.h"
#include "Core/Math/Headers/MathHelper.h"

#include <iostream>

namespace Divide {

//TEST_FAILURE_INDENT(4);
// We are using third party string libraries (STL, Boost, EASTL) that went through proper testing
// This list of tests only verifies utility functions

TEST(TestReplaceInPlace)
{
    stringImpl input("STRING TO BE TESTED");

    const stringImpl match("TO BE");
    const stringImpl replacement("HAS BEEN");
    const stringImpl output("STRING HAS BEEN TESTED");

    Util::ReplaceStringInPlace(input, match, replacement);
    CHECK_EQUAL(input, output);
}

TEST(TestPermutations)
{
    const stringImpl input("ABC");
    vectorImpl<stringImpl> permutations;
    Util::GetPermutations(input, permutations);
    CHECK_TRUE(permutations.size() == 6);
}

TEST(TestNumberParse)
{
    const stringImpl input1("2");
    const stringImpl input2("b");
    CHECK_TRUE(Util::IsNumber(input1));
    CHECK_FALSE(Util::IsNumber(input2));
}

TEST(TestCharTrail)
{
    const stringImpl input("abcdefg");
    const stringImpl extension("efg");
    CHECK_TRUE(Util::GetTrailingCharacters(input, 3) == extension);
    CHECK_TRUE(Util::GetTrailingCharacters(input, 20) == input);

    size_t length = 4;
    CHECK_TRUE(Util::GetTrailingCharacters(input, length).size() == length);
}

TEST(TestCmpNoCase)
{
    const stringImpl inputA("aBcdEf");
    const stringImpl inputB("ABCdef");
    const stringImpl inputC("abcdefg");
    CHECK_TRUE(Util::CompareIgnoreCase(inputA, inputA));
    CHECK_TRUE(Util::CompareIgnoreCase(inputA, inputB));
    CHECK_FALSE(Util::CompareIgnoreCase(inputB, inputC));
}

TEST(TestExtension) {
    const stringImpl input("something.ext");
    const stringImpl ext1("ext");
    const stringImpl ext2("bak");
    CHECK_TRUE(Util::HasExtension(input, ext1));
    CHECK_FALSE(Util::HasExtension(input, ext2));
}

TEST(TestStringSplit) {
    const stringImpl input1("a b c d");
    const vectorImpl<stringImpl> result = {"a", "b", "c", "d"};

    CHECK_EQUAL(Util::Split(input1, ' '), result);
    CHECK_TRUE(Util::Split(input1, ',').size() == 1);
    CHECK_TRUE(Util::Split(input1, ',')[0] == input1);

    const stringImpl input2("a,b,c,d");
    CHECK_EQUAL(Util::Split(input2, ','), result);
}

TEST(TestStringTrim) {
    const stringImpl input1("  abc");
    const stringImpl input2("abc  ");
    const stringImpl input3("  abc  ");
    const stringImpl result("abc");

    CHECK_EQUAL(Util::Ltrim(input1), result);
    CHECK_EQUAL(Util::Ltrim(input2), input2);
    CHECK_EQUAL(Util::Ltrim(input3), input2);
    CHECK_EQUAL(Util::Ltrim(result), result);

    CHECK_EQUAL(Util::Rtrim(input1), input1);
    CHECK_EQUAL(Util::Rtrim(input2), result);
    CHECK_EQUAL(Util::Rtrim(input3), input1);
    CHECK_EQUAL(Util::Rtrim(result), result);

    CHECK_EQUAL(Util::Trim(input1), result);
    CHECK_EQUAL(Util::Trim(input2), result);
    CHECK_EQUAL(Util::Trim(input3), result);
    CHECK_EQUAL(Util::Trim(result), result);
}

TEST(TestStringFormat) {
    const stringImpl input1("A %s b is %d %s");
    const stringImpl input2("%2.2f");
    const stringImpl result1("A is ok, b is 2 \n");
    const stringImpl result2("12.21");

    CHECK_EQUAL(Util::StringFormat(input1, "is ok,", 2, "\n"), result1);
    CHECK_EQUAL(Util::StringFormat(input2, 12.2111f), result2);
}

TEST(TestCharRemove) {
    char input[] = {'a', 'b', 'c', 'b', 'd', '7', 'b', '\0' };
    char result[] = { 'a', 'c', 'd', '7', '\0'};

    Util::CStringRemoveChar(&input[0], 'b');

    CHECK_EQUAL(strlen(input), strlen(result));

    char* in = input;
    char* res = result;
    while(*in != '\0') {
        CHECK_EQUAL(*in, *res);

        in++; res++;
    }
}

TEST(TestRuntimeID) {
    const char* str = "TEST String garbagegarbagegarbage";
    ULL input1 = _ID(str);
    CHECK_EQUAL(input1, _ID_RT(str));
    CHECK_EQUAL(_ID_RT(str), _ID_RT(stringImpl(str)));
    CHECK_EQUAL(input1, _ID_RT(stringImpl(str)));
}

TEST(TestStringAllocator)
{
    const char* input = "TEST test TEST";
    stringImpl input1(input);
    stringImplAligned input2(input);
    for( U8 i = 0; i < input1.size(); ++i) {
        CHECK_EQUAL(input1[i], input2[i]);
    }
}

TEST(TestStringStream)
{
    stringImpl result1;
    stringImplAligned result2;
    stringstreamImplAligned s;
    const char* input = "TEST-test-TEST";
    s << input;
    s >> result1;
    CHECK_EQUAL(result1, stringImpl(input));
    s.clear();
    s << input;
    s >> result2;
    CHECK_EQUAL(result2, stringImplAligned(input));
}

};
