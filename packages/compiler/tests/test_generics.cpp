#include "utils.h"
#include <gtest/gtest.h>

using namespace stride::tests;

// ============================================================================
//  Generic type definitions (type aliases and objects)
// ============================================================================

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
        const some_var: BoxedArray<i32> = [Boxed<i32>::{ value: 1 }];
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

TEST(Generics, ObjectInstantiationName)
{
    // Test that generic objects are correctly instantiated and named
    assert_compiles(R"(
        type Vector<T> = { x: T; y: T; };
        const v: Vector<i32> = Vector<i32>::{ x: 1, y: 2 };
    )");
}

TEST(Generics, NestedObjectInstantiation)
{
    assert_compiles(R"(
        type Point<T> = { x: T; y: T; };
        type Line<T> = { start: Point<T>; end: Point<T>; };
        const l: Line<f32> = Line<f32>::{
            start: Point<f32>::{ x: 0.0, y: 0.0 },
            end: Point<f32>::{ x: 1.0, y: 1.0 }
        };
    )");
}

TEST(Generics, FunctionSignatureMismatch)
{
    assert_compiles(R"(
        type Array<T> = {
            length: i32;
            data: T[];
        };

        type SomeCar = {
            make: string;
        };

        type SomePerson = {
            cars: Array<SomeCar>;
        };

        fn make_person(cars: Array<SomeCar>): SomePerson {
            return SomePerson::{ cars };
        }

        fn main(): i32 {
            const p = make_person(Array<SomeCar>::{
                length: 1,
                data: [SomeCar::{ make: "Toyota" }]
            });
            return 0;
        }
    )");
}

TEST(Generics, GenericObjectWithMixedGenericAndConcreteFields)
{
    assert_compiles(R"(
        type Tagged<T> = { tag: string; value: T; };
        const t = Tagged<i32>::{ tag: "count", value: 42 };
    )");
}

// ============================================================================
//  Generic function definitions — body type resolution
// ============================================================================

