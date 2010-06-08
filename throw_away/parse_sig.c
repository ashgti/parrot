#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef struct signature_t {
  char* sig_string;
  unsigned int argc;
  void* return_pmc;
  ffi_type *return_type;
  ffi_type **args;
  ffi_cif cif;
  void* arg_pmc_list; 
} signature;

char parse_sig(char* sig, signature* sig_obj);
size_t parse_return(char* sig, signature* sig_obj);
size_t parse_args(char* sig, signature*  sig_obj);
size_t parse_identifier(char* sig, ffi_type** sig_obj, void* pmc_type);
size_t parse_structure(char* sig, ffi_type** sig_obj, void* pmc_type);
int parse_prefix(char sig);
size_t structure_length(char* sig);
size_t count_args(char* sig);
void apply_modifiers();

#define IS_DIGIT(x)               (x == '0') || (x == '1') || (x == '2') || (x == '3') || (x == '4') || (x == '5') || (x == '6') || (x == '7') || (x == '8') || (x == '9')
#define IS_PREFIX(x)              (x == '*') || (x == 'u') || (x == 's')
#define IS_TYPE(x)                (x == 'i') || (x == 'v') || (x == 'l') || (x == 't') || (x == 'c') || (x == 'b') || (x == 'f') || (x == 'd')
#define IS_POSTFIX(x)             (x == '{') || IS_DIGIT(x)
#define IS_INFIX(x)               (x == '|')
#define IS_START_CIRCUMFIX(x)     (x == '(')
#define IS_END_CIRCUMFIX(x)       (x == ')')
#define IS_NOT_END_CIRCUMFIX(x)   (x != ')')

#define PREFIX_POINTER  1<<0
#define PREFIX_SIGNED      0
#define PREFIX_UNSIGNED 1<<1
#define PREFIX_NATIVE   1<<2

char
parse_sig(char* sig, signature* sig_obj) {
  size_t i = parse_return(sig, sig_obj);
  parse_args(sig + i, sig_obj);

  if (ffi_prep_cif(&sig_obj->cif, FFI_DEFAULT_ABI, sig_obj->argc, sig_obj->return_type, sig_obj->args) == FFI_OK) {
    return 1;
  }

  printf("Bad signature");
  return 0;
}

size_t
parse_return(char* sig, signature* sig_obj) {
  return parse_identifier(sig, &sig_obj->return_type, sig_obj->return_pmc);
}

size_t 
parse_args(char* sig, signature* sig_obj) {
  size_t i = 0;
  size_t arg_count = 0;
  size_t argc = count_args(sig);

  sig_obj->args = malloc(sizeof(ffi_type*) * argc + 1);

  while (sig[i] != '\0') {
    i += parse_identifier(sig + i, &sig_obj->args[arg_count], sig_obj->arg_pmc_list + arg_count);
    arg_count += 1;
  }

  sig_obj->argc = argc;

  return i;
}

