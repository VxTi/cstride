# Structs

Structs allow you to create custom data types by grouping related values together.

!!! warning "Work in Progress"
    Structs are partially implemented. You can define and initialize them, but member access (e.g., `p.x`) is currently under development.

## Definition

Structs are defined using the `struct` keyword followed by a name and a block of fields. Each field has a name and a type.

```stride
struct Point {
    x: i32,
    y: i32,
}

struct Color {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}
```

## Initialization

You can initialize a struct using the `StructName::{ field: value, ... }` syntax.

```stride
fn main(): void {
    const p: Point = Point::{ x: 10, y: 20 }
    
    const transparent_red: Color = Color::{ 
        r: 255u8, 
        g: 0u8, 
        b: 0u8, 
        a: 128u8 
    }
}
```

## Type Aliasing and Nominal Typing

Stride supports creating new types based on existing structs. These are **nominal** types, meaning even if they have the same structure, they are treated as distinct types by the compiler.

```stride
struct Vector2d = Point

fn main(): void {
    const p1: Point = Point::{ x: 5, y: 10 }
    const v1: Vector2d = Vector2d::{ x: 5, y: 10 }
    
    // The following would result in a type error:
    // const p2: Point = v1 
}
```
