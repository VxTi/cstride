#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Structs, Definition)
{
    assert_parses(R"(
        type Point = {
            x: int32;
            y: int32;
        };

        type Color = {
            r: uint8;
            g: uint8;
            b: uint8;
            a: uint8;
        };
    )");
}

TEST(Structs, Initialization)
{
    assert_parses(R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const p: Point = Point::{ x: 10, y: 20 };
    )");
}

TEST(Structs, Composition)
{
    assert_parses(R"(
        type Point = { x: int32; y: int32; };
        type Rect = {
            origin: Point;
            width: int32;
            height: int32;
        };
    )");
}

TEST(Structs, Referencing)
{
    assert_parses(R"(
        type Point = { x: int32; y: int32; };
        type Vector2d = Point;

        const p: Point = Point::{ x: 10, y: 20 };
        const v: Vector2d = Vector2d::{ x: 10, y: 20 };
    )");
}
