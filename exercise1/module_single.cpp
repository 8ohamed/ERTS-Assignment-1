#include <systemc.h>
#include <iostream>


SC_MODULE(ModuleSingle) {
    sc_uint<4> counter;

    // Event for communication between thread and method
    sc_event notify_event;

    SC_CTOR(ModuleSingle) : counter(0) {
        SC_THREAD(thread_process);
        
        SC_METHOD(increamenter);
        sensitive << notify_event;
        dont_initialize();
    }
    
    void thread_process() {
        while (true) {
            wait(2, SC_MS);  // Wait for 2 milliseconds
            notify_event.notify();  // Notify the method
        }
    }
    
    void increamenter() {
        counter++;  
        std::cout << "Time: " << sc_time_stamp() 
                  << " - Counter: " << counter
                  << std::endl;
    }
};

int sc_main(int argc, char* argv[]) {
    ModuleSingle module("ModuleSingle");
    sc_start(200, SC_MS);
    
    return 0;
}
