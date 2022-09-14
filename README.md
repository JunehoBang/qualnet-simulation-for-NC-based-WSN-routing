# qualnet-simulation-for-NC-based-WSN-routing

This project includes the QualNet simulation code for verifying the gains of network coding based wireless sensor network routing algorithm 
for enhanced resource efficiency and communication reliability specified in the Ref. [^1]. 

The `routing_wsnrandomcoder.cpp` file is the main file wherein the distributed coding node determination, coding based routing, and decoding algorithms are implemented.
The coder role determination is based on the hopcroft-karp algorithm specified in the `maxmatch.cpp` file.
On the receiver's side, whether the arrival packet is innovative packet is determined by matrix rank calculation specified in the `cal_rank.cpp`

Please note that the `maxmatch.cpp` was written in reference to a source code  found in the google search.
However, because the reference was made too long time ago (aproximately 5 years...), I couldn't find the source again.
I apologize to the unknown author that I do not specify the source of the reference. 

[^1]: J. Bang et. al, **"An Energy-Efficient Data Delivery Scheme Exploiting Network Coding and Maximum Node-Disjoint Paths in Wireless Sensor Networks,"** *Adhoc & Sensor Wireless Networks*, vol. 42, no 1/2, pp. 87-106, Oct. 2018, URL: https://web.s.ebscohost.com/abstract?direct=true&profile=ehost&scope=site&authtype=crawler&jrnl=15519899&AN=133161149&h=AJlvm8HHXgMqES8sNTbCgc8iePL%2bJjE5qxZ8hL%2bIdx9dR4YU3LgrNWHLBmN7JZZB1rhAbK2rNM61Z9xWP4hlTw%3d%3d&crl=c&resultNs=AdminWebAuth&resultLocal=ErrCrlNotAuth&crlhashurl=login.aspx%3fdirect%3dtrue%26profile%3dehost%26scope%3dsite%26authtype%3dcrawler%26jrnl%3d15519899%26AN%3d133161149

