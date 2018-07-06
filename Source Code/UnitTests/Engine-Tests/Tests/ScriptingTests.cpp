#include "Headers/Defines.h"

#include "Scripting/Headers/Script.h"

namespace Divide {

TEST(TestSimpleMath)
{
    Script input("5.3 + 2.1");
    double result = 7.4;

    CHECK_EQUAL(input.eval<double>(), result);
}

TEST(TestCFunctionCall)
{
    Script input("var my_fun = fun(x) { return x + 2; };"
                 "something(my_fun)");

    I32 variable = 0;
    auto testFunc = [&variable](const DELEGATE_CBK<I32, I32>& t_func) {
        variable = t_func(variable);
    };

    input.registerFunction(testFunc, "something");

    input.eval();
    CHECK_EQUAL(variable, 2);
}
}; //namespace Divide