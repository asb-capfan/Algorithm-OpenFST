// All right, Mr. Google monkey.  Calling exit(1) in a library a bad
// thing, m'kay?
// #define exit(n) (((n) == 0) ? exit(0) : (throw exception("error"),1))
#include "fst/lib/compat.h"
// #undef exit

#include "fst/lib/closure.h"
#include "fst/lib/compose.h"
#include "fst/lib/concat.h"
#include "fst/lib/determinize.h"
#include "fst/lib/encode.h"
#include "fst/lib/fst.h"
#include "fst/lib/intersect.h"
#include "fst/lib/minimize.h"
#include "fst/lib/mutable-fst.h"
#include "fst/lib/project.h"
#include "fst/lib/prune.h"
#include "fst/lib/push.h"
#include "fst/lib/rmepsilon.h"
#include "fst/lib/shortest-path.h"
#include "fst/lib/symbol-table.h"
#include "fst/lib/union.h"
#include "fst/lib/vector-fst.h"
#include <sstream>
#include <fstream>
using namespace std;
using namespace fst;

#include "openfst.h"
#include "openfst-io.h"

//////////////////////////////////////////////////////////////////////
// FST Impls
template <typename Arc>
struct FSTImpl : public FST
{
    typedef fst::MutableFst<Arc> Fst;

    Fst * fst;

    FSTImpl() : fst(NULL) { }
    ~FSTImpl()
        { delete fst; }

    FSTImpl(const char * file)
        {
            fst = Fst::Read(file);
        }

    FSTImpl(const Fst& f)
        : fst(f.Copy()) { }
    FSTImpl(Fst * f)
        : fst(f) { }

    FSTImpl(const FSTImpl& f)
        : fst(f.fst->Copy()) { }

    virtual FST * Copy() const
        {
            FSTImpl * ret = new FSTImpl(*this);
            ret->fst = fst->Copy();
            return ret;
        }

    // Combination
    // XXX: why are only some destructive?
    virtual FST * Compose(const FST * that) const
        {
            const FSTImpl * f = dynamic_cast<const FSTImpl<Arc>*>(that);
            if (f) {
                // XXX: stupid copy
                FSTImpl<Arc> * ret = new FSTImpl<Arc>(*this);
                fst::Compose(*(const Fst*)fst, *f->fst, ret->fst);
                return ret;
            }
            return NULL;
        }
    virtual FST * Intersect(const FST * that) const
        {
            const FSTImpl * f = dynamic_cast<const FSTImpl<Arc>*>(that);
            if (f) {
                // XXX: stupid copy
                FSTImpl<Arc> * ret = new FSTImpl<Arc>(*this);
                fst::Intersect(*(const Fst*)fst, *f->fst, ret->fst);
                return ret;
            }
            return NULL;
        }
    virtual void _Union(const FST * that)
        {
            const FSTImpl * f = dynamic_cast<const FSTImpl*>(that);
            if (f)
                fst::Union(fst, *f->fst);
        }
    virtual void _Concat(const FST * that)
        {
            const FSTImpl * f = dynamic_cast<const FSTImpl*>(that);
            if (f)
                fst::Concat(fst, *f->fst);
        }

    // Transformation
    virtual void _Closure(int type)
        { fst::Closure(fst, (fst::ClosureType)type); }

    virtual void _Project(int type)
        { fst::Project(fst, (fst::ProjectType)type); }

    virtual void _Encode(int type);
    virtual void _Decode(int type);

    // Cleanup
    // XXX: why only some destructive?
    virtual FST * Determinize() const;
    virtual void _RmEpsilon()
        {
            fst::RmEpsilon((Fst*)this);
        }
    virtual void _Minimize()
        {
            fst::Minimize((Fst*)this);
        }
    virtual FST * Minimize() const;

    virtual void _Prune(float w)
        {
            Prune(fst, w);
        }

    virtual void _Push(int type)
        {
            Push(fst, (fst::ReweightType)type);
        }

    virtual void AddState()
        { fst->AddState(); }
    virtual void SetStart(int i)
        { fst->SetStart(i); }
    virtual void SetFinal(int i, float w = 0)
        { fst->SetFinal(i, w); }
    virtual void AddArc(int from, int to, float w, int in, int out)
        { fst->AddArc(from, Arc(in, out, w, to)); }

