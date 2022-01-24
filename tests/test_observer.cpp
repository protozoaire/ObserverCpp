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


struct ASVs
{
    template <typename T>
    Observer::AbstractSet<T> operator()(std::unordered_set<T>&)
    {
        return {};
    }
};


TEST_CASE("Testing AbstractSetTuple") {

    //AbstractSetTuple<typename ... Ts>
    std::unordered_set<int> set_int;
    std::unordered_set<char> set_char;

    //ctor from already Abstract sets
    auto Set_int = Observer::abstract_set_view(set_int);
    auto Set_char = Observer::abstract_set_view(set_char);
    Observer::AbstractSetTuple<int,char> TSet(Set_int,Set_char);

    //ction using abstract_set_tuple_view
    //1. supported impls
    TSet = Observer::abstract_set_tuple_view(set_int,set_char);
    //2. tuple of supported impls
    std::tuple<std::unordered_set<int>, std::unordered_set<char>> tset;
    TSet = Observer::abstract_set_tuple_view(tset);
    //3. custom impls
    TSet = Observer::abstract_set_tuple_view<ASVs>(set_int,set_char);
    //4. tuple of custom impls
    TSet = Observer::abstract_set_tuple_view<ASVs>(tset);

    //Ops
    TSet.Set(Set_int).Set(Set_char);//chaining Set(.)
    auto Set_int2 = TSet.Get(int());//Get
    
}


TEST_CASE("Testing AbstractSetDataTuple") {

    //AbstractSetTuple<typename ... Ts>
    using TSet_t = Observer::AbstractSetDataTuple<int,char>;
    //ction from already AbstractSetData;
    {
        std::unordered_set<int> set_int;
        std::unordered_set<char> set_char;
        
        auto Set_int = Observer::abstract_set_data(std::move(set_int));
        auto Set_char = Observer::abstract_set_data(std::move(set_char));
        TSet_t TSet(std::move(Set_int),std::move(Set_char));
    }

    //ction using abstract_set_tuple_data
    //1. supported impls
    {
        std::unordered_set<int> set_int;
        std::unordered_set<char> set_char;

        TSet_t TSet = Observer::abstract_set_tuple_data(std::move(set_int),std::move(set_char));
    }
    //2. tuple of supported impls
    {
        std::tuple<std::unordered_set<int>, std::unordered_set<char>> tset;
        TSet_t TSet = Observer::abstract_set_tuple_data(std::move(tset));
    }
    //3. custom impls
    {
        std::unordered_set<int> set_int;
        std::unordered_set<char> set_char;
        TSet_t TSet = Observer::abstract_set_tuple_data<ASVs>(std::move(set_int),std::move(set_char));
    }
    //4. tuple of custom impls
    {
        std::tuple<std::unordered_set<int>, std::unordered_set<char>> tset;
        TSet_t TSet = Observer::abstract_set_tuple_data<ASVs>(std::move(tset));
    }
    //Ops
    {
        auto Set_int = Observer::abstract_set_data(std::unordered_set<int>());
        auto Set_char = Observer::abstract_set_data(std::unordered_set<char>());
        TSet_t TSet;
        TSet.Set(std::move(Set_int)).Set(std::move(Set_char));//chaining Set(.)
        auto const& Set_int2 = TSet.Get(int());//Get
        (void) Set_int2;
    }

}


TEST_CASE("Testing AbstractMap") {

    //AbstractMap<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractMap<char,int> Map;
    
    //Define
    auto _define = [&map](char c, int i) { map.insert_or_assign(c,i); };
    Map.Define = _define;
    Map.Define('a',1);
    CHECK(map.count('a')>0);
    CHECK(map.size()==1);
    Map.Define('a',2);
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
    Map.Define('b',5);
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
    Map.Define('c',3);//new entry
    n = 1;
    auto g = [&n](char, int i){ n+=2*i; };
    Map.Signal(g);
    CHECK(n==(1+2*2+2*3));

}


