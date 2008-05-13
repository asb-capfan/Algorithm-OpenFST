#include "fst/lib/symbol-table.h"
extern "C" {
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
}

#undef Copy
#undef Zero
#undef Move
#include "openfst-pre.h"
#include "openfst.h"
#include "const-c.inc"

MODULE = Algorithm::OpenFST	PACKAGE = Algorithm::OpenFST
PROTOTYPES: DISABLE

INCLUDE: const-xs.inc

FST *
VectorFST(smr)
	int	smr

FST *
ReadBinary(file, type)
	char *	file
	int	type

FST *
ReadText(file, smr, acceptor, isyms, osyms, ssyms)
	const char * file
	int smr
	int acceptor
        const char * isyms
        const char * osyms
	const char * ssyms

FST *
Acceptor(f, smr=1, sy=0, ssy=0)
	const char * f
	int smr
	const char * sy
	const char * ssy

FST *
Transducer(f, smr=1, is=0, os=0, ss=0)
	const char * f
	int smr
	const char * is
	const char * os
	const char * ss

MODULE = Algorithm::OpenFST	PACKAGE = Algorithm::OpenFST::FST
PROTOTYPES: DISABLE

void
FST::DESTROY()

FST *
FST::Copy()

FST *
FST::Compose(fst)
	FST *	fst

FST *
FST::Intersect(fst)
	FST *	fst

FST *
FST::Difference(fst)
	FST *	fst

void
FST::_Union(fst)
	FST *	fst

void
FST::_Concat(fst)
	FST *	fst

void
FST::_Closure(n)
	int	n

void
FST::_Project(n)
	int	n

FST *
FST::Reverse()

void
FST::_Invert()

FST *
FST::Determinize(del = 1.024e-3)
	float	del

void
FST::_RmEpsilon(del = 1.024e-3)
	float	del

void
FST::_Minimize()

FST *
FST::Minimize()

void
FST::_Prune(w)
	float	w

void
FST::_Push(i)
	int	i

void
FST::AddState()

void
FST::SetStart(n)
	int	n

int
FST::Start()

void
FST::SetFinal(n, w = 0)
	int	n
	float	w

float
FST::Final(n)
	int	n

void
FST::AddArc(a, b, w, i, o)
	int	a
	int	b
	float	w
	int	i
	int	o

void
FST::WriteBinary(file)
	 char *	file

void
FST::WriteText(file)
	 char *	file

SV *
FST::String()

SV *
FST::Draw()

int
FST::NumStates()

int
FST::NumArcs(n)
	unsigned	n

unsigned
FST::Properties(compute = 0)
	int	compute

FST *
FST::ShortestPath(n = 1, uniq = 0)
	unsigned	n
	int	uniq

void
FST::normalize()

FST *
FST::markovize(n)
	int	n

FST *
FST::change_semiring(smr)
	int	smr

int
FST::semiring()

void
FST::strings()
    PREINIT:
      vector<string> tmp;
    PPCODE:
      THIS->strings(tmp);
      EXTEND(SP, tmp.size());
      for (int i = 0; i < tmp.size(); i++)
          PUSHs(sv_2mortal(newSVpvn(tmp[i].c_str(), tmp[i].size())));
      XSRETURN(tmp.size());

int
FST::add_input_symbol(s)
	char *	s

int
FST::add_output_symbol(s)
	char *	s

void
FST::add_arc(a, b, w, i, o)
	int	a
	int	b
	float	w
	char *	i
	char *	o

SymbolTable *
FST::InputSymbols()

SymbolTable *
FST::OutputSymbols()

void
FST::SetInputSymbols(s)
	SymbolTable *	s

void
FST::SetOutputSymbols(s)
	SymbolTable *	s

void
FST::out_syms()
    PPCODE:
	if (THIS->OutputSymbols()) {
            fst::SymbolTableIterator i(*THIS->OutputSymbols());
            while (!i.Done()) {
                XPUSHs(sv_2mortal(newSVpv(i.Symbol(), 0)));
                i.Next();
            }
        }

void
FST::in_syms()
    PPCODE:
	if (THIS->InputSymbols()) {
            fst::SymbolTableIterator i(*THIS->InputSymbols());
            while (!i.Done()) {
                XPUSHs(sv_2mortal(newSVpv(i.Symbol(), 0)));
                i.Next();
            }
        }

FST *
FST::EpsNormalize(dir)
	int	dir
