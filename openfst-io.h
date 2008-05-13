#ifndef _OPENFST_IO_H
#define _OPENFST_IO_H

// XXX: stolen from {compile,print}-main.h, but avoiding exit(1) stupidity.

using namespace fst;
using namespace std;

template <class A> class FstReader {
public:
    typedef A Arc;
    typedef typename A::StateId StateId;
    typedef typename A::Label Label;
    typedef typename A::Weight Weight;

    FstReader()
        : nline_(0), nstates_(0) { }
    VectorFst<A> * read(istream &istrm, const string &source,
                        const SymbolTable *isyms, const SymbolTable *osyms,
                        const SymbolTable *ssyms, bool accep, bool ikeep,
                        bool okeep, bool nkeep);

private:
    // Maximum line length in text file.
    static const int kLineLen = 8096;

    int64 StrToId(const char *s, const SymbolTable *syms,
                  const char *name) const {
        int64 n;

        if (syms) {
            n = syms->Find(s);
            if (n < 0) {
                cerr << "FstReader: Symbol \"" << s
                     << "\" is not mapped to any integer " << name
                     << ", symbol table = " << syms->Name()
                     << ", source = " << source_ << ", line = " << nline_;
            }
        } else {
            char *p;
            n = strtoll(s, &p, 10);
            if (p < s + strlen(s) || n < 0) {
                cerr << "FstReader: Bad " << name << " integer = \"" << s
                     << "\", source = " << source_ << ", line = " << nline_;
            }
        }
        return n;
    }

    StateId StrToStateId(const char *s) {
        StateId n = StrToId(s, ssyms_, "state ID");

        if (keep_state_numbering_)
            return n;

        // remap state IDs to make dense set
        typename hash_map<StateId, StateId>::const_iterator it = states_.find(n);
        if (it == states_.end()) {
            states_[n] = nstates_;
            return nstates_++;
        } else {
            return it->second;
        }
    }

    StateId StrToILabel(const char *s) const {
        return StrToId(s, isyms_, "arc ilabel");
    }

    StateId StrToOLabel(const char *s) const {
        return StrToId(s, osyms_, "arc olabel");
    }

    Weight StrToWeight(const char *s, bool allow_zero) const {
        Weight w;
        istringstream strm(s);
        strm >> w;
        if (strm.fail() || !allow_zero && w == Weight::Zero()) {
            cerr << "FstReader: Bad weight = \"" << s
                 << "\", source = " << source_ << ", line = " << nline_;
        }
        return w;
    }

    VectorFst<A> * fst_;
    size_t nline_;
    string source_;                      // text FST source name
    const SymbolTable *isyms_;           // ilabel symbol table
    const SymbolTable *osyms_;           // olabel symbol table
    const SymbolTable *ssyms_;           // slabel symbol table
    hash_map<StateId, StateId> states_;  // state ID map
    StateId nstates_;                    // number of seen states
    bool keep_state_numbering_;
    bool bad;
};

template <class A>
VectorFst<A> *
FstReader<A>::read(istream &istrm, const string &source,
                   const SymbolTable *isyms, const SymbolTable *osyms,
                   const SymbolTable *ssyms, bool accep, bool ikeep,
                   bool okeep, bool nkeep)
{
    fst_ = new VectorFst<A>;
    bad = false;
    source_ = source;
    isyms_ = isyms;
    osyms_ = osyms;
    ssyms_ = ssyms;
    keep_state_numbering_ = nkeep;
    char line[kLineLen];
    while (!bad && istrm.getline(line, kLineLen)) {
        ++nline_;
        vector<char *> col;
        SplitToVector(line, "\n\t ", &col, true);
        if (col.size() == 0 || col[0][0] == '\0')  // empty line
            continue;
        if (col.size() > 5 ||
            col.size() > 4 && accep ||
            col.size() == 3 && !accep) {
            cerr << "FstReader: Bad number of columns, source = "
                 << source_ << ", line = " << nline_;
            bad = true;
        }
        StateId s = StrToStateId(col[0]);
        if (!fst_)
            return NULL;
        while (s >= fst_->NumStates())
            fst_->AddState();
        if (nline_ == 1)
            fst_->SetStart(s);

        Arc arc;
        StateId d = s;
        switch (col.size()) {
        case 1:
            fst_->SetFinal(s, Weight::One());
            break;
        case 2:
            fst_->SetFinal(s, StrToWeight(col[1], true));
            break;
        case 3:
            arc.nextstate = d = StrToStateId(col[1]);
            arc.ilabel = StrToILabel(col[2]);
            arc.olabel = arc.ilabel;
            arc.weight = Weight::One();
            fst_->AddArc(s, arc);
            break;
        case 4:
            arc.nextstate = d = StrToStateId(col[1]);
            arc.ilabel = StrToILabel(col[2]);
            if (accep) {
                arc.olabel = arc.ilabel;
                arc.weight = StrToWeight(col[3], false);
            } else {
                arc.olabel = StrToOLabel(col[3]);
                arc.weight = Weight::One();
            }
            fst_->AddArc(s, arc);
            break;
        case 5:
            arc.nextstate = d = StrToStateId(col[1]);
            arc.ilabel = StrToILabel(col[2]);
            arc.olabel = StrToOLabel(col[3]);
            arc.weight = StrToWeight(col[4], false);
            fst_->AddArc(s, arc);
        }
        while (d >= fst_->NumStates())
            fst_->AddState();
    }
    if (bad) {
        delete fst_;
        return NULL;
    }
    if (ikeep)
        fst_->SetInputSymbols(isyms);
    if (okeep)
        fst_->SetOutputSymbols(osyms);
    return fst_;
}

