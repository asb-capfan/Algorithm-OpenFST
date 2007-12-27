// All right, Mr. Google monkey.  Calling exit(1) in a library a bad
// thing, m'kay?

#include <exception>
#include <string>
using namespace std;

class fst_exception : public exception
{
    const string msg;
public:
    fst_exception(const string& _msg) throw ()
	: msg(_msg) { }
    ~fst_exception() throw ()
	{ }
    const char * what () const throw ()
	{ return msg.c_str(); }
};

#define exit(n) (throw fst_exception((n) ? "ugh" : "ok"))
#include "fst/lib/compat.h"
#undef exit

#include "fst/lib/arcsort.h"
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
#include "fst/bin/draw-main.h"
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

    virtual int semiring() const;

    virtual FST * change_semiring(int smr) const;
    void copy_to(FST * fst) const;
    // Combination
    // XXX: why are only some destructive?
    virtual FST * Compose(FST * that);
    virtual FST * Intersect(FST * that);
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
            fst::RmEpsilon(fst);
        }
    virtual void _Minimize();
    virtual FST * Minimize() const;

    virtual void _Prune(float w)
        {
            Prune(fst, w);
        }

    virtual void _Push(int type)
        {
            Push(fst, (fst::ReweightType)type);
        }
    virtual void normalize();

    virtual void AddState()
        { fst->AddState(); }
    virtual void SetStart(int i)
        { fst->SetStart(i); }
    virtual int Start() const
        { return fst->Start(); }
    virtual void SetFinal(int i, float w = 0)
        { fst->SetFinal(i, w); }
    virtual float Final(int i) const
        { return fst->Final(i).Value(); }
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

    virtual string _String() const
        {
            ostringstream os;
            FstPrinter<Arc>(*fst, NULL, NULL, NULL, false)
                .Print(&os, "<string>");
            return os.str();
        }

    virtual string _Draw() const;

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
            try {
                fst::ShortestPath(*fst, ret->fst, n, uniq);
                return ret;
            } catch (fst_exception e) {
                cerr << e.what() << endl;
                delete ret;
                return NULL;
            }
        }
    void _strings(vector<string>& out, int st, const string& cur) const;
    virtual void strings(vector<string>&) const;
};

template <>
int
FSTImpl<LogArc>::semiring() const
{ return SMRLog; }

template <>
int
FSTImpl<StdArc>::semiring() const
{ return SMRTropical; }

template <class Arc>
void
FSTImpl<Arc>::copy_to(FST * to) const
{
    typedef typename Arc::StateId StateId;
    for (fst::StateIterator<fst::Fst<Arc> > it(*fst); !it.Done(); it.Next()) {
        to->AddState();
        if (fst->Final(it.Value()) != Arc::Weight::Zero())
            to->SetFinal(it.Value(), fst->Final(it.Value()).Value());
    }
    to->SetStart(fst->Start());
    for (fst::StateIterator<fst::Fst<Arc> > it(*fst); !it.Done(); it.Next()) {
        StateId from(it.Value());
        for (fst::ArcIterator<fst::Fst<Arc> > ai(*fst, from);
             !ai.Done(); ai.Next()) {
            Arc a(ai.Value());
            to->AddArc(from, a.nextstate, a.weight.Value(),
                       a.ilabel, a.olabel);
        }
    }
}

template <>
FST *
FSTImpl<StdArc>::change_semiring(int smr) const
{
    if (smr == SMRTropical)
        return Copy();
    FST * ret;
    if (smr == SMRLog) {
        ret = new FSTImpl<LogArc>(new VectorFst<LogArc>);
    } else {
        cerr << "Unknown semiring " << smr << endl;
        return NULL;
    }
    copy_to(ret);
    return ret;
}

template <>
FST *
FSTImpl<LogArc>::change_semiring(int smr) const
{
    if (smr == SMRLog)
        return Copy();
    FST * ret;
    if (smr == SMRTropical) {
        ret = new FSTImpl<StdArc>(new VectorFst<StdArc>);
    } else {
        cerr << "Unknown semiring " << smr << endl;
        return NULL;
    }
    copy_to(ret);
    return ret;
}

