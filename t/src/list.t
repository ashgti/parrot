#! perl -w

use Parrot::Test tests => 2;

c_output_is(<<'CODE', <<'OUTPUT', "data types");

#include <stdio.h>
#include "parrot/parrot.h"
#include "parrot/embed.h"

int main(int argc, char* argv[]) {
    int x, i, j;
    char c;
    short t;
    FLOATVAL f;
    STRING *s;
    List* list, *list2;
    PMC *p1, *p2;

    Interp* interpreter = Parrot_new();
    if (interpreter == NULL) return 1;
    Parrot_init(interpreter, (void*) &x);

    list = list_new(interpreter, enum_type_int);
    if (list == NULL) {
	printf("list creation failed\n");
	return 1;
    }

    for (i = 0, x = 32; x < 43; x++, i++) {
	list_push(interpreter, list, INTVAL2PTR(void*, x) ,enum_type_int);
	j = *(int*) list_get(interpreter, list, i, enum_type_int);
	if (i < 10)
	    ;
	else
	    printf("The answer is %d.\n", j);
    }

    /* test various data types */
    list = list_new(interpreter, enum_type_char);

    list_assign(interpreter, list, 0 ,(void*) 'a', enum_type_char);
    list_assign(interpreter, list, 1 ,(void*) 'b', enum_type_char);
    list_assign(interpreter, list, 2 ,(void*) 'c', enum_type_char);
    list_assign(interpreter, list, 3 ,(void*) 'd', enum_type_char);
    c = *(char*) list_get(interpreter, list, 1, enum_type_char);
    printf("char %c\n",  c);

    list = list_new(interpreter, enum_type_short);

    list_assign(interpreter, list, 20 ,(void*) 15, enum_type_short);
    list_assign(interpreter, list, 21 ,(void*) -15, enum_type_short);
    list_assign(interpreter, list, 22 ,(void*) 99, enum_type_short);
    t = *(short*) list_get(interpreter, list, 21, enum_type_short);
    printf("short %d\n",  t);

    list = list_new(interpreter, enum_type_FLOATVAL);

    f = 1234.56;
    list_assign(interpreter, list, 20 ,(void*) &f, enum_type_FLOATVAL);
    f = *(FLOATVAL*) list_get(interpreter, list, 20, enum_type_FLOATVAL);
    printf("num " FLOATVAL_FMT "\n",  f);

    s = string_make(interpreter, "Seems ok\n", 9, 0, 0,0);

    list = list_new(interpreter, enum_type_PMC);

    p1 = pmc_new(interpreter, enum_class_PerlString);
    VTABLE_set_string_native(interpreter, p1, s);

    list_push(interpreter, list, p1, enum_type_PMC);

    p2 = *(PMC**)list_shift(interpreter, list, enum_type_PMC);
    s = VTABLE_get_string(interpreter, p2);
    printf("string %s", string_to_cstring(interpreter, s));

    i = list_length(interpreter, list);
    printf("len now %d\n", i);

    s = string_make(interpreter, "list\n", 5, 0, 0,0);
    p1 = pmc_new(interpreter, enum_class_PerlString);
    VTABLE_set_string_native(interpreter, p1, s);
    list_assign(interpreter, list, 0, p1, enum_type_PMC);

    /* clone */
    list2 = list_clone(interpreter, list);

    s = string_make(interpreter, "list 2\n", 7, 0, 0,0);
    p1 = pmc_new(interpreter, enum_class_PerlString);
    VTABLE_set_string_native(interpreter, p1, s);
    list_assign(interpreter, list2, 0, p1, enum_type_PMC);

    p1 = *(PMC**) list_shift(interpreter, list, enum_type_PMC);
    s = VTABLE_get_string(interpreter, p1);
    printf("string %s", string_to_cstring(interpreter, s));

    p2 = *(PMC**) list_shift(interpreter, list2, enum_type_PMC);
    s = VTABLE_get_string(interpreter, p2);
    printf("string %s", string_to_cstring(interpreter, s));

    list = list_new(interpreter, enum_type_STRING);
    s = string_make(interpreter, "list\n", 5, 0, 0,0);
    list_assign(interpreter, list, 0, s, enum_type_STRING);

    list2 = list_clone(interpreter, list);

    s = string_make(interpreter, "list 2\n", 7, 0, 0,0);
    list_assign(interpreter, list2, 0, s, enum_type_STRING);
    s = *(STRING**) list_shift(interpreter, list, enum_type_STRING);
    printf("string %s", string_to_cstring(interpreter, s));
    s = *(STRING**) list_shift(interpreter, list2, enum_type_STRING);
    printf("string %s", string_to_cstring(interpreter, s));

    /* delete test */
    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 20; i++) {
	int c = 'a' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    list_delete(interpreter, list, 1, 1);
    list_delete(interpreter, list, 1, 4);
    j = *(char*) list_get(interpreter, list, 1, enum_type_char);
    printf("delete 1 %s\n", (j == 6+'a') ? "ok" : "nok");

    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 26; i++) {
	int c = 'a' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    list_delete(interpreter, list, 1, 20);
    j = *(char*) list_get(interpreter, list, 1, enum_type_char);
    printf("delete 2 %s\n", (j == 21+'a') ? "ok" : "nok");

    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 26; i++) {
	int c = 'a' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    list_delete(interpreter, list, 2, 30);
    for (i = 0; i < 26; i++) {
	int c = 'A' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    j = *(char*) list_get(interpreter, list, 6, enum_type_char);
    printf("delete 3 %s\n", (j == 'E' && list_length(interpreter, list) == 28)
	? "ok" : "nok");

    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 5; i++) {
	int c = 'a' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    list_assign(interpreter, list, 2000 ,(void*)'Y', enum_type_char);
    list_assign(interpreter, list, 4000 ,(void*)'Z', enum_type_char);
    list_delete(interpreter, list, 5, 3000);
    list_delete(interpreter, list, 2, 996);
    j = *(char*) list_get(interpreter, list, -1, enum_type_char);
    printf("delete 4 %s\n", (j == 'Z' && list_length(interpreter, list) == 5)
	? "ok" : "nok");
#if 0
    /* insert */
    printf("\nnew list(0..5), shift\n");
    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 6; i++)
	list_push(interpreter, list, (void*) 'a'+i, enum_type_char);
    list_shift(interpreter, list, enum_type_char);
    list_dump(0, list, 0);
    printf("insert(2, 5)\n");
    list_insert(interpreter, list, 2, 5);
    list_dump(0, list, 0);
    printf("insert(1, 1)\n");
    list_insert(interpreter, list, 1, 1);
    list_dump(0, list, 0);
    printf("list(1..) = 'A'..'F'\n");
    for (i = 0; i < 6; i++)
        list_assign(interpreter, list, 1+i ,(void*)'A'+i, enum_type_char);
    list_dump(0, list, 0);
    printf("4 x pop\n");
    for (i = 0; i < 4; i++)
        list_pop(interpreter, list, enum_type_char);
    list_dump(0, list, 0);
#endif

    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 6; i++) {
	int c = 'a' + i;
	list_push(interpreter, list, (void*) c, enum_type_char);
    }
    list_shift(interpreter, list, enum_type_char);
    list_insert(interpreter, list, 0, 3);
    for (i = 0; i < 5; i++) {
	int c = 'A' + i;
	list_assign(interpreter, list, i, (void*) c, enum_type_char);
    }
    j = *(char*) list_get(interpreter, list, 0, enum_type_char);
    printf("insert 2 %s\n", (j == 'A') ? "ok" : "nok");

