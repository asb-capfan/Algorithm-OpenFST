#define PERL_constant_NOTFOUND	1
#define PERL_constant_NOTDEF	2
#define PERL_constant_ISIV	3
#define PERL_constant_ISNO	4
#define PERL_constant_ISNV	5
#define PERL_constant_ISPV	6
#define PERL_constant_ISPVN	7
#define PERL_constant_ISSV	8
#define PERL_constant_ISUNDEF	9
#define PERL_constant_ISUV	10
#define PERL_constant_ISYES	11

#ifndef NVTYPE
typedef double NV; /* 5.6 and later define NVTYPE, and typedef NV to it.  */
#endif
#ifndef aTHX_
#define aTHX_ /* 5.6 or later define this for threading support.  */
#endif
#ifndef pTHX_
#define pTHX_ /* 5.6 or later define this for threading support.  */
#endif

static int
constant (pTHX_ const char *name, STRLEN len, IV *iv_return) {
  /* Initially switch on the length of the name.  */
  /* When generated this function returned values for the list of names given
     in this section of perl code.  Rather than manually editing these functions
     to add or remove constants, which would result in this comment and section
     of code becoming inaccurate, we recommend that you edit this section of
     code, and use it to regenerate a new set of constant functions which you
     then use to replace the originals.

     Regenerate these constant functions by feeding this entire source file to
     perl -x

#!perl -w
use ExtUtils::Constant qw (constant_types C_constant XS_constant);

my $types = {map {($_, 1)} qw(IV)};
my @names = (qw(ACCEPTOR ENCODE_LABEL ENCODE_WEIGHT FINAL INITIAL INPUT
	       NOT_ACCEPTOR OUTPUT PLUS SMRLog SMRTropical STAR));

print constant_types(); # macro defs
foreach (C_constant ("Algorithm::OpenFST", 'constant', 'IV', $types, undef, 3, @names) ) {
    print $_, "\n"; # C constant subs
}
print "#### XS Section:\n";
print XS_constant ("Algorithm::OpenFST", $types);
__END__
   */

  switch (len) {
  case 4:
    /* Names all of length 4.  */
    /* PLUS STAR */
    /* Offset 3 gives the best switch position.  */
    switch (name[3]) {
    case 'R':
      if (memEQ(name, "STA", 3)) {
      /*                  R     */
#ifdef STAR
        *iv_return = STAR;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    case 'S':
      if (memEQ(name, "PLU", 3)) {
      /*                  S     */
#ifdef PLUS
        *iv_return = PLUS;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    }
    break;
  case 5:
    /* Names all of length 5.  */
    /* FINAL INPUT */
    /* Offset 2 gives the best switch position.  */
    switch (name[2]) {
    case 'N':
      if (memEQ(name, "FINAL", 5)) {
      /*                 ^        */
#ifdef FINAL
        *iv_return = FINAL;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    case 'P':
      if (memEQ(name, "INPUT", 5)) {
      /*                 ^        */
#ifdef INPUT
        *iv_return = INPUT;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    }
    break;
  case 6:
    /* Names all of length 6.  */
    /* OUTPUT SMRLog */
    /* Offset 2 gives the best switch position.  */
    switch (name[2]) {
    case 'R':
      if (memEQ(name, "SMRLog", 6)) {
      /*                 ^         */
#ifdef SMRLog
        *iv_return = SMRLog;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    case 'T':
      if (memEQ(name, "OUTPUT", 6)) {
      /*                 ^         */
#ifdef OUTPUT
        *iv_return = OUTPUT;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    }
    break;
  case 7:
    if (memEQ(name, "INITIAL", 7)) {
#ifdef INITIAL
      *iv_return = INITIAL;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 8:
    if (memEQ(name, "ACCEPTOR", 8)) {
#ifdef ACCEPTOR
      *iv_return = ACCEPTOR;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 11:
    if (memEQ(name, "SMRTropical", 11)) {
#ifdef SMRTropical
      *iv_return = SMRTropical;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  case 12:
    /* Names all of length 12.  */
    /* ENCODE_LABEL NOT_ACCEPTOR */
    /* Offset 1 gives the best switch position.  */
    switch (name[1]) {
    case 'N':
      if (memEQ(name, "ENCODE_LABEL", 12)) {
      /*                ^                 */
#ifdef ENCODE_LABEL
        *iv_return = ENCODE_LABEL;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    case 'O':
      if (memEQ(name, "NOT_ACCEPTOR", 12)) {
      /*                ^                 */
#ifdef NOT_ACCEPTOR
        *iv_return = NOT_ACCEPTOR;
        return PERL_constant_ISIV;
#else
        return PERL_constant_NOTDEF;
#endif
      }
      break;
    }
    break;
  case 13:
    if (memEQ(name, "ENCODE_WEIGHT", 13)) {
#ifdef ENCODE_WEIGHT
      *iv_return = ENCODE_WEIGHT;
      return PERL_constant_ISIV;
#else
      return PERL_constant_NOTDEF;
#endif
    }
    break;
  }
  return PERL_constant_NOTFOUND;
}

