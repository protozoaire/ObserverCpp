# ObserverCpp / doc / events

Events often come with associated data under a dedicated format. For this reason, it has been chosen to represent events as independent static types.
Typical events are raw structs holding the associated data, e.g.

    struct WindowGetsResized
    {
        int x,y;
    };

In a situation when new events should be created at runtime, it would still be possible to wrap them in a static event type, of course, so this is a
harmless convention.

Event types should be trivial to copy. This assumption is hardcoded everywhere because in the context of the Observer pattern it makes sense that it
be so. In the Observer pattern, Subjects help Observers keep in pace with them, so the differential data that they push to Observers 
should allow them to update immediately.

The trivial-to-copy assumption is here to remind us that. Of course, it is does not preclude workarounds where the data pushed to Observers is actually
a pointer to where true update data is. But then it is the responsibility of Observers to retrieve them independently not to hamper the
notification process.

A few tools related to events are provided by the library which are not mandatory to know.

    SubjectEvents<typename ... Events>
    {
        static constexpr bool AssertNoDuplicate();
        static constexpr bool AssertAllDefined();
        SubjectEvents(); 
    };

can be used to test if a sequence of types constitutes a valid event set. One may use the individual predicates or trigger construction to assert them both. A valid event set contains no duplicates and every type must be defined (not just declared).

    template <typename E, typename _SubjectEvents>
    struct IsSupportedEvent
    {
        static constexpr bool value;
    };

is a static predicate checking whether an event type `E` belongs to a static event set.

    template <typename Event, typename _SubjectEvents>
    struct EventIndex
    {
        static constexpr size_t value;
    };

returns the index of a particular event within an event set.