    virtual void WriteBinary(const char * file) const
        {
            fst->fst::Fst<Arc>::Write(file);
        }

    virtual void WriteText(const char * file) const
        {
            ofstream os(file);
            FstPrinter<Arc>(*fst, NULL, NULL, NULL, false).Print(&os, file);
        }

    virtual string String() const
        {
            ostringstream os;
            FstPrinter<Arc>(*fst, NULL, NULL, NULL, false)
                .Print(&os, "<string>");
            return os.str();
        }

    virtual int NumStates() const
        {
            VectorFst<Arc> * v = dynamic_cast<VectorFst<Arc>*>(fst);
            return v ? (int)v->NumStates() : -1;
        }

    virtual int NumArcs(unsigned st) const
        {
            return fst->NumArcs(st);
        }

    // XXX: maybe compute them by default?
    virtual unsigned Properties(bool compute) const
        {
            return fst->Properties(0xffffffff, compute);
        }
    virtual FST * ShortestPath(unsigned n, int uniq) const
        {
            FSTImpl * ret = new FSTImpl<Arc>(new VectorFst<Arc>);
            fst::ShortestPath(*fst, ret->fst, n, uniq);
            return ret;
        }
};

template <class Arc>
FST *
FSTImpl<Arc>::Determinize() const
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
    FSTImpl * tmp = new FSTImpl(fst->Copy());
    Encode(tmp->fst, &enc);
    VectorFst<Arc> * ret = new VectorFst<Arc>;
    fst::Determinize(*tmp->fst, ret);
    Decode(ret, enc);
    return new FSTImpl<Arc>(ret);
}

template <class Arc>
FST *
FSTImpl<Arc>::Minimize() const
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
    FSTImpl * tmp = new FSTImpl(fst->Copy());
    Encode(tmp->fst, &enc);
    VectorFst<Arc> * ret = new VectorFst<Arc>;
    fst::Minimize(tmp->fst);
    Decode(tmp->fst, enc);
    return tmp;
}

inline static bool strok(const char * s)
{ return s && *s; }

FST *
ReadText(const char * file, int smr, bool acceptor,
         const char * isyms,
         const char * osyms, const char * ssyms)
{
    ifstream in(file);
    // osyms ||= isyms;
    fst::SymbolTable * is = NULL, *os = NULL, *ss = NULL;
    if (strok(isyms))
        is = SymbolTable::ReadText(isyms);
    if (strok(osyms))
        os = SymbolTable::ReadText(osyms);
    else if (strok(isyms))
        os = is;
    if (strok(ssyms))
        ss = SymbolTable::ReadText(ssyms);

    switch (smr) {
    case SMRLog: {
        FstReader<fst::LogArc> r;
        MutableFst<fst::LogArc> * f
            = r.read(in, file, is, os, ss, acceptor, true, true, false);
        return f ? new FSTImpl<fst::LogArc>(f) : NULL;
        break;
    }

    case SMRTropical: {
        FstReader<fst::StdArc> r;
        MutableFst<fst::StdArc> * f 
            = r.read(in, file, is, os, ss, acceptor, true, true, false);
        return f ? new FSTImpl<fst::StdArc>(f) : NULL;
        break;
    }
    default:
        cerr << "aiee: don't recognize semiring " << smr << endl;
        return NULL;
    };
}

template <class Arc>
void
FSTImpl<Arc>::_Encode(int flags)
{
    EncodeMapper<Arc> tmp(flags, ENCODE);
    Encode(fst, &tmp);
}

template <class Arc>
void
FSTImpl<Arc>::_Decode(int flags)
{
    EncodeMapper<Arc> tmp(flags, DECODE);
    Decode(fst, tmp);
}

FST *
ReadBinary(const char * file, int smr)
{
    switch (smr) {
    case SMRLog:
        return new FSTImpl<fst::LogArc>(file);
        break;

    case SMRTropical:
        return new FSTImpl<fst::StdArc>(file);
        break;

    default:
        return NULL;
    };
}

FST *
VectorFST(int smr)
{
    switch (smr) {
    case SMRLog:
        return new FSTImpl<LogArc>(new VectorFst<LogArc>);
    case SMRTropical:
        return new FSTImpl<StdArc>(new VectorFst<StdArc>);
    default:
        return NULL;
    };
}
