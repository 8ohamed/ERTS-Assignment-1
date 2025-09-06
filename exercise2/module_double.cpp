#include <systemc.h>
#include <iostream>

SC_MODULE(ModuleDouble) {
    sc_event A, B, Aack, Back;
    bool wait_for_A;

    SC_CTOR(ModuleDouble) : wait_for_A(true) {
        SC_THREAD(processA);
        SC_THREAD(processB);

        SC_METHOD(methodA);
        dont_initialize();
    }

    void processA() {
        while (true) {
            wait(3, SC_MS);
            A.notify();
            std::cout << "Process A: Event A notified at " << sc_time_stamp() << std::endl;

            // Wait for acknowledgment with timeout of 3 ms
            wait(sc_time(3, SC_MS),Aack);

            if (Aack.triggered()) {
                std::cout << "Process A: Acknowledgment received at " << sc_time_stamp() << std::endl;
            }
            else {
                std::cout << "Process A: Timeout - no acknowledgment received at " << sc_time_stamp() << std::endl;
            }
        }
    }

    void processB() {
        while (true) {
            wait(2, SC_MS);
            B.notify();
            std::cout << "Process B: Event B notified at " << sc_time_stamp() << std::endl;

            wait(sc_time(2, SC_MS), Back);

            if (Back.triggered()) {
                std::cout << "Process B: Acknowledgment received at " << sc_time_stamp() << std::endl;
            }
            else {
                std::cout << "Process B: Timeout - no acknowledgment received at " << sc_time_stamp() << std::endl;
            }
        }
    }

    void methodA() {
        if (A.triggered() && wait_for_A) {
            std::cout << "Method A: Event A triggered at " << sc_time_stamp() << std::endl;
            Aack.notify(); // Send acknowledgment
            wait_for_A = false; // Next time wait for B
            next_trigger(B); // Dynamic sensitivity - wait for event B next
        }
        else if (B.triggered() && !wait_for_A) {
            std::cout << "Method A: Event B triggered at " << sc_time_stamp() << std::endl;
            Back.notify(); // Send acknowledgment
            wait_for_A = true; // Next time wait for A
            next_trigger(A); // Dynamic sensitivity - wait for event A next
        }
        else {
            next_trigger(A);
        }
    }
};

int sc_main(int argc, char* argv[]) {
    ModuleDouble module2("ModuleDouble");
    sc_start(50, SC_MS);

    return 0;
}