TEST_CASE("Testing AbstractConstMap") {

    //AbstractConstMap<typename K, typename V>
    std::unordered_map<char,int> map;
    Observer::AbstractConstMap<char,int> Map;
    map['a'] = 0;
    map['b'] = 1;

    //Lookup
    auto _lookup = [&map](char c){ return map.at(c); };
    Map.Lookup = _lookup;
    CHECK(Map.Lookup('a')==0);
    CHECK(Map.Lookup('b')==1);
    CHECK_THROWS(Map.Lookup('c'));//Never defined; map.at throws;

    //constructing form abstract_lookup_view
    Map = Observer::abstract_const_map_view(map);
    CHECK(Map.Lookup('a')==0);//indeed
    Map.Lookup = [&map](char c){ return map.at(c)*2; };
    CHECK(Map.Lookup('b')==2);
 
    //Signal
    using F = std::function<void(char,int)>;
    auto _signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    Map.Signal = _signal;
    int n = 0;
    auto f = [&n](char, int i){ n += i; };
    CHECK(map.size()==2);
    map['a'] = 1;
    map['b'] = 5;
    Map.Signal(f);
    CHECK(n==6);

    //constructing form abstract_lookup_view
    map.clear(); map['a'] = 3;
    Map = Observer::abstract_const_map_view(map);
    CHECK(Map.Lookup('a')==3);//indeed
    CHECK_THROWS(Map.Lookup('b'));

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


TEST_CASE("Testing _Subject1[view] / Attach, Detach") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

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


TEST_CASE("Testing _Subject1[data] / Attach, Detach") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //static object
    CHECK(TestNoCopy<_subject1_t>::AssertNoCopy());
    CHECK(TestNoCopy<_subject1_t>::AssertNoMove());
   
    //Attach
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::AbstractSetData<_observer1_t*>(std::move(set)));
    _observer1_t obs; 
    CHECK(subA.Attach(&obs)==true);
    _observer1_t obs2;
    CHECK(subA.Attach(&obs2)==true);
    CHECK(subA.Attach(&obs2)==false);//already attached fails
    
    //Detach
    CHECK(subA.Detach(&obs)==true);
    _observer1_t obs3;
    CHECK(subA.Detach(&obs3)==false);//never attached fails

}


TEST_CASE("Testing _Observer1 / Notify") {

    //_Observer1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

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
    obs.bindHandlerSubject1(h).bindHandlerSubject1(h2);//h2 should be in effect
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
    obs.bindHandlerSubject1(h);
    subA.Notify(A{4});
    CHECK(n==16);

    //Notify to multiple observers
    _observer1_t obs2;
    obs2.bindHandlerSubject1(h2);
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
    obs.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
    obs.onEvent(A{2},&subA);
    CHECK(n==5);
    obs.onEvent(A{3},&subA2);
    CHECK(n==11);
    subA2.Notify(A{4});
    CHECK(n==19);

}


TEST_CASE("Testing _Observer1 / auto-Detach") {

//_Observer1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;
    
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;
    subject_handlers[&subA] = h;
    

    //notify ObserverClose
    //1. no handler -> no Detach
    set.clear(); 
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
    }
    CHECK(set.size()==1);
    //2. unique handler -> no Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindHandlerSubject1(h);
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==1);
    //3. subject_handlers -> Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==0);
    //4. subject_handlers then unique handler -> no Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        obs3.bindHandlerSubject1(h);
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==1);
    //4. subject_handlers then unique handler and unique Subject -> Detach
    set.clear();
    n = 0;
    {
        _observer1_t obs3;
        subA.Attach(&obs3);
        obs3.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        obs3.bindHandlerSubject1(h,Observer::SubjectID<A>(&subA));
        subA.Notify(A{4});
        CHECK(n==4);
    }
    CHECK(set.size()==0);

}


TEST_CASE("Testing _Subject1 / backend rebinding") {

    //_Subject1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;

    //impl: view
    using set_t = std::unordered_set<_observer1_t*>;
    set_t set;
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

    //impl: data
    _subject1_t subA2(Observer::AbstractSetData<_observer1_t*>(std::move(set)));
    CHECK(subA2.Attach(&obs)==false);//already here
    subA.bindObserverSet(Observer::abstract_set_data(set_t()));
    set = set_t();
    set.insert(&obs);
    subA.bindObserverSet(Observer::abstract_set_data(std::move(set)));
    CHECK(subA.Attach(&obs)==false);//already here

}


