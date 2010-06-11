#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

typedef struct Parrot_NCI_attributes_t {
	char* sig_string;
	unsigned int arity;
	void* return_pmc;
	ffi_type *return_type;
	ffi_type **args;
	void* cif;
	void* arg_pmc_list; 
	char* pcc_return_signature;
	char* pcc_params_signature;
} Parrot_NCI_attributes;

#define INTVAL int
#define STRING char


int Parrot_str_indexed(int i, char* string, int index) {
	return string[index];
}

#define PARROT_INTERP int interp


INTVAL parse_sig(PARROT_INTERP, STRING *sig, size_t sig_length, Parrot_NCI_attributes *nci_info);

size_t parse_return(PARROT_INTERP, STRING* sig, size_t sig_length, Parrot_NCI_attributes * nci_info, ffi_type **return_type);

size_t parse_args(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length, Parrot_NCI_attributes *nci_info, ffi_type ***arg_types);

size_t parse_identifier(PARROT_INTERP, STRING* sig, size_t start, size_t end, ffi_type** sig_obj, void* pmc_type);

size_t parse_structure(PARROT_INTERP, STRING* sig, size_t start, size_t end, ffi_type** sig_obj, void* pmc_type);

INTVAL parse_prefix(INTVAL c);

size_t structure_length(PARROT_INTERP, STRING* sig, size_t start, size_t end);

size_t count_args(PARROT_INTERP, STRING* sig, size_t start, size_t end);

size_t find_matching(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length, INTVAL start_character, INTVAL end_character);

#define IS_DIGIT(x)               (x == '0') || (x == '1') || (x == '2') || (x == '3') || (x == '4') || (x == '5') || (x == '6') || (x == '7') || (x == '8') || (x == '9')
#define IS_PREFIX(x)              (x == '*') || (x == 'u') || (x == 's')
#define IS_OLD_TYPE(x)            (x == 'P') || (x == 'J') || (x == 'N') || (x == 'S') || (x == 'O') || (x == '@') || (x == 'B') || (x == 'p') || (x == '2') || (x == '3') || (x == '4') || (x == 'U')
#define IS_TYPE(x)                (x == 'i') || (x == 'v') || (x == 'l') || (x == 't') || (x == 'c') || (x == 'b') || (x == 'f') || (x == 'd') || IS_OLD_TYPE(x)
#define IS_POSTFIX(x)             (x == '{') || IS_DIGIT(x)
#define IS_INFIX(x)               (x == '|')
#define IS_START_CIRCUMFIX(x)     (x == '(')
#define IS_END_CIRCUMFIX(x)       (x == ')')
#define IS_NOT_END_CIRCUMFIX(x)   (x != ')')

#define PREFIX_POINTER  1<<0
#define PREFIX_SIGNED      0
#define PREFIX_UNSIGNED 1<<1
#define PREFIX_NATIVE   1<<2

INTVAL
parse_sig(PARROT_INTERP, STRING *sig, size_t sig_length, Parrot_NCI_attributes *nci_info) {
    ffi_cif cif;
    ffi_type *return_type;
    ffi_type **arg_types = calloc(1, sizeof(ffi_type*));
    
    size_t i = parse_return(interp, sig, sig_length, nci_info, &return_type);
    parse_args(interp, sig, i, sig_length, nci_info, &arg_types);
	    
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nci_info->arity, return_type, arg_types) == FFI_OK) {
        nci_info->cif = (void*)malloc(sizeof(ffi_cif));
        memcpy(nci_info->cif, &cif, sizeof(ffi_cif));
        return 1;
    }
	
    // TODO: Throw Error here.
    printf("Bad signature\n");
    return 0;
}

size_t
parse_return(PARROT_INTERP, STRING *sig, size_t sig_length, Parrot_NCI_attributes* nci_info, ffi_type **return_type) {
    return parse_identifier(interp, sig, 0, sig_length, return_type, nci_info->pcc_return_signature);
}

size_t 
parse_args(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length, Parrot_NCI_attributes *nci_info, ffi_type ***arg_types) {
    size_t i = start;
    size_t arg_count = 0;
    size_t argc = count_args(interp, sig, start, sig_length);
	
    /* TODO: Fix this malloc */
    *arg_types = calloc(argc, sizeof(ffi_type*));
	
    while (i < sig_length) {
        i = parse_identifier(interp, sig, i, sig_length, &*arg_types[arg_count], nci_info->pcc_params_signature + arg_count);
        arg_count++;
    }
	
	*arg_types[0] = &ffi_type_pointer;

    nci_info->arity = arg_count;
	
    return i;
}

