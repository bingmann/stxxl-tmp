/***************************************************************************
 *  containers/stack_benchmark.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/stack_benchmark.cpp
//! This is a benchmark mentioned in the paper
//! R. Dementiev, L. Kettner, P. Sanders "STXXL: standard template library for XXL data sets"
//! Software: Practice and Experience
//! Volume 38, Issue 6, Pages 589-637, May 2008
//! DOI: 10.1002/spe.844


#include <stxxl/stack>

#define MEM_2_RESERVE    (768 * 1024 * 1024)

#ifndef BLOCK_SIZE
 #define    BLOCK_SIZE (2 * 1024 * 1024)
#endif

#ifndef DISKS
 #define DISKS 4
#endif

template <unsigned RECORD_SIZE>
struct my_record_
{
    char data[RECORD_SIZE];
    my_record_()
    {
        memset(data, sizeof(data), 0);
    }
};

template <unsigned RECORD_SIZE>
inline std::ostream & operator << (std::ostream & o, const my_record_<RECORD_SIZE> &)
{
    o << ".";
    return o;
}

template <class my_record>
void run_stxxl_growshrink2_stack(stxxl::int64 volume)
{
    typedef typename stxxl::STACK_GENERATOR<my_record, stxxl::external,
                                            stxxl::grow_shrink2, DISKS, BLOCK_SIZE>::result stack_type;
    typedef typename stack_type::block_type block_type;

    STXXL_MSG("Record size: " << sizeof(my_record) << " bytes");

    stxxl::prefetch_pool<block_type> p_pool(DISKS * 4);
    stxxl::write_pool<block_type> w_pool(DISKS * 4);

    stack_type Stack(p_pool, w_pool);

    stxxl::int64 ops = volume / sizeof(my_record);

    stxxl::int64 i;

    my_record cur;

    stxxl::stats * Stats = stxxl::stats::get_instance();
    Stats->reset();

    stxxl::timer Timer;
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.push(cur);
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (i != Stack.size())
    {
        STXXL_MSG("Size does not match");
        abort();
    }

    STXXL_MSG("Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    std::cout << *Stats;
    Stats->reset();

    Stack.set_prefetch_aggr(DISKS * 8);

    ////////////////////////////////////////////////
    Timer.reset();
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.pop();
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (!Stack.empty())
    {
        STXXL_MSG("Stack must be empty");
        abort();
    }

    STXXL_MSG("Deletions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    std::cout << *Stats;
}


template <class my_record>
void run_stxxl_normal_stack(stxxl::int64 volume)
{
    typedef typename stxxl::STACK_GENERATOR<my_record, stxxl::external,
                                            stxxl::normal, DISKS, BLOCK_SIZE>::result stack_type;
    typedef typename stack_type::block_type block_type;

    STXXL_MSG("Record size: " << sizeof(my_record) << " bytes");

    stack_type Stack;

    stxxl::int64 ops = volume / sizeof(my_record);

    stxxl::int64 i;

    my_record cur;

    stxxl::stats * Stats = stxxl::stats::get_instance();
    Stats->reset();

    stxxl::timer Timer;
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.push(cur);
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (i != Stack.size())
    {
        STXXL_MSG("Size does not match");
        abort();
    }

    STXXL_MSG("Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    std::cout << *Stats;
    Stats->reset();


    ////////////////////////////////////////////////
    Timer.reset();
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.pop();
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (!Stack.empty())
    {
        STXXL_MSG("Stack must be empty");
        abort();
    }

    STXXL_MSG("Deletions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    std::cout << *Stats;
}


int main(int argc, char * argv[])
{
    STXXL_MSG("stxxl::pq block size: " << BLOCK_SIZE << " bytes");

#ifdef STXXL_DIRECT_IO_OFF
    STXXL_MSG("STXXL_DIRECT_IO_OFF is defined");
#else
    STXXL_MSG("STXXL_DIRECT_IO_OFF is NOT defined");
#endif

    if (argc < 3)
    {
        STXXL_MSG("Usage: " << argv[0] << " version #volume");
        STXXL_MSG("\t version = 1: grow-shrink-stack2 with 4 byte records");
        STXXL_MSG("\t version = 2: grow-shrink-stack2 with 32 byte records");
        STXXL_MSG("\t version = 3: normal-stack with 4 byte records");
        STXXL_MSG("\t version = 4: normal-stack with 32 byte records");
        return 0;
    }

    int version = atoi(argv[1]);
#ifdef BOOST_MSVC
    stxxl::int64 volume = _atoi64(argv[2]);
#else
    stxxl::int64 volume = atoll(argv[2]);
#endif

    STXXL_MSG("Allocating array with size " << MEM_2_RESERVE
                                            << " bytes to prevent file buffering.");
    int * array = new int[MEM_2_RESERVE / sizeof(int)];
    std::fill(array, array + (MEM_2_RESERVE / sizeof(int)), 0);

    STXXL_MSG("Running version: " << version);
    STXXL_MSG("Data volume    : " << volume << " bytes");

    switch (version)
    {
    case 1:
        run_stxxl_growshrink2_stack<my_record_<4> >(volume);
        break;
    case 2:
        run_stxxl_growshrink2_stack<my_record_<32> >(volume);
        break;
    case 3:
        run_stxxl_normal_stack<my_record_<4> >(volume);
        break;
    case 4:
        run_stxxl_normal_stack<my_record_<32> >(volume);
        break;
    default:
        STXXL_MSG("Unsupported version " << version);
    }

    delete[] array;
}
