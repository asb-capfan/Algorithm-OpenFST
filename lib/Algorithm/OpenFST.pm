package Algorithm::OpenFST;

=head1 NAME

OpenFST -- Perl bindings for the OpenFST library.

=head1 SYNOPSIS

This module provides a Perl interface to the OpenFST finite state
transducer library.  If you don't know what it is, you probably don't
need it.

=head1 DESCRIPTION

The interface is very incomplete, since I have only implemented the
parts I need right now.  At the lowest level, the function and method
names are the same as the C++ interface, except that some destructive
methods have a leading underscore.

C<Algorithm::OpenFST> provides a convenient higher-level interface to
OpenFST's basic operations.  Methods in this interface have lower_case
names, while those in the raw library interface use StudlyCaps.

=cut

BEGIN {
$VERSION = '0.01_03'
}
require Exporter;
use vars qw(@ISA @EXPORT_OK @EXPORT_TAGS);

BEGIN {
require XSLoader;
XSLoader::load('Algorithm::OpenFST', $VERSION);
my @CONST = qw(INPUT OUTPUT INITIAL FINAL STAR PLUS SMRLog SMRTropical
               ENCODE_LABEL ENCODE_WEIGHT ACCEPTOR NOT_ACCEPTOR);
eval "sub $_ () { ".Algorithm::OpenFST::constant($_)."}" for @CONST;

@ISA = qw(Exporter);
@EXPORT_OK = (qw(
acceptor
compose
concat
from_list
transducer
union
universal
), @CONST);
%EXPORT_TAGS = (
    all => \@EXPORT_OK,
    constants => \@CONST,
);
}

=head2 C<$fsa = acceptor $file, %opts>

Create a finite-state acceptor from file $file with options %opts.

=head2 C<$fst = transducer $file, %opts>

Create a finite-state transducer from file $file with options %opts.

=head2 C<$fst = from_list $init, $final, @edges>

Create a transducer with initial state $init, final state $final, and
edges @edges, where each edge is of the form [FROM, TO, IN, OUT, WEIGHT].

=cut

sub acceptor
{
    my $file = shift;
    my %o = (smr => SMRLog, @_);
    $o{is} ||= $o{os};
    return Algorithm::OpenFST::Acceptor($file, @o{qw(smr is ss)});
}

sub transducer
{
    my $file = shift;
    my %o = (smr => SMRLog, @_);
    return Algorithm::OpenFST::Transducer($file, @o{qw(smr is os ss)});
}

sub from_list
{
    my $ret = Algorithm::OpenFST::VectorFST(Algorithm::OpenFST::SMRLog);
    my ($init, $final, $syms) = splice @_, 0, 3;
    ## Add edges
    for (@$syms) {
        $ret->add_input_symbol($_);
        $ret->add_output_symbol($_);
    }
    $ret->add_arc_safe(@$_) for @_;
    ## initial
    $ret->SetStart($init);
    ## Final states
    if (!ref $final) {
        $ret->SetFinal($final, 0);
    } else {
        for (@$final) {
            if (ref $_) {
                $ret->SetFinal(@$_);
            } else {
                $ret->SetFinal($_, 0);
            }
        }
    }
    $ret;
}


=head2 C<$fst = universal $max>

Create a universal acceptor with output symbols 1..$max.

=cut

sub universal
{
    my $max = shift;
    my $fst = Algorithm::OpenFST::VectorFST SMRLog;
    $fst->AddState;
    $fst->SetStart(0);
    $fst->SetFinal(0, 0);
    if (ref $max eq 'ARRAY') {
        $fst->add_arc(0, 0, 0, $_, '-') for @$max;
    } elsif (ref $max eq 'Algorithm::OpenFST::FST') {
        $fst->SetInputSymbols($max->OutputSymbols);
        # $fst->SetOutputSymbols($max->OutputSymbols);
        $fst->add_arc(0, 0, 0, $_, '-') for $max->out_syms;
    } else {
        $fst->AddArc(0, 0, 0, $_, 0) for 1..$max;
    }
    $fst;
}

=head2 C<$fst = compose @fsts>

Compose transducers @fsts into a single transducer $fst.

=head2 C<$fst = concat @fsts>

Concatenate transducers @fsts into a single transducer $fst.

=head2 C<$fst = union @fsts>

Union transducers @fsts into a single transducer $fst.

=cut

sub compose
{
    my $ret = shift;
    unless (ref $ret eq 'Algorithm::OpenFST::FST') {
        die "aiee: ($ret) ain't no fst!\n";
    }
    for (@_) {
        unless (ref $_ eq 'Algorithm::OpenFST::FST') {
            die "aiee: (@_) contains a non-fst!\n";
        }
        $ret = $ret->Compose($_);
    }
    $ret
}# {
#     my $ret = shift;
#     $ret = $ret->Compose($_) for @_;
#     $ret
# }