size_t
parse_identifier(PARROT_INTERP, STRING *sig, size_t start, size_t sig_length, ffi_type **type_obj, void *type) {
    size_t i = start;
    INTVAL done = 0;
	
    while (!done && i < sig_length) {
        INTVAL c = Parrot_str_indexed(interp, sig, i);
        // printf("C: %c %d %d \n\n", (char) c, i, sig_length);
        int prefix = 0;
        while (IS_PREFIX(c)) {
            prefix |= parse_prefix(c);
			
            i++;
			
            if (i < sig_length)
                c = Parrot_str_indexed(interp, sig, i);
            else
                return i;
        }
		
        if (IS_START_CIRCUMFIX(c)) {
            i = parse_structure(interp, sig, i + 1, sig_length, type_obj, type);
            i++;
			
            if (i < sig_length)
                c = Parrot_str_indexed(interp, sig, i);
            else
                return i;
        }
        else if (IS_TYPE(c)) {
            if (IS_OLD_TYPE(c)) {
                switch (c) {
					case (INTVAL)'0':    /* null ptr or such - doesn't consume a reg */
						break;
					case (INTVAL)'f':
					case (INTVAL)'N':
					case (INTVAL)'d':
						/* sig_buf[j++] = 'N'; */
						*type_obj = &ffi_type_double;
						break;
					case (INTVAL)'I':   /* INTVAL */
					case (INTVAL)'l':   /* long */
					case (INTVAL)'i':   /* int */
					case (INTVAL)'s':   /* short */
					case (INTVAL)'c':   /* char */
						/* sig_buf[j++] = 'I'; */
						*type_obj = &ffi_type_sint;
						break;
					case (INTVAL)'S':
					case (INTVAL)'t':   /* string, pass a cstring */
						/* sig_buf[j++] = 'S'; */
						*type_obj = &ffi_type_pointer;
						break;
					case (INTVAL)'J':   /* interpreter */
						break;
					case (INTVAL)'p':   /* push pmc->data */
					case (INTVAL)'P':   /* push PMC * */
					case (INTVAL)'V':   /* push PMC * */
					case (INTVAL)'2':
					case (INTVAL)'3':
					case (INTVAL)'4':
						/* sig_buf[j++] = 'P'; */
						*type_obj = &ffi_type_pointer;   
						break;
					case (INTVAL)'v':
						/* null return */
						*type_obj = &ffi_type_void;
						break;
					case (INTVAL)'O':   /* push PMC * invocant */
						/* TODO: Fix invocant */
						*type_obj = &ffi_type_pointer;
						/* sig_buf[j++] = 'P';
						 sig_buf[j++] = 'i'; */
						break;
					case (INTVAL)'@':   /* push PMC * slurpy */
						*type_obj = &ffi_type_pointer;
						/* 
						 sig_buf[j++] = 'P';
						 sig_buf[j++] = 's'; */
						break;
					case (INTVAL)'b': /* buffer (void*) pass Buffer_bufstart(SReg) */
					case (INTVAL)'B': /* buffer (void**) pass &Buffer_bufstart(SReg) */
						/* sig_buf[j++] = 'S'; */
						*type_obj = &ffi_type_pointer;
						break;
					default:
						printf("Error Unknown param Signature %c\n", (char)c);
						break;
                }
                
            }
            else if (prefix & PREFIX_POINTER) {
                *type_obj = &ffi_type_pointer;
            }
            else if (prefix & PREFIX_NATIVE) {
                if (prefix & PREFIX_UNSIGNED) {
                    switch (c) {
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
                    switch (c) {
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
                    switch (c) {
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
                    switch (c) {
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
            i++;
			
            if (i < sig_length)
                c = Parrot_str_indexed(interp, sig, i);
            else
                return i;
        }
        else {
			    printf("error C was: %c %ld\n", c, sig_length);

          exit(0);
        }
		
        // Parse postfix ops
        // TODO: Parse postfix ops, currently I skip them.
        while (IS_POSTFIX(c)) {
            i++;
            if (i < sig_length)
                c = Parrot_str_indexed(interp, sig, i);
            else
                return i;
        }
		
        // apply_modifiers(type, prefix, postfix);
		
        if ((i < sig_length) && (c == (INTVAL)'|')) {
            // Its a union, parse it special. 
        } else {
            done = 1;
        }
    }
	
    return i;
}

INTVAL
parse_prefix(INTVAL c) {
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
parse_structure(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length, ffi_type **type_obj, void* pmc_type) {
    size_t i = start;
    size_t struct_len = structure_length(interp, sig, start, sig_length);
	
    // TODO: Fix Malloc
    *type_obj = malloc(sizeof(ffi_type));
    (*type_obj)->elements = calloc(struct_len + 1, sizeof(ffi_type*));
	
    (*type_obj)->size = (*type_obj)->alignment = 0;
    (*type_obj)->type = FFI_TYPE_STRUCT;
	
    size_t element_counter = 0;
	
    INTVAL c = Parrot_str_indexed(interp, sig, i);
    while (i < sig_length && IS_NOT_END_CIRCUMFIX(c)) {
        i = parse_identifier(interp, sig, i, sig_length, &(*type_obj)->elements[element_counter], NULL);
        element_counter++;
        c = Parrot_str_indexed(interp, sig, i);
    }
	
    (*type_obj)->elements[struct_len] = NULL;
	
    return i;
}

size_t 
structure_length(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length) {
    size_t len = 0;
    size_t i = start;
    INTVAL depth = 0;
    INTVAL c = Parrot_str_indexed(interp, sig, i);
    while (i < sig_length && depth != -1) {
        if (IS_START_CIRCUMFIX(c)) depth++;
        else if (IS_END_CIRCUMFIX(c)) depth--;
        else if (depth == 0 && (IS_TYPE(c))) len++;
        i++;
        c = Parrot_str_indexed(interp, sig, i);
    }
	
    return len;
}

size_t
find_matching(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length, INTVAL start_character, INTVAL end_character) {
    size_t i = start;
    INTVAL c = Parrot_str_indexed(interp, sig, i);
    while (i < sig_length && c != end_character) {
        if (c == start_character)
            i = find_matching(interp, sig, i, sig_length, start_character, end_character);
        i++;
        c = Parrot_str_indexed(interp, sig, i);
    }
	
    return i;
}

size_t
count_args(PARROT_INTERP, STRING* sig, size_t start, size_t sig_length) {
    size_t length = 0;
    size_t i = start;
	
    while (i < sig_length) {
        const INTVAL c = Parrot_str_indexed(interp, sig, i);    
        if (IS_START_CIRCUMFIX(c)) {
            i = find_matching(interp, sig, i + 1, sig_length, '(', ')');
            length++;
        }
        else if (IS_TYPE(c)) {
            length++;
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
	Parrot_NCI_attributes sig;
	parse_sig(1, "it", 2, &sig);
	
	void* values[2];
	
	char *s;
	values[0] = &s;
	
	int r1;
	
	unsigned int r2;
	
	s = "plan 1";
	
	ffi_cif cif1, holder;
	ffi_type * args1[1];
	args1[0] = &ffi_type_pointer;
	
	holder = *(ffi_cif*)sig.cif;
	
	/* Initialize the cif */
	ffi_prep_cif(&cif1, FFI_DEFAULT_ABI, 1, &ffi_type_sint, args1);
	
	ffi_call(&cif1, FFI_FN(puts), &r1, values);
	
	void * foo = &cif1;
	
	s = "plan 1";
	
	ffi_call((ffi_cif*)foo, FFI_FN(puts), &r1, values);
	
	s = "plan 2";
	
	ffi_call((ffi_cif*)sig.cif, FFI_FN(puts), &r1, values);
	
	printf("plan 1 r: %d\n", r1);
	
	s = "plan 2";
	parse_sig(1, "uit", 3, &sig);
	ffi_call(sig.cif, FFI_FN(puts), &r2, values);
	
	printf("plan 2r : %u\n", r2);
	
	int a = 3;
	int *b = &a;
	
	add_four(&a);
	
	printf("a: %d\n", a);
	
	values[0] = &b;
	
	printf("&a : %p\n", &a);
	
	parse_sig(1, "v*i", 3, &sig);
	ffi_call(sig.cif, FFI_FN(add_four), NULL, values);
	
	printf("a: %d\n\n", a);
	
	
	parse_sig(1, "i(ii)", 5, &sig);
	
	struct {int a; int b;} x; 
	x.a = 13;
	x.b = 3;
	
	values[0] = &x;
	int r3 = 0;
	
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
	
	r3 = 0;
	
	x.a = 13;
	x.b = 3;
	
	values[0] = &x;
	
	ffi_call(sig.cif, FFI_FN(add_strcut), &r3, values);
	
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
	
	parse_sig(1, "dd", 2, &sig);
	
	double r4 = 0, arg4 = 2.00;
	values[0] = &arg4;
	
	ffi_call(sig.cif, FFI_FN(fn), &r4, values);
	
	printf ("%f\n", r4);
	dlclose(handle);
	
	return 0;
}

