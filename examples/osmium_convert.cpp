/*

  Convert OSM files from one format into another.

*/

/*

Copyright 2012 Jochen Topf <jochen@topf.org> and others (see README).

This file is part of Osmium (https://github.com/joto/osmium).

Osmium is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License or (at your option) the GNU
General Public License as published by the Free Software Foundation, either
version 3 of the Licenses, or (at your option) any later version.

Osmium is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License and the GNU
General Public License for more details.

You should have received a copy of the Licenses along with Osmium. If not, see
<http://www.gnu.org/licenses/>.

*/

#include <iostream>
#include <getopt.h>

#define OSMIUM_WITH_PBF_INPUT
#define OSMIUM_WITH_XML_INPUT
#define OSMIUM_WITH_PBF_OUTPUT
#define OSMIUM_WITH_XML_OUTPUT
#include <osmium.hpp>
#include <osmium/handler/progress.hpp>

void print_help() {
    std::cout << "osmium_convert [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE or OUTFILE is not given stdin/stdout is assumed.\n" \
              << "File format is given as suffix in format .TYPE[.ENCODING].\n" \
              << "Use -f and -t options to force format.\n" \
              << "\nFile types:\n" \
              << "  osm     normal OSM file\n" \
              << "  osh     OSM file with history information\n" \
              << "\nFile encodings:\n" \
              << "  (none)  XML encoding\n" \
              << "  gz      XML encoding compressed with gzip\n" \
              << "  bz2     XML encoding compressed with bzip2\n" \
              << "  pbf     binary PBF encoding\n" \
              << "\nOptions:\n" \
              << "  -h, --help                This help message\n" \
              << "  -d, --debug               Enable debugging output\n" \
              << "  -f, --from-format=FORMAT  Input format\n" \
              << "  -t, --to-format=FORMAT    Output format\n";
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"debug",       no_argument, 0, 'd'},
        {"help",        no_argument, 0, 'h'},
        {"from-format", required_argument, 0, 'f'},
        {"to-format",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    bool debug = false;

    std::string input_format;
    std::string output_format;

    while (true) {
        int c = getopt_long(argc, argv, "dhf:t:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                debug = true;
                break;
            case 'h':
                print_help();
                exit(0);
            case 'f':
                input_format = optarg;
                break;
            case 't':
                output_format = optarg;
                break;
            default:
                exit(1);
        }
    }

    std::string input;
    std::string output;
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]" << std::endl;
        exit(1);
    } else if (remaining_args == 2) {
        input =  argv[optind];
        output = argv[optind+1];
    } else if (remaining_args == 1) {
        input =  argv[optind];
    }

    Osmium::OSMFile infile(input);
    if (!input_format.empty()) {
        try {
            infile.set_type_and_encoding(input_format);
        } catch (Osmium::OSMFile::ArgumentError& e) {
            std::cerr << "Unknown format for input: " << e.value() << std::endl;
            exit(1);
        }
    }

    Osmium::OSMFile outfile(output);
    if (!output_format.empty()) {
        try {
            outfile.set_type_and_encoding(output_format);
        } catch (Osmium::OSMFile::ArgumentError& e) {
            std::cerr << "Unknown format for output: " << e.value() << std::endl;
            exit(1);
        }
    }

    if (infile.type() == Osmium::OSMFile::FileType::OSM() && outfile.type() == Osmium::OSMFile::FileType::History()) {
        std::cerr << "Warning! You are converting from an OSM file without history information to one with history information.\nThis will almost certainly not do what you want!" << std::endl;
    } else if (infile.type() == Osmium::OSMFile::FileType::History() && outfile.type() == Osmium::OSMFile::FileType::OSM()) {
        std::cerr << "Warning! You are converting from an OSM file with history information to one without history information.\nThis will almost certainly not do what you want!" << std::endl;
    } else if (infile.type() != outfile.type()) {
        std::cerr << "Warning! Source and destination are not of the same type." << std::endl;
    }

    Osmium::Output::Base* out = Osmium::Output::open(outfile);
    out->set_debug_level(debug ? 1 : 0);

    Osmium::Handler::Progress progress_handler;

    typedef Osmium::Handler::Sequence<Osmium::Output::Base, Osmium::Handler::Progress> sequence_handler_t;
    sequence_handler_t sequence_handler(*out, progress_handler);

    Osmium::Input::read(infile, sequence_handler);

    delete out;

    google::protobuf::ShutdownProtobufLibrary();
}