#if 0
    printf("\nnew list(0..5)\n");
    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 6; i++)
	list_push(interpreter, list, (void*) 'a'+i, enum_type_char);
    list_dump(0, list, 0);
    printf("insert(5, 3)\n");
    list_insert(interpreter, list, 5, 3);
    list_dump(0, list, 0);
    printf("list(5..9) = 'A'..'E'\n");
    for (i = 0; i < 5; i++)
        list_assign(interpreter, list, 5+i ,(void*)'A'+i, enum_type_char);
    list_dump(0, list, 0);

    printf("\nnew list(0..5)\n");
    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 6; i++)
	list_push(interpreter, list, (void*) 'a'+i, enum_type_char);
    list_dump(0, list, 0);
    printf("insert(6, 3)\n");
    list_insert(interpreter, list, 6, 3);
    list_dump(0, list, 0);
    printf("list(5..9) = 'A'..'E'\n");
    for (i = 0; i < 5; i++)
        list_assign(interpreter, list, 5+i ,(void*)'A'+i, enum_type_char);
    list_dump(0, list, 0);
    printf("delete(0,1)\n");
    list_delete(interpreter, list, 0, 1);
    list_dump(0, list, 0);

    printf("\nnew list(0..5)\n");
    list = list_new(interpreter, enum_type_char);
    printf("insert(0, 3)\n");
    list_insert(interpreter, list, 0, 3);
    list_dump(0, list, 0);

    printf("\nnew list(0..25)\n");
    list = list_new(interpreter, enum_type_char);
    for (i = 0; i < 26; i++)
	list_push(interpreter, list, (void*) 'a'+i, enum_type_char);
    printf("set_length(4)\n");
    list_set_length(interpreter, list, 4);
    list_dump(0, list, 0);
    printf("set_length(-1)\n");
    list_set_length(interpreter, list, -1);
    list_dump(0, list, 0);
