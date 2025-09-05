#include <systemc.h>

SC_MODULE(ModuleSingle) {
    sc_event trigger_event;
    sc_uint<4> counter;

    void thread_process() {
        while (true) {
            wait(2, SC_MS);
            trigger_event.notify();
        }
    }

    void method_process() {
        counter++;
        std::cout << "Counter: " << counter << ", Time: " << sc_time_stamp() << std::endl;
    }

    SC_CTOR(ModuleSingle) : counter(0) {
        SC_THREAD(thread_process);
        SC_METHOD(method_process);
        sensitive << trigger_event;
        dont_initialize();
    }
};

int sc_main(int argc, char* argv[]) {
    ModuleSingle mod_single("mod_single");
    sc_start(200, SC_MS);
    return 0;
}

