# $Id$

use strict;
use lib '../../lib';

use Parrot::Test tests => 6;

sub test {
    language_output_is('python', $_[0], '', $_[1]);
}

test(<<'CODE', 'rgen seed');
class Random:

    def __init__(self, x, y, z):
        self._seed = (x, y, z)

    def random(self):
        x, y, z = self._seed
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        self._seed = x, y, z
        return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0

    def randint(self, a, b):
        return a + int((b+1-a) * self.random())

rgen = Random(57, 86, 708 % 650)

def main():
    print rgen._seed

main()
CODE

test(<<'CODE', 'randint');
class Random:

    def __init__(self, x, y, z):
        self._seed = (x, y, z)

    def random(self):
        x, y, z = self._seed
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        self._seed = x, y, z
        return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0

    def randint(self, a, b):
        return a + int((b+1-a) * self.random())

rgen = Random(57, 86, 708 % 650)

def main():
    print rgen.randint(0, 0x7ffffffe)

main()
CODE

test(<<'CODE', 'randint data');
class Random:

    def __init__(self, x, y, z):
        self._seed = (x, y, z)

    def random(self):
        x, y, z = self._seed
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        self._seed = x, y, z
        return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0

    def randint(self, a, b):
        return a + int((b+1-a) * self.random())

rgen = Random(57, 86, 708 % 650)

N = 8

def main():
    data = [int(rgen.randint(0, 0x7ffffffe)) for x in xrange(N)]
    print data

main()
CODE

test(<<'CODE', 'TT class');
T = int

class TT(T):
    def __repr__(self):
        return "T(%d)" % self

def main():
    i = TT(5)
    print i, `i`

main()

CODE

test(<<'CODE', 'Int __new__');
T = int

class TT(T):
    def __repr__(self):
        return "T(%d)" % self

class Int(TT):
    def __new__(cls, value=None):
        if value is None:
            value = 42
        return TT.__new__(cls, value)

def main():
    i = Int(5)
    print i, `i`
    i = Int()
    print i, `i`

main()

CODE

test(<<'CODE', 'data slice');
def main():
    a = []
    N = 20000
    K = 1
    for i in range(N):
	a.append(i)
    for i in range(N):
	if a[i] != i:
	    print a[i]
    print "A", a[:K], a[N//2:N//2+K], a[-K:]

main()

CODE