#endif
    printf("Done.\n");
    return 0;
}

CODE
The answer is 42.
char b
short -15
num 1234.560000
string Seems ok
len now 0
string list
string list 2
string list
string list 2
delete 1 ok
delete 2 ok
delete 3 ok
delete 4 ok
insert 2 ok
Done.
OUTPUT

c_output_is(<<'CODE', <<'OUTPUT', "arbitrary sized");

#include <stdio.h>
#include "parrot/parrot.h"
#include "parrot/embed.h"

int main(int argc, char* argv[]) {
    int x;
    List* list;
    PMC *p;
    INTVAL key;
    char buf[100];
    int i, j, ok;

    Interp* interpreter = Parrot_new();
    if (interpreter == NULL) return 1;
    Parrot_init(interpreter, (void*) &x);

    p = pmc_new(interpreter, enum_class_Array);

    VTABLE_set_integer_native(interpreter, p, 6);
    /* set data type */
    key = 0;
    VTABLE_set_integer_keyed_int(interpreter, p, key, 2);
    key++;
    VTABLE_set_integer_keyed_int(interpreter, p, key, enum_type_sized);
    /* set item_size to 100 */
    key++;
    VTABLE_set_integer_keyed_int(interpreter, p, key, 3);
    key++;
    VTABLE_set_integer_keyed_int(interpreter, p, key, 100);
    /* set item_per_chunk to 3 */
    key++;
    VTABLE_set_integer_keyed_int(interpreter, p, key, 4);
    key++;
    VTABLE_set_integer_keyed_int(interpreter, p, key, 3);

    list = list_new_init(interpreter, enum_type_sized, p);
    printf("ok 1\n");

    for (j = 0; j < 5; j++) {
	for (i = 0; i < 100; i++)
	    buf[i] = j;
	list_assign(interpreter, list, j, buf, enum_type_sized);
    }
    for (j = 4; j >= 0; j--) {
	char *p = (char *) list_get(interpreter, list, j, enum_type_sized);
	ok = 1;
	for (i = 0; i < 100; i++)
	    if (p[i] != j) {
		ok = 0;
		break;
	    }
    }
    if (ok)
	printf("ok 2\n");
    else
	printf("failed 2: i=%d j=%d\n", i, j);
    Parrot_exit(0);
    return 0;
}
CODE
ok 1
ok 2
OUTPUT
# TODO
1;
