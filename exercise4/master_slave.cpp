#include <systemc.h>
#include <iostream>
#include <fstream>
using namespace std;

// Parameters from the exercise sheet
#define CHANNEL_BITS  4   // Number of channel wires
#define ERROR_BITS    2   // Number of error wires
#define DATA_BITS     16  // Number of data wires
#define MAX_CHANNEL   2   // Maximum number of channels used
#define CLK_PERIODE   20  // Clock period in ns


SC_MODULE(Master) {
    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_out<bool> valid;
    sc_out<sc_uint<DATA_BITS>> data;
    sc_out<sc_uint<CHANNEL_BITS>> channel;
    sc_out<sc_uint<ERROR_BITS>> error;

    sc_in<bool> ready;

    sc_uint<DATA_BITS> counter;

    void generate_data() {
        valid.write(false);
        data.write(0);
        channel.write(0);
        error.write(0);
        counter = 0;

        wait(); // wait first clock

        while (true) {
            if (reset.read()) {
                valid.write(false);
                data.write(0);
                channel.write(0);
                error.write(0);
                counter = 0;
            }
            else {
                if (ready.read()) {
                    data.write(counter);
                    channel.write(0);
                    error.write(0);
                    valid.write(true);

                    cout << sc_time_stamp()
                        << " [Master] Sent data = " << counter
                        << " (valid=1)" << endl;

                    counter++;          // prepare next data
                    wait();             // one cycle with valid high
                    valid.write(false); // drop valid after handshake
                }

                else {
					valid.write(false); // if slave is not ready valid must be low
                    data.write(counter);
                    cout << sc_time_stamp()
                        << " [Master] Waiting, slave not ready..."
                        << endl;
                }
            }
            wait(); // advance simulation one clock
        }
    }

    SC_CTOR(Master) {
        SC_CTHREAD(generate_data, clk.pos());
        reset_signal_is(reset, true);
    }
};

SC_MODULE(Slave) {
    sc_in<bool> clk;
    sc_in<bool> reset;

    sc_in<bool> valid;
    sc_in<sc_uint<DATA_BITS>> data;
    sc_in<sc_uint<CHANNEL_BITS>> channel;
    sc_in<sc_uint<ERROR_BITS>> error;

    sc_out<bool> ready;

    ofstream outfile;

    SC_CTOR(Slave) {
        outfile.open("slave_output.txt", ios::out);
        if (!outfile.is_open()) {
            SC_REPORT_ERROR("Slave", "Could not open slave_output.txt");
        }

        SC_CTHREAD(receive_data, clk.pos());
        reset_signal_is(reset, true);
    }

    void receive_data() {
        ready.write(true);

        wait();
        while (true) {
            if (reset.read()) {
                ready.write(true);
            }
            else {
                if (valid.read() && ready.read()) {
                    auto d = data.read();
                    outfile << d << endl;
                    outfile.flush();

                    cout << sc_time_stamp() << " [Slave] Received data = " << d << endl;


					ready.write(false);  // low ready immediately after receiving data
                    wait();              // wait 1 cycle
                    ready.write(true);   // back to ready again
                }
            }
            wait();
        }
    }

    ~Slave() {
        if (outfile.is_open())
            outfile.close();
    }
};


SC_MODULE(TopLevel) {
    // Signals
    sc_clock clk;
    sc_signal<bool> reset;
    sc_signal<bool> valid, ready;
    sc_signal<sc_uint<DATA_BITS>> data;
    sc_signal<sc_uint<CHANNEL_BITS>> channel;
    sc_signal<sc_uint<ERROR_BITS>> error;

    Master master;
    Slave slave;

    SC_CTOR(TopLevel)
        : clk("clk", CLK_PERIODE, SC_NS),
        master("master"),
        slave("slave")
    {
        master.clk(clk);
        master.reset(reset);
        master.valid(valid);
        master.data(data);
        master.channel(channel);
        master.error(error);
        master.ready(ready);

        slave.clk(clk);
        slave.reset(reset);
        slave.valid(valid);
        slave.data(data);
        slave.channel(channel);
        slave.error(error);
        slave.ready(ready);
    }
};


int sc_main(int argc, char* argv[]) {
    TopLevel top("TopLevel");

    // VCD trace
    sc_trace_file* wf = sc_create_vcd_trace_file("avalon_wave");
    sc_trace(wf, top.clk, "clk");
    sc_trace(wf, top.reset, "reset");
    sc_trace(wf, top.valid, "valid");
    sc_trace(wf, top.ready, "ready");
    sc_trace(wf, top.data, "data");
    sc_trace(wf, top.channel, "channel");
    sc_trace(wf, top.error, "error");

    top.reset.write(true);
    sc_start(50, SC_NS);  // reset active for 50ns
    top.reset.write(false);


    sc_start(5000, SC_NS);

    sc_close_vcd_trace_file(wf);

    return 0;
}
