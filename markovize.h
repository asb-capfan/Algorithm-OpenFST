#include <list>
#include <iostream>
#include "hash.h"
// #include "openfst-pre.h"

using namespace std;
using namespace fst;

//////////////////////////////////////////////////////////////////////
// Please just let me print!
ostream&
operator<<(ostream& os, const fst::LogArc& a)
{
    return os << '(' << a.ilabel << ':' << a.olabel << '/' << a.weight.Value()
              << " -> " << a.nextstate << ')';
}

ostream&
operator<<(ostream& os, const fst::StdArc& a)
{
    return os << '(' << a.ilabel << ':' << a.olabel << '/' << a.weight.Value()
              << " -> " << a.nextstate << ')';
}

template <typename Container>
struct merger
{
    void operator()(Container& a, const Container& b) const
        {
            a.insert(a.end(), b.begin(), b.end());
        }
};

template <typename A, typename B>
ostream&
operator<<(ostream& os, const pair<A, B>& p)
{
    return os << '(' << p.first << ' ' << p.second << ')';
}

template <typename Arc>
ostream&
operator<<(ostream& os, typename Arc::StateId id)
{
    return os << id;
}

template <typename T>
ostream&
operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

#include "trie.h"

template <typename Arc>
struct markov_t
{
    typedef Trie<pair<typename Arc::Label, typename Arc::Label>,
                 vector<pair<typename Arc::StateId, Arc> > > trie;
    typedef merger<vector<pair<typename Arc::StateId, Arc> > > merge;
    typedef typename Arc::StateId State;
    typedef typename Arc::Label Label;
};

/// Extend trie T to depth ORDER from state ID of FST.
template <typename Arc>
void
extend_tree(typename markov_t<Arc>::trie & t,
            const Fst<Arc>& fst, typename Arc::StateId id,
            int order)
{
    typedef typename Arc::StateId State;
    typedef typename Arc::Label Label;
    typedef typename markov_t<Arc>::trie trie_t;
    typedef typename markov_t<Arc>::merge merge_t;
    typedef ArcIterator<Fst<Arc> > iterator_t;
    if (order == 0)
        return;

    for (iterator_t a(fst, id); !a.Done(); a.Next()) {
        vector<pair<State, Arc> > v;
        Arc tmp(a.Value());
        v.push_back(make_pair(id, tmp));
        t.insert(make_pair(tmp.ilabel, tmp.olabel), v, merge_t());
        typename trie_t::map_t::iterator it
            = t.m.find(make_pair(tmp.ilabel, tmp.olabel));
        if (order > 1) {
            if (it->second.second == NULL)
                it->second.second = new trie_t;
            extend_tree(*it->second.second, fst, tmp.nextstate, order-1);
        }
    }
}

/// Merge two states
template <typename Arc>
struct add_merged_state
{
    typedef typename Arc::StateId State;
    typedef typename Arc::Label Label;
    typedef typename Arc::Weight Weight;
    typedef typename markov_t<Arc>::trie trie_t;

    const Fst<Arc>& ifst;
    MutableFst<Arc> * ofst;
    hash_map<State, State>& st;

    add_merged_state(const Fst<Arc>& i, MutableFst<Arc> * o,
                     hash_map<State, State>& s)
        : ifst(i), ofst(o), st(s) { }
    trie_t * operator()(const pair<Label, Label>& k,
                        const vector<pair<State, Arc> >& v);
};