TEST_CASE("Testing _Observer1 / backend rebinding, const ops") {

    //_Observer1<typename E>
    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;

    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;
    
    //single handler
    {
        _observer1_t obs;
        subA.Attach(&obs);
        obs.bindHandlerSubject1(h);
        subA.Notify(A{3});
        CHECK(n==3);
    }
    CHECK(set.size()==1);//no auto-detach
    set.clear();
    n = 0;

    //map view
    {
        _observer1_t obs;
        obs.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        subject_handlers[&subA] = h;
        subA.Attach(&obs);
        subA.Notify(A{3});
        CHECK(n==3);
        obs.bindSubjectHandlers(Observer::abstract_const_map_view(subject_handlers));
        subA.Notify(A{2});
        CHECK(n==5);
    }
    CHECK(set.size()==0);//auto-detach
    n = 0;

    //map data
    {
        std::unordered_map<_subject1_t*,handler_t> sh;
        sh[&subA] = h;
        auto sh2 = sh;
        _observer1_t obs;
        obs.bindSubjectHandlers(Observer::abstract_map_data(std::move(sh)));
        subA.Attach(&obs);
        subA.Notify(A{3});
        CHECK(n==3);
        obs.bindSubjectHandlers(Observer::abstract_const_map_data(std::move(sh2)));
        subA.Notify(A{2});
        CHECK(n==5);
    }
    CHECK(set.size()==0);//auto-detach

}


TEST_CASE("Testing _Observer1 / define, remove + (subscribe, unsubscribe)") {

    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
    using handler_t = std::function<void(A)>;

    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));
    
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    std::unordered_map<_subject1_t*,handler_t> subject_handlers;

    //1. no backend, no known pSub
    {
        _observer1_t obs;
        CHECK(obs.Subscribe(&subA)==true);
        CHECK(set.size()==1);
        obs.bindHandlerSubject1(h);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Unsubscribe(&subA)==true);
    }
    //2. no backend, pSub
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        CHECK(obs.Subscribe(&subA)==true);
        CHECK(set.size()==1);
        obs.bindHandlerSubject1(h,&subA);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Unsubscribe(&subA)==true);
    }
    //3. with backend view
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        obs.bindSubjectHandlers(Observer::abstract_map_view(subject_handlers));
        CHECK(set.count(&obs)==0);
        CHECK(obs.Subscribe(&subA)==true);
        obs.Define(&subA,h);
        CHECK(subject_handlers.size()==1);
        obs.onEvent(A{1},&subA);
        CHECK(n==1);
        CHECK(obs.Subscribe(&subA)==false);
        CHECK(obs.Unsubscribe(&subA)==true);
        CHECK(obs.Remove(&subA)==true);
        CHECK(subject_handlers.size()==0);
        CHECK(set.size()==0);
        subA.Notify(A{1}); 
        CHECK(n==1);
    }
    //4. with const backend view
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        obs.bindSubjectHandlers(Observer::abstract_const_map_view(subject_handlers));
        CHECK(set.count(&obs)==0);
        obs.Define(&subA,h);
        CHECK(subject_handlers.size()==0);//Define does not work on Const backend
        CHECK(obs.Remove(&subA)==false);
        CHECK(obs.Remove(&subA)==false);//Remove always return true when disabled
    }
    //5. with backend data
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        auto sh = subject_handlers;
        obs.bindSubjectHandlers(Observer::abstract_map_data(std::move(sh)));
        obs.Define(&subA,h);
        CHECK(obs.Remove(&subA)==true);
        CHECK(obs.Remove(&subA)==false);//already removed
        _subject1_t subA_(Observer::abstract_set_view(set));
        CHECK(obs.Remove(&subA_)==false);//never registered
    }
    //6. with const backend data
    n = 0;
    set.clear();
    {
        _observer1_t obs;
        auto sh = subject_handlers;
        obs.bindSubjectHandlers(Observer::abstract_const_map_data(std::move(sh)));
        obs.Define(&subA,h);
        CHECK(obs.Remove(&subA)==false);//Define does not work on Const backend
        _subject1_t subA_(Observer::abstract_set_view(set));
    }

}


template <typename E>
using _pObs1 = Observer::_Observer1<E>*;

template <typename E>
using _set1 = std::unordered_set<_pObs1<E>>;

