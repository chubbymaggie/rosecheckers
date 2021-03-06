/*
OBJ00-A. Declare data members private

If data members are declared public or protected, it is difficult to control how they are accessed. It is possible that they could be manipulated in unintented ways, with undefined consequences. Better is to declare all data members private and define accessor functions that control their uses to those intended. Also, modification of data members can be monitored as appropriate (e.g., by logging). Public modifier functions should preserve any invariants of the type.
*/

typedef int someType;

class Widget {
    public:
        int total; // diagnostic
        // ...
        Widget (): total(0) {}
        // ...
        void add (someType someParameters) {
            // ...
            total++;
            // ...
        }
        void remove (someType someParameters) {
            // ...
            total--;
            // ...
        }
        // ...
};

class Widget2 {
    public:
        Widget2 (): total(0) {}
        // ...
        void add (someType someParameters) {
            // ...
            total++;
            // ...
        }
        void remove (someType someParameters) {
            // ...
            total--;
            // ...
        }
        // ...
        int getTotal () {return total;}
        // ...
    private:
        int total; // no diagnostic
        // ...
};
