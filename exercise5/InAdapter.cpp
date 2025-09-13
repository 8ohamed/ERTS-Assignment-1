#include <systemc.h>
#include <iostream>
#include <fstream>

using namespace std;

#define CHANNEL_BITS  4
#define ERROR_BITS    2
#define DATA_BITS     16
#define CLK_PERIODE   20 

template <class T>
class InAdapter : public sc_fifo_out_if<T>, public sc_module {
public:
    // Clock and reset
    sc_in_clk clock;
    sc_in<sc_logic> reset;

    // Handshake ports for ST bus
    sc_in<sc_logic> ready;
    sc_out<sc_logic> valid;

    // Channel, error and data ports ST bus
    sc_out<sc_int<CHANNEL_BITS>> channel;
    sc_out<sc_int<ERROR_BITS>> error;
    sc_out<sc_int<DATA_BITS>> data;

    SC_CTOR(InAdapter) {
        valid.initialize(SC_LOGIC_0);
        channel.initialize(0);
        error.initialize(0);
        data.initialize(0);
    }

    void write(const T& value) {
        if (reset.read() == SC_LOGIC_0) {  
            // Output sample data on negative edge of clock
            while (ready.read() == SC_LOGIC_0)
                wait(clock.posedge_event());

            wait(clock.posedge_event());
            data.write(value);
            channel.write(0);
            error.write(0);
            valid.write(SC_LOGIC_1);

            cout << sc_time_stamp() << " [InAdapter] forwarded value " << value << endl;

            wait(clock.posedge_event());
            valid.write(SC_LOGIC_0);
        }
        else {
            wait(clock.posedge_event());
        }
    }

private:
    bool nb_write(const T&) {
        SC_REPORT_FATAL("/InAdapter", "Called nb_write()");
        return false;
    }
    virtual int num_free() const {
        SC_REPORT_FATAL("/InAdapter", "Called num_free()");
        return 0;
    }
    virtual const sc_event& data_read_event() const {
        SC_REPORT_FATAL("/InAdapter", "Called data_read_event()");
        static sc_event dummy;
        return dummy;
    }
};



SC_MODULE(Master) {
    sc_in_clk clock;
    sc_in<sc_logic> reset;
    sc_fifo_out<int> out;   

    SC_CTOR(Master) {
		SC_CTHREAD(generate_data, clock.pos()); // run generate_data on positive clock edge
    }

    void generate_data() {
        int counter = 0;
        wait(); 

        while (true) {
            if (reset.read()==SC_LOGIC_1) {
                counter = 0;
            }
            else {
                out.write(counter); 
                cout << sc_time_stamp() << " [Master] wrote " << counter << endl;
                counter++;
            }
            wait();
        }
    }
};

SC_MODULE(Slave) {
    sc_in_clk clock;
    sc_in<sc_logic> reset;

    sc_in<sc_logic> valid;
    sc_in<sc_int<DATA_BITS>> data;
    sc_in<sc_int<CHANNEL_BITS>> channel;
    sc_in<sc_int<ERROR_BITS>> error;

    sc_out<sc_logic> ready;

    SC_CTOR(Slave) {
        SC_CTHREAD(recieve_data, clock.pos());
    }

    void recieve_data() {
        ready.write(SC_LOGIC_1);
        wait();
        while (true) {
            if (reset.read() == SC_LOGIC_1) {
                ready.write(SC_LOGIC_1);
            }
            if (valid.read() == SC_LOGIC_1 && ready.read() == SC_LOGIC_1) {
                sc_int<DATA_BITS> d = data.read();
                cout << sc_time_stamp() << " [Slave] Received data = " << d << endl;
            }
            wait();
        }
    }
};

SC_MODULE(TopLevel) {
    sc_clock clock;
    sc_signal<sc_logic> reset;
    sc_signal<sc_logic> valid;
    sc_signal<sc_logic> ready;
    sc_signal<sc_int<DATA_BITS>> data;
    sc_signal<sc_int<CHANNEL_BITS>> channel;
    sc_signal<sc_int<ERROR_BITS>> error;

    Master master;
    InAdapter<int> inAdapt;
    Slave slave;

    SC_CTOR(TopLevel)
        : clock("clock", CLK_PERIODE, SC_NS)
        , master("master")
        , inAdapt("inAdapt")
        , slave("slave")
    {
        master.clock(clock);
        master.reset(reset);
        master.out(inAdapt);

        inAdapt.clock(clock);
        inAdapt.reset(reset);
        inAdapt.valid(valid);
        inAdapt.ready(ready);
        inAdapt.channel(channel);
        inAdapt.error(error);
        inAdapt.data(data);

        slave.clock(clock);
        slave.reset(reset);
        slave.valid(valid);
        slave.ready(ready);
        slave.data(data);
        slave.channel(channel);
        slave.error(error);
    }
};

int sc_main(int argc, char* argv[]) {
    TopLevel top("top");

    top.reset.write(SC_LOGIC_1);
    sc_start(50, SC_NS);
    top.reset.write(SC_LOGIC_0);

    sc_start(1000, SC_NS);

    return 0;
}
