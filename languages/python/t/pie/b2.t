# $Id$

use strict;
use lib '../../lib';

use Parrot::Test tests => 5;

sub test {
    language_output_is('python', $_[0], '', $_[1]);
}

test(<<'CODE', 'pi() func');
def pi():
    k, a, b, a1, b1 = 2, 4, 1, 12, 4
    n = 0
    while 1:
	p, q, k = k*k, 2*k+1, k+1
	a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
	d, d1 = a//b, a1//b1
	while d == d1:
	    n += 1
	    if n > 20:
		print
		return
	    print d,
	    a, a1 = 10*(a%b), 10*(a1%b1)
	    d, d1 = a//b, a1//b1

def main():
    pi()

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'pi() func 1000 digits');
def pi():
    k, a, b, a1, b1 = 2, 4, 1, 12, 4
    n = 0
    while 1:
	p, q, k = k*k, 2*k+1, k+1
	a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
	d, d1 = a//b, a1//b1
	while d == d1:
	    n += 1
	    if n > 1000:
		print
		return
	    print d,
	    a, a1 = 10*(a%b), 10*(a1%b1)
	    d, d1 = a//b, a1//b1

def main():
    pi()

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'pi() generator');
def pi():
    k, a, b, a1, b1 = 2, 4, 1, 12, 4
    n = 0
    while 1:
	p, q, k = k*k, 2*k+1, k+1
	a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
	d, d1 = a//b, a1//b1
	while d == d1:
	    n += 1
	    if n > 20:
		return	#raise StopIteration
	    yield d
	    a, a1 = 10*(a%b), 10*(a1%b1)
	    d, d1 = a//b, a1//b1

def main():
    i = pi()
    for y in i:
        print y,
    print

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'izip() generator');
def izip(*args):
    # (xrange, iterable) =>
    args = map(iter, args)
    # [range_iter, iter]
    while 1:
        yield tuple([x.next() for x in args])

def main():
    for x, y in izip(xrange(10), "abcdefghijklmnop"):
        print y,
    print

if __name__ == '__main__':
    main()
CODE

test(<<'CODE', 'izip() generator with pi() object');
def izip(*args):
    args = map(iter, args)
    while 1:
        yield tuple([x.next() for x in args])

class PI(object):
    def __iter__(self):
	i = 0
	while 1:
	    yield "314159265"[i]
	    i = i + 1

def main():
    pi = PI()
    for x, y in izip(xrange(6), pi):
        print y,
    print "ok"

if __name__ == '__main__':
    main()
CODE