template <typename ... Events>
using _tset = std::tuple<_set1<Events> ... >;


TEST_CASE("Testing _Subject") {

    //_Subject<typename ... Events>
    struct A { int value; };
    struct B {};
    struct C {};
    using _subject_t = Observer::_Subject<A,B,C>;

    //ctor from an AbstractSet list
    _set1<A> _setA;
    _set1<B> _setB;
    _set1<C> _setC;
    auto SetA = Observer::abstract_set_view(_setA);
    auto SetB = Observer::abstract_set_view(_setB);
    auto SetC = Observer::abstract_set_view(_setC);
    _subject_t subABC_(SetA,SetB,SetC);
    
    //ctor from an AbstractSetTuple
    _tset<A,B,C> tset;
    _subject_t subABC(Observer::abstract_set_tuple_view(tset));

    //Attach some Observer
    struct D {};
    using _observer_t = Observer::_Observer<A,B,D>;
    _observer_t obs;
    auto obs_id_A = Observer::ObserverID<A>(&obs);
    auto obs_id_B = Observer::ObserverID<B>(&obs);
    CHECK(subABC.Attach(A(),obs_id_A)==true);
    CHECK(subABC.Detach(B(),obs_id_B)==false);//never attached
    CHECK(subABC.Detach(A(),obs_id_A)==true);
    
    //backend rebinding
    CHECK(subABC.Attach(A(),obs_id_A)==true);
    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    static_cast<Observer::_Observer1<A>*>(&obs)->bindHandlerSubject1(h);
    subABC.Notify(A{3});
    CHECK(n==3);
    _set1<A> _setA2;
    auto SetA2 = Observer::abstract_set_view(_setA2);
    subABC.bindObserverSet(A(),SetA2);
    subABC.Notify(A{3});
    CHECK(n==3);

    //data backend
    auto dA = Observer::abstract_set_data(std::move(_setA));
    auto dB = Observer::abstract_set_data(std::move(_setB));
    auto dC = Observer::abstract_set_data(std::move(_setC));
    _subject_t sub(std::move(dA),std::move(dB),std::move(dC));
    dA = Observer::abstract_set_data(std::move(_setA));
    sub.bindObserverSet(A(),std::move(dA));

    //ctor from AbstractSetDataTuple
    _tset<A,B,C> tset2;
    auto subABC2(Observer::abstract_set_tuple_data(std::move(tset2)));

}


TEST_CASE("Testing AbstractSetData") {

    using cont_t = std::unordered_set<int>;
    auto pOpaque = Observer::make_opaque(cont_t());
    static_cast<cont_t*>(pOpaque.get())->insert(3);
    auto Set = Observer::AbstractSetData<int>(cont_t());
    CHECK(Set.Append(4)==true);

    //with custom abstract_set_view object
    auto asv = [](cont_t& set){ return Observer::abstract_set_view(set); };
    auto Set2 = Observer::AbstractSetData<int>(cont_t(),asv);
    CHECK(Set2.Append(4)==true);

    //with custom abstract_set_view policy container
    struct ASV {
        auto operator()(cont_t& set) {
            return Observer::abstract_set_view(set);
        }
    };
    auto Set3 = Observer::AbstractSetData<int>(cont_t(),ASV());
    CHECK(Set3.Append(4)==true);

    //No copy, just move
    using Set_t = Observer::AbstractSetData<int>;
    CHECK(std::is_copy_constructible<Set_t>::value == false);
    CHECK(std::is_copy_assignable<Set_t>::value == false);
    CHECK(std::is_move_constructible<Set_t>::value == true);
    CHECK(std::is_move_assignable<Set_t>::value == true);
 
}


TEST_CASE("Testing AbstractMapData") {

    using map_t = std::unordered_map<char,int>;
    auto Map = Observer::AbstractMapData<char,int>(map_t());
    Map.Define('a',4);
    CHECK(Map.Remove('a')==true);

    //with custom abstract_set_view object
    auto asv = [](map_t& map){ return Observer::abstract_map_view(map); };
    auto Map2 = Observer::AbstractMapData<char,int>(map_t(),asv);
    Map2.Define('a',4);
    CHECK(Map2.Remove('a')==true);

}


