#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Structs, Definition)
{
    assert_parses(R"(
        struct Point {
            x: int32;
            y: int32;
        }

        struct Color {
            r: u8;
            g: u8;
            b: u8;
            a: u8;
        }
    )");
}

TEST(Structs, Initialization)
{
    assert_parses(R"(
        struct Point {
            x: int32;
            y: int32;
        }

        const p: Point = Point::{ x: 10, y: 20 };
    )");
}

TEST(Structs, Composition)
{
    assert_parses(R"(
        struct Point { x: int32; y: int32; }
        struct Rect {
            origin: Point;
            width: int32;
            height: int32;
        }
    )");
}

TEST(Structs, Referencing)
{
    assert_parses(R"(
        struct Point { x: int32; y: int32; }
        struct Vector2d = Point;

        const p: Point = Point::{ x: 10, y: 20 };
        const v: Vector2d = Vector2d::{ x: 10, y: 20 };
    )");
}
