#include "utils.h"

using namespace stride::tests;

TEST(Array, SimpleIntDefinition)
{
    assert_parses(R"(
      fn main(): void {
        const arr: i32[] = [ 123 ];
      }
    )");
}

TEST(Array, SimpleFloatDefinition)
{
    assert_parses(R"(
      fn main(): void {
        const arr: f64[] = [ 1.0D ];
      }
    )");
}

TEST(Array, OptionalDefinition)
{
    assert_parses(R"(
      fn main(): void {
        const arr: f64[]?;
      }
    )");
}

TEST(Array, MultiDimensionalDefinition)
{
    assert_parses(R"(
      fn main(): void {
        const arr: i32[][] = [
          [123], [456]
        ];
      }
    )");
}
