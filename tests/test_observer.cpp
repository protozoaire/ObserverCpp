#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../observer.hpp"


TEST_CASE("Testing _Count") {
    
    //_Count<typename E, typename ... List>
    struct E;
    CHECK(Observer::_Count<E>::value == 0);
    CHECK(Observer::_Count<E,E>::value == 1);
    CHECK(Observer::_Count<E,E,E>::value == 2);
    struct A;
    CHECK(Observer::_Count<E,A>::value == 0);
    CHECK(Observer::_Count<E,A,E>::value == 1);
    CHECK(Observer::_Count<E,E,A>::value == 1);
    CHECK(Observer::_Count<E,A,E,E>::value == 2);
    CHECK(Observer::_Count<E,A,E,A>::value == 1);
    CHECK(Observer::_Count<E,A,E,A,E>::value == 2);

}


TEST_CASE("Testing _EachOnce") {
    
    //_EachOnce<typename ...>
    CHECK(Observer::_EachOnce<>::value == true);
    struct A;
    CHECK(Observer::_EachOnce<A>::value == true);
    CHECK(Observer::_EachOnce<A,A>::value == false);
    CHECK(Observer::_EachOnce<A,A,A>::value == false);
    struct B;
    CHECK(Observer::_EachOnce<A,B>::value == true);
    CHECK(Observer::_EachOnce<A,B,A>::value == false);

}


TEST_CASE("Testing _ForEach") {

    //_ForEach<template <typename> class Pred, typename ... >
    CHECK(Observer::_ForEach<std::is_pointer>::value == true);
    struct A;
    CHECK(Observer::_ForEach<std::is_pointer,A>::value == false);
    CHECK(Observer::_ForEach<std::is_pointer,A*>::value == true);
    CHECK(Observer::_ForEach<std::is_pointer,A*,A>::value == false);
    CHECK(Observer::_ForEach<std::is_pointer,A*,A*>::value == true);

}


TEST_CASE("Testing _IsDefined") {

    //_IsDefined<typename T>
    struct A;
    CHECK(Observer::_IsDefined<A>::value == false);
    struct B {};
    CHECK(Observer::_IsDefined<B>::value == true);

}


TEST_CASE("Testing SubjectEvents") {

    //SubjectEvents<typename ... Events>
    
    struct A;
    CHECK(Observer::SubjectEvents<A,A>::AssertNoDuplicate() == false);
    struct B;
    CHECK(Observer::SubjectEvents<A,B>::AssertNoDuplicate() == true);
   
    CHECK(Observer::SubjectEvents<A>::AssertAllDefined() == false);
    struct C {};
    CHECK(Observer::SubjectEvents<C>::AssertAllDefined() == true);
    CHECK(Observer::SubjectEvents<C,A>::AssertAllDefined() == false);
    CHECK(Observer::SubjectEvents<C,C>::AssertAllDefined() == true);

}


TEST_CASE("Testing IsSupportedEvent") {

    //IsSupportedEvent<typename E, typename _SubjectEvents>
    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    struct C {};

    CHECK(Observer::IsSupportedEvent<A,events_t>::value == true);
    CHECK(Observer::IsSupportedEvent<B,events_t>::value == true);
    CHECK(Observer::IsSupportedEvent<C,events_t>::value == false);

}


TEST_CASE("Testing EventIndex") {

    //EventIndex<typename E, typename _SubjectEvents>
    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    
    CHECK(Observer::EventIndex<A,events_t>::value == 0);
    CHECK(Observer::EventIndex<B,events_t>::value == 1);

}
