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


TEST_CASE("Testing AbstractSet") {

    //AbstractSet<typename T>
    std::unordered_set<int> set;
    Observer::AbstractSet<int> Set;
    
    //Append
    auto _append = [&set](int i){ return set.insert(i).second; };
    Set.Append = _append;
    CHECK(Set.Append(3)==true);
    CHECK(set.count(3)==1);
    CHECK(set.size()==1);
    CHECK(Set.Append(3)==false);//already in

    //Remove
    set.insert(1);
    auto _remove = [&set](int i){ return set.erase(i)==1; };
    CHECK(set.count(1)==1);
    Set.Remove = _remove;
    CHECK(Set.Remove(1)==true);
    CHECK(set.count(1)==0);
    CHECK(Set.Remove(2)==false);//never added

    //Signal
    using F = std::function<void(int)>;
    auto _signal = [&set](F f){ for(auto t : set) f(t); };
    Set.Signal = _signal;
    int n = 0;
    auto f = [&n](int i){ n+=i; };
    Set.Signal(f);
    CHECK(n==3);
    CHECK(Set.Append(1)==true);
    Set.Signal(f);
    CHECK(n==7);
    CHECK(Set.Remove(1)==true);

    //constructing with abstract_set_view
    Set = Observer::abstract_set_view(set);
    CHECK(set.size()==1);//3 is still in
    CHECK(Set.Remove(2)==false);//not there
    CHECK(Set.Remove(3)==true);
    CHECK(Set.Append(1)==true);
    CHECK(set.count(1)>0);
    n = 2;
    auto g = [&n](int i){ n+=2*i; };
    Set.Signal(g);
    CHECK(n==4);

}


TEST_CASE("Testing AbstractMap") {

    //AbstractMap<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractMap<char,int> Map;
    
    //Define
    auto _define = [&map](char c, int i) { map.insert_or_assign(c,i); return true; };
    Map.Define = _define;
    CHECK(Map.Define('a',1)==true);
    CHECK(map.count('a')>0);
    CHECK(map.size()==1);
    CHECK(Map.Define('a',2)==true);
    CHECK(map.size()==1);
    CHECK(map['a']==2);
    
    //Remove
    map.insert_or_assign('b',3);
    auto _remove = [&map](char i) { return map.erase(i)==1; };
    CHECK(map.count('b')==1);
    Map.Remove = _remove;
    CHECK(Map.Remove('b')==true);
    CHECK(map.count('b')==0);
    CHECK(Map.Remove('b')==false);//already removed
    CHECK(Map.Remove('c')==false);//never defined

    //Signal
    using F = std::function<void(char,int)>;
    auto _signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    Map.Signal = _signal;
    int n = 0;
    auto f = [&n](char, int i){ n += i; };
    Map.Signal(f);
    CHECK(map.size()==1);
    CHECK(map.count('a')>0);
    CHECK(n==2);
    CHECK(Map.Define('b',5));
    Map.Signal(f);
    CHECK(n==9);

    //Lookup
    auto _lookup = [&map](char c){ return map.at(c); };
    Map.Lookup = _lookup;
    CHECK(Map.Lookup('a')==2);
    CHECK(Map.Lookup('b')==5);
    CHECK_THROWS(Map.Lookup('c'));//Never defined; map.at throws;

    //constructing from abstract_map_view
    Map = Observer::abstract_map_view(map);
    CHECK(map.size()==2);
    CHECK(Map.Remove('c')==false);//not there
    CHECK(Map.Remove('b')==true);
    CHECK(map.count('a')>0);//still here, with value 2
    CHECK(Map.Define('c',3)==true);//new entry
    n = 1;
    auto g = [&n](char, int i){ n+=2*i; };
    Map.Signal(g);
    CHECK(n==(1+2*2+2*3));

}


TEST_CASE("Testing AbstractLookup") {

    //AbstractLookup<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractLookup<char,int> Lookup;
    map['a'] = 0;
    map['b'] = 1;

    //Lookup
    auto _lookup = [&map](char c){ return map.at(c); };
    Lookup.Lookup = _lookup;
    CHECK(Lookup.Lookup('a')==0);
    CHECK(Lookup.Lookup('b')==1);
    CHECK_THROWS(Lookup.Lookup('c'));//Never defined; map.at throws;

    //constructing form abstract_lookup_view
    Lookup = Observer::abstract_lookup_view(map);
    CHECK(Lookup.Lookup('a')==0);//indeed
    Lookup.Lookup = [&map](char c){ return map.at(c)*2; };
    CHECK(Lookup.Lookup('b')==2);

}


template <typename T>
struct TestNoCopy
{
    static constexpr bool AssertNoCopy() {
        return !std::is_copy_constructible<T>::value && !std::is_copy_assignable<T>::value;
    }
    