template <typename Arc>
typename add_merged_state<Arc>::trie_t *
add_merged_state<Arc>::operator()(const pair<Label, Label>& k,
                                  const vector<pair<State, Arc> >& v)
{
    hash_map<State, Weight> w;
    // Weight w = Weight::Zero();
    State s = ofst->AddState();
    for (typename vector<pair<State, Arc> >::const_iterator i = v.begin();
         i != v.end(); ++i) {
        assert(st.find(i->first) != st.end());
        State s0 = st[i->first];
        st[i->second.nextstate] = s;
        // XXX: make sure either all or none is final?  Add weights?
        Weight fw = ifst.Final(i->second.nextstate);
        if (fw != Weight::Zero())
            ofst->SetFinal(s, fw);
        typename hash_map<State, Weight>::iterator it = w.find(s0);
        if (it == w.end()) {
            w.insert(make_pair(s0, i->second.weight));
        } else {
            it->second = Plus(it->second, i->second.weight);
        }
    }
    typename hash_map<State, Weight>::iterator i = w.begin(), end = w.end();
    // typename vector<pair<State, Arc> >::const_iterator j, jend = v.end();
    // XXX: add arcs from all to all, or just from true origin?
    for (; i != end; ++i) {
        // for (j = v.begin(); j != jend; ++j) {
            Arc a(k.first, k.second, i->second, s);
            ofst->AddArc(i->first, a);
        // }
    }
    return NULL;
}

template <typename Arc>
struct extend_by_one
{
    typedef typename Arc::StateId State;
    typedef typename Arc::Label Label;
    typedef typename Arc::Weight Weight;
    typedef typename markov_t<Arc>::trie trie_t;
    typedef typename markov_t<Arc>::merge merge_t;

    const Fst<Arc>& ifst;
    const hash_map<State, State>& st;
    extend_by_one(const Fst<Arc>& i, const hash_map<State, State>& s)
        : ifst(i), st(s) { }
    trie_t * operator()(const pair<Label, Label>& k,
                        vector<pair<State, Arc> >& v) const;
};

template <typename Arc>
typename extend_by_one<Arc>::trie_t *
extend_by_one<Arc>::operator()(const pair<Label, Label>& k,
                               vector<pair<State, Arc> >& v) const
{
    trie_t * ret = new trie_t;
    typedef ArcIterator<Fst<Arc> > iterator_t;
    for (typename vector<pair<State, Arc> >::const_iterator i = v.begin();
         i != v.end(); ++i) {
        State id = i->second.nextstate;
        // if (ifst.Final(id) != Weight::Zero())
        //     continue;
        for (iterator_t a(ifst, id); !a.Done(); a.Next()) {
            Arc tmp(a.Value());
            vector<pair<State, Arc> > v;
            v.push_back(make_pair(id, tmp));
            ret->insert(make_pair(tmp.ilabel, tmp.olabel), v, merge_t());
        }
    }
    return ret;
}

template <typename Arc>
void
markovize(MutableFst<Arc> * ofst, const Fst<Arc>& ifst, int order)
{
    typedef typename Arc::StateId State;
    typedef typename Arc::Label Label;
    typedef typename markov_t<Arc>::trie trie_t;
    typedef typename markov_t<Arc>::merge merge_t;

    trie_t q;
    hash_map<State, State> st;

    // Build up initial set
    ofst->SetStart(st[ifst.Start()] = ofst->AddState());
    // Add first edges
    extend_tree(q, ifst, ifst.Start(), 1);

    // Add up to depth
    for (int i = 0; i < order; i++) {
        // Add leaves of q to fst:
        add_merged_state<Arc> ams(ifst, ofst, st);
        q.map_leaves(ams);
        // Extend leaves with next state:
        extend_by_one<Arc> ebo(ifst, st);
        q.map_leaves(ebo);
    }

    // Proceed breadth-first through FST.
    while (q.size() > 0) {
        // Add leaves of q to fst:
        add_merged_state<Arc> ams(ifst, ofst, st);
        q.map_leaves(ams);

        // Discard symbol and merge:
        trie_t tmp;
        typename trie_t::map_t::iterator it = q.m.begin(),
            end = q.m.end();
        for (; it != end; ++it)
            if (it->second.second)
                tmp.insert(*it->second.second, merge_t());
        q.swap(tmp);
        // Extend leaves with next state:
        extend_by_one<Arc> ebo(ifst, st);
        q.map_leaves(ebo);
    }
    // Add the leftovers
}
