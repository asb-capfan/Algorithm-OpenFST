#include <sstream>
#include <fstream>
#include "openfst-pre.h"
#include "openfst.h"
#include "openfst-io.h"
#include "markovize.h"

using namespace std;
using namespace fst;

//////////////////////////////////////////////////////////////////////
// FST Impls
template <typename Arc>
struct FSTImpl : public FSTBase<Arc>
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
        { return new FSTImpl(*this); }

    virtual int semiring() const;

    virtual FST * change_semiring(int smr) const;
    void copy_to(FST * fst) const;
    virtual const fst::Fst<Arc> * get_fst() const
        { return fst; }

    // Combination
    // XXX: why are only some destructive?
    virtual FST * Compose(FST * that) const;
    virtual FST * Intersect(FST * that) const;
    virtual FST * Difference(FST * that) const;
    virtual void _Union(const FST * that)
        {
            const FSTBase<Arc> * f = dynamic_cast<const FSTBase<Arc>*>(that);
            if (f)
                fst::Union(fst, *f->get_fst());
        }
    virtual void _Concat(const FST * that)
        {
            const FSTBase<Arc> * f = dynamic_cast<const FSTBase<Arc>*>(that);
            if (f)
                fst::Concat(fst, *f->get_fst());
        }

    // Transformation
    virtual void _Closure(int type)
        { fst::Closure(fst, (fst::ClosureType)type); }

    virtual void _Project(int type)
        { fst::Project(fst, (fst::ProjectType)type); }

    virtual void _Encode(int type);
    virtual void _Decode(int type);

    virtual FST * Reverse() const;
    virtual void _Invert()
        { fst::Invert(fst); }

    // Cleanup
    // XXX: why only some destructive?
    virtual FST * Determinize(float) const;
    virtual void _RmEpsilon(float delta)
        {
            try {
                vector<typename Arc::Weight> d;
                AutoQueue<typename Arc::StateId>
                    queue(*fst, &d, EpsilonArcFilter<Arc>());
                RmEpsilonOptions<Arc, AutoQueue<typename Arc::StateId> >
                    opts(&queue, delta, true);
                RmEpsilon(fst, &d, opts);
            } catch (fst_exception e) {
                croak("%s", e.what());
            }
        }
    virtual FST * EpsNormalize(int ) const;
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
    virtual FST * markovize(int ) const;

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

    void init_symtab() const
        {
            if (!fst->InputSymbols()) {
                SymbolTable * st = new SymbolTable("");
                // remember to add epsilon!
                st->AddSymbol("-");
                fst->SetInputSymbols(st);
            }
            if (!fst->OutputSymbols())
                fst->SetOutputSymbols(fst->InputSymbols());
        }
    virtual int add_input_symbol(const char * s)
        {
            if (!fst->InputSymbols())
                init_symtab();
            return ((SymbolTable*)fst->InputSymbols())->AddSymbol(string(s));
        }
    virtual int add_output_symbol(const char * s)
        {
            if (!fst->OutputSymbols())
                init_symtab();
            return ((SymbolTable*)fst->OutputSymbols())->AddSymbol(string(s));
        }

    virtual void add_arc(int, int, float, const char *, const char *);

    virtual void WriteBinary(const char * file) const
        {
            fst->fst::Fst<Arc>::Write(file);
        }

    virtual void WriteText(const char * file) const
        {
            ofstream os(file);
            FstPrinter<Arc>(*fst, fst->InputSymbols(),
                            fst->OutputSymbols(),
                            NULL, false).Print(&os, file);
        }

    virtual string _String() const
        {
            ostringstream os;
            FstPrinter<Arc>(*fst, fst->InputSymbols(),
                            fst->OutputSymbols(), NULL, false)
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
                delete ret;
                croak(e.what());
            }
        }
    void _strings(vector<string>& out, int st, const string& cur) const;
    virtual void strings(vector<string>&) const;
    virtual SymbolTable * InputSymbols() const
        { return (SymbolTable *)fst->InputSymbols(); }
    virtual SymbolTable * OutputSymbols() const
        { return (SymbolTable *)fst->OutputSymbols(); }
    virtual void SetInputSymbols(const SymbolTable * s)
        { fst->SetInputSymbols(s); }
    virtual void SetOutputSymbols(const SymbolTable * s)
        { fst->SetOutputSymbols(s); }
};