    static constexpr bool AssertNoMove() {
        return !std::is_move_constructible<T>::value && !std::is_move_assignable<T>::value;
    }
};


TEST_CASE("Testing _Subject1 / Attach, Detach") {

    //_Subject1<typename E, typename _SubjectEvents>
    struct A { int value; };
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    using _subject1_t = Observer::_Subject1<A,events_t>;
    using _observer1_t = Observer::_Observer1<A,events_t>;

    //static object
    CHECK(TestNoCopy<_subject1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_subject1_t>::AssertNoMove());
   
    //Attach
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    _observer1_t obs; 
    CHECK(set.size()==0);
    CHECK(subA.Attach(&obs)==true);
    CHECK(set.size()==1);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(set.size()==2);
    CHECK(subA.Attach(&obs2)==false);//already attached fails
    CHECK(set.size()==2);
    
    //Detach
    CHECK(subA.Detach(&obs)==true);
    CHECK(set.size()==1);
    _observer1_t obs3;
    CHECK(subA.Detach(&obs3)==false);//never attached fails
    CHECK(set.size()==1);

}


TEST_CASE("Testing _Observer1 / Notify") {

    //_Observer1<typename E, typename _SubjectEvents>
    struct A { int value; };
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    using _subject1_t = Observer::_Subject1<A,events_t>;
    using _observer1_t = Observer::_Observer1<A,events_t>;

    //static object
    CHECK(TestNoCopy<_observer1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_observer1_t>::AssertNoMove());
    
    //observer onEvent
    _observer1_t obs;
    obs.onEvent(A{1});

    //observer bindHandler;
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    auto h2 = [&n](A a){ n+=a.value*2; };
    obs.bindHandler(h).bindHandler(h2);//h2 should be in effect
    obs.onEvent(A{1});
    CHECK(n==2);
    obs.onEvent(A{2});
    CHECK(n==6);

    //through Notify
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    CHECK(subA.Attach(&obs)==true);
    subA.Notify(A{3});
    CHECK(n==12);
    obs.bindHandler(h);
    subA.Notify(A{4});
    CHECK(n==16);

    //Notify to multiple observers
    _observer1_t obs2;
    obs2.bindHandler(h2);
    CHECK(subA.Attach(&obs2)==true);
    n = 0;
    subA.Notify(A{5});
    CHECK(n==15);
    CHECK(subA.Detach(&obs2)==true);
    subA.Notify(A{5});
    CHECK(n==20); 

    //observer bindSubjectHandlers;
    std::unordered_set<_observer1_t*> set2;
    _subject1_t subA2(Observer::abstract_set_view(set2));
    CHECK(subA2.Attach(&obs)==true);
    n = 0;
    subA2.Notify(A{3});
    CHECK(n==3);//h is active
    using handler_t = std::function<void(A)>;
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;
    subject_handlers[&subA] = h;
    subject_handlers[&subA2] = h2;
    obs.bindSubjectHandlers(Observer::abstract_lookup_view(subject_handlers));
    obs.onEvent(A{2},&subA);
    CHECK(n==5);
    obs.onEvent(A{3},&subA2);
    CHECK(n==11);
    subA2.Notify(A{4});
    CHECK(n==19);

}


TEST_CASE("Testing _Subject1 / backend rebinding") {

    //_Subject1<typename E, typename _SubjectEvents>
    struct A { int value; };
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>; 
    using _subject1_t = Observer::_Subject1<A,events_t>;
    using _observer1_t = Observer::_Observer1<A,events_t>;

    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    _observer1_t obs; 
    CHECK(set.size()==0);
    CHECK(subA.Attach(&obs)==true);
    CHECK(set.size()==1);
    auto empty = Observer::AbstractSet<_observer1_t*>();
    subA.bindObserverSet(empty);
    CHECK_THROWS(subA.Attach(&obs));
    auto old_one = Observer::abstract_set_view(set);
    subA.bindObserverSet(empty).bindObserverSet(old_one);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(subA.Attach(&obs)==false);//already here

}


template <typename O, typename E>
struct onEventExists
{
    template <typename T = E, void (O::*)(T) = &O::template onEvent<T>>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};

TEST_CASE("Testing _Subject / Create") {

    //_Subject<typename _SubjectEvents>

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;

    //static object
    CHECK(TestNoCopy<Observer::_Subject<events_t>>::AssertNoCopy());
    CHECK(TestNoCopy<Observer::_Subject<events_t>>::AssertNoMove());

}