TEST(GenericFunctions, ComparisonEqualsInBody)
{
    // The body uses `==` on generic params — requires resolve_generics_in_body
    // to resolve T to a primitive so the comparison validator accepts it
    assert_compiles(R"(
        fn are_equal<T>(a: T, b: T): bool {
            return a == b;
        }

        fn main(): i32 {
            const result = are_equal<i32>(5, 5);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ComparisonNotEqualInBody)
{
    assert_compiles(R"(
        fn not_equal<T>(a: T, b: T): bool {
            return a != b;
        }

        fn main(): i32 {
            const r = not_equal<i32>(1, 2);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ComparisonLessThanInBody)
{
    assert_compiles(R"(
        fn less_than<T>(a: T, b: T): bool {
            return a < b;
        }

        fn main(): i32 {
            const r = less_than<i32>(1, 2);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ComparisonGreaterThanOrEqualInBody)
{
    assert_compiles(R"(
        fn gte<T>(a: T, b: T): bool {
            return a >= b;
        }

        fn main(): i32 {
            const r = gte<i32>(5, 3);
            return 0;
        }
    )");
}

TEST(GenericFunctions, AdditionInBody)
{
    assert_compiles(R"(
        fn add<T>(a: T, b: T): T {
            return a + b;
        }

        fn main(): i32 {
            const sum = add<i32>(3, 4);
            return 0;
        }
    )");
}

TEST(GenericFunctions, SubtractionInBody)
{
    assert_compiles(R"(
        fn subtract<T>(a: T, b: T): T {
            return a - b;
        }

        fn main(): i32 {
            const d = subtract<i32>(10, 3);
            return 0;
        }
    )");
}

TEST(GenericFunctions, MultiplicationInBody)
{
    assert_compiles(R"(
        fn multiply<T>(a: T, b: T): T {
            return a * b;
        }

        fn main(): i32 {
            const p = multiply<i32>(6, 7);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ReturnsGenericType)
{
    // The function's return type is the generic parameter itself
    assert_compiles(R"(
        fn identity<T>(value: T): T {
            return value;
        }

        fn main(): i32 {
            const x = identity<i32>(42);
            return 0;
        }
    )");
}

TEST(GenericFunctions, VoidReturnType)
{
    // Generic function that returns void — body uses generic params but
    // doesn't return them
    assert_compiles(R"(
        fn consume<T>(value: T): void {
        }

        fn main(): void {
            consume<i32>(42);
        }
    )");
}

// ============================================================================
//  Generic function — multiple type parameters
// ============================================================================

TEST(GenericFunctions, TwoTypeParams)
{
    assert_compiles(R"(
        fn first_of<A, B>(a: A, b: B): A {
            return a;
        }

        fn main(): i32 {
            const x = first_of<i32, bool>(10, true);
            return 0;
        }
    )");
}

TEST(GenericFunctions, TwoTypeParamsReturnSecond)
{
    assert_compiles(R"(
        fn second_of<A, B>(a: A, b: B): B {
            return b;
        }

        fn main(): i32 {
            const x = second_of<bool, i32>(true, 42);
            return 0;
        }
    )");
}

// ============================================================================
//  Generic function — mixed generic and concrete parameters
// ============================================================================

TEST(GenericFunctions, MixedGenericAndConcreteParams)
{
    assert_compiles(R"(
        fn apply_flag<T>(value: T, flag: bool): T {
            return value;
        }

        fn main(): i32 {
            const x = apply_flag<i32>(7, true);
            return 0;
        }
    )");
}

// ============================================================================
//  Generic function — multiple instantiations of the same definition
// ============================================================================

TEST(GenericFunctions, MultipleInstantiationsSameFunction)
{
    // Same generic function instantiated with different concrete types
    assert_compiles(R"(
        fn identity<T>(v: T): T {
            return v;
        }

        fn main(): i32 {
            const a = identity<i32>(1);
            const b = identity<bool>(true);
            return 0;
        }
    )");
}

TEST(GenericFunctions, MultipleInstantiationsComparison)
{
    assert_compiles(R"(
        fn is_equal<T>(a: T, b: T): bool {
            return a == b;
        }

        fn main(): i32 {
            const ri = is_equal<i32>(5, 5);
            const rf = is_equal<f64>(1.0D, 2.0D);
            return 0;
        }
    )");
}

TEST(GenericFunctions, FloatInstantiation)
{
    assert_compiles(R"(
        fn add<T>(a: T, b: T): T {
            return a + b;
        }

        fn main(): i32 {
            const sum = add<f64>(1.5D, 2.5D);
            return 0;
        }
    )");
}

// ============================================================================
//  Generic function — complex body patterns (control flow)
// ============================================================================

TEST(GenericFunctions, ConditionalWithGenericComparison)
{
    assert_compiles(R"(
        fn max_of<T>(a: T, b: T): T {
            if (a > b) {
                return a;
            }
            return b;
        }

        fn main(): i32 {
            const m = max_of<i32>(3, 7);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ConditionalWithElse)
{
    assert_compiles(R"(
        fn min_of<T>(a: T, b: T): T {
            if (a < b) {
                return a;
            } else {
                return b;
            }
        }

        fn main(): i32 {
            const m = min_of<i32>(3, 7);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ClampFunction)
{
    assert_compiles(R"(
        fn clamp<T>(value: T, lo: T, hi: T): T {
            if (value < lo) {
                return lo;
            }
            if (value > hi) {
                return hi;
            }
            return value;
        }

        fn main(): i32 {
            const r = clamp<i32>(15, 0, 10);
            return 0;
        }
    )");
}

TEST(GenericFunctions, WhileLoopInBody)
{
    assert_compiles(R"(
        fn count_down<T>(start: T): T {
            let n = start;
            while (n > 0) {
                n = n - 1;
            }
            return n;
        }

        fn main(): i32 {
            const z = count_down<i32>(5);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ForLoopInBody)
{
    assert_compiles(R"(
        fn accumulate<T>(base: T, n: i32): T {
            let result = base;
            for (let i: i32 = 0; i < n; i++) {
                result = result + base;
            }
            return result;
        }

        fn main(): i32 {
            const s = accumulate<i32>(3, 4);
            return 0;
        }
    )");
}

TEST(GenericFunctions, LocalVariableWithGenericType)
{
    // A local variable inside a generic function body whose type derives
    // from the generic parameter
    assert_compiles(R"(
        fn double_it<T>(x: T): T {
            const result: T = x + x;
            return result;
        }

        fn main(): i32 {
            const d = double_it<i32>(21);
            return 0;
        }
    )");
}

// ============================================================================
//  Generic functions interacting with generic object types
// ============================================================================

TEST(GenericFunctions, AcceptsGenericObject)
{
    assert_compiles(R"(
        type Pair<T> = { first: T; second: T; };

        fn get_first<T>(p: Pair<T>): T {
            return p.first;
        }

        fn main(): i32 {
            const p = Pair<i32>::{ first: 1, second: 2 };
            const v = get_first<i32>(p);
            return 0;
        }
    )");
}

TEST(GenericFunctions, ReturnsGenericObject)
{
    assert_compiles(R"(
        type Wrapper<T> = { value: T; };

        fn wrap<T>(v: T): Wrapper<T> {
            return Wrapper<T>::{ value: v };
        }

        fn main(): i32 {
            const w = wrap<i32>(99);
            return 0;
        }
    )");
}

TEST(GenericFunctions, CreatesGenericObjectInBody)
{
    assert_compiles(R"(
        type Box<T> = { inner: T; };

        fn make_box<T>(v: T): Box<T> {
            const b = Box<T>::{ inner: v };
            return b;
        }

        fn main(): i32 {
            const b = make_box<i32>(5);
            return 0;
        }
    )");
}

TEST(GenericFunctions, GenericObjectFieldAccess)
{
    assert_compiles(R"(
        type Container<T> = { data: T; };

        fn extract<T>(c: Container<T>): T {
            return c.data;
        }

        fn main(): i32 {
            const c = Container<i32>::{ data: 123 };
            const v = extract<i32>(c);
            return 0;
        }
    )");
}

// ============================================================================
//  Generic function — uninstantiated (should compile without errors)
// ============================================================================

TEST(GenericFunctions, UninstantiatedGenericIsNotCodegenerated)
{
    // A generic function that is never called should not cause errors;
    // it simply won't be code-generated.
    assert_compiles(R"(
        fn unused_generic<T>(x: T): T {
            return x;
        }

        fn main(): i32 {
            return 0;
        }
    )");
}

// ============================================================================
//  Generic function — error cases
// ============================================================================

TEST(GenericFunctions, ReturnTypeMismatch)
{
    // Generic function declared to return T, but instantiated as i32 and
    // returning a bool literal — should fail type checking
    assert_throws_message(R"(
        fn wrong_return<T>(v: T): T {
            return true;
        }

        fn main(): i32 {
            const x = wrong_return<i32>(1);
            return 0;
        }
    )", "expected a return type of");
}
