# Structs

Structs are still in development, but they allow for grouping related data.

## Definition

```stride
struct Point {
    x: i32,
    y: i32,
}
```

## Initialization

```stride
const p = Point::{ x: 10, y: 20 }
```

## Type Aliasing with Structs

You can create structs that reference other structs. These function as separate types and will cause a type error if used inappropriately.

```stride
struct Vector2d = Point

const p2: Vector2d = Vector2d::{ x: 5, y: 15 } // This is valid
const p3: Point = p2 // This is invalid
```
