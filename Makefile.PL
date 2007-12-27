use ExtUtils::MakeMaker qw(WriteMakefile prompt);
use Config;

eval <<'EOS' if ($ENV{USER} eq 'seano');
use ExtUtils::Constant qw(WriteConstants);
WriteConstants(
    NAME => 'Algorithm::OpenFST',
    NAMES => [qw(INPUT OUTPUT INITIAL FINAL STAR PLUS SMRLog SMRTropical
                 ENCODE_LABEL ENCODE_WEIGHT ACCEPTOR NOT_ACCEPTOR)],
);
EOS

$FST = prompt("OpenFST source directory: ",
              "$ENV{HOME}/research/external/openfst"
              # 'FAIL'
);
die 'Sorry, you need OpenFST on your machine.' unless -d $FST && -d "$FST/fst";

## Avoid teh suck of building fat binaries:
if ($^O eq 'darwin') {
    ($LDDLFLAGS = $Config{lddlflags}) =~ s/-arch (\S+)//g;
    ($LDFLAGS = $Config{ldflags}) =~ s/-arch (\S+)//g;
}

WriteMakefile(
    NAME              => 'Algorithm::OpenFST',
    VERSION_FROM      => 'lib/Algorithm/OpenFST.pm',
    AUTHOR            => q|Sean O'Rourke <seano@cpan.org>|,
    ABSTRACT          => 'Perl interface to OpenFST.',
    LIBS              => "-L$FST/fst/bin -L$FST/fst/lib -lfst -lfstmain -lstdc++",
    dynamic_lib       => { OTHERLDFLAGS => "-L$FST/fst/bin -L$FST/fst/lib -lfst -lfstmain -lstdc++", },
    INC               => "-I$FST",
    XSOPT             => '-C++',
    C                 => [qw(openfst-impl.cc)],
    OBJECT            => 'openfst-impl.o OpenFST.o',
    CCFLAGS           => '-fno-strict-aliasing',
    CC                => 'g++',
    LDDLFLAGS         => $LDDLFLAGS,
    LDFLAGS           => $LDFLAGS,
    LD                => 'g++',
    ## OpenFST is stupidly written as a big pile of headers (does
    ## anyone know how to write modular C++ template libraries
    ## anymore?!), so optmization sucks ram like no one's business.
    OPTIMIZE          => '-O -g',
);
