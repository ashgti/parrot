# $Id$

use strict;
use lib '../../lib';

use Parrot::Test tests => 40;

sub test {
    language_output_is('python', $_[0], '', $_[1]);
}

test(<<'CODE', 'func()');
def foo():
    print 0.1, 0.02
    print 1.1, 1.02

if __name__ == "__main__":
    foo()
CODE

test(<<'CODE', 'func(a, b)');
def foo(a,b):
    c = 3
    print a,b,c

if __name__ == "__main__":
    a = 1
    b = 2
    foo(a, b)
    print foo(1, 2)
CODE

test(<<'CODE', 'func(a, b=x, c=y)');
def foo(a,b="x",c=3+4):
    print a,b,c

if __name__ == "__main__":
    a = 1
    b = 2
    foo(a)
    foo(a, b)
    foo(a, b, 2)
CODE

test(<<'CODE', 'abs() builtin opcode');
def check(a,b):
    print a,b

def check_functions(i=0, j=0):
    check(abs(42*i), 42*j)

if __name__ == '__main__':
    check_functions()
    check_functions(-1, 2)
CODE

test(<<'CODE', 'callable() builtin function');
if __name__ == '__main__':
    o = callable
    i = callable(o)
    print i
    a = 5
    i = callable(a)
    print i
CODE

test(<<'CODE', 'chr() builtin function');
if __name__ == '__main__':
    print chr(65), chr(97)
CODE

test(<<'CODE', 'hash() builtin function');
if __name__ == '__main__':
    print hash(65), hash(97L)
CODE

test(<<'CODE', 'hash() builtin function, num, complex');
if __name__ == '__main__':
    print hash(42.0), hash(42+0j)
CODE

test(<<'CODE', 'range 1');
if __name__ == '__main__':
    for i in range(10):
        print i,
    print
CODE

test(<<'CODE', 'range 2');
if __name__ == '__main__':
    for i in range(2,5):
        print i,
    print
CODE

test(<<'CODE', 'range 3');
if __name__ == '__main__':
    for i in range(2,10,3):
        print i,
    print
CODE

test(<<'CODE', 'range 3 negative step');

def main():
    for i in range(10, 0, -2):
        print i

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'range 3 0 step');
def main():
    try:
        for i in range(1,2,0):
            print i
    except ValueError,e:
        print e
if __name__ == '__main__':
    main()

CODE

test(<<'CODE', 'tuple 1');
def main():
    print tuple("abcd")[1]

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'tuple iter');
def main():
    s = "abcd"
    i = iter(s)
    print tuple(i)[2]

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'filter no func');

def main():
    for i in filter(None, range(3)):
        print i
if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'filter no func ar');
def main():
    for i in filter(None, (0,1,2,0,3,0,4,0)):
        print i
if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'filter func');
def f(x):
    return x < 5

def main():
    for i in filter(f, range(10)):
        print i

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'map None');
def main():
    for i in map(None, range(3)):
        print i

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'map func');
def f(x):
    return x+2

def main():
    for i in map(f, range(3)):
        print i

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'reduce');
def f(x,y):
    return x+y

def main():
    print reduce(f, "abc")

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'enumerate');
if __name__ == '__main__':
    for i,j in enumerate("abc"):
        print i,j
CODE

test(<<'CODE', 'enumerate - break');
if __name__ == '__main__':
    it = iter("abcdef")
    for i, c in enumerate(it):
        print i, c
        if i == 2:
            break
    print "Ok"
CODE

test(<<'CODE', 'dict(), dict({h})');
if __name__ == '__main__':
    h = dict()
    print "ok"
    # cant have more key w/o sort
    it = iter( dict( { "a": 1 }))
    for i, c in enumerate(it):
        print i, c
    print "ok"
CODE

test(<<'CODE', 'dict(seq)');
if __name__ == '__main__':
    it = iter( dict( [[ "a", 1] , ["a" , 2 ]]) )
    for i, c in enumerate(it):
        print i, c
CODE

test(<<'CODE', 'list()');
if __name__ == '__main__':
    for i in list("abc"):
        print i
CODE

test(<<'CODE', 'max(x,...)');
if __name__ == '__main__':
    print max(1,2)
    print max(1,2,3,4,5)
    print max(list("bcaBCA"))
CODE

test(<<'CODE', 'max(seq)');
if __name__ == '__main__':
    print max([1,2])
    print max([1,2,3,4,5])
    print max("bcaBCA")
CODE

test(<<'CODE', 'min(x,...)');
if __name__ == '__main__':
    print min(1,2)
    print min(1,2,3,4,5)
    print min(list("bcaBCA"))
CODE

test(<<'CODE', 'min(seq)');
if __name__ == '__main__':
    print min([1,2])
    print min([1,2,3,4,5])
    print min("bcaBCA")
CODE

test(<<'CODE', 'named arguments 1');
def f(i=0, j=1):
    print i, j

if __name__ == '__main__':
    f(j=4, i=20)
    f(i=4, j=20)
    f()
    #f(j=4)
    #f(17, j=4)
CODE

test(<<'CODE', 'lambda(x)');
if __name__ == '__main__':
    print filter(lambda x: x < 5, range(10)) == range(5)
CODE

test(<<'CODE', 'hex()');
if __name__ == '__main__':
    print hex(42)
CODE

test(<<'CODE', 'f(*a)');
def p(x):
    print x

def f(f, *a):
    f(a)

def main():
    f(p,2)

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'f(*a), CALL_FUNCTION_VAR');
def p(x):
    print x

def f(f, *a):
    f(*a)

def main():
    f(p,2)

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'ord()');
if __name__ == '__main__':
   print ord('A')
CODE

test(<<'CODE', 'xrange(start)');
if __name__ == '__main__':
    for i in xrange(5):
        print i
CODE

test(<<'CODE', 'xrange(start, stop)');
if __name__ == '__main__':
    for i in xrange(2,5):
        print i
CODE

test(<<'CODE', 'xrange(star, stop, step)');
if __name__ == '__main__':
    for i in xrange(2,10,3):
        print i
CODE

test(<<'CODE', 'xrange(star, stop, -step)');
for i in xrange(10,5,-1):
    print i
CODE

