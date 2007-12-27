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
$VERSION = '0.01_02'
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
@EXPORT_OK = (qw(transducer acceptor compose union), @CONST);
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
    my ($init, $final) = splice @_, 0, 2;
    ## Add edges
    $ret->add_arc(@$_) for @_;
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
    $ret = $ret->Compose($_) for @_;
    $ret
}

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
for (qw(Union Concat Closure Project RmEpsilon Prune Push Encode Decode)) {
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

=head2 C<$fst-E<gt>add_arc($from, $to [, $in [, $out [, $wt]]])

Add an arc from $from to $to with input and output $in and $out, with
weight $wt.

=cut

sub in
{
    shift->Project(Algorithm::OpenFST::INPUT);
}

sub out
{
    shift->Project(Algorithm::OpenFST::OUTPUT);
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

sub add_arc
{
    my $t = shift;
    $t->ensure_state($_[0] > $_[1] ? $_[0] : $_[1]);
    # my $acc = $t->Properties & Algorithm::OpenFST::ACCEPTOR;
    if (@_ == 2) {
        $t->AddArc(@_, 0, 0, 0);
    } else {
        if (@_ == 3) {
            $t->AddArc(@_[0,1], 0, $_[2], 0);
        } elsif (@_ == 4) {
            $t->AddArc(@_[0,1], 0, @_[2,3]);
        } elsif (@_ == 5) {
            $t->AddArc(@_[0,1,4,2,3]);
        } else {
            warn "add_arc: Bad arc (@_)\n";
        }
    }
}

=head2 C<$ofst = $fst->best_paths($n [, $unique])>

Compute the best $n paths through $fst.  Compute unique paths if
$unique is true (UNIMPLEMENTED).  If $fst does not use the tropical
semiring, it is directly converted to and from the tropical semiring.

=head2 C<$ofst = $fst->prune($w)

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
