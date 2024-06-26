#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stdlib.h>
typedef uint64_t Value;

// Masks for important segments of a float value
#define MASK_SIGN        0x8000000000000000
#define MASK_EXPONENT    0x7ff0000000000000
#define MASK_QUIET       0x0008000000000000
#define MASK_TYPE        0x0007000000000000
#define MASK_SIGNATURE   0xffff000000000000
#define MASK_PAYLOAD_PTR 0x0000ffffffffffff
#define MASK_PAYLOAD_INT 0x00000000ffffffff

// Type IDs for short encoded types
#define MASK_TYPE_NAN      0x0000000000000000
#define MASK_TYPE_SPECIAL  0x0001000000000000
#define MASK_TYPE_INTEGER  0x0002000000000000
#define MASK_TYPE_STRING   0x0003000000000000
#define MASK_TYPE_FUNCTION 0x0004000000000000
#define MASK_TYPE_FUNCENV  0x0005000000000000
#define MASK_TYPE_CLOSURE  0x0006000000000000

// Constant short encoded values
#define kNaN   (MASK_EXPONENT | MASK_QUIET)
#define kNull  (kNaN | MASK_TYPE_SPECIAL)

// Signatures of encoded types
#define SIGNATURE_NAN      kNaN
#define SIGNATURE_SPECIAL  kNull
#define SIGNATURE_INTEGER  (kNaN | MASK_TYPE_INTEGER)
#define SIGNATURE_STRING   (kNaN | MASK_TYPE_STRING)
#define SIGNATURE_FUNCTION (kNaN | MASK_TYPE_CLOSURE)
#define SIGNATURE_FUNCENV  (kNaN | MASK_TYPE_FUNCENV)
#define SIGNATURE_POINTER  (kNaN | MASK_SIGN)

typedef int32_t reg;

char *type_of(Value v);
Value equal(Value x, Value y);
char *constructor_name(Value x);
void native_print(Value value);

// The type of the stored value
typedef enum {
  TYPE_INTEGER = 0,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_LIST,
  TYPE_SPECIAL,
  TYPE_MUTABLE,
  TYPE_FUNCTION,
  TYPE_FUNCENV,
  TYPE_CLOSURE,
  TYPE_UNKNOWN,
} ValueType;

// Container for arrays
typedef struct {
  Value* data;
  uint32_t length;
} Array;

typedef struct {
  char* data;
  uint32_t length;
} String;

// Container type for values
typedef struct {
  ValueType type;
  uint32_t length;

  union {
    char* as_string;
    Value* as_ptr;
  };
} HeapValue;

#define MAKE_INTEGER(x) (SIGNATURE_INTEGER | (uint32_t) (x))
#define MAKE_FLOAT(x) (*(Value*)(&(x)))
#define MAKE_PTR(x) ( SIGNATURE_POINTER | (uint64_t) (x))

typedef Value Closure[2];

#define MAKE_FUNCTION(x, y) (SIGNATURE_FUNCTION | (uint64_t) (x) | ((uint64_t) (y) << 16))
#define MAKE_FUNCENV(pc, sp, bp) (SIGNATURE_FUNCENV | (uint64_t) (pc) | ((uint64_t) (sp) << 16) | ((uint64_t) (bp) << 32))

static inline Value MAKE_STRING(char* x, uint32_t len) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = len;
  v->type = TYPE_STRING;
  v->as_string = x;
  return MAKE_PTR(v);
}

static inline Value MAKE_CLOSURE(reg pc, reg bp) {
  Closure clos = { pc, bp };
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = 2;
  v->type = TYPE_CLOSURE;
  v->as_ptr = clos;
  return MAKE_PTR(v);
}

static inline Value MAKE_LIST(Value* x, uint32_t len) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = len;
  v->type = TYPE_LIST;
  v->as_ptr = x;
  return MAKE_PTR(v);
}

static inline Value MAKE_MUTABLE(Value x) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = 1;
  v->type = TYPE_MUTABLE;
  v->as_ptr = &x;
  return MAKE_PTR(v);
}

#define MAKE_SPECIAL() kNull
#define MAKE_ADDRESS(x) MAKE_INTEGER(x)
#define MAKE_NATIVE(x) MAKE_STRING(x, strlen(x))

#define GET_PTR(x) ((HeapValue*)((x) & MASK_PAYLOAD_PTR))
#define GET_STRING(x) GET_PTR(x)->as_string
#define GET_LIST(x) GET_PTR(x)->as_ptr
#define GET_MUTABLE(x) *(GET_PTR(x)->as_ptr)

#define GET_INT(x) ((x) & MASK_PAYLOAD_INT)
#define GET_FLOAT(x) (*(double*)(&(x)))
#define GET_ADDRESS(x) GET_INT(x)
#define GET_NATIVE(x) GET_STRING(x)
#define GET_NTH_ELEMENT(x, n) ((x >> (n * 16)) & MASK_PAYLOAD_INT)

#define IS_PTR(x) (((x) & MASK_SIGNATURE) == SIGNATURE_POINTER)
#define IS_FUN(x) (((x) & MASK_SIGNATURE) == SIGNATURE_FUNCTION)

static inline ValueType get_type(Value value) {
  uint64_t signature = value & MASK_SIGNATURE;
  if ((~value & MASK_EXPONENT) != 0) return TYPE_FLOAT;

  // Check for encoded pointer
  if (signature == SIGNATURE_POINTER) {
    HeapValue* ptr = GET_PTR(value);
    return ptr->type;
  }

  // Short encoded types
  switch (signature) {
    case SIGNATURE_NAN:      return TYPE_FLOAT;
    case SIGNATURE_SPECIAL:  return TYPE_SPECIAL;
    case SIGNATURE_INTEGER:  return TYPE_INTEGER;
    case SIGNATURE_FUNCTION: return TYPE_FUNCTION;
    case SIGNATURE_FUNCENV:  return TYPE_FUNCENV;
  }

  return TYPE_UNKNOWN;
}

#endif  // VALUE_H