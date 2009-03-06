#ifndef PQ_LOSERTREE_HEADER
#define PQ_LOSERTREE_HEADER

/***************************************************************************
 *            pq_losertree.h
 *
 *  Thu Jul  3 15:22:50 2003
 *  Copyright  1999  Peter Sanders
 *  sanders@mpi-sb.mpg.de
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 *  Copyright  2008, 2009  Johannes Singler
 *  singler@ira.uka.de
 *  Copyright © 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 ****************************************************************************/

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local
{

//////////////////////////////////////////////////////////////////////
// The data structure from Knuth, "Sorting and Searching", Section 5.4.1
/**
 *!  \brief  Loser tree from Knuth, "Sorting and Searching", Section 5.4.1
 *!  \param  KNKMAX  maximum arity of loser tree, has to be a power of two
 */
template <class ValTp_, class Cmp_, unsigned KNKMAX>
class loser_tree : private noncopyable
{
public:
    typedef ValTp_ value_type;
    typedef Cmp_ comparator_type;
    typedef value_type Element;

private:
#if STXXL_PQ_INTERNAL_LOSER_TREE
    struct Entry
    {
        value_type key; // Key of Loser element (winner for 0)
        unsigned_type index; // number of losing segment
    };
#endif //STXXL_PQ_INTERNAL_LOSER_TREE

    comparator_type cmp;
    // stack of free segment indices
    internal_bounded_stack<unsigned_type, KNKMAX> free_slots;

    unsigned_type size_; // total number of elements stored
    unsigned_type logK; // log of current tree size
    unsigned_type k; // invariant (k == 1 << logK), always a power of two

    Element sentinel; // target of free segment pointers

#if STXXL_PQ_INTERNAL_LOSER_TREE
    // upper levels of loser trees
    // entry[0] contains the winner info
    Entry entry[KNKMAX];
#endif //STXXL_PQ_INTERNAL_LOSER_TREE

    // leaf information
    // note that Knuth uses indices k..k-1
    // while we use 0..k-1
    Element * current[KNKMAX]; // pointer to current element
    Element * current_end[KNKMAX]; // pointer to end of block for current element
    Element * segment[KNKMAX]; // start of Segments
    unsigned_type segment_size[KNKMAX]; // just to count the internal memory consumption, in bytes

    unsigned_type mem_cons_;

    // private member functions
    unsigned_type initWinner(unsigned_type root);
    void update_on_insert(unsigned_type node, const Element & newKey, unsigned_type newIndex,
                        Element * winnerKey, unsigned_type * winnerIndex, unsigned_type * mask);
    void deallocate_segment(unsigned_type slot);
    void doubleK();
    void compactTree();
    void rebuildLoserTree();
    bool is_segment_empty(unsigned_type slot);
    void multi_merge_k(Element * target, unsigned_type length);

#if STXXL_PQ_INTERNAL_LOSER_TREE
    template <unsigned LogK>
    void multi_merge_f(Element * target, unsigned_type length)
    {
        //Entry *currentPos;
        //Element currentKey;
        //int currentIndex; // leaf pointed to by current entry
        Element * done = target + length;
        Entry * regEntry   = entry;
        Element * * regStates = current;
        unsigned_type winnerIndex = regEntry[0].index;
        Element winnerKey   = regEntry[0].key;
        Element * winnerPos;
        //Element sup = sentinel; // supremum

        assert(logK >= LogK);
        while (target != done)
        {
            winnerPos = regStates[winnerIndex];

            // write result
            *target   = winnerKey;

            // advance winner segment
            ++winnerPos;
            regStates[winnerIndex] = winnerPos;
            winnerKey = *winnerPos;

            // remove winner segment if empty now
            if (is_sentinel(winnerKey))
            {
                deallocate_segment(winnerIndex);
            }
            ++target;

            // update loser tree
#define TreeStep(L)\
if (1 << LogK >= 1 << L) { \
    Entry * pos ## L = regEntry + ((winnerIndex + (1 << LogK)) >> (((int (LogK - L) + 1) >= 0) ? ((LogK - L) + 1) : 0)); \
    Element key ## L = pos ## L->key; \
    if (cmp(winnerKey, key ## L)) { \
        unsigned_type index ## L  = pos ## L->index; \
        pos ## L->key   = winnerKey; \
        pos ## L->index = winnerIndex; \
        winnerKey     = key ## L; \
        winnerIndex   = index ## L; \
    } \
}
            TreeStep(10);
            TreeStep(9);
            TreeStep(8);
            TreeStep(7);
            TreeStep(6);
            TreeStep(5);
            TreeStep(4);
            TreeStep(3);
            TreeStep(2);
            TreeStep(1);
#undef TreeStep
        }
        regEntry[0].index = winnerIndex;
        regEntry[0].key   = winnerKey;
    }
#endif //STXXL_PQ_INTERNAL_LOSER_TREE

public:
    bool is_sentinel(const Element & a)
    {
        return !(cmp(cmp.min_value(), a));
    }
    bool not_sentinel(const Element & a)
    {
        return cmp(cmp.min_value(), a);
    }

public:
    loser_tree();
    ~loser_tree();
    void init();

    void swap(loser_tree & obj)
    {
        std::swap(cmp, obj.cmp);
        std::swap(free_slots, obj.free_slots);
        std::swap(size_, obj.size_);
        std::swap(logK, obj.logK);
        std::swap(k, obj.k);
        std::swap(sentinel, obj.sentinel);
#if STXXL_PQ_INTERNAL_LOSER_TREE
        swap_1D_arrays(entry, obj.entry, KNKMAX);
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
        swap_1D_arrays(current, obj.current, KNKMAX);
        swap_1D_arrays(current_end, obj.current_end, KNKMAX);
        swap_1D_arrays(segment, obj.segment, KNKMAX);
        swap_1D_arrays(segment_size, obj.segment_size, KNKMAX);
        std::swap(mem_cons_, obj.mem_cons_);
    }

    void multi_merge(Element * begin, Element * end)
    {
        multi_merge(begin, end - begin);
    }
    void multi_merge(Element *, unsigned_type length);

    unsigned_type mem_cons() const { return mem_cons_; }

    bool is_space_available() const // for new segment
    {
        return (k < KNKMAX) || !free_slots.empty();
    }

    void insert_segment(Element * target, unsigned_type length); // insert segment beginning at target
    unsigned_type size() { return size_; }
};

///////////////////////// LoserTree ///////////////////////////////////
template <class ValTp_, class Cmp_, unsigned KNKMAX>
loser_tree<ValTp_, Cmp_, KNKMAX>::loser_tree() : size_(0), logK(0), k(1), mem_cons_(0)
{
    free_slots.push(0);
    segment[0] = NULL;
    current[0] = &sentinel;
    current_end[0] = &sentinel;
    // entry and sentinel are initialized by init
    // since they need the value of supremum
    init();
}

template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::init()
{
    assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering
    sentinel      = cmp.min_value();
    rebuildLoserTree();
#if STXXL_PQ_INTERNAL_LOSER_TREE
    assert(current[entry[0].index] == &sentinel);
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
}


// rebuild loser tree information from the values in current
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::rebuildLoserTree()
{
#if STXXL_PQ_INTERNAL_LOSER_TREE
    assert(LOG2<KNKMAX>::floor == LOG2<KNKMAX>::ceil); // KNKMAX needs to be a power of two
    unsigned_type winner = initWinner(1);
    entry[0].index = winner;
    entry[0].key   = *(current[winner]);
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
}


#if STXXL_PQ_INTERNAL_LOSER_TREE
// given any values in the leaves this
// routing recomputes upper levels of the tree
// from scratch in linear time
// initialize entry[root].index and the subtree rooted there
// return winner index
template <class ValTp_, class Cmp_, unsigned KNKMAX>
unsigned_type loser_tree<ValTp_, Cmp_, KNKMAX>::initWinner(unsigned_type root)
{
    if (root >= k) { // leaf reached
        return root - k;
    } else {
        unsigned_type left  = initWinner(2 * root    );
        unsigned_type right = initWinner(2 * root + 1);
        Element lk    = *(current[left ]);
        Element rk    = *(current[right]);
        if (!(cmp(lk, rk))) { // right subtree loses
            entry[root].index = right;
            entry[root].key   = rk;
            return left;
        } else {
            entry[root].index = left;
            entry[root].key   = lk;
            return right;
        }
    }
}


// first go up the tree all the way to the root
// hand down old winner for the respective subtree
// based on new value, and old winner and loser
// update each node on the path to the root top down.
// This is implemented recursively
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::update_on_insert(
    unsigned_type node,
    const Element & newKey,
    unsigned_type newIndex,
    Element * winnerKey,
    unsigned_type * winnerIndex,       // old winner
    unsigned_type * mask)       // 1 << (ceil(log KNK) - dist-from-root)
{
    if (node == 0) { // winner part of root
        *mask = 1 << (logK - 1);
        *winnerKey   = entry[0].key;
        *winnerIndex = entry[0].index;
        if (cmp(entry[node].key, newKey))
        {
            entry[node].key   = newKey;
            entry[node].index = newIndex;
        }
    } else {
        update_on_insert(node >> 1, newKey, newIndex, winnerKey, winnerIndex, mask);
        Element loserKey   = entry[node].key;
        unsigned_type loserIndex = entry[node].index;
        if ((*winnerIndex & *mask) != (newIndex & *mask)) { // different subtrees
            if (cmp(loserKey, newKey)) { // newKey will have influence here
                if (cmp(*winnerKey, newKey) ) { // old winner loses here
                    entry[node].key   = *winnerKey;
                    entry[node].index = *winnerIndex;
                } else { // new entry loses here
                    entry[node].key   = newKey;
                    entry[node].index = newIndex;
                }
            }
            *winnerKey   = loserKey;
            *winnerIndex = loserIndex;
        }
        // note that nothing needs to be done if
        // the winner came from the same subtree
        // a) newKey <= winnerKey => even more reason for the other tree to lose
        // b) newKey >  winnerKey => the old winner will beat the new
        //                           entry further down the tree
        // also the same old winner is handed down the tree

        *mask >>= 1; // next level
    }
}
#endif //STXXL_PQ_INTERNAL_LOSER_TREE


// make the tree two times as wide
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::doubleK()
{
    STXXL_VERBOSE2("loser_tree::doubleK (before) k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " #free=" << free_slots.size());
    assert(k > 0);
    assert(k < KNKMAX);
    assert(free_slots.empty()); // stack was free (probably not needed)

    // make all new entries free
    // and push them on the free stack
    for (unsigned_type i = 2 * k - 1;  i >= k;  i--) // backwards
    {
        current[i] = &sentinel;
        current_end[i] = &sentinel;
        segment[i] = NULL;
        free_slots.push(i);
    }

    // double the size
    k *= 2;
    logK++;

    STXXL_VERBOSE2("loser_tree::doubleK (after)  k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " #free=" << free_slots.size());
    assert(!free_slots.empty());

    // recompute loser tree information
    rebuildLoserTree();
}


// compact nonempty segments in the left half of the tree
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::compactTree()
{
    STXXL_VERBOSE2("loser_tree::compactTree (before) k=" << k << " logK=" << logK << " #free=" << free_slots.size());
    assert(logK > 0);

    // compact all nonempty segments to the left
    unsigned_type pos = 0;
    unsigned_type last_empty = 0;
    for ( ;  pos < k;  pos++)
    {
        if (not_sentinel(*(current[pos])))
        {
            segment_size[last_empty] = segment_size[pos];
            current[last_empty] = current[pos];
            current_end[last_empty] = current_end[pos];
            segment[last_empty] = segment[pos];
            last_empty++;
        }/*
            else
            {
            if(segment[pos])
            {
            STXXL_VERBOSE2("loser_tree::compactTree() deleting segment "<<pos<<
                                    " address: "<<segment[pos]<<" size: "<<segment_size[pos]);
            delete [] segment[pos];
            segment[pos] = 0;
            mem_cons_ -= segment_size[pos];
            }
            }*/
    }

    // half degree as often as possible
    while ((k > 1) && ((k/2) >= last_empty))
    {
        k /= 2;
        logK--;
    }

    // overwrite garbage and compact the stack of free segment indices
    free_slots.clear(); // none free
    for ( ; last_empty < k; last_empty++)
    {
        current[last_empty] = &sentinel;
        current_end[last_empty] = &sentinel;
        free_slots.push(last_empty);
    }

    STXXL_VERBOSE2("loser_tree::compactTree (after)  k=" << k << " logK=" << logK << " #free=" << free_slots.size());

    // recompute loser tree information
    rebuildLoserTree();
}


// insert segment beginning at target
// require: is_space_available() == 1
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::insert_segment(Element * target, unsigned_type length)
{
    STXXL_VERBOSE2("loser_tree::insert_segment(" << target << "," << length << ")");
    //std::copy(target,target + length,std::ostream_iterator<ValTp_>(std::cout, "\n"));

    if (length > 0)
    {
        assert( not_sentinel(target[0])   );
        assert( not_sentinel(target[length - 1]));
        assert( is_sentinel(target[length]));

        // get a free slot
        if (free_slots.empty())
        { // tree is too small
            doubleK();
        }
        assert(!free_slots.empty());
        unsigned_type index = free_slots.top();
        free_slots.pop();


        // link new segment
        current[index] = segment[index] = target;
        current_end[index] = target + length;
        segment_size[index] = (length + 1) * sizeof(value_type);
        mem_cons_ += (length + 1) * sizeof(value_type);
        size_ += length;

#if STXXL_PQ_INTERNAL_LOSER_TREE
        // propagate new information up the tree
        Element dummyKey;
        unsigned_type dummyIndex;
        unsigned_type dummyMask;
        update_on_insert((index + k) >> 1, *target, index,
                        &dummyKey, &dummyIndex, &dummyMask);
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
    } else {
        // immediately deallocate
        // this is not only an optimization
        // but also needed to keep free segments from
        // clogging up the tree
        delete [] target;
    }
}


template <class ValTp_, class Cmp_, unsigned KNKMAX>
loser_tree<ValTp_, Cmp_, KNKMAX>::~loser_tree()
{
    STXXL_VERBOSE1("loser_tree::~loser_tree()");
    for (unsigned_type i = 0; i < k; ++i)
    {
        if (segment[i])
        {
            STXXL_VERBOSE2("loser_tree::~loser_tree() deleting segment " << i);
            delete [] segment[i];
            mem_cons_ -= segment_size[i];
        }
    }
    // check whether we have not lost any memory
    assert(mem_cons_ == 0);
}

// free an empty segment .
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::deallocate_segment(unsigned_type slot)
{
    // reroute current pointer to some empty sentinel segment
    // with a sentinel key
    STXXL_VERBOSE2("loser_tree::deallocate_segment() deleting segment " <<
                    slot << " address: " << segment[slot] << " size: " << (segment_size[slot] / sizeof(value_type)) - 1);
    current[slot] = &sentinel;
    current_end[slot] = &sentinel;

    // free memory
    delete [] segment[slot];
    segment[slot] = NULL;
    mem_cons_ -= segment_size[slot];

    // push on the stack of free segment indices
    free_slots.push(slot);
}


// delete the length smallest elements and write them to target
// empty segments are deallocated
// require:
// - there are at least length elements
// - segments are ended by sentinels
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::multi_merge(Element * target, unsigned_type length)
{
    STXXL_VERBOSE3("loser_tree::multi_merge(target=" << target << ", len=" << length << ") k=" << k);

    if (length == 0)
        return;

    assert(k > 0);
    assert(length <= size_);

    //This is the place to make statistics about internal multi_merge calls.

#if (defined(_GLIBCXX_PARALLEL) || defined(__MCSTL__)) && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
    priority_queue_local::invert_order<Cmp_, value_type, value_type> inv_cmp(cmp);
#endif
    switch (logK) {
    case 0:
        assert(k == 1);
#if STXXL_PQ_INTERNAL_LOSER_TREE
        assert(entry[0].index == 0);
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
        assert(free_slots.empty());
        memcpy(target, current[0], length * sizeof(Element));
        //std::copy(current[0], current[0] + length, target);
        current[0] += length;
#if STXXL_PQ_INTERNAL_LOSER_TREE
        entry[0].key = **current;
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
        if (is_segment_empty(0))
            deallocate_segment(0);

        break;
    case 1:
        assert(k == 2);
#if (defined(_GLIBCXX_PARALLEL) || defined(__MCSTL__)) && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        {
            std::pair<Element*, Element*> seqs[2] =
                                { std::make_pair(current[0], current_end[0]),
                                  std::make_pair(current[1], current_end[1]) };
            __STXXL_PQ_multiway_merge_sentinel(seqs, seqs + 2, target, inv_cmp, length);
            current[0] = seqs[0].first;
            current[1] = seqs[1].first;
        }
#else
        merge_iterator(current[0], current[1], target, length, cmp);
        rebuildLoserTree();
#endif
        if (is_segment_empty(0))
            deallocate_segment(0);

        if (is_segment_empty(1))
            deallocate_segment(1);

        break;
    case 2:
        assert(k == 4);
#if (defined(_GLIBCXX_PARALLEL) || defined(__MCSTL__)) && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        {
            std::pair<Element*, Element*> seqs[4] =
                                { std::make_pair(current[0], current_end[0]),
                                  std::make_pair(current[1], current_end[1]),
                                  std::make_pair(current[2], current_end[2]),
                                  std::make_pair(current[3], current_end[3]) };
            __STXXL_PQ_multiway_merge_sentinel(seqs, seqs + 4, target, inv_cmp, length);
            current[0] = seqs[0].first;
            current[1] = seqs[1].first;
            current[2] = seqs[2].first;
            current[3] = seqs[3].first;
        }
#else
        if (is_segment_empty(3))
            merge3_iterator(current[0], current[1], current[2], target, length, cmp);
        else
            merge4_iterator(current[0], current[1], current[2], current[3], target, length, cmp);

        rebuildLoserTree();
#endif
        if (is_segment_empty(0))
            deallocate_segment(0);

        if (is_segment_empty(1))
            deallocate_segment(1);

        if (is_segment_empty(2))
            deallocate_segment(2);

        if (is_segment_empty(3))
            deallocate_segment(3);

        break;
#if !((defined(_GLIBCXX_PARALLEL) || defined(__MCSTL__)) && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL)
    case  3: multi_merge_f < 3 > (target, length);
        break;
    case  4: multi_merge_f < 4 > (target, length);
        break;
    case  5: multi_merge_f < 5 > (target, length);
        break;
    case  6: multi_merge_f < 6 > (target, length);
        break;
    case  7: multi_merge_f < 7 > (target, length);
        break;
    case  8: multi_merge_f < 8 > (target, length);
        break;
    case  9: multi_merge_f < 9 > (target, length);
        break;
    case 10: multi_merge_f < 10 > (target, length);
        break;
#endif
    default:
#if (defined(_GLIBCXX_PARALLEL) || defined(__MCSTL__)) && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        {
            std::vector<std::pair<Element*, Element*> > seqs;
            std::vector<int_type> orig_seq_index;
            for(unsigned int i = 0; i < k; ++i)
            {
                if(current[i] != current_end[i] && !is_sentinel(*current[i]))
                {
                    seqs.push_back(std::make_pair(current[i], current_end[i]));
                    orig_seq_index.push_back(i);
                }
            }

            __STXXL_PQ_multiway_merge_sentinel(seqs.begin(), seqs.end(), target, inv_cmp, length);

            for(unsigned int i = 0; i < seqs.size(); ++i)
            {
                int_type seg = orig_seq_index[i];
                current[seg] = seqs[i].first;
            }

            for(unsigned int i = 0; i < k; ++i)
                if (is_segment_empty(i))
                {
                    STXXL_VERBOSE1("deallocated " << seg);
                    deallocate_segment(i);
                }
        }
#else
        multi_merge_k(target, length);
#endif
        break;
    }



    size_ -= length;

    // compact tree if it got considerably smaller
    {
        const unsigned_type num_segments_used = k - free_slots.size();
        const unsigned_type num_segments_trigger = k - (3 * k / 5);
        // using k/2 would be worst case inefficient (for large k)
        // for k \in {2, 4, 8} the trigger is k/2 which is good
        // because we have special mergers for k \in {1, 2, 4}
        // there is also a special 3-way-merger, that will be
        // triggered if k == 4 && is_segment_empty(3)
        STXXL_VERBOSE3("loser_tree  compact? k=" << k << " #used=" << num_segments_used
                        << " <= #trigger=" << num_segments_trigger << " ==> "
                        << ((k > 1 && num_segments_used <= num_segments_trigger) ? "yes" : "no ")
                        << " || "
                        << ((k == 4 && !free_slots.empty() && !is_segment_empty(3)) ? "yes" : "no ")
                        << " #free=" << free_slots.size());
        if (k > 1 && ((num_segments_used <= num_segments_trigger) ||
                        (k == 4 && !free_slots.empty() && !is_segment_empty(3))))
        {
            compactTree();
        }
    }
    //std::copy(target,target + length,std::ostream_iterator<ValTp_>(std::cout, "\n"));
}


// is this segment empty and does not point to sentinel yet?
template <class ValTp_, class Cmp_, unsigned KNKMAX>
inline bool loser_tree<ValTp_, Cmp_, KNKMAX>::is_segment_empty(unsigned_type slot)
{
    return (is_sentinel(*(current[slot])) && (current[slot] != &sentinel));
}

#if STXXL_PQ_INTERNAL_LOSER_TREE
// multi-merge for arbitrary K
template <class ValTp_, class Cmp_, unsigned KNKMAX>
void loser_tree<ValTp_, Cmp_, KNKMAX>::
multi_merge_k(Element * target, unsigned_type length)
{
    Entry * currentPos;
    Element currentKey;
    unsigned_type currentIndex; // leaf pointed to by current entry
    unsigned_type kReg = k;
    Element * done = target + length;
    unsigned_type winnerIndex = entry[0].index;
    Element winnerKey   = entry[0].key;
    Element * winnerPos;

    while (target != done)
    {
        winnerPos = current[winnerIndex];

        // write result
        *target   = winnerKey;

        // advance winner segment
        ++winnerPos;
        current[winnerIndex] = winnerPos;
        winnerKey = *winnerPos;

        // remove winner segment if empty now
        if (is_sentinel(winnerKey))  //
            deallocate_segment(winnerIndex);


        // go up the entry-tree
        for (unsigned_type i = (winnerIndex + kReg) >> 1;  i > 0;  i >>= 1) {
            currentPos = entry + i;
            currentKey = currentPos->key;
            if (cmp(winnerKey, currentKey)) {
                currentIndex      = currentPos->index;
                currentPos->key   = winnerKey;
                currentPos->index = winnerIndex;
                winnerKey         = currentKey;
                winnerIndex       = currentIndex;
            }
        }

        ++target;
    }
    entry[0].index = winnerIndex;
    entry[0].key   = winnerKey;
}
#endif //STXXL_PQ_INTERNAL_LOSER_TREE
} //priority_queue_local

//! \}

__STXXL_END_NAMESPACE

#endif
