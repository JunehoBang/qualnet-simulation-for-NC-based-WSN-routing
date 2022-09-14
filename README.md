# qualnet-simulation-for-NC-based-WSN-routing

This project includes the QualNet simulation code for verifying the gains of network coding based wireless sensor network routing algorithm 
for enhanced resource efficiency and communication reliability. 

The `routing_wsnrandomcoder.cpp` file is the main file wherein the distributed coding node determination, coding based routing, and decoding algorithms are implemented.
The coder role determination is based on the hopcroft-karp algorithm specified in the `maxmatch.cpp` file.
On the receiver's side, whether the arrival packet is innovative packet is determined by matrix rank calculation specified in the `cal_rank.cpp`

Please note that the `maxmatch.cpp` was written in reference to a source code  found in the google search.
However, because the reference was made too long time ago (aproximately 5 years...), I couldn't find the source again.
I apologize to the unknown author that I do not specify the source of the reference. 
