# $Id$

use strict;
use lib '../../lib';

use Parrot::Test tests => 4;

sub test {
    language_output_is('python', $_[0], '', $_[1]);
}

test(<<'CODE', 'b6 - mul array x');
# from b5 import check
show=True
def check(a, b):
    if __debug__:
        if show:
            print `a`, "==", `b`
    if not a == b:
        raise AssertionError("%.30r != %.30r" % (a, b))

def main():
    L = [1]*100
    L[-1] = 42
    n = 0
    for i in L:
        n += i
    check(i, 42)

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'b6 - iter');
# from b5 import check
show=True
def check(a, b):
    if __debug__:
        if show:
            print `a`, "==", `b`
    if not a == b:
        raise AssertionError("%.30r != %.30r" % (a, b))

def main():
    L = [1]*100000
    L[-1] = 42
    n = 0
    for i in L:
        n += i
    check(i, 42)
    check(n, 100041)

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'b6 - fdiv');
# from b5 import check
show = True
def check(a, b):
    if __debug__:
        if show:
            print `a`, "==", `b`
    if not a == b:
        raise AssertionError("%.30r != %.30r" % (a, b))
def x():
    n = 2000000000
    print `n`
    n += 1000000000
    print `n`

def main():
    n = 0
    for i in xrange(100000):
        n += i
    check(i, 99999)
    check(n, 99999*100000//2)

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'b6');
show=True
# from b5 import check
def check(a, b):
    if __debug__:
        if show:
            print `a`, "==", `b`
    if not a == b:
        raise AssertionError("%.30r != %.30r" % (a, b))

def main():
    L = [1]*100000
    L[-1] = 42
    n = 0
    for i in L:
        n += i
    check(i, 42)
    check(n, 100041)
    n = 0
    for i in xrange(100000):
        n += i
    check(i, 99999)
    check(n, 99999*100000//2)
    d = dict.fromkeys(xrange(100000))
    n = 0
    for i in d:
        n += i
    check(n, 99999*100000//2)

if __name__ == '__main__':
    main()

CODE
