// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_ARGS_H
#define BASIC_ARGS_H

namespace basic {

// The following structure allows passing pointer or reference arguments
template <typename T>
class ptr_or_ref
{
  private:
    T * p;

  public:
    // (implicit) cast from pointer
    ptr_or_ref(T * const np = 0):p(np) { }

    // (implicit) cast from reference
    ptr_or_ref(T & np):p(&np) { }

    // Default copy constructor

    // Default assignment operator

    // Default destructor

    // Provide implicit cast back to pointer and reference
    operator T *() const { return p; }
    operator T &() const { return *p; }

    // Accessor functions
    T * ptr() const { return p; }
    T & ref() const { return *p; }
};

// The following structure is the same, but allows passing pointer or reference arguments of two different types
template <typename T, typename R>
class ptr_or_ref_2
{
  private:
    T * p1;
    R * p2;

  public:
    // Default constructor: both pointers null
    ptr_or_ref_2():p1(0), p2(0) { }

    // (implicit) cast from pointers
    ptr_or_ref_2(T * const np):p1(np), p2(0) { }
    ptr_or_ref_2(R * const np):p1(0), p2(np) { }

    // (implicit) cast from references
    ptr_or_ref_2(T & np):p1(&np), p2(0) { }
    ptr_or_ref_2(R & np):p1(0), p2(&np) { }

    // Default copy constructor

    // Default assignment operator

    // Default destructor

    // Accessor functions
    T * ptr1() const { return p1; }
    R * ptr2() const { return p2; }
    T & ref1() const { return *p1; }
    R & ref2() const { return *p2; }
};

}

#endif