TEST_CASE("Testing AbstractConstMapData") {

    using map_t = std::unordered_map<char,int>;
    map_t map;
    map['a'] = 4;
    auto Map = Observer::AbstractConstMapData<char,int>(std::move(map));
    CHECK(Map.Lookup('a')==4);

    //with custom abstract_set_view object
    auto asv = [](map_t& map){ return Observer::abstract_const_map_view(map); };
    map.clear();
    map['a'] = 4;
    auto Map2 = Observer::AbstractConstMapData<char,int>(std::move(map),asv);
    CHECK(Map2.Lookup('a')==4);

}


TEST_CASE("Testing AbstractSetVariant") {

    using set_t = std::unordered_set<int>;
    set_t _set = set_t();

    using vSet_t = Observer::AbstractSet<int>;
    using dSet_t = Observer::AbstractSetData<int>;

    auto vSet = Observer::abstract_set_view(_set);
    auto dSet = Observer::AbstractSetData<int>(set_t());
    
    auto Set = std::variant<vSet_t,dSet_t>(vSet);
    CHECK(std::visit([](auto&& s){ return s.Append(4); },Set)==true);
    CHECK(_set.size()==1);
    Set = std::move(dSet);
    CHECK(std::visit([](auto&& s){ return s.Append(5); },Set)==true);
    CHECK(_set.size()==1);
    
    auto Set2 = Observer::AbstractSetVariant<int>(vSet);
    _set.clear();
    CHECK(Set2.Append(4)==true);
    CHECK(_set.size()==1);
    Set2 = vSet;
    CHECK(Set2.Append(5)==true);
    CHECK(_set.size()==2);
    Set2 = Observer::AbstractSetData<int>(set_t());
    CHECK(Set2.Append(5)==true);
    CHECK(_set.size()==2);

}


TEST_CASE("Testing AbstractMapVariant") {

    using map_t = std::unordered_map<char,int>;
    map_t _map = map_t();

    using vMap_t = Observer::AbstractMap<char,int>;
    using dMap_t = Observer::AbstractMapData<char,int>;

    auto vMap = Observer::abstract_map_view(_map);
    auto dMap = Observer::AbstractMapData<char,int>(map_t());
    
    auto Map = std::variant<vMap_t,dMap_t>(vMap);
    std::visit([](auto&& m){ m.Define('a',4); },Map);
    CHECK(_map.size()==1);
    Map = std::move(dMap);
    std::visit([](auto&& m){ m.Define('a',4); },Map);
    CHECK(_map.size()==1);
   
    auto Map2 = Observer::AbstractMapVariant<char,int>(vMap);
    _map.clear();
    Map2.Define('a',4);
    CHECK(_map.size()==1);
    Map2 = vMap;
    Map2.Define('b',5);
    CHECK(_map.size()==2);
    Map2 = Observer::AbstractMapData<char,int>(map_t());
    Map2.Define('b',5);
    CHECK(_map.size()==2);

}


TEST_CASE("Testing AbstractMapVariant") {

    using map_t = std::unordered_map<char,int>;
    map_t _map = map_t();
    _map['a'] = 4;

    using vMap_t = Observer::AbstractConstMap<char,int>;
    using dMap_t = Observer::AbstractConstMapData<char,int>;

    auto vMap = Observer::abstract_const_map_view(_map);
    
    auto Map = std::variant<vMap_t,dMap_t>(vMap);
    CHECK(std::visit([](auto&& m){ return m.Lookup('a'); },Map)==4);
    CHECK(_map.size()==1);
    _map['a'] = 5;
    auto dMap = Observer::AbstractConstMapData<char,int>(std::move(_map));
    Map = std::move(dMap);
    CHECK(std::visit([](auto&& m){ return m.Lookup('a'); },Map)==5);
   
    auto Map2 = Observer::AbstractConstMapVariant<char,int>(vMap);
    _map = map_t();
    _map['a'] = 4;
    CHECK(Map2.Lookup('a')==4);
    CHECK(_map.size()==1);
    Map2 = vMap;
    _map['a'] = 5;
    Map2 = Observer::AbstractConstMapData<char,int>(std::move(_map));
    CHECK(Map2.Lookup('a')==5);

}


template <typename E>
using handler_T = std::function<void(E)>;


