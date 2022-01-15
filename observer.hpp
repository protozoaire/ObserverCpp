#ifndef OBSERVER
#  define OBSERVER
#  define OBSERVER_INCOMPLETE
///////////////////////////////


#include <cstddef>
#include <type_traits>
#include <memory>
#include <variant>
#include <unordered_set>
#include <unordered_map>


namespace Observer {


template <typename E, typename ... List>
struct _Count;

template <typename E, typename H, typename ... Rs>
struct _Count<E,H,Rs...>
{
    static constexpr size_t value = _Count<E,Rs...>::value;
};

template <typename E, typename ... Rs>
struct _Count<E,E,Rs...>
{
    static constexpr size_t value = 1+_Count<E,Rs...>::value;
};

template <typename E>
struct _Count<E>
{
    static constexpr size_t value = 0;
};

template <typename ... >
struct _EachOnce
{
    static constexpr bool value = true;
};

template <typename H, typename ... Rs>
struct _EachOnce<H,Rs...>
{
    static constexpr bool value = (_Count<H,Rs...>::value == 0) && _EachOnce<Rs...>::value;
};

template <>
struct _EachOnce<>
{
    static constexpr bool value = true;
};

template <template <typename> class Pred, typename ... >
struct _ForEach;

template <template <typename> class Pred, typename H, typename ... Rs>
struct _ForEach<Pred,H,Rs...>
{
    static constexpr bool value = Pred<H>::value && _ForEach<Pred,Rs...>::value;
};

template <template <typename> class Pred>
struct _ForEach<Pred>
{
    static constexpr bool value = true;
};

template <typename T>
struct _IsDefined
{
    template <typename _T = T, size_t = sizeof(_T)>
    static std::true_type test(int);

    static std::false_type test(...);

    static constexpr bool value = decltype(test(0))::value;
};

template <typename ... Events>
struct SubjectEvents
{
    
    static constexpr bool AssertNoDuplicate() {
        //each event must appear once in the list;
        return _EachOnce<Events...>::value;
    }

    static constexpr bool AssertAllDefined() {
        //each event must be defined;
        return _ForEach<_IsDefined,Events...>::value;
    }

    SubjectEvents() {
        static_assert(AssertNoDuplicate(),
            "ERROR SubjectEvents<...>: any event can appear only once");
        static_assert(AssertAllDefined(),
            "ERROR SubjectEvents<...>: all events must be defined (not just declared)");
    }

};


//
//
//


template <typename E, typename _SubjectEvents>
struct IsSupportedEvent;

template <typename E, typename ... Events>
struct IsSupportedEvent< E, SubjectEvents<Events...> >
{
    static constexpr bool value = (_Count<E,Events...>::value > 0);
    //we need not optimize that because the template instance already exists anyway;
};

template <typename E, typename NotSubjectEvents>
struct IsSupportedEvent
{
    struct Wrong;
    static_assert(std::is_same<NotSubjectEvents,Wrong>::value,
        "USAGE: Observer::IsSupportedEvent<.,SubjectEvents<...>>");
};


//
//
//

template <typename E, typename _SubjectEvents>
struct Event;

template <typename E, typename ... Events>
struct Event< E, SubjectEvents<Events...> >
{
    static_assert(IsSupportedEvent<E,SubjectEvents<Events...>>::value,
        "ERROR Observer::Event<E,SubjectEvents<...>>: 'E' is not in the SubjectEvents list");
};

template <typename E, typename NotSubjectEvents>
struct Event
{
    struct Wrong;
    static_assert(std::is_same<NotSubjectEvents,Wrong>::value,
        "USAGE: Observer::Event<.,SubjectEvents<...>>");
};


//
//
//


template <typename Event, typename _SubjectEvents>
struct EventIndex;

template <typename Event, typename hEvent, typename ... rEvents>
struct EventIndex< Event, SubjectEvents<hEvent,rEvents...> >
{ static constexpr size_t value = 1+EventIndex< Event,SubjectEvents<rEvents...> >::value; };

template <typename Event, typename ... rEvents>
struct EventIndex< Event, SubjectEvents<Event,rEvents...> >
{ static constexpr size_t value = 0; };


//
//
//


struct NoCopy
{
    NoCopy(NoCopy const&) = delete;
    NoCopy& operator=(NoCopy const&) = delete;

    NoCopy(NoCopy&&) = delete;
    NoCopy& operator=(NoCopy&&) = delete;