TEST_CASE("Testing _Subject / Subscribe") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    
    using subject_t = Observer::_Subject<events_t>;
    using observer_t = Observer::_Observer<events_t>;
    
    subject_t sub;
    observer_t obs;
    //good event index succeeds:
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.Attach(&obs,1) == true);
    //bad event index fails
    CHECK(sub.Attach(&obs,2) == false);
    //know current registration of observer
    CHECK(sub.nEvents(&obs)==2);
    //already event does nothing;
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 2);

    //multiple subscription
    observer_t obs2;
    //any bad event -> global fail
    CHECK(sub.Attach(&obs2,{0,2}) == false);
    CHECK(sub.nEvents(&obs2) == 0);
    //all good -> succeed
    CHECK(sub.Attach(&obs2,{1}) == true);
    CHECK(sub.nEvents(&obs2) == 1);
    //all good / redundant -> succeed
    CHECK(sub.Attach(&obs2,{1,1}) == true);
    CHECK(sub.nEvents(&obs2) == 1);

}


TEST_CASE("Testing _Subject / Unsubscribe") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    
    using subject_t = Observer::_Subject<events_t>;
    using observer_t = Observer::_Observer<events_t>;

    subject_t sub;
    observer_t obs;
    CHECK(sub.Attach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 1);
    //bad event fails (count not changed) 
    CHECK(sub.Detach(&obs,2) == false);
    CHECK(sub.nEvents(&obs) == 1);
    //unsubscribed event succeeds (count not changed) 
    CHECK(sub.Detach(&obs,1) == true);
    CHECK(sub.nEvents(&obs) == 1);
    //subscribed event succeeds (count changed)
    CHECK(sub.Detach(&obs,0) == true);
    CHECK(sub.nEvents(&obs) == 0);
    //unregistered observer succeeds
    observer_t obs2;
    CHECK(sub.Detach(&obs2,0) == true);
    CHECK(sub.nEvents(&obs2) == 0);

    //multiple unsubscribe
    observer_t obs3;
    CHECK(sub.Attach(&obs3,{0,1}) == true);
    //any bad event -> global fail (count not changed)
    CHECK(sub.Detach(&obs3,{1,2}) == false);
    CHECK(sub.nEvents(&obs3) == 2);
    //all good -> succeed
    CHECK(sub.Detach(&obs3,{0}) == true);
    CHECK(sub.nEvents(&obs3) == 1);
    //all good / redundant -> succeed
    CHECK(sub.Detach(&obs3,{1,1}) == true);
    CHECK(sub.nEvents(&obs3) == 0);

}


TEST_CASE("Testing _Observer / Create") {

    //_Observer<typename _SubjectEvents>

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;

    //static object
    CHECK(TestNoCopy<Observer::_Observer<events_t>>::AssertNoCopy());
    CHECK(TestNoCopy<Observer::_Observer<events_t>>::AssertNoMove());

}




TEST_CASE("Testing _Observer / onEvent methods") {

    struct A {};
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    struct C {};
    using O = Observer::_Observer<events_t>;

    //methods exist
    CHECK(onEventExists<O,A>::value == true);
    CHECK(onEventExists<O,B>::value == true);
    CHECK(onEventExists<O,C>::value == false);

}


template <typename E, typename _SubjectEvents>
struct bindHandlerExists
{
    using O = Observer::_Observer<_SubjectEvents>;
    
    template <typename T = E, void (O::*)(std::function<void(T)>*) = &O::template bindHandler<T>>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};


TEST_CASE("Testing _Observer / handlers") {

    struct A { size_t value; };
    struct B {};
    using events_t = Observer::SubjectEvents<A,B>;
    struct C {};

    //binding methods exist
    CHECK(bindHandlerExists<A,events_t>::value == true);
    CHECK(bindHandlerExists<B,events_t>::value == true);
    CHECK(bindHandlerExists<C,events_t>::value == false);

    using observer_t = Observer::_Observer<events_t>;
    
    //handler for event A
    size_t accA = 0;
    std::function<void(A)> handlerA = [&accA](A a){ accA += a.value; };
    observer_t obs;
    obs.bindHandler(&handlerA);
    CHECK(accA == 0);
    obs.onEvent(A{2});
    CHECK(accA == 2);
    obs.onEvent(A{3});
    CHECK(accA == 5);

    //redefine handlerA
    handlerA = [&accA](A a){ accA -= a.value; };
    obs.onEvent(A{2});
    CHECK(accA == 3);

    //add handler for B
    size_t nB = 0;
    std::function<void(B)> handlerB = [&nB](B){ ++nB; };
    obs.bindHandler(&handlerB);
    CHECK(nB == 0);
    obs.onEvent(B());
    CHECK(nB == 1);

    //handler for A still works
    obs.onEvent(A{2});
    CHECK(accA == 1);

    //handler and virtual calls
    struct H { virtual void event(A) {} };
    struct h : H {
        size_t value = 0;
        virtual void event(A a) override { value += a.value; }
    };
    auto pObjH = std::unique_ptr<H>(new h());
    handlerA = [&pObjH](A a){ pObjH->event(a); };
    obs.onEvent(A{2});
    CHECK(static_cast<h*>(pObjH.get())->value == 2);
    
}
