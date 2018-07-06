#include "Headers/Defines.h"

#include "Scripting/Headers/Script.h"

namespace Divide {
    
class ScriptTestClass
{
public:
    ScriptTestClass()
        : _script(nullptr)
    {
    }

    ~ScriptTestClass()
    {
        delete _script;
    }

    void setCode(const char* code) {
        delete _script;
        _script = new Script(code);
    }

    template<typename T>
    T eval() {
        return _script->eval<T>();
    }

    Script& script() { return *_script; }
private:
    Script* _script;
};

ScriptTestClass* g_local_test_class;

TEST_SETUP(ScriptTestClass)
{
    PlatformInit(0, nullptr);
    g_local_test_class = new ScriptTestClass();
}

TEST_TEARDOWN(ScriptTestClass)
{
    delete g_local_test_class;
}

TEST_MEMBER_FUNCTION(ScriptTestClass, eval, Simple)
{
    g_local_test_class->setCode("5.3 + 2.1");
    double result = 7.4;

    CHECK_EQUAL(g_local_test_class->eval<double>(), result);
}

TEST_MEMBER_FUNCTION(ScriptTestClass, eval, ExternalFunction)
{
    g_local_test_class->setCode("use(\"utility.chai\");"
                                "var my_fun = fun(x) { return x + 2; };"
                                "something(my_fun)");

    I32 variable = 0;
    auto testFunc = [&variable](const DELEGATE_CBK<I32, I32>& t_func) {
        variable = t_func(variable);
    };

    g_local_test_class->script().registerFunction(testFunc, "something");
    g_local_test_class->eval<void>();
    CHECK_EQUAL(variable, 2);
}

}; //namespace Divide