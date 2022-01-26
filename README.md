# ObserverCpp

ObserverCpp is an implementation of the [Observer](https://en.wikipedia.org/wiki/Observer_pattern) design pattern in C++.

In this pattern, Subjects expose interface for Observers to connect and receive instant notification of change. 


## Usage

Subject events are modeled as trivially copyable data of a certain type. For instance,

    struct WindowGetsResized
    {
        int x,y;
    };

A Subject exposing events `A` and `B` owns a resource `_Subject<A,B>` and forwards the interface necessary for Observers of `A`, `B` or both, to attach
and detach.

    class BenevolentSubject
    {
        private:
        Observer::_Subject<A,B> _sub;

        public:
        //custom subject interface forwarding to _sub
        //or inherit Observer::SubjectConnect<A,B> to expose the default

    };

Then the Subject needs to install the `Notify` calls in its own code as appropriate.

    void BenevolentSubject::clicked() {
        //update state
        _sub.Notify(A{...});
    }

An Observer interested in event `A` and `C` owns a resource `_Observer<A,C>` from which messages flow in once it has subscribed
to Subjects. 

    class InterestedObserver
    {
        private:
        Observer::_Observer<A,C> _obs;

        public:
        //custom observer interface forwarding to _obs
        //or inherit Observer::ObserverConnect<A,C> to expose the default

    };

Then it needs to set up on its resource what happens on event `A` or `C`.
In most cases, it just sets one handler per event:

    _obs.bindHandlerSubject1(A(),h);

Here `h` is any callable taking an `A` argument. The empty `A` object passed as a first argument is only to indicate the target event.
When a designated Subject is passed as an extra argument, the Observer will automatically detach from this Subject when destroyed.

If an Observer wants to track several Subjects on the same event, and auto-detach from all of them when destroyed, 
it needs to back against a map container to store these identities. It also makes it possible to define behaviour on a per-Subject basis.

    _obs.bindSubjectHandlers(A()); //default map container is used

Then, it might use

    _obs.Define(A(),subjectId_A_2,h);

to create or reset behaviour for `subjectId_A_2` on event `A`, or

    _obs.Remove(A(),subjectId_A_2);

to clean up the entry after successfully detached.

Here `subjectId_A_2` is an opaque Subject identity. It contains the address of a `_Subject<A>` resource, which only `_Observer<A>` resources 
can access. (And the other way around for opaque Observer identities.)

The following are equivalent ways to enter the Observer protocol.

    _sub.Attach(A(),opaqueObsId);
    _obs.Subscribe(A(),opaqueSubId);

Subject and Observer resources can neither be copied nor moved. They may be wrapped with smart pointers to be passed around. 
Observer resources should be packaged with the state edited by their handlers.


## Installation

ObserverCpp is a header-only library. It requires C++17.


## Tests

The tests use [doctest](https://github.com/doctest/doctest), a header-only testing framework.


## Container Backends

ObserverCpp acts as a thin translation layer on top of abstract container interface. As such, container policies can be fully customized.

The default backend for Subjects is `std::unordered_set` and the default backend for multi-subject auto-detaching Observers is `std::unordered_map`.

A backend is either owned or unowned by the resource. When it is unowned, the resource works on an abstract view of the container, 
while the actual container remains managed by the resource host. When it is owned, the abstract view becomes an abstract *wrapper*, carrying the same
interface, yet holding the actual data as an opaque internal.

To give a taste of how easy it is to support custom backends, here is what defines the abstract set interface at use (this is the view version):

    template <typename T>
    struct AbstractSet
    {
        std::function<bool(T)> Append;
        std::function<bool(T)> Remove;
        using F = std::function<void(T)>;
        std::function<void(F)> Signal;
    };

The `Signal` method abstracts an iterator over all elements, taking in a visitor function.

Then, the support for the default `std::unordered_set` is just this:

    template <typename T>
    AbstractSet<T> abstract_set_view(std::unordered_set<T>& set)
    {
        auto append = [&set](T t){ return set.insert(t).second; };
        auto remove = [&set](T t){ return set.erase(t)==1; };
        using F = std::function<void(T)>;
        auto signal = [&set](F f){ for(auto t : set) f(t); };
        return {append,remove,signal};
    };

(The wrapper version reuses the same factory function.)


## Related Work

An object-oriented implementation of the Observer pattern in C++ was presented by Gamma et al. in their book 
"Design Patterns: Elements of Reusable Object-Oriented Software".

[Signals and Slots](https://en.wikipedia.org/wiki/Signals_and_slots) are a popular primitive to implement Observer patterns in C++.
Signals are basically function containers. They go to the bare-bone abstraction of active notification: passing a piece of data along to many functions.

Our implementation stands in between. While the Observer role is mostly informal in Signals and Slots, it is restated here as a point of indirection
between subscription and behaviour. Then the idea of dynamic callbacks is completely moved into the Observer.


## Documentation

See the `doc` folder.


## Future Work

- thread-safety
