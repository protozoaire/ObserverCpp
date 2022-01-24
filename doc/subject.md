# ObserverCpp / doc / subject

Subjects hold a `_Subject` resource.

    template <typename ... Events>
    struct _Subject;

It is but a simple concatenation of one-event resources `_Subject1`, which could alternatively be used if only one event needs to be supported. 

    template <typename E>
    struct _Subject1;

The interface is almost the same in both cases, only most `_Subject` methods take an empty event as first parameter for disambiguation.

    _subA.Attach(pObs);
    _subABC.Attach(A(),pObs);

On Subject interface, Observers are designated either by opaque IDs or raw pointers. Any Observer has a per-event opaque ID which is of
type `ObserverID<A>` for an event type `A`.


## One Event

Here is the `_Subject1` interface as per Observer pattern interaction:
    
    template <typename E>
    struct _Subject1
    {
        bool Attach(ObserverID<E>); 
        bool Detach(ObserverID<E>);
        void Notify(E e);
    
        ...
    };

A `_Subject1` resource requires an abstract set container to work. It must be defined at construction-time, and it can be reset afterwards. An
abstract set container is either provided as a view of an external container, or a wrapper owning the container internally (that's the default). In the first
case, an `AbstractSet` object is taken in; an `AbstractSetData` rvalue reference in the second. 

    template <typename E>
    struct _Subject1
    {
        private:
        using observers_view_t = AbstractSet<_Observer1<E>*>;
        using observers_data_t = AbstractSetData<_Observer1<E>*>;

        public:

        explicit _Subject1(observers_view_t);
        explicit _Subject1(observers_data_t&& = observers_data_t());

        _Subject1& bindObserverSet(observers_view_t observers_);
        _Subject1& bindObserverSet(observers_data_t&& observers_);

        ...
    };

(Note that while opaque Observer IDs are used externally, they are not used in storage.)

When no construction argument are provided, `_Subject1` hosts an abstract container wrapper, which is based on the default 
`std::unordered_set`. Otherwise, refer to the "containers" doc file for creating abstract set views and wrappers.


## Multiple Events

Similarly, multi-event `_Subject` takes a sequence of abstract set containers, in event order, or an abstract tuple of them.

    template <typename ... Events>
    struct _Subject
    {
        ...
        
        explicit _Subject(AbstractSet<_Observer1<Events>*> ... );
        explicit _Subject(AbstractSetData<_Observer1<Events>*>&& ... );
        
        explicit _Subject(AbstractSetTuple<_Observer1<Events>*...>);
        explicit _Subject(AbstractSetDataTuple<_Observer1<Events>*...>&&);
        
        _Subject();
       
        template <typename E>
        _Subject& bindObserverSet(E,AbstractSet<_Observer1<E>*>);
        
        template <typename E>
        _Subject& bindObserverSet(E,AbstractSetData<_Observer1<E>*>&&);

        ...
    };

The `bind` methods are just trivial forwarders to the corresponding one-event versions.

Here again, when no construction arguments are provided, all one-event layers are equipped with an abstract container wrapper 
on top of `std::unordered_set`.

Otherwise, in the case of many events, the tuple variants may be worth considering. 
They result from a batch-construction of views or wrappers reusing the same logic.
As before, refer to the "containers" doc file for more information.


## Connect Mixin

Concrete subjects need to forward the connect interface of their resource. This means either exposing the opaque SubjectID of its resource for all
supported events (it is trivially constructible from a raw pointer). Or `Attach`/`Detach` methods. Or both.

The `SubjectConnect` mixin base provides a default connect-forwarding interface.

    template <typename ... Events>
    struct SubjectConnect
    {

        template <typename E>
        SubjectID<E> subject_id(E);

        template <typename E>
        bool Attach(ObserverID<E>);

        template <typename E>
        bool Detach(ObserverID<E>);
    
        ...

    };

To use it, a concrete subject needs to publicly inherit it and connects it with its actual resource. 
This is done with the `reset` method; for instance,

    struct ConcreteSubject
    : Observer::SubjectConnect<A,B>
    {
        private:
        Observer::_Subject<A,B> sub;

        public:
        ConcreteSubject()
        {
            Observer::SubjectConnect<A,B>::reset(&sub);
        }

        ...
    
    };

Note that Subject resources can be neither copied or moved. When smart pointers are used, the `SubjectConnect` layer must be bound against the actual
resource. 

