#ifndef _trie_h
#define _trie_h

#include "hash.h"
#include <utility>
using namespace std;

template <typename Key, typename Value>
struct Trie
{
    typedef hash_map<Key, pair<Value, Trie*> > map_t;
    map_t m;

    Trie() { }
    ~Trie();
    template <typename Merge> void insert(Key k, const Value& v, Merge);
    template <typename Merge> void insert(const Trie& , Merge);
    template <typename Iter, typename Merge> void insert(Iter , Iter, Merge);
    template <typename Iter, typename Out> bool find(Out, Iter, Iter);
    bool leaf() const;
    int size() const;
    void swap(Trie& t)
        { m.swap(t.m); }
    template <typename Fn>
    void map_leaves(Fn& f);
    void write(ostream& , const string& ) const;
};

template <typename Key, typename Value>
Trie<Key, Value>::~Trie()
{
    typename map_t::iterator i = m.begin(), end = m.end();
    for (; i != end; ++i)
        if (i->second.second)
            delete i->second.second;
}

template <typename Key, typename Value>
template <typename Merge>
void
Trie<Key, Value>::insert(Key k, const Value& v, Merge merge)
{
    if (m.find(k) == m.end()) {
        m[k].first = v;
        m[k].second = NULL;
    } else {
        merge(m[k].first, v);
    }
}

template <typename Key, typename Value>
template <typename Iter, typename Merge>
void
Trie<Key, Value>::insert(Iter a, Iter b, Merge merge)
{
    if (a == b)
        return;
    insert(a->first, a->second, merge);
    if (++a == b)
        return;
    if (!m[a->first].second)
        m[a->first].second = new Trie;
    m[a->first].second->insert(a, b, merge);
}

template <typename Key, typename Value>
template <typename Merge>
void
Trie<Key, Value>::insert(const Trie& that, Merge merge)
{
    typename map_t::const_iterator i(that.m.begin()), end(that.m.end());
    for (; i != end; ++i) {
        insert(i->first, i->second.first, merge);
        if (i->second.second) {
            if (!m[i->first].second)
                m[i->first].second = new Trie;
            m[i->first].second->insert(*i->second.second, merge);
        }
    }
}

template <typename Key, typename Value>
template <typename Iter, typename Out>
bool
Trie<Key, Value>::find(Out o, Iter a, Iter b)
{
    if (a == b)
        return true;
    typename map_t::iteartor it = m.find(*a);
    if (it == m.end())
        return false;
    *o++ = it->second.first;
    if (++a == b)
        return it->second.second == NULL;
    if (!it->second.second)
        return false;
    return it->second.second->find(o, a, b);
}

template <typename Key, typename Value>
bool
Trie<Key, Value>::leaf() const
{
    for (typename map_t::const_iterator i = m.begin(); i != m.end(); i++)
        if (i->second.second)
            return false;
    return true;
}

template <typename Key, typename Value>
int
Trie<Key, Value>::size() const
{
    // if (leaf())
    //     return m.size();
    int ret = 0;
    // assert(m.size() > 0);
    for (typename map_t::const_iterator i = m.begin(); i != m.end(); i++) {
        if (i->second.second)
            ret += i->second.second->size();
        else
            ++ret;
    }
    return ret;
}

template <typename Key, typename Value>
template <typename Fn>
void
Trie<Key, Value>::map_leaves(Fn& f)
{
    for (typename map_t::iterator i = m.begin(); i != m.end(); i++) {
        if (i->second.second)
            i->second.second->map_leaves(f);
        else {
            i->second.second = f(i->first, i->second.first);
        }
    }
}

template <typename Key, typename Value>
void
Trie<Key, Value>::write(ostream& os, const string& pad) const
{
    for (typename map_t::const_iterator i = m.begin(); i != m.end(); i++) {
        os << pad << i->first;// << ' ' << i->second.first;
        if (i->second.second) {
            os << " => " << endl;
            i->second.second->write(os, pad + ' ');
        } else {
            os << endl;
        }
    }
}

template <typename Key, typename Value>
ostream&
operator<<(ostream& os, const Trie<Key, Value>& t)
{
    t.write(os, "");
    return os;
}

#endif // _trie_h
