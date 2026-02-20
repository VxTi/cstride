#include "utils.h"

using namespace stride::tests;

TEST(Modules, BasicModuleDefinition)
{
    assert_compiles(R"(
        module Foo { fn main(): void {} }
    )");
}

TEST(Modules, MultipleModules)
{
    assert_compiles(R"(
        module Foo { }
        module Bar { }
    )");
}

TEST(Modules, ModuleWithFunction)
{
    assert_compiles(R"(
        module Foo {
            fn foo(): void {}
        }
        module Bar {
            fn foo(): void {}
        }

    )");
}

TEST(Modules, NestedModules)
{
    assert_compiles(R"(
        module Outer {
            module Inner {
                fn nested(): void {}
            }
        }

        fn main(): void {
            Outer::Inner::nested();
        }
    )");
}

TEST(Modules, ModuleAccess)
{
    assert_compiles(R"(
        module Foo { fn foo(): void {} }
        module Bar { fn bar(): void {} }

        fn main(): void {
            Foo::foo();
            Bar::bar();
        }
    )");
}

TEST(Modules, ModuleWithSeveralSameSymbols)
{
    assert_compiles(R"(
        module Foo {
            const field = "foo";
        }

        const field = "bar";
    )");
}