size_t
parse_identifier(char* sig, ffi_type ** type_obj, void* type, char ) {
  size_t i = 0;
  char done = 0;
  while (!done) {
    int prefix = 0;
    while (IS_PREFIX(sig[i])) {
      prefix |= parse_prefix(sig[i]);

      i += 1;
    }

    if (IS_START_CIRCUMFIX(sig[i])) {
      i += parse_structure(sig + i++, type_obj, type);
    }
    else if (IS_TYPE(sig[i])) {
      if (prefix & PREFIX_POINTER) {
        *type_obj = &ffi_type_pointer;
      }
      else if (prefix & PREFIX_NATIVE) {
        if (prefix & PREFIX_UNSIGNED) {
          switch (sig[i]) {
            case 'c':
              *type_obj = &ffi_type_uchar;
              break;
            case 'b':
              *type_obj = &ffi_type_uint8;
              break;
            case 's':
              *type_obj = &ffi_type_ushort;
              break;
            case 'i':
              *type_obj = &ffi_type_uint;
              break;
           case 'l':
              *type_obj = &ffi_type_ulong;
              break;
            case 'q':
              *type_obj = &ffi_type_uint64;
              break;
            case 't':
              *type_obj = &ffi_type_pointer;
              break;
            case 'v':
              *type_obj = &ffi_type_void;
              break;
            case 'f':
              *type_obj = &ffi_type_float;
              break;
            case 'd':
              *type_obj = &ffi_type_double;
              break;
            case 'D':
              *type_obj = &ffi_type_longdouble;
              break;
          }

        }
        else {
          switch (sig[i]) {
            case 'c':
              *type_obj = &ffi_type_schar;
              break;
            case 'b':
              *type_obj = &ffi_type_sint8;
              break;
            case 's':
              *type_obj = &ffi_type_sshort;
              break;
            case 'i':
              *type_obj = &ffi_type_sint;
              break;
           case 'l':
              *type_obj = &ffi_type_slong;
              break;
            case 'q':
              *type_obj = &ffi_type_sint64;
              break;
            case 't':
              *type_obj = &ffi_type_pointer;
              break;
            case 'v':
              *type_obj = &ffi_type_void;
              break;
            case 'f':
              *type_obj = &ffi_type_float;
              break;
            case 'd':
              *type_obj = &ffi_type_double;
              break;
            case 'D':
              *type_obj = &ffi_type_longdouble;
              break;
          }
        }
      }
      else {
        if (prefix & PREFIX_UNSIGNED) {
          switch (sig[i]) {
            case 'c':
            case 'b':
              *type_obj = &ffi_type_uint8;
              break;
            case 's':
              *type_obj = &ffi_type_uint16;
              break;
            case 'i':
            case 'l':
              *type_obj = &ffi_type_uint32;
              break;
            case 'q':
              *type_obj = &ffi_type_uint64;
              break;
            case 't':
              *type_obj = &ffi_type_pointer;
              break;
            case 'v':
              *type_obj = &ffi_type_void;
              break;
            case 'f':
              *type_obj = &ffi_type_float;
              break;
            case 'd':
              *type_obj = &ffi_type_double;
              break;
            case 'D':
              *type_obj = &ffi_type_longdouble;
              break;
          }
        }
        else {
          switch (sig[i]) {
            case 'c':
            case 'b':
              *type_obj = &ffi_type_sint8;
              break;
            case 's':
              *type_obj = &ffi_type_sint16;
              break;
            case 'i':
            case 'l':
              *type_obj = &ffi_type_sint32;
              break;
            case 'q':
              *type_obj = &ffi_type_sint64;
              break;
            case 't':
              *type_obj = &ffi_type_pointer;
              break;
            case 'v':
              *type_obj = &ffi_type_void;
              break;
            case 'f':
              *type_obj = &ffi_type_float;
              break;
            case 'd':
              *type_obj = &ffi_type_double;
              break;
            case 'D':
              *type_obj = &ffi_type_longdouble;
              break;
          }

        }
      }
      ++i;
    }
    else {
      //"Error, need an identifier";
    }

    // Parse postfix ops
    while (IS_POSTFIX(sig[i])) {
    }
    
    // apply_modifiers(type, prefix, postfix);

    if (sig[i+1] != '|') done = 1; // Its a union, lets do something special
  }

  return i;
}

int
parse_prefix(char c) {
  switch (c) {
    case '*':
      return PREFIX_POINTER;
    case 'u':
      return PREFIX_UNSIGNED;
    case 's':
      return PREFIX_SIGNED;
  }

  return 0;
}

size_t
parse_structure(char* sig, ffi_type** type_obj, void* pmc_type) {
  size_t i = 1;
  size_t struct_len = structure_length(sig + 1);

  *type_obj = malloc(sizeof(ffi_type));
  (*type_obj)->elements = malloc(sizeof(ffi_type*) * struct_len + 1);

  (*type_obj)->size = (*type_obj)->alignment = 0;
  (*type_obj)->type = FFI_TYPE_STRUCT;

  size_t element_counter = 0;

  while (sig[i] != '\0' && IS_NOT_END_CIRCUMFIX(sig[i])) {
    i += parse_identifier(sig + i, &(*type_obj)->elements[element_counter], NULL);

    element_counter += 1;
  }

  (*type_obj)->elements[struct_len] = NULL;

  return i;
}