    protected:
    NoCopy() = default;
};


//
//
//


template <typename T>
struct AbstractSet
{
    std::function<bool(T)> Append;
    std::function<bool(T)> Remove;
    using F = std::function<void(T)>;
    std::function<void(F)> Signal;
};


template <typename T>
AbstractSet<T> abstract_set_view(std::unordered_set<T>& set)
{
    auto append = [&set](T t){ return set.insert(t).second; };
    auto remove = [&set](T t){ return set.erase(t)==1; };
    using F = std::function<void(T)>;
    auto signal = [&set](F f){ for(auto t : set) f(t); };
    return {append,remove,signal};
};
 

//
//
//


template <typename K, typename V>
struct AbstractMap
{
    std::function<bool(K,V)> Define;
    std::function<bool(K)> Remove;
    std::function<V(K)> Lookup;
    using F = std::function<void(K,V)>;
    std::function<void(F)> Signal;
};


template <typename K, typename V>
AbstractMap<K,V> abstract_map_view(std::unordered_map<K,V>& map)
{
    auto define = [&map](K k, V v){ map.insert_or_assign(k,v); return true; };
    auto remove = [&map](K k){ return map.erase(k)==1; };
    auto lookup = [&map](K k){ return map.at(k); };
    using F = std::function<void(K,V)>; 
    auto signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    return {define,remove,lookup,signal};
};


//
//
//


template <typename K, typename V>
struct AbstractConstMap
{
    std::function<V(K)> Lookup;
    using F = std::function<void(K,V)>;
    std::function<void(F)> Signal;
};


template <typename K, typename V>
AbstractConstMap<K,V> abstract_const_map_view(std::unordered_map<K,V> const& map)
{
    auto lookup = [&map](K k){ return map.at(k); };
    using F = std::function<void(K,V)>; 
    auto signal = [&map](F f){ for(auto [k,v] : map) f(k,v); }; 
    return {lookup,signal};
};


//
//
//


template <typename E>
struct _Observer1;

template <typename E>
struct _Subject1
: private NoCopy
{
    private:

        using pObs_t = _Observer1<E>*;
        using observers_t = AbstractSet<pObs_t>;
        observers_t observers;

    public:

    explicit _Subject1(observers_t observers_)
    : NoCopy()
    , observers(observers_)
    {}

    bool Attach(pObs_t pObs) 
    { return observers.Append(pObs); }

    bool Detach(pObs_t pObs)
    { return observers.Remove(pObs); }

    void Notify(E e)
    { observers.Signal([e,this](pObs_t pObs){ pObs->onEvent(e,this); }); }
    /* Note: not const; Observers may respond by Detaching. */

    _Subject1& bindObserverSet(observers_t observers_)
    { observers = observers_; return *this; }

};  

template <typename E>
struct _Observer1
: private NoCopy
{
    private:

        using pSub_t = _Subject1<E>*;
        using handler_t = std::function<void(E)>;
        using subject_handlers_t = AbstractConstMap<pSub_t,handler_t>;
        using signal_func_t = std::function<void(pSub_t,handler_t)>;

        static constexpr auto lookup_ignore = [](pSub_t){ return [](E){}; };
        static constexpr auto signal_ignore = [](signal_func_t){};

        subject_handlers_t subject_handlers = { lookup_ignore, signal_ignore }; 

    public:

    _Observer1() = default;

    void onEvent(E e, pSub_t pSub = nullptr)
    { subject_handlers.Lookup(pSub)(e); }

    _Observer1& bindSubjectHandlers(subject_handlers_t subject_handlers_)
    { subject_handlers = subject_handlers_; return *this; }

    _Observer1& bindSubjectHandler1(pSub_t pSub, handler_t handler)
    {
        subject_handlers.Lookup = [handler](pSub_t){ return handler; };
        if(pSub) {
            auto detach_unique = [pSub,this](signal_func_t){ pSub->Detach(this); };
            subject_handlers.Signal = detach_unique;
        }
        else subject_handlers.Signal = signal_ignore;
        return *this; 
    }
    
    ~_Observer1()
    { subject_handlers.Signal([this](pSub_t pSub,handler_t){ pSub->Detach(this); }); }
    //only works when a subject_handlers was bound;

};


//
//
//


template <typename _SubjectEvents>
struct _Subject;

template <typename _SubjectEvents>
struct _Observer;

template <typename ... Events>
struct _Subject< SubjectEvents<Events...> >
{
    private:

        using events_t = SubjectEvents<Events...>;
        
        using _subject1s_t = std::tuple<_Subject1<Events>...>;
        _subject1s_t _subject1s;

    public:

    /*
    void onEvent(E data)
    {
        static constexpr auto i = EventIndex<E,events_t>::value;
        handlers[i](&data);
    }

    template <typename E, typename = typename std::enable_if< IsSupportedEvent<E,events_t>::value >::type>
    void bindHandler(std::function<void(E)>* pFunc)
    {
        static constexpr auto i = EventIndex<E,events_t>::value;
        handlers[i] = [pFunc](void* pE){ (*pFunc)(*static_cast<E*>(pE)); };
    }
    */

};

template <typename ... Events>
struct _Observer< SubjectEvents<Events...> >
{
    private:

        using events_t = SubjectEvents<Events...>;
        
        using _subject1s_t = std::tuple<_Subject1<Events>...>;
        _subject1s_t _subject1s;

    public:

    _Observer() = default;

};

/*
template
<
    typename _SubjectEvents,
    typename event_idx_t = size_t,
    template <typename> class observers_T = std::unordered_set,
    template <typename,typename> class event2observers_T = std::unordered_map,
    template <typename,typename> class observer2count_T = std::unordered_map
>
struct _Subject;


template <typename ... Events, typename event_idx_t, template <typename> class observers_T,
         template <typename,typename> class event2observers_T, 
         template <typename,typename> class observer2count_T>
struct _Subject
< 
    SubjectEvents<Events...>,
    event_idx_t,
    observers_T,
    event2observers_T,
    observer2count_T
>
{
    
    _Subject(_Subject const&) = delete;
    _Subject& operator=(_Subject const&) = delete;

    _Subject(_Subject&&) = delete;
    _Subject& operator=(_Subject&&) = delete;
   
    private:

        using events_t = SubjectEvents<Events...>;

        using pObs_t = _Observer<events_t>*;
      
        using observers_t = observers_T<pObs_t>;
        using event2observers_t = event2observers_T<event_idx_t,observers_t>;
        event2observers_t event2observers;

        bool event_add_observer(event_idx_t eventIdx_, pObs_t pObs_)
        {
            auto const it = event2observers.find(eventIdx_);
            if(it == event2observers.cend()) {
                event2observers.insert(std::make_pair(eventIdx_,observers_t{ pObs_ }));
                return true;
            }
            else {
                auto const jt = it->second.find(pObs_);
                if(jt == it->second.cend()) {
                    it->second.insert(pObs_);
                    return true;
                }
                else {
                    //already subscribed
                    return false;
                }
            }
        }

        bool event_del_observer(event_idx_t eventIdx_, pObs_t pObs_)
        {
            auto const it = event2observers.find(eventIdx_);
            if(it == event2observers.cend()) {
                //not subscribed by anybody
                return false;
            }
            else {
                auto const jt = it->second.find(pObs_);
                if(jt == it->second.cend()) {
                    //not subscribed
                    return false;
                }
                else {
                    it->second.erase(jt);
                    return true;
                }
            }
        }

        using observer2count_t = observer2count_T<pObs_t,size_t>;
        observer2count_t observer2count;
        
        void incr_observer_count(pObs_t pObs_, size_t n)
        {
            auto it = observer2count.find(pObs_);
            if(it == observer2count.end()) { 
                observer2count.insert(std::make_pair(pObs_,n));
            }
            else it->second += n;
        }
 
        void decr_observer_count(pObs_t pObs_, size_t n)
        {
            auto it = observer2count.find(pObs_);
            if(it == observer2count.end()) {
                // nothing to do 
            }
            else {
                if(it->second <= n) {
                    // clean up observer
                    observer2count.erase(it); 
                }
                else it->second -= n;
            }
        }
       
        size_t read_observer_count(pObs_t pObs_) const
        {
            auto const it = observer2count.find(pObs_);
            if(it==observer2count.cend()) return 0;
            else return it->second;
        }

        //

        void _Attach(pObs_t pObs_, event_idx_t eventIdx_)
        {
            if(event_add_observer(eventIdx_,pObs_)) incr_observer_count(pObs_,1);
            else { 
                // already subscribed -> do nothing
            }
        }

        void _Detach(pObs_t pObs_, event_idx_t eventIdx_)
        {
            if(event_del_observer(eventIdx_,pObs_)) decr_observer_count(pObs_,1);
            else {
                // not subscribed
            }
        }

    public:

    _Subject() = default;

    bool Attach(pObs_t pObs_, event_idx_t eventIdx_)
    {
        if(eventIdx_ >= sizeof...(Events)) return false;
        _Attach(pObs_,eventIdx_); 
        return true;
    }

    bool Detach(pObs_t pObs_, event_idx_t eventIdx_)
    {
        if(eventIdx_ >= sizeof...(Events)) return false;
        _Detach(pObs_,eventIdx_);
        return true;
    }

    bool Attach(pObs_t pObs_, std::initializer_list<event_idx_t> eventIdxList_)
    {
        for(auto const eventIdx_ : eventIdxList_) { if(eventIdx_ >= sizeof...(Events)) return false; }
        for(auto const eventIdx_ : eventIdxList_) { _Attach(pObs_,eventIdx_); }
        return true;
    }

    bool Detach(pObs_t pObs_, std::initializer_list<event_idx_t> eventIdxList_)
    {
        for(auto const eventIdx_ : eventIdxList_) { if(eventIdx_ >= sizeof...(Events)) return false; }
        for(auto const eventIdx_ : eventIdxList_) { _Detach(pObs_,eventIdx_); }
        return true;
    }

    size_t nEvents(pObs_t pObs_) const
    {
        return read_observer_count(pObs_);
    }

};
*/



}//close Observer


///////////////////////////////
#  undef OBSERVER_INCOMPLETE
#else
#  ifdef OBSERVER_INCOMPLETE
#    error circular inclusion of observer.hpp
#  endif
#endif