template <class Arc>
FST *
FSTImpl<Arc>::Intersect(FST * that)
{
    const FSTImpl * f = dynamic_cast<const FSTImpl<Arc>*>(that);
    if (f) {
        // XXX: maybe encode and try again here?
        if (!fst->Properties(kAcceptor, true))
            return NULL;
        if (!f->fst->Properties(kAcceptor, true))
            return NULL;
        // XXX: apparently intersect requires arc-sorting.
        ArcSort(f->fst, ILabelCompare<Arc>());
        ArcSort(fst, OLabelCompare<Arc>());
        // XXX: stupid copy
        FSTImpl<Arc> * ret = new FSTImpl<Arc>(new VectorFst<Arc>);
        fst::Intersect(*(const Fst*)fst, *f->fst, ret->fst);
        return ret;
    }
    return NULL;
}

template <class Arc>
FST *
FSTImpl<Arc>::Compose(FST * that)
{
    FSTImpl * f = dynamic_cast<FSTImpl<Arc>*>(that);
    if (f) {
        ArcSort(f->fst, ILabelCompare<Arc>());
        ArcSort(fst, OLabelCompare<Arc>());
        // XXX: stupid copy
        FSTImpl<Arc> * ret = new FSTImpl<Arc>(new VectorFst<Arc>);
        fst::Compose(*(const Fst*)fst, *f->fst, ret->fst);
        return ret;
    }
    return NULL;
}

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
void
FSTImpl<Arc>::_Minimize()
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
    Encode(fst, &enc);
    fst::Minimize(fst);
    Decode(fst, enc);
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
    fst::Minimize(tmp->fst);
    Decode(tmp->fst, enc);
    return tmp;
}

template <class Arc>
string
FSTImpl<Arc>::_Draw() const
{
    // OMGWTFBBQ!!!
    // FstDrawer(const Fst<A> &fst,
    //           const SymbolTable *isyms,
    //           const SymbolTable *osyms,
    //           const SymbolTable *ssyms,
    //           bool accep,
    //           string title,
    //           float width,
    //           float height,
    //           bool portrait,
    //           bool vertical,
    //           float ranksep,
    //           float nodesep,
    //           int fontsize,
    //           int precision)
    FstDrawer<Arc> fd(*fst,
                      NULL, NULL, NULL, // isyms, osyms, ssyms
                      fst->Properties(kAcceptor, true),
                      "",
                      8.5, 11,
                      false,
                      false,
                      0.2, 0.2,
                      10, 3);
    ostringstream os;
    fd.Draw(&os, "<string>");
    return os.str();
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

template <class Arc>
void
FSTImpl<Arc>::strings(vector<string>& out) const
{
    _strings(out, fst->Start(), "");
}

template <class Arc>
void
FSTImpl<Arc>::_strings(vector<string>& out, int st, const string& cur) const
{
    if (fst->Final(st) != Arc::Weight::Zero()) {
        out.push_back(cur);
    } else {
        for (ArcIterator<fst::Fst<Arc> > ai(*fst, st); !ai.Done(); ai.Next()) {
            ostringstream il;
            il << cur << ai.Value().ilabel;
            _strings(out, ai.Value().nextstate, il.str());
        }
    }
}

template <class Arc>
void
FSTImpl<Arc>::normalize()
{
    typedef typename Arc::Weight Weight;
    Push(fst, (fst::ReweightType)INITIAL);
    Weight w = Weight::Zero();
    for (ArcIterator<fst::Fst<Arc> > ai(*fst, fst->Start());
         !ai.Done(); ai.Next())
        w = Plus(w, ai.Value().weight);
    for (MutableArcIterator<Fst> ai(fst, fst->Start());
         !ai.Done(); ai.Next()) {
        Arc a = ai.Value();
        a.weight = Divide(a.weight, w);
        ai.SetValue(a);
    }
}

SV*
FST::String() const
{
    string tmp = _String();
    return newSVpvn(tmp.c_str(), tmp.size());
}

SV*
FST::Draw() const
{
    string tmp = _Draw();
    return newSVpvn(tmp.c_str(), tmp.size());
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
