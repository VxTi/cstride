#include "utils.h"

using namespace stride::tests;

TEST(Array, SimpleIntDefinition)
{
    assert_compiles(R"(
      fn main(): void {
        const arr: int32[] = [ 123 ];
      }
    )");
}

TEST(Array, SimpleFloatDefinition)
{
    assert_compiles(R"(
      fn main(): void {
        const arr: float64[] = [ 1.0D ];
      }
    )");
}

TEST(Array, OptionalDefinition)
{
    assert_compiles(R"(
      fn main(): void {
        const arr: float64[]?;
      }
    )");
}

TEST(Array, MultiDimensionalDefinition)
{
    assert_compiles(R"(
      fn main(): void {
        const arr: int32[][] = [
          [123], [456]
        ];
      }
    )");
}
