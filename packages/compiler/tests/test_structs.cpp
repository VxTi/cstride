#include <gtest/gtest.h>
#include "utils.h"

using namespace stride::tests;

TEST(Structs, Definition) {
    const std::string code = R"(
        struct Point {
            x: i32;
            y: i32;
        }

        struct Color {
            r: u8;
            g: u8;
            b: u8;
            a: u8;
        }
    )";
    assert_parses(code);
}

TEST(Structs, Initialization) {
    const std::string code = R"(
        struct Point {
            x: i32;
            y: i32;
        }

        struct Color {
            r: i32;
            g: i32;
            b: i32;
            a: i32;
        }

        fn main(): void {
            const p: Point = Point::{ x: 10, y: 20 };

            const transparent_red: Color = Color::{
                r: 255,
                g: 0,
                b: 0,
                a: 128
            };
        }
    )";
    assert_parses(code);
}

TEST(Structs, Composition) {
    /*
       Parsing support for nested nominal types is implied if types are parsed generically.
       Testing nested struct defs if supported, or composition.
    */
    const std::string code = R"(
        struct Point { x: i32; y: i32; }
        struct Rect {
            origin: Point;
            width: i32;
            height: i32;
        }
    )";
    assert_parses(code);
}

TEST(Structs, Aliasing) {
    const std::string code = R"(
        struct Point { x: i32; y: i32; }
        struct Vector2d = Point;

        fn main(): void {
            // Usage of alias
            // Assuming alias can be initialized same way or it's just type equality
            // The docs say: "nominal types... treated as distinct types"
            // So Vector2d::{...} should work if it behaves like a struct name
            // Or maybe Point::{...} assigned to Vector2d?
            // "distinct types" implies assignment might fail without cast,
            // but initialization syntax for Vector2d might exist.

            // Checking if parser accepts alias declaration:
        }
    )";
    assert_parses(code);
}
