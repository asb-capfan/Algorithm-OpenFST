use Test::Simple tests => 4;
use Algorithm::OpenFST;
ok(1, 'loaded');

my $fst = Algorithm::OpenFST::Transducer('g2hd.txt');
ok("$fst" eq <<EOS);
0	0	1	1
0	0	1	5
0	0	1	6
0	0	2	2
0	0	2	7
0	0	2	8
0	0	3	3
0	0	3	4
0	0	4	1
0	0	4	2
0	0	4	3
0	0	4	4
0	0	4	5
0	0	4	6
0	0	4	7
0	0	4	8
0
EOS
my $fsa = Algorithm::OpenFST::Acceptor('h0.txt');
ok("$fsa" eq <<EOS);
0	1	2	2
1	2	1	1
2	3	1	1
3	4	2	2
4	5	1	1
5	6	2	2
6	7	2	2
7	8	1	1
8	9	2	2
9	10	1	1
10
EOS

ok($fsa->Compose($fst).'' eq <<EOS);
0	1	2	2
0	1	2	7
0	1	2	8
1	2	1	1
1	2	1	5
1	2	1	6
2	3	1	1
2	3	1	5
2	3	1	6
3	4	2	2
3	4	2	7
3	4	2	8
4	5	1	1
4	5	1	5
4	5	1	6
5	6	2	2
5	6	2	7
5	6	2	8
6	7	2	2
6	7	2	7
6	7	2	8
7	8	1	1
7	8	1	5
7	8	1	6
8	9	2	2
8	9	2	7
8	9	2	8
9	10	1	1
9	10	1	5
9	10	1	6
10
EOS