template <class A>
class FstPrinter {
public:
    typedef A Arc;
    typedef typename A::StateId StateId;
    typedef typename A::Label Label;
    typedef typename A::Weight Weight;

    FstPrinter(const Fst<A> &fst,
               const SymbolTable *isyms,
               const SymbolTable *osyms,
               const SymbolTable *ssyms,
               bool accep)
        : fst_(fst), isyms_(isyms), osyms_(osyms), ssyms_(ssyms),
          accep_(accep && fst.Properties(kAcceptor, true)), ostrm_(0) {}

    // Print Fst to an output strm
    void Print(ostream *ostrm, const string &dest) {
        ostrm_ = ostrm;
        dest_ = dest;
        StateId start = fst_.Start();
        if (start == kNoStateId)
            return;
        // initial state first
        PrintState(start);
        for (StateIterator< Fst<A> > siter(fst_);
             !siter.Done();
             siter.Next()) {
            StateId s = siter.Value();
            if (s != start)
                PrintState(s);
        }
    }

private:
    // Maximum line length in text file.
    static const int kLineLen = 8096;

    void PrintId(int64 id, const SymbolTable *syms,
                 const char *name) const {
        if (syms) {
            string symbol = syms->Find(id);
            if (symbol == "") {
                cerr << "FstPrinter: Integer " << id
                     << " is not mapped to any textual symbol"
                     << ", symbol table = " << syms->Name()
                     << ", destination = " << dest_;
                *ostrm_ << '<' << id << '>';
            } else
                *ostrm_ << symbol;
        } else {
            *ostrm_ << id;
        }
    }

    void PrintStateId(StateId s) const {
        PrintId(s, ssyms_, "state ID");
    }

    void PrintILabel(Label l) const {
        PrintId(l, isyms_, "arc input label");
    }

    void PrintOLabel(Label l) const {
        PrintId(l, osyms_, "arc output label");
    }

    void PrintState(StateId s) const {
        bool output = false;
        for (ArcIterator< Fst<A> > aiter(fst_, s);
             !aiter.Done();
             aiter.Next()) {
            Arc arc = aiter.Value();
            PrintStateId(s);
            *ostrm_ << "\t";
            PrintStateId(arc.nextstate);
            *ostrm_ << "\t";
            PrintILabel(arc.ilabel);
            if (!accep_) {
                *ostrm_ << "\t";
                PrintOLabel(arc.olabel);
            }
            if (arc.weight != Weight::One())
                *ostrm_ << "\t" << arc.weight;
            *ostrm_ << "\n";
            output = true;
        }
        Weight final = fst_.Final(s);
        if (final != Weight::Zero() || !output) {
            PrintStateId(s);
            if (final != Weight::One()) {
                *ostrm_ << "\t" << final;
            }
            *ostrm_ << "\n";
        }
    }

    const Fst<A> &fst_;
    const SymbolTable *isyms_;     // ilabel symbol table
    const SymbolTable *osyms_;     // olabel symbol table
    const SymbolTable *ssyms_;     // slabel symbol table
    bool accep_;                   // print as acceptor when possible
    ostream *ostrm_;                // binary FST destination
    string dest_;                  // binary FST destination name
};

#endif // _OPENFST_IO_H