//////////////////////////////////////////////////////////////////////
// Standard functions/methods:

template <class Arc>
FST *
FSTImpl<Arc>::Intersect(FST * that) const
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

#define CAST_OR_CROAK(Y, X, T) do {                             \
        Y = dynamic_cast<T>(X);                                 \
        if (!Y) {                                               \
            croak("Argument %s not of type %s.", #X, #T);       \
        }                                                       \
    } while (0)

template <class Arc>
FST *
FSTImpl<Arc>::Compose(FST * that) const
{
    FSTImpl * f;
    CAST_OR_CROAK(f, that, FSTImpl<Arc>*);
    if (!CompatSymbols(fst->OutputSymbols(), f->fst->InputSymbols())) {
        cerr << "incompatible symbol tables in Compose()" << endl;
        if (fst->OutputSymbols())
            fst->OutputSymbols()->WriteText(cerr);
        else
            cerr << "<NULL>" << endl;
        if (f->fst->InputSymbols())
            f->fst->InputSymbols()->WriteText(cerr);
        else
            cerr << "<NULL>" << endl;
        return NULL;
    }
    ArcSort(f->fst, ILabelCompare<Arc>());
    ArcSort(fst, OLabelCompare<Arc>());
    // XXX: stupid copy
    FSTImpl<Arc> * ret = new FSTImpl<Arc>(new VectorFst<Arc>);
    // Feeding my output into his input.
    if (fst->OutputSymbols() || f->fst->InputSymbols()) {
        SymbolTable * rst = new SymbolTable("");
        if (fst->OutputSymbols())
            rst->AddTable(*fst->OutputSymbols());
        if (f->fst->InputSymbols())
            rst->AddTable(*f->fst->InputSymbols());
        ret->fst->SetInputSymbols(fst->InputSymbols());
        ret->fst->SetOutputSymbols(f->fst->OutputSymbols());
    }
    fst::Compose(*(const Fst*)fst, *f->fst, ret->fst);
    return ret;
}

template <class Arc>
FST *
FSTImpl<Arc>::Difference(FST * that) const
{
    FSTBase<Arc> * f;
    CAST_OR_CROAK(f, that, FSTBase<Arc>*);
    if (!get_fst()->Properties(kAcceptor, true)
        || !f->get_fst()->Properties(kAcceptor, true))
        return NULL;
    VectorFst<Arc> * ret = new VectorFst<Arc>;
    // Stupid library requires second FST to be unweighted.
    ArcSortFst<Arc, ILabelCompare<Arc> > tmp(
        MapFst<Arc, Arc, RmWeightMapper<Arc> >(
            *f->get_fst(), RmWeightMapper<Arc>()),
        ILabelCompare<Arc>());
    fst::Difference(*get_fst(), tmp, ret);
    return new FSTImpl<Arc>(ret);
}

template <class Arc>
FST *
FSTImpl<Arc>::Determinize(float del) const
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
    FSTImpl * tmp = new FSTImpl(fst->Copy());
    Encode(tmp->fst, &enc);
    VectorFst<Arc> * ret = new VectorFst<Arc>;
    fst::Determinize(*tmp->fst, ret, DeterminizeOptions(del));
    Decode(ret, enc);
    return new FSTImpl<Arc>(ret);
}

template <class Arc>
FST *
FSTImpl<Arc>::EpsNormalize(int type) const
{
    FSTImpl * ret = new FSTImpl(fst->Copy());
    // XXX: type-1 so project and epsnormalize use the same constants
    try {
        fst::EpsNormalize(*fst, ret->fst, (EpsNormalizeType)(type-1));
    } catch (fst_exception e) {
        delete ret;
        croak("EpsNormalize failed.");
    }
    return ret;
}

template <class Arc>
void
FSTImpl<Arc>::_Minimize()
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    try {
        EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
        Encode(fst, &enc);
        fst::Minimize(fst);
        Decode(fst, enc);
    } catch (fst_exception e) {
        croak("%s", e.what());
    }
}