TEST_CASE("Testing SubjectID / one event layer") {

    using namespace Observer;

    struct A { int value; };

    std::unordered_set<_Observer1<A>*> set;
    _Subject1<A> subA(Observer::abstract_set_view(set));
    auto subA_id = Observer::SubjectID<A>(&subA); 

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    auto h2 = [&n](A a){ n+=2*a.value; };
    std::unordered_map<_Subject1<A>*,handler_T<A>> subject_handlers;

    //1. no backend
    {
        _Observer1<A> obs;
        obs.bindHandlerSubject1(h,subA_id);
        obs.Subscribe(subA_id);
        n = 0;
        obs.Define(subA_id,h2);//no op, see below
        subA.Notify(A{1});
        CHECK(n==1);//still bound to h;
        obs.Unsubscribe(subA_id);
        obs.Remove(subA_id);
    }
    //2. backend
    {
        _Observer1<A> obs;
        auto Map = Observer::abstract_map_view(subject_handlers);
        obs.bindSubjectHandlers(Map);
        obs.Subscribe(subA_id);
        obs.Define(subA_id,h);
        obs.Unsubscribe(subA_id);
        obs.Remove(subA_id);
    }

}


TEST_CASE("Testing SubjectID / multiple events") {

    using namespace Observer;

    struct A { int value; };
    struct B {};
    struct C {};

    std::unordered_set<_Observer1<A>*> setA;
    std::unordered_set<_Observer1<C>*> setC;
    _Subject<A,C> sub(abstract_set_view(setA),abstract_set_view(setC));
    auto sub_id = SubjectID<A>(&sub); 

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    auto h2 = [&n](A a){ n+=2*a.value; };
    using subject_handlers_A_t = std::unordered_map<_Subject1<A>*,handler_T<A>>;
    subject_handlers_A_t subject_handlers_A;

    //1. no backend
    {
        _Observer<A,B> obs;
        obs.bindHandlerSubject1(A(),h,sub_id);
        obs.Subscribe(A(),sub_id);
        n = 0;
        obs.Define(A(),sub_id,h2);//no op, see below
        sub.Notify(A{1});
        CHECK(n==1);//still bound to h;
        obs.Unsubscribe(A(),sub_id);
        obs.Remove(A(),sub_id);
    }
    //2. view backend
    {
        _Observer<A,B> obs;
        auto Map = Observer::abstract_map_view(subject_handlers_A);
        obs.bindSubjectHandlers(A(),Map);
        obs.Subscribe(A(),sub_id);
        obs.Define(A(),sub_id,h);
        obs.Unsubscribe(A(),sub_id);
        obs.Remove(A(),sub_id);
    }
    //3. const view backend
    {
        _Observer<A,B> obs;
        auto Map = Observer::abstract_const_map_view(subject_handlers_A);
        obs.bindSubjectHandlers(A(),Map);
    }
    //4. data backend
    {
        _Observer<A,B> obs;
        auto Map = Observer::abstract_map_data(subject_handlers_A_t());
        obs.bindSubjectHandlers(A(),std::move(Map));
    }
    //5. const data backend
    {
        _Observer<A,B> obs;
        auto Map = Observer::abstract_const_map_data(subject_handlers_A_t());
        obs.bindSubjectHandlers(A(),std::move(Map));
    }

}


TEST_CASE("Testing ObjectID / one event layer") {

    struct A { int value; };
    using _subject1_t = Observer::_Subject1<A>;
    using _observer1_t = Observer::_Observer1<A>;
     
    std::unordered_set<_observer1_t*> set;
    _subject1_t subA(Observer::abstract_set_view(set));

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    _observer1_t obs;
    obs.bindHandlerSubject1(h);
    auto obs_id = Observer::ObserverID<A>(&obs);
    CHECK(subA.Attach(obs_id)==true);
    subA.Notify(A{3});
    CHECK(n==3);
    CHECK(subA.Detach(obs_id)==true);
    CHECK(set.size()==0);

}


