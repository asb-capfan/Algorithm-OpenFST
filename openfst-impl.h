#ifndef _OPENFST_IMPL_H
#define _OPENFST_IMPL_H

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

#include "openfst-pre.h"
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
    virtual FST * Determinize(float) const;
    virtual void _RmEpsilon()
        {
            try {
                fst::RmEpsilon(fst);
            } catch (fst_exception e) { }
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
                cerr << e.what() << endl;
                delete ret;
                return NULL;
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

#endif // _OPENFST_IMPL_H
