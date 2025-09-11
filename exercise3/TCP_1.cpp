#include <systemc.h>
#include <iostream>
#include <random>
#include <chrono>

#define PACKET_SIZE 512
#define DATA_SIZE (PACKET_SIZE - 20)

typedef struct {
    sc_uint<16> SourcePort;
    sc_uint<16> DestinationPort;
    sc_uint<32> SequenceNumber;
    sc_uint<32> Acknowledge;
    sc_uint<16> StatusBits;
    sc_uint<16> WindowSize;
    sc_uint<16> Checksum;
    sc_uint<16> UrgentPointer;
    char Data[DATA_SIZE];
} TCPHeader;

std::ostream& operator<<(std::ostream& os, const TCPHeader& packet) {
    os << "TCP[Seq=" << packet.SequenceNumber
        << ", Src=" << packet.SourcePort
        << ", Dst=" << packet.DestinationPort << "]";
    return os;
}

SC_MODULE(Producer) {
    sc_fifo_out<TCPHeader> out_port;

    SC_CTOR(Producer) : sequence_number(1) {
        SC_THREAD(producer_thread);
    }

private:
    void producer_thread();
    sc_uint<32> sequence_number;
    std::mt19937 rng;
    std::uniform_int_distribution<int> time_dist{ 2, 10 }; 
};

void Producer::producer_thread() {
    rng.seed(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));

    while (true) {
        int delay_ms = time_dist(rng);
        wait(delay_ms, SC_MS);

        TCPHeader packet = {};
        packet.SourcePort = 8080;
        packet.DestinationPort = 80;
        packet.SequenceNumber = sequence_number++;
        packet.Acknowledge = 0;
        packet.StatusBits = 0x0018; 
        packet.WindowSize = 65535;
        packet.Checksum = 0;
        packet.UrgentPointer = 0;

        for (int i = 0; i < DATA_SIZE; i++) {
            packet.Data[i] = static_cast<char>('A' + (i % 26));
        }

        out_port.write(packet);

        std::cout << "Producer: Packet sent at " << sc_time_stamp()
            << " with sequence number " << packet.SequenceNumber << std::endl;
    }
}

SC_MODULE(Consumer) {
    sc_fifo_in<TCPHeader> in_port;

    SC_CTOR(Consumer) {
        SC_THREAD(consumer_thread);
    }

private:
    void consumer_thread();
};

void Consumer::consumer_thread() {
    while (true) {
        TCPHeader packet = in_port.read();

        std::cout << name() << ": Received packet at " << sc_time_stamp()
            << " with sequence number " << packet.SequenceNumber
            << std::endl;
    }
}

SC_MODULE(TopLevel) {
    Producer producer;
    Consumer consumer;
    sc_fifo<TCPHeader> fifo_channel;

    SC_CTOR(TopLevel) : producer("Producer"), consumer("Consumer"), fifo_channel("FIFO", 16) {
        producer.out_port(fifo_channel);

        consumer.in_port(fifo_channel);
    }
};

int sc_main(int argc, char* argv[]) {
    TopLevel top("TopLevel");
    sc_start(100, SC_MS);

    return 0;
}
