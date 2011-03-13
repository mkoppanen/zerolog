/*
+-----------------------------------------------------------------------------------+
|  zerolog, network log daemon                                                      |
|  Copyright (c) 2011, Mikko Koppanen <mikko@kuut.io>                               |
|  All rights reserved.                                                             |
+-----------------------------------------------------------------------------------+
|  Redistribution and use in source and binary forms, with or without               |
|  modification, are permitted provided that the following conditions are met:      |
|     * Redistributions of source code must retain the above copyright              |
|       notice, this list of conditions and the following disclaimer.               |
|     * Redistributions in binary form must reproduce the above copyright           |
|       notice, this list of conditions and the following disclaimer in the         |
|       documentation and/or other materials provided with the distribution.        |
|     * Neither the name of the copyright holder nor the                            |
|       names of its contributors may be used to endorse or promote products        |
|       derived from this software without specific prior written permission.       |
+-----------------------------------------------------------------------------------+
|  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  |
|  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    |
|  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           |
|  DISCLAIMED. IN NO EVENT SHALL KUUTIO LIMITED BE LIABLE FOR ANY                   |
|  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES       |
|  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     |
|  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      |
|  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       |
|  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS    |
|  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     |
+-----------------------------------------------------------------------------------+
*/

#include "pipe.hpp"
#include <iostream>
#include <vector>
#include <zmq.hpp>

#ifndef BUFFER_SIZE
#  define BUFFER_SIZE 4096
#endif

#define ALLOC_PIPES 20

int main (int argc, char *argv []) 
{
    // parse args
    bool bound = false;
    int c;
    size_t p = 0;

    zerolog::pipe_t *pipes[ALLOC_PIPES];

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_PUB);

    opterr = 0;
    while ((c = getopt(argc, argv, "b:hp:")) != -1) {
        switch (c) {

            case 'b':
                socket.bind (optarg);
                bound = true;
                break;

            case 'p':
                std::string a = optarg;
                if (p >= ALLOC_PIPES) {
                    std::cerr << "Maximum number of pipes reached. Will fix this one day" << std::endl;
                    return 1;
                }
                pipes[p++] = new zerolog::pipe_t (a, 0755);
                break;
        }
    }

    if (!bound) {
        std::cerr << "Specify -b argument to bind the socket"
                  << std::endl;
        return 1;
    }

    if (!p) {
        std::cerr << "-p argument needs to be specified at least once"
                  << std::endl;
        return 1;
    }

    zmq::pollitem_t *items = new zmq::pollitem_t [p];

    for (size_t i = 0; i < p; i++) {
        items [i].socket  = 0;
        items [i].fd      = pipes[i]->get_fd ();
        items [i].events  = ZMQ_POLLIN;
        items [i].revents = 0;
    }

    while (true) {
        int rc = zmq::poll (&items [0], p, -1);
        assert (rc >= 0);

        for (size_t i = 0; i < p; i++) {
            if (items [i].revents & ZMQ_POLLIN) {
                size_t buffer_size = BUFFER_SIZE;
                unsigned char buffer [BUFFER_SIZE];

                if (pipes [i]->read (buffer, &buffer_size)) {
                    const char *filename = pipes [i]->get_filename ();

                    zmq::message_t msg (buffer_size + 1 + strlen (filename));

                    memcpy ((char *) msg.data (), filename, strlen (filename));
                    memcpy ((char *) msg.data () + strlen (filename), "|", 1);
                    memcpy ((char *) msg.data () + strlen (filename) + 1, buffer, buffer_size);

                    socket.send (msg, 0);
                }
            }
            if (items [i].revents & ZMQ_POLLERR) {
                std::cerr << "Error: " << zmq_strerror (errno) << std::endl;
            }
        }
    }
    for (size_t i = 0; i < p; i++) {
        delete pipes[p];
    }
    delete[] items;
    return 0;
}