template <class Arc>
FST *
FSTImpl<Arc>::Minimize() const
{
    // NOTE: we encode/decode here to make the FST functional
    // (acceptors always are) and thereby avoid library bitching.
    EncodeMapper<Arc> enc(ENCODE_LABEL, ENCODE);
    FSTImpl * tmp = new FSTImpl(fst->Copy());
    try {
        Encode(tmp->fst, &enc);
        fst::Minimize(tmp->fst);
        Decode(tmp->fst, enc);
        return tmp;
    } catch (fst_exception e) {
        delete tmp;
        croak("%s", e.what());
    }
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
FST *
FSTImpl<Arc>::Reverse() const
{
    FSTImpl * ret = new FSTImpl(fst->Copy());
    fst::Reverse(*fst, ret->fst);
    return ret;
}

//////////////////////////////////////////////////////////////////////
// Extension functions/methods:

// template <>
// int
// FSTImpl<RealArc>::semiring() const
// { return SMRReal; }

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

// template <>
// FST *
// FSTImpl<RealArc>::change_semiring(int smr) const
// {
//     if (smr == SMRReal)
//         return Copy();
//     FST * ret;
//     if (smr == SMRLog) {
//         ret = new FSTImpl<LogArc>(new VectorFst<LogArc>);
//     } else if (smr == SMRTropical) {
//         ret = new FSTImpl<StdArc>(new VectorFst<StdArc>);
//     } else {
//         cerr << "Unknown semiring " << smr << endl;
//         return NULL;
//     }
//     copy_to(ret);
//     return ret;
// }

template <>
FST *
FSTImpl<StdArc>::change_semiring(int smr) const
{
    if (smr == SMRTropical)
        return Copy();
    FSTImpl<LogArc> * ret;
    if (smr == SMRLog) {
        ret = new FSTImpl<LogArc>(new VectorFst<LogArc>);
        ret->fst->SetInputSymbols(fst->InputSymbols());
        ret->fst->SetOutputSymbols(fst->OutputSymbols());
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
    FSTImpl<StdArc> * ret;
    if (smr == SMRTropical) {
        ret = new FSTImpl<StdArc>(new VectorFst<StdArc>);
        ret->fst->SetInputSymbols(fst->InputSymbols());
        ret->fst->SetOutputSymbols(fst->OutputSymbols());
    } else {
        cerr << "Unknown semiring " << smr << endl;
        return NULL;
    }
    copy_to(ret);
    return ret;
}

template <class Arc>
void
FSTImpl<Arc>::add_arc(int from, int to, float w,
                      const char * in, const char * out)
{
    int iin = add_input_symbol(in);
    int iout = add_output_symbol(out);
    // fprintf(stderr, "iin = %d, iout = %d\n", iin, iout);
    AddArc(from, to, w, iin, iout);
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
    int st = fst->Start();
    if (st < 0) {
        cerr << "tried to normalize empty fst" << endl;
        return;
    }
    for (ArcIterator<fst::Fst<Arc> > ai(*fst, st); !ai.Done(); ai.Next())
        w = Plus(w, ai.Value().weight);
    for (MutableArcIterator<Fst> ai(fst, st); !ai.Done(); ai.Next()) {
        Arc a = ai.Value();
        a.weight = Divide(a.weight, w);
        ai.SetValue(a);
    }
}

template <class Arc>
FST *
FSTImpl<Arc>::markovize(int order) const
{
    VectorFst<Arc> * ret = new VectorFst<Arc>;
    ret->SetInputSymbols(fst->InputSymbols());
    ret->SetOutputSymbols(fst->OutputSymbols());
    ::markovize(ret, *fst, order);
    return new FSTImpl<Arc>(ret);
}

SV*
FST::String() const
{
    string tmp = _String();
    return newSVpvn(tmp.c_str(), tmp.size());
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

//////////////////////////////////////////////////////////////////////
// I/O

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
                      fst->InputSymbols(),
                      fst->OutputSymbols(),
                      NULL, // isyms, osyms, ssyms
                      fst->Properties(kAcceptor, true),
                      "",
                      8.5, 11,
                      true,
                      false,
                      0.3, 0.2,
                      10, 3);
    ostringstream os;
    fd.Draw(&os, "<string>");
    return os.str();
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
