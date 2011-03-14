<?php
$c = new zmqcontext(); 
$s = new zmqsocket($c, ZMQ::SOCKET_SUB); 
$s->setSockopt(ZMQ::SOCKOPT_SUBSCRIBE, ""); 
$s->connect("tcp://localhost:5566"); 
while (1) { echo $s->recv() . PHP_EOL; };