size_t 
structure_length(char* sig) {
  size_t len = 0;
  size_t i = 0;
  int depth = 0;
  while (sig[i] != '\0' && depth != -1) {
    if (IS_START_CIRCUMFIX(sig[i])) depth++;
    else if (IS_END_CIRCUMFIX(sig[i])) depth--;
    else if (depth == 0 && (IS_TYPE(sig[i]))) len++;
    i += 1;
  }

  return len;
}

size_t
find_matching(char* sig, char start, char end) {
  size_t i = 0;
  while (sig[i] != '\0' && sig[i] != end) {
    if (sig[i] == start)
      i += find_matching(sig + i, start, end);
    i += 1;
  }

  return i;
}

size_t
count_args(char* sig) {
  size_t length = 0;
  size_t i = 0;

  while (sig[i] != '\0') {
    if (IS_START_CIRCUMFIX(sig[i])) {
      i += find_matching(sig + ++i, '(', ')');
      length += 1;
    }

    if (IS_TYPE(sig[i])) {
      length += 1;
    }

    i++;
  }

  return length;
}

void
add_four(int *a) {
  *a += 4;
}

int
add_strcut(struct { int x; int y; } a) {
  return a.x + a.y;
}

int main() {
  signature sig;
  parse_sig("it", &sig);

  void* values[2];

  char *s;
  values[0] = &s;

  int r1;

  unsigned int r2;

  s = "plan 1";
  ffi_call(&sig.cif, FFI_FN(puts), &r1, values);

  printf("plan 1 r: %d\n", r1);

  s = "plan 2";
  parse_sig("uit", &sig);
  ffi_call(&sig.cif, FFI_FN(puts), &r2, values);

  printf("plan 2r : %u\n", r2);

  int a = 3;
  int *b = &a;

  add_four(&a);

  printf("a: %d\n", a);

  values[0] = &b;

  printf("&a : %p\n", &a);
  
  parse_sig("v*i", &sig);
  ffi_call(&sig.cif, FFI_FN(add_four), NULL, values);

  printf("a: %d\n\n", a);


  parse_sig("i(ii)", &sig);

  struct {int a; int b;} x; 
  x.a = 13;
  x.b = 3;

  values[0] = &x;
  int r3 = 0;

  ffi_cif cif;
  ffi_type ts1_type;
  ffi_type *ts1_type_elements[3];
  ts1_type.size = 0;
  ts1_type.alignment = 0;
  ts1_type.type = FFI_TYPE_STRUCT;
  ts1_type.elements = ts1_type_elements;
  ts1_type_elements[0] = &ffi_type_sint;
  ts1_type_elements[1] = &ffi_type_sint;
  ts1_type_elements[2] = NULL;

  ffi_type **args = malloc(sizeof(ffi_type*));

  args[0] = &ts1_type;

  /* Initialize the cif */
  ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, sig.return_type, sig.args);

  ffi_call(&cif, FFI_FN(add_strcut), &r3, values);

  printf("Done? %d\n\n", r3);

  r3 = 0;

  x.a = 13;
  x.b = 3;

  values[0] = &x;

  ffi_call(&sig.cif, FFI_FN(add_strcut), &r3, values);

  printf("Done? %d\n\n", r3);


  // Dynamic libs
  void *handle;
  void *(*fn)(void*);
  char *error;
  handle = dlopen("libm.dylib", RTLD_LAZY);
  if (!handle) {
    fprintf (stderr, "%s\n", dlerror());
    exit(1);
  }
  dlerror();    /* Clear any existing error */
  fn = dlsym(handle, "cos");
  if ((error = dlerror()) != NULL)  {
    fprintf (stderr, "%s\n", error);
    exit(1);
  }

  parse_sig("dd", &sig);

  double r4 = 0, arg4 = 2.00;
  values[0] = &arg4;

  ffi_call(&sig.cif, FFI_FN(fn), &r4, values);

  printf ("%f\n", r4);
  dlclose(handle);

  return 0;
}