sub concat
{
    my $ret = shift->Copy;
    $ret->_Concat($_) for @_;
    $ret;
}

sub union
{
    my $ret = shift->Copy;
    $ret->_Union($_) for @_;
    $ret;
}

## Supplementary methods
package Algorithm::OpenFST::FST;

use overload '""' => sub { shift->String };

## Non-destructive versions of destructive ops.
BEGIN {
for (qw(Union Concat Closure Project RmEpsilon Prune Push Encode Decode Invert)) {
    eval '
sub '.$_.' {
    my $ret = shift->Copy;
    $ret->_'.$_.'(@_);
    $ret
}';
    die $@ if $@;
}
}

=head2 C<$fst-E<gt>in>

=head2 C<$fst-E<gt>out>

=head2 C<$fst-E<gt>ensure_state($n)>

Ensure that states up to $n exist in $fst.

=head2 C<$fst-E<gt>add_state($n)>

Add state $n (and previous states, if necessary).

=head2 C<$fst-E<gt>add_arc($from, $to [, $in [, $out [, $wt]]])>

Add an arc from $from to $to with input and output $in and $out, with
weight $wt.

=cut

sub in
{
    my $ret = shift->Project(Algorithm::OpenFST::INPUT);
    $ret->SetOutputSymbols($ret->InputSymbols);
    $ret;
}

sub out
{
    my $ret = shift->Project(Algorithm::OpenFST::OUTPUT);
    $ret->SetInputSymbols($ret->OutputSymbols);
    $ret;
}

sub ensure_state
{
    my ($t, $st) = @_;
    if ($st >= $t->NumStates) {
        $t->AddState for 0..$st - $t->NumStates;
    }
}

sub add_state
{
    my $t = shift;
    $t->ensure_state($_[0]);
    $t->AddState($_[0]);
}

sub add_arc_safe
{
    my $t = shift;
    $t->ensure_state($_[0] > $_[1] ? $_[0] : $_[1]);
    if (@_ == 2) {
        $t->add_arc(@_, 0, '-', '-');
    } else {
        if (@_ == 3) {
            $t->add_arc(@_[0,1], 0, $_[2], '-');
        } elsif (@_ == 4) {
            $t->add_arc(@_[0,1], 0, @_[2,3]);
        } elsif (@_ == 5) {
            $t->add_arc(@_[0,1,4,2,3]);
        } else {
            warn "add_arc: Bad arc (@_)\n";
        }
    }
}

sub _syms
{
    my ($fst, $f) = @_;
    my %h;
    for my $x (split /\n/, "$fst") {
        my @f = split ' ', $x;
        next unless @f > 2;
        undef $h{$f[$f]};
    }
    sort { $a <=> $b } keys %h;
}

# sub in_syms
# {
#     shift->_syms(2);
# }

# sub out_syms
# {
#     shift->_syms(3);
# }

=head2 C<$ofst = $fst-E<gt>best_paths($n [, $unique])>

Compute the best $n paths through $fst.  Compute unique paths if
$unique is true (UNIMPLEMENTED).  If $fst does not use the tropical
semiring, it is directly converted to and from the tropical semiring.

=head2 C<$ofst = $fst-E<gt>prune($w)>

Prune $fst so paths worse than $w from the best path are removed.

=cut

sub best_paths
{
    my $fst = shift;
    my $smr = $fst->semiring;
    my $ret;
    if ($smr == Algorithm::OpenFST::SMRTropical) {
        $ret = $fst->ShortestPath(@_);
    } else {
        $ret = $fst->change_semiring(Algorithm::OpenFST::SMRTropical)
            ->ShortestPath(@_)->change_semiring($smr);
    }
    $ret->normalize;
    $ret;
}

sub prune
{
    my $fst = shift;
    my $smr = $fst->semiring;
    my $ret;
    if ($smr == Algorithm::OpenFST::SMRTropical) {
        $ret = $fst->_Prune(@_);
    } else {
        $ret = $fst->change_semiring(Algorithm::OpenFST::SMRTropical)
            ->Prune(@_)->change_semiring($smr);
    }
    $ret->normalize;
    $ret;
}

1;
__END__

=head1 SEE ALSO

OpenFST -- http://www.openfst.org/

=head1 AUTHOR

Sean O'Rourke E<lt>seano@cpan.orgE<gt>

Bug reports welcome, patches even more welcome.

=head1 COPYRIGHT

Copyright (C) 2007, Sean O'Rourke.  All rights reserved, some wrongs
reversed.  This module is distributed under the same terms as Perl
itself.

=cut
