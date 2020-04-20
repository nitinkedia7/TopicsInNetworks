# To run the code:
1. Compile using: ```g++ switch.cpp```
2. Run the program with eg. ```./a.out -N 8 -B 4 -p 0.5 -queue INQ -K 0.6 -out output.csv -T 10000```

# Usage of command-line parameters
| Parameter | Usage | Value Range
| --- | --- | --- |
| Switch Port Count | ```-N 8``` | 4-32
| Buffer Size | ```-B 4``` | 2-4
| Packet Generation probability | ```-p 0.5``` | [0,1]
| Scheduler Algo | ```-queue INQ``` | {"INQ", "KOUQ", "iSLIP"}
| Knockout ratio | ```-K 0.6``` | [0,1]
| Output File Name | ```-out output.csv ``` | Any file name (will be created / overridden)
| Max Time Slots | ```-T 10000``` | Upto 10^5
| Graph Data | ```-G 1``` | 0 or 1

Note that there should be even number of arguments since each parameter has a value.

# Description of parameters
1. switchportcount: the number of input/output ports to be present in the switch.
2. buffersize : the value of buffer of input/output queue in the switch.
3. packetgenprob : the value of packet generation probability mus lie between (0.0,1,0].
4. queue : type of scheduling to be done in switch (INQ / KOUQ / ISLIP) must be oen of these 3 types to run the simulation.
5. knockout : the knockout ratio to be used in KOUQ scheduling not required in case of INQ or ISLIP.
6. outputfile : the name of output file in which the simulation result has to saved , ideally should be of csv format.
7. maxtimeslots : number of slots for which the simulation has to be run.

# To obtain graph data
A commandline parameter ```-G 1``` can be specified to get every data. We have built a function getGraph() which will be run in this case.
All other parameter passed will be ignored.
