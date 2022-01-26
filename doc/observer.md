# ObserverCpp / doc / observer

Observers hold an `_Observer` resource.

    template <typename ... Events>
    struct _Observer;

It is but a simple concatenation of one-event resources `_Observer1`, which could alternatively be used if only one event needs to be supported.

    template <typename E>
    struct _Observer1;

The interface is almost the same in both cases, only most `_Observer` methods take an empty event as first parameter for disambiguation.

    _obsA.Subscribe(pSub);
    _obsABC.Subscribe(A(),pSub);

On Observer interface, Subjects are designated either by opaque IDs or raw pointers. 
Any Subject has a per-event opaque ID which is of type `SubjectID<A>` for an event type `A`.


## One Event

Here is the `_Observer1` interface as per Observer pattern interaction:

    template <typename E>
    struct _Observer1
    {
        bool Subscribe(SubjectID<E>);
        bool Unsubscribe(SubjectID<E>);
        void onEvent(E,_Subject1<E>*);
    
        ...
    };

That `onEvent` takes a pointer to a one-event Subject instead of an ID is on purpose: this method is intended for direct call by Subjects; by passing
their address, Subjects let Observers know the origin of the event.

The `Subscribe`/`Unsubscribe` methods let connections be established with SubjectIDs as an equivalent to `Attach`/`Detach` on Subject
interface.

Observers need to define their response to the event they have subscribed on Subjects. They have several options to do so.
Observers may watch more than one Subjects; also they might need to auto-detach from Subjects on destruction.

Observers that watch only one Subject, use the `bindHandlerSubject1`, optionally providing its `SubjectID` to enable the auto-detach feature.

    template <typename E>
    struct _Observer1
    {
        private:
        using handler_t = std::function<void(E)>;

        public:
        _Observer1& bindHandlerSubject1(handler_t = [](E){}, SubjectID<E> = nullptr);

        ...
    };

Calling this method with no arguments is also a means to reset the handling of the event to factory "no response" defaults.

Else, Observers can watch multiple Subjects, have a single response for all, and don't need to notify Subjects of their destruction.
In this case, they can use `bindHandlerSubject1` again, where optionally one Subject can be set to notify on the Observer destruction.

Else, Observers watch multiple Subjects and need to notify them on their destruction. In this case, they need a container backend. This container is
an abstract map which optionally allows them do define behaviour on a per-subject basis.  

The abstract map is either a view of an external container, or a wrapper owning the container internally (that's the default). 
In the first case, an `AbstractMap` object is taken in; an `AbstractMapData` rvalue reference in the second.

    template <typename E>
    struct _Observer1
    {
        private:
        using handler_t = std::function<void(E)>;
        using subject_handlers_view_t = AbstractMap<_Subject1<E>*,handler_t>;
        using subject_handlers_data_t = AbstractMapData<_Subject1<E>*,handler_t>;

        public:
        
        _Observer1& bindSubjectHandlers(subject_handlers_view_t);
        _Observer1& bindSubjectHandlers(subject_handlers_data_t&&);
        _Observer1& bindSubjectHandlers();

        void Define(SubjectID<E>, handler_t = [](E){});
        bool Remove(SubjectID<E>);

        ...
    
    };

(Note that while opaque Subject IDs are used externally, they are not used in storage.)

The `bind` method with no argument creates an empty `AbstractMapData` as a backend, which is based on the default `std::unordered_map`. 
Otherwise, refer to the "containers" doc file for creating abstract map views and wrappers.

After a backend has been set, the `Define`/`Remove` methods can be used to
append/edit or remove entries. Note that an entry should only be removed after successful detach, or else an error will throw on notify. If it is
wished to cancel response for a particular subject (without detaching), using `Define` with the only SubjectID argument is the correct idiom.

Optionally, abstract read-only maps are also supported, by using `AbstractConstMap` (view) or `AbstractConstMapData` instead of the above. 
This could be useful to avoid defining the entire abstract map interface for a custom container, when all subjects IDs are known at binding-time, 
or if the backend is edited externally (used by the Observer as a const view). As in the case of a uniform handler, the `Define` and `Remove` are no-ops 
against a const backend.

    template <typename E>
    struct _Observer1
    {
        private:
        using handler_t = std::function<void(E)>;
        using subject_handlers_const_view_t = AbstractConstMap<_Subject1<E>*,handler_t>;
        using subject_handlers_const_data_t = AbstractConstMapData<_Subject1<E>*,handler_t>;

        public:
        
        _Observer1& bindSubjectHandlers(subject_handlers_const_view_t);
        _Observer1& bindSubjectHandlers(subject_handlers_const_data_t&&);

        ...
    
    };


## Multiple Events

The multi-event case has but the same interface with the single difference that all methods take an event object as a first parameter for disambiguation. Example:

    obsAC.bindHandlerSubject1(A(),h,sID); //uniform handler + single subject auto-detach
    obsBD.bindSubjectHandler(B()); //initiate internal default map container backend.


## Connect Mixin

Concrete observers need to forward the connect interface of their resource. 
This means either exposing the opaque ObserverID of its resource for all supported events (it is trivially constructible from a raw pointer). 
Or `Subscribe`/`Unsubscribe` methods. Or both.

The `ObserverConnect` mixin base provides a default connect-forwarding interface.

    template <typename ... Events>
    struct ObserverConnect
    {
    
        template <typename E>
        ObserverID<E> observer_id(E);

        template <typename E>
        bool Subscribe(SubjectID<E>);

        template <typename E>
        bool Unsubscribe(SubjectID<E>);

        ... 
    
    };

To use it, a concrete observer needs to publicly inherit it and connects it with its actual resource. 
This is done with the `reset` method; for instance,

    struct ConcreteObserver
    : Observer::ObserverConnect<A,C>
    {
        private:
        Observer::_Observer<A,C> obs;

        public:
        ConcreteObserver()
        {
            Observer::ObserverConnect<A,C>::reset(&obs);
        }

        ...

    };

Note that Observer resources can be neither copied or moved. 
When smart pointers are used, the `ObserverConnect` layer must be bound against the actual resource.

