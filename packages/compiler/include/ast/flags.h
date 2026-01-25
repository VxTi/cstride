#pragma once

#define SRFLAG_EXPR_TYPE_STANDALONE (0x0)
#define SRFLAG_EXPR_TYPE_INLINE     (0x1)

#define SR_PROPERTY_ACCESSOR_SEPARATOR ("@")

#define SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION (100)

#define SRFLAG_NONE                (0x0)
#define SRFLAG_TYPE_PTR            (0x1)
#define SRFLAG_TYPE_REFERENCE      (0x2)
#define SRFLAG_TYPE_MUTABLE        (0x4)
#define SRFLAG_TYPE_OPTIONAL       (0x8)
#define SRFLAG_TYPE_GENERIC        (0x10)
#define SRFLAG_TYPE_ARRAY          (0x20)
#define SRFLAG_TYPE_GLOBAL         (0x40)
#define SRFLAG_TYPE_EXTERN         (0x80)
#define SRFLAG_TYPE_VARIADIC       (0x100)

#define SRFLAG_FN_VARIADIC (0x1)
