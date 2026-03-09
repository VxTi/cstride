#include "utils.h"
#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Generics, ResolveGenericNamedTypeUnderlyingType)
{
    // type SomeNamed<T> = T[]
    // const some_var: SomeNamed<i32> = 12; // This should FAIL if i32[] is correctly resolved as underlying type, 
    // because 12 is not an array.
    // However, the issue description says:
    // "I expect that instantiate_named_type(SomeNamed<i32>) yields i32[]"
    
    // If it yields i32[], then:
    // const some_var: SomeNamed<i32> = [1, 2, 3];
    // should WORK.
    
    assert_compiles(R"(
        type SomeNamed<T> = T[];
        const some_var: SomeNamed<i32> = [1, 2, 3];
    )");
}

TEST(Generics, NestedGenericResolution)
{
    assert_compiles(R"(
        type Wrap<T> = T;
        type SomeNamed<T> = Wrap<T>[];
        const some_var: SomeNamed<i32> = [1, 2, 3];
    )");
}

TEST(Generics, DeepGenericResolution)
{
    assert_compiles(R"(
        type Boxed<T> = { value: T; };
        type BoxedArray<T> = Boxed<T>[];
        const some_var: BoxedArray<i32> = [Boxed::{ value: 1 }];
    )");
}

TEST(Generics, ResolveGenericNamedTypeUnderlyingTypeMismatch)
{
    // This should fail because i32 is not i32[]
    assert_throws_message(R"(
        type SomeNamed<T> = T[];
        const some_var: SomeNamed<i32> = 12;
    )", "Type mismatch in variable declaration: cannot assign value of type 'i32' to type 'SomeNamed<i32>'");
}
