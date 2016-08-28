// Copyright Steinwurf ApS 2016.
// Distributed under the "STEINWURF RESEARCH LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

/// @example reed_solomon.cpp
///
/// Simple example showing how to encode and decode a block
/// of memory using a Reed-Solomon codec.

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

#include <kodocpp/kodocpp.hpp>

#include "reed_solomon.h"



#ifdef ASLIBRARY
int demo_main()
{
    // Seed the random number generator to produce different data every time
    srand((uint32_t)time(0));

    // Set the number of symbols and the symbol size
    uint32_t max_symbols = 10;
    uint32_t max_symbol_size = 100;

    // Create encoder/decoder factories that we will use to build the actual
    // encoder and decoder
    kodocpp::encoder_factory encoder_factory(
        kodocpp::codec::reed_solomon,
        kodocpp::field::binary8,
        max_symbols,
        max_symbol_size);

    kodocpp::encoder encoder = encoder_factory.build();

    kodocpp::decoder_factory decoder_factory(
        kodocpp::codec::reed_solomon,
        kodocpp::field::binary8,
        max_symbols,
        max_symbol_size);

    kodocpp::decoder decoder = decoder_factory.build();

    // Allocate some storage for a "payload" the payload is what we would
    // eventually send over a network
    std::vector<uint8_t> payload(encoder.payload_size());

    // Allocate input and output data buffers
    std::vector<uint8_t> data_in(encoder.block_size());
    std::vector<uint8_t> data_out(decoder.block_size());

    // Fill the input buffer with random data
    std::generate(data_in.begin(), data_in.end(), rand);

    // Set the symbol storage for the encoder and decoder
    encoder.set_const_symbols(data_in.data(), encoder.block_size());
    decoder.set_mutable_symbols(data_out.data(), decoder.block_size());

    // Install a custom trace function for the decoder
    auto callback = [](const std::string& zone, const std::string& data)
    {
        std::set<std::string> filters =
        {
            "decoder_state", "symbol_coefficients_before_read_symbol"
        };

        if (filters.count(zone))
        {
            std::cout << zone << ":" << std::endl;
            std::cout << data << std::endl;
        }
    };
    decoder.set_trace_callback(callback);

    uint32_t lost_payloads = 0;
    uint32_t received_payloads = 0;

    while (!decoder.is_complete())
    {
        // The encoder will use a certain amount of bytes of the payload buffer
        uint32_t bytes_used = encoder.write_payload(payload.data());
        std::cout << "Payload generated by encoder, bytes used = "
                  << bytes_used << std::endl;

        // Simulate a channel with a 50% loss rate
        if ((rand() % 2) == 0)
        {
            lost_payloads++;
            continue;
        }

        // Pass the generated packet to the decoder
        received_payloads++;
        decoder.read_payload(payload.data());
        std::cout << "Payload processed by decoder, current rank = "
                  << decoder.rank() << std::endl << std::endl;
    }

    std::cout << "Number of lost payloads: " << lost_payloads << std::endl;
    std::cout << "Number of received payloads: " << received_payloads
              << std::endl;

    // Check if we properly decoded the data
    if (data_in == data_out)
    {
        std::cout << "Data decoded correctly" << std::endl;
    }
    else
    {
        std::cout << "Unexpected failure to decode, "
                  << "please file a bug report :)" << std::endl;
    }

    return 0;
}
#endif
