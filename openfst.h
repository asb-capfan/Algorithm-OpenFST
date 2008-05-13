#ifndef _OPENFST_H
#define _OPENFST_H

#include <string>
#include <vector>
#include <exception>

extern "C" {
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
}
#undef Copy
#undef Zero
#undef Move
using namespace std;

#define SMRLog 1
#define SMRTropical 2
#define SMRReal 3
// XXX: sync w/ project.h
#define INPUT 1
#define OUTPUT 2
// XXX: sync w/ reweight.h
#define INITIAL 0
#define FINAL 1
// XXX: sync w/ rational.h
#define STAR 0
#define PLUS 1
// XXX: sync w/ encode.h
#define ENCODE_LABEL 1
#define ENCODE_WEIGHT 2
// XXX: sync w/ properties.h
#define ACCEPTOR 0x0000000000010000ULL
#define NOT_ACCEPTOR 0x0000000000020000ULL

namespace fst {
    class SymbolTable;
};

using fst::SymbolTable;

/// Base class for Perl FSTs
struct FST
{
    virtual SymbolTable * InputSymbols() const = 0;
    virtual SymbolTable * OutputSymbols() const = 0;
    virtual void SetInputSymbols(const SymbolTable *) = 0;
    virtual void SetOutputSymbols(const SymbolTable *) = 0;
    virtual FST * Copy() const = 0;
    virtual FST * change_semiring(int ) const = 0;
    virtual int semiring() const = 0;
    virtual ~FST() { }
    // Combination
    // XXX: why are only some destructive?
    virtual FST * Compose(FST * ) const = 0;
    virtual FST * Intersect(FST * ) const = 0;
    virtual FST * Difference(FST * ) const = 0;
    // Destructive versions:
    virtual void _Union(const FST * ) = 0;
    virtual void _Concat(const FST * ) = 0;

    // Transformation
    virtual void _Closure(int ) = 0;
    virtual void _Project(int ) = 0;
    virtual void _Encode(int ) = 0;
    virtual void _Decode(int ) = 0;
    virtual FST * Reverse() const = 0;
    virtual void _Invert() = 0;

    // Cleanup
    // XXX: why only some destructive?
    virtual FST * Determinize(float) const = 0;
    virtual void _Prune(float) = 0;
    virtual void _RmEpsilon(float) = 0;
    virtual FST * EpsNormalize(int ) const = 0;
    virtual void _Minimize() = 0;
    virtual FST * Minimize() const = 0;
    virtual void _Push(int) = 0;

    // Construction
    virtual void AddState() = 0;
    virtual void SetStart(int) = 0;
    virtual int Start() const = 0;
    virtual void SetFinal(int, float = 0) = 0;
    virtual float Final(int) const = 0;
    virtual void AddArc(int, int, float, int, int) = 0;

    virtual int add_input_symbol(const char * s) = 0;
    virtual int add_output_symbol(const char * s) = 0;
    virtual void add_arc(int, int, float, const char *, const char *) = 0;

    virtual void WriteBinary(const char * ) const = 0;
    virtual void WriteText(const char *) const = 0;
    virtual string _String() const = 0;
    virtual string _Draw() const = 0;
    virtual void strings(vector<string>& ) const = 0;
    SV* String() const;
    SV* Draw() const;
    virtual int NumStates() const = 0;
    virtual int NumArcs(unsigned) const = 0;
    virtual unsigned Properties(bool ) const = 0;

    // "Other" algorithms
    virtual FST * ShortestPath(unsigned , int ) const = 0;
    virtual void normalize() = 0;
    virtual FST * markovize(int ) const = 0;
};

template <typename Arc>
struct FSTBase : public FST
{
    virtual const fst::Fst<Arc> * get_fst() const = 0;

    virtual SymbolTable * InputSymbols() const
        { return (SymbolTable *)get_fst()->InputSymbols(); }
    virtual SymbolTable * OutputSymbols() const
        { return (SymbolTable *)get_fst()->OutputSymbols(); }
    virtual float Final(int i) const
        { return get_fst()->Final(i).Value(); }
};

FST *
ReadBinary(const char *, int);

FST *
ReadText(const char *, int, bool, const char * = NULL, const char * = NULL,
         const char * = NULL);

inline FST *
Acceptor(const char * f, int smr, const char * sy = NULL,
         const char * ssy = NULL)
{
    return ReadText(f, smr, true, sy, sy, ssy);
}

inline FST *
Transducer(const char * f, int smr,
           const char * is = NULL,
           const char * os = NULL,
           const char * ss = NULL)
{
    return ReadText(f, smr, false, is, os, ss);
}

FST *
VectorFST(int );

#endif // _OPENFST_H
