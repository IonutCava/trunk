#include "Headers/StringTests.h"
#include "Core/Math/Headers/MathHelper.h"

namespace Divide {

// We are using third party string libraries (STL, Boost, EASTL) that went through proper testing
// This list of test only verifies utility functions

U32 TestUtility() {
    {
        stringImpl input("STRING TO BE TESTED");

        const stringImpl match("TO BE");
        const stringImpl replacement("HAS BEEN");
        const stringImpl output("STRING HAS BEEN TESTED");

        Util::ReplaceStringInPlace(input, match, replacement);
        CHECK_EQUAL(input, output);
    }
    {
        const stringImpl input("ABC");
        vectorImpl<stringImpl> permutations;
        Util::GetPermutations(input, permutations);
        CHECK_TRUE(permutations.size() == 9);
    }
    {
        const stringImpl input1("2");
        const stringImpl input2("b");
        CHECK_TRUE(Util::IsNumber(input1));
        CHECK_FALSE(Util::IsNumber(input2));
    }
    {
        const stringImpl input("abcdefg");
        const stringImpl extension("efg");
        CHECK_TRUE(Util::GetTrailingCharacters(input, 3) == extension);
        CHECK_TRUE(Util::GetTrailingCharacters(input, 20) == input);

        size_t length = 4;
        CHECK_TRUE(Util::GetTrailingCharacters(input, length).size() == length);
    }
    {
        const stringImpl inputA("aBcdEf");
        const stringImpl inputB("ABCdef");
        const stringImpl inputC("abcdefg");
        CHECK_TRUE(Util::CompareIgnoreCase(inputA, inputA));
        CHECK_TRUE(Util::CompareIgnoreCase(inputA, inputB));
        CHECK_FALSE(Util::CompareIgnoreCase(inputB, inputC));
    }
    {
        const stringImpl input("something.ext");
        const stringImpl ext1("ext");
        const stringImpl ext2("bak");
        CHECK_TRUE(Util::HasExtension(input, ext1));
        CHECK_FALSE(Util::HasExtension(input, ext2));
    }

    {
        const stringImpl input1("  abc");
        const stringImpl input2("abc  ");
        const stringImpl input3("  abc   ");
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
    {
        const stringImpl input1("A %s b is %d %s");
        const stringImpl input2("%2.2f");
        const stringImpl result1("A is ok, b is 2 \n");
        const stringImpl result2("12.21");

        CHECK_EQUAL(Util::StringFormat(input1, "is ok,", 2, "\n"), result1);
        CHECK_EQUAL(Util::StringFormat(input2, 12.2111f), result2);
    }
    {
        char input[] = {'a', 'b', 'c', 'b', 'd', '7', 'b' };
        char result[] = { 'a', 'c', 'd', '7', '\0', '\0', '\0' };
        Util::CStringRemoveChar(&input[0], 'b');
        for (U8 i = 0; i < 7; ++i) {
            CHECK_EQUAL(input[i], result[i]);
        }
    }

    return NO_ERROR;
}

U32 run_StringTests() {
    U32 ret = NO_ERROR;

    if ((ret = TestUtility()) != NO_ERROR) {
        return ret;
    }

    return ret;
}

};