TEST_CASE("Testing ObjectID / multiple events") {

    using namespace Observer;

    struct A { int value; };
    struct B {};
    using _subject_t = _Subject<A,B>;
    using _observer1_t = _Observer1<A>;
     
    std::unordered_set<_Observer1<A>*> setA;
    std::unordered_set<_Observer1<B>*> setB;
    _subject_t sub(abstract_set_view(setA),abstract_set_view(setB));

    int n = 0;
    auto h = [&n](A a){ n+=a.value; };
    _observer1_t obs;
    obs.bindHandlerSubject1(h);
    auto obs_id = Observer::ObserverID<A>(&obs);
    CHECK(sub.Attach(A(),obs_id)==true);
    sub.Notify(A{3});
    CHECK(n==3);
    CHECK(sub.Detach(A(),obs_id)==true);
    CHECK(setA.size()==0);

}


TEST_CASE("Testing Defaults") {

    using namespace Observer;

    AbstractSetData<int> asd;
    _Subject1<int> sub;
    AbstractSetDataTuple<int,char> asdt;
    _Subject<int,char> sub2;

    AbstractMapData<int,char> amd;
    AbstractConstMapData<int,char> acmd;
    _Observer1<int> obs;
    obs.bindSubjectHandlers();
    _Observer<int,char> obs2;
    obs2.bindSubjectHandlers(int());

}


struct A {};
struct B {};

struct BenevolentSubject
: Observer::SubjectConnect<A,B>
{
    private:
    Observer::_Subject<A,B> sub;

    public:
    void Clicked()
    { sub.Notify(A()); }

    BenevolentSubject()
    : Observer::SubjectConnect<A,B>(nullptr)
    , sub()
    {
        reset(&sub);    
    }

};

struct C {};

struct InterestedObserver
: Observer::ObserverConnect<A,C>
{
    int n = 0;
    std::shared_ptr<Observer::_Observer<A,C>> pObs;

    public:
    InterestedObserver()
    : Observer::ObserverConnect<A,C>(nullptr)
    , pObs(new Observer::_Observer<A,C>())
    {
        reset(pObs.get());
    }

    void bindHandler()
    { pObs->bindHandlerSubject1(A(),[this](A){ ++n; }); }

};


TEST_CASE("Testing Connect mixins") {

    BenevolentSubject Sub;
    InterestedObserver Obs;

    CHECK(Sub.Attach(Obs.observer_id(A()))==true); 
    //Sub.Attach(Obs.observer_id(C()));//does not compile: Sub not supporting C
    CHECK(Sub.Attach(Obs.observer_id(A()))==false);//already 
    CHECK(Sub.Detach(Obs.observer_id(A()))==true); 
    
    CHECK(Obs.Subscribe(Sub.subject_id(A()))==true); 
    CHECK(Obs.Subscribe(Sub.subject_id(A()))==false);//already 
    //Obs.Subscribe(Sub.subject_id(B()));//does not compile: Obs not supporting B
    CHECK(Obs.Unsubscribe(Sub.subject_id(A()))==true);

    CHECK(Sub.Attach(Obs.observer_id(A()))==true); 
    Sub.Clicked(); 
    CHECK(Obs.n==0);
    Obs.bindHandler();
    Sub.Clicked(); 
    CHECK(Obs.n==1);
    auto Obs2 = Obs;
    CHECK(Obs2.pObs.get()==Obs.pObs.get());
    CHECK(Obs2.n==1);
    Sub.Clicked(); 
    CHECK(Obs.n==2);
    CHECK(Obs2.n==1);//indeed the handler is bound to Obs only! 

}

struct ASV
{
    template <typename T>
    Observer::AbstractSet<T> operator()(std::unordered_set<T>&)
    {
        return {};
    }
};


struct AMV
{
    template <typename K, typename V>
    Observer::AbstractMap<K,V> operator()(std::unordered_map<K,V>&)
    {
        return {};
    }
};


struct ACMV
{
    template <typename K, typename V>
    Observer::AbstractConstMap<K,V> operator()(std::unordered_map<K,V>&)
    {
        return {};
    }
};

    
TEST_CASE("Testing Wrapper Custom Impl Constructor") {

    using namespace Observer;

    auto Set = abstract_set_data<ASV>(std::unordered_set<int>());
    auto Map = abstract_map_data<AMV>(std::unordered_map<char,int>());
    auto ConstMap = abstract_const_map_data<ACMV>(std::unordered_map<char,int>());

}
