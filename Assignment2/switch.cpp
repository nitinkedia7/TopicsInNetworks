#include <bits/stdc++.h>
using namespace std;

/* Packet class implements the attributes of a packet*/
class Packet {
    public :
    int arrivalTime;            // arrival slot of packet
    int transmissionTime;       // transmission slot
    int outputPort;             // destined output port 

    Packet(int arrivalTime, int outputPort) {
        this->arrivalTime = arrivalTime;
        this->outputPort = outputPort;
    }
};

class BaseSwitch {
    public:
    /* Parameters for the simulation */ 
    int numberOfPorts;
    int inputBufferSize;
    int outputBuffferSize;
    double packetGenProb;
    int maxTimeSlots;

    /* Metrics to be recorded on the go */
    long long cumulativeLinkUtilisation;
    // cumulativeLinkUtilisation is incremented for each input-output port match in scheduling
    double averageLinkUtilization;
    long long cumulativePacketDelay;
    double avgPacketDelay;  
    double stddevPacketDelay;
    long dropPacketCount;
    double dropRatio;
    string outputFileName;
    string queueType;
    vector<Packet> transmittedPackets; // list of all packets transmitted

    // Buffers present in the switch
    vector<queue<Packet>> inputPorts;
    vector<queue<Packet>> outputPorts;

    /* Initialise simulation, log parameters below */
    BaseSwitch(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName) {
        this->numberOfPorts = n;
        this->inputBufferSize = bufferSize;
        this->outputBuffferSize = 1;
        this->packetGenProb = packetGenProb;
        this->maxTimeSlots = maxTimeSlots;
        this->cumulativeLinkUtilisation = 0;
        this->cumulativePacketDelay = 0;
        this->dropPacketCount = 0;
        this->outputFileName = outputFileName;
        this->dropRatio = 0.0;
        inputPorts.resize(n);
        outputPorts.resize(n);
    }
    
    /* Utility function to return true with probability p */ 
    bool randomWithProb(double p) {
        // Randomly generate a value in [0,1]
        double rndDouble = ((double) rand()) / RAND_MAX;
        // Return true if that value is less than or equals p
        return rndDouble <= p;
    }    

    /* Randomly generate a packet with prob p for each input
        queue and push it into the that buffer */ 
    virtual void generateInputPackets(int slotNumber) {
        for (int i = 0 ;i< numberOfPorts; i++) {
            // This input buffer is already full, no need to generate a packet
            if (inputPorts[i].size() == inputBufferSize){
                dropPacketCount++;
                continue;   
            }
            if (randomWithProb(packetGenProb)) {
                // Output port is to be uniformly assigned to each packet
                int targetOutputPort = rand() % numberOfPorts; 
                Packet newPacket = Packet(slotNumber,targetOutputPort);
                inputPorts[i].push(newPacket);
            }
        }
    }

    virtual void schedule() {
        // Dummy virtual function to be defined for each type of queue
        cout << "ERROR : VIRTUAL SCHEDULE RAN" << endl;
    }

    // This function handles packet transmission out of output buffer
    void transmission(int slotNumber) {
        for (int i = 0; i < outputPorts.size(); i++) {
            if (outputPorts[i].empty()) continue;
            Packet packet = outputPorts[i].front();
            packet.transmissionTime = slotNumber; // stamp the transmission slot
            transmittedPackets.push_back(packet);
            outputPorts[i].pop();
        }
    }
    
    // This function is used to calculate all metrics at simulation end 
    void metricsCalculation() {
        // 1. Packet delay, first sum the delays of all transmitted packets then divide by their count
        int noOfTransmittedPacket = transmittedPackets.size();
        for (int i = 0; i < noOfTransmittedPacket; i++) {
            cumulativePacketDelay += (transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime + 1);
        }
        avgPacketDelay = ((double) cumulativePacketDelay) / noOfTransmittedPacket;
        // 2. Standard deviation of delay: First calculate sum of square of difference with mean, then divide, then root.
        long long variancePacketDelay = 0;
        for(int i = 0 ; i < noOfTransmittedPacket; i++) {
            variancePacketDelay += pow(transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime - avgPacketDelay, 2);
        }
        stddevPacketDelay = sqrt(((double)variancePacketDelay) / noOfTransmittedPacket);
        // 3. Average link utilsation: At best every slot each output port should get a new packet, hence the denomiator
        averageLinkUtilization = (1.0 * cumulativeLinkUtilisation) / (numberOfPorts * maxTimeSlots);
        // Packet drop ratio is ratio of dropped packets by count of all transmitted/dropped packets
        // Ignoring packets in buffers as they are very few compared to whole simulation.
        dropRatio = (1.0 * dropPacketCount) / ( dropPacketCount + (int) transmittedPackets.size());
    }

    /* Print the metrics as a csv row */
    void printMetrics() {
        ofstream fout;
        fout.open(outputFileName, ios::app);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << "," << inputBufferSize << "," << "NA" << "," ;
        fout<< dropRatio << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << averageLinkUtilization << "," << "NA" << endl;
        fout.close();
    }

    /* Handles the whole simulation */
    void run() {
        for (int slotNumber = 0; slotNumber < maxTimeSlots; slotNumber++) {
            // Note the order, it is the reverse of the journey of a packet
            // This ensures that one packet spends atleast one slot in each phase.
            // This order is equivalent to running the three phases in parallel
            transmission(slotNumber);
            schedule();
            generateInputPackets(slotNumber);
        }
    }
};

// class for INQ switch instance
class INQ : public BaseSwitch { 
    public:
    
    //constructor to initialize the member variables of the class object
    INQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
        : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
            queueType = "INQ";
        }

    void schedule() { // Implements INQ scheduling
        vector<vector<int>> options(numberOfPorts);
        // For each output port, find the input ports that target it
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPorts[i].empty()) continue;
            int outputPort = inputPorts[i].front().outputPort;
            options[outputPort].push_back(i); 
        }

        vector<int> inputOutputMapping(numberOfPorts, -1);
        // Now, each output port randomly selects an input port out of its options
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() == 0) continue;
            int j = rand() % options[i].size();
            inputOutputMapping[options[i][j]] = i;    
        }
        options.clear();
        
        // Transfer packets from input to output port using the mapping obtained above
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] == -1) continue;
            Packet p = inputPorts[i].front();
            inputPorts[i].pop();
            
            int outputPortId = inputOutputMapping[i];
            if (outputPorts[outputPortId].size() < outputBuffferSize) {
                outputPorts[outputPortId].push(p);
            }
        }
        // Update link utilisation parameters
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] != -1) {
                cumulativeLinkUtilisation++;
            }
        }
    }
};

class KOUQ : public BaseSwitch {  // class for KOUQ switch instance
    public:
    int K;                         // value of knockout ratio * N
    double K_ratio;                // value of knockout ratio
    long dropOutputPort;           // variable to store number of time destined packets to output port exceeded the K count 
    double dropProbablity;         // variable to store the KOUQ drop probablity


    /* Constructor to initialize the variables */
    KOUQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, double K_ratio, string outputFileName) 
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
        this->K = K_ratio * n;
        this->K_ratio = K_ratio; 
        this->dropOutputPort = 0;
        this->queueType = "KOUQ";
        this->outputBuffferSize = bufferSize;
    }

    /* Fucntion for the scheduling process of KOUQ */
    void schedule(){
        //data structure to store all the packets for the output ports
        vector<vector<Packet>> options(numberOfPorts);
        
        // For each output port, find the packets that target it
        for (int i = 0; i < numberOfPorts; i++){
            if (inputPorts[i].empty()) continue;
            Packet headOfLine = inputPorts[i].front();
            inputPorts[i].pop();
            int outputPort = headOfLine.outputPort;
            options[outputPort].push_back(headOfLine); 
        }
        
        // Now, each output port randomly selects atmost K packets
        for (int i = 0; i < numberOfPorts; i++) {
            //if more the K packets we need to take random K
            if (options[i].size() > K) {
                this->dropOutputPort++;
                random_shuffle(options[i].begin(),options[i].end());
                dropPacketCount += (int)options[i].size() - K;
            }
            
            for(int j = 0; j < min(K,(int) options[i].size());j++){
                //if buffer is full packets dropped
                if (outputPorts[i].size() == outputBuffferSize){
                    dropPacketCount +=  min(K,(int) options[i].size()) - j;
                    break;
                } 
                cumulativeLinkUtilisation++;
                outputPorts[i].push(options[i][j]);
            }   
        }
        options.clear();
    }

    //function to calculate drop probablity
    void calcDropprobability() {
        dropProbablity = (100.0 * dropOutputPort) / (numberOfPorts * maxTimeSlots);
    }
    
    //function to print metrics
    void printMetrics() {
        ofstream fout;
        fout.open(outputFileName, ios::app);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << ",";
        fout << inputBufferSize << "," << K_ratio << ",";
        fout<< dropRatio << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << averageLinkUtilization << "," << dropProbablity << endl;
        fout.close();
    }
};

/* Implementation of iSLIP switch */
class SwitchISLIP : public BaseSwitch {
    public:
    // Each input port has a queue for each outport queues i.e. N^2 queues
    vector<vector<queue<Packet>>> inputPorts;
    // But, queues belonging to same input port have a global buffer size
    // Thus if l = 4, an input buffer can have packets for atmost 4 output ports
    vector<int> inputPortsBufferUsed;

    /* Initialise the data structures */
    SwitchISLIP(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName)
    {
        inputPorts.clear();
        inputPorts.resize(n, vector<queue<Packet>> (n));
        inputPortsBufferUsed.resize(n);
        fill(inputPortsBufferUsed.begin(), inputPortsBufferUsed.end(), 0);
        queueType = "iSLIP";
    }

    /* Need a new generator function as input buffer structure has changed */
    void generateInputPackets(int slotNumber) {
        for (int i = 0; i < numberOfPorts; i++) {
            // No need to generate packet if input buffer is already full
            if (inputPortsBufferUsed[i] == inputBufferSize){
                dropPacketCount++;
                continue;   
            }
            
            if (randomWithProb(packetGenProb)) {
                int targetOutputPort = rand() % outputPorts.size();
                Packet newPacket = Packet(slotNumber, targetOutputPort);
                inputPorts[i][targetOutputPort].push(newPacket);
                // Effectively pair (input port, output port) decides a queue
                inputPortsBufferUsed[i]++;
            }
        }
    }

    /* Implementation of iSLIP scheduling */
    void schedule() {
        // In one invocation of this function one ROUND of iSLIP matching is done
        // Target is to a find a bijective matching between input and output ports
        vector<int> inputOutputMapping(numberOfPorts, -1); 
        // Prepare request map, if v[i][j] then i'th input port has a packet for j'th output port
        vector<vector<int>> v(numberOfPorts, vector<int> (numberOfPorts, 0));
        for (int ip = 0; ip < numberOfPorts; ip++) {
            for (int op = 0; op < numberOfPorts; op++) {
                if (!inputPorts[ip][op].empty()) {
                    v[ip][op] = 1;
                }
            }
        }
        // Marks the ports available for current iteration
        vector<int> ipAvailable(numberOfPorts, 1);
        vector<int> opAvailable(numberOfPorts, 1);
        
        bool changed = true;
        while (changed) { // Executing iterations till no new matching can be found
            changed = false;
            // 1. Prepare Grant Map: Each output selects the lowest index input port requesting it
            vector<int> grantMap(numberOfPorts, -1); // op -> ip
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                for (int ip = 0; ip < numberOfPorts; ip++) {
                    if (!ipAvailable[ip]) continue;
                    if (v[ip][op] == 1) {
                        grantMap[op] = ip;
                        break;
                    }
                }
            }
            // 2. Prepare Accept Map: Each input port accepts the lowest index output port
            // which granted this input port
            vector<int> acceptMap(numberOfPorts, -1); // ip -> op
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                
                int ip = grantMap[op];
                if (ip != -1 && acceptMap[ip] == -1) {
                    changed = true;
                    acceptMap[ip] = op;
                }
            }
            // Copy Accepted matchings to results of this round
            for (int ip = 0; ip < numberOfPorts; ip++) {
                if (acceptMap[ip] != -1) {
                    inputOutputMapping[ip] = acceptMap[ip];
                    ipAvailable[ip] = 0;
                    opAvailable[acceptMap[ip]] = 0; 
                }
            }
            grantMap.clear();
            acceptMap.clear();
        }
        v.clear();

        // Transfer packets from input to output port using the mapping obtained above
        for (int ip = 0; ip < numberOfPorts; ip++) {
            int op = inputOutputMapping[ip];
            if (op == -1) continue;
            Packet p = inputPorts[ip][op].front();
            inputPorts[ip][op].pop();
            inputPortsBufferUsed[ip]--;

            if (outputPorts[op].size() < outputBuffferSize) {
                outputPorts[op].push(p);
            }
        }

        // Update link utilisation parameters
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputOutputMapping[i] != -1) {
                cumulativeLinkUtilisation++;
            }
        }
    }
};


// function to print header of the csv files
void printTableHeaders(string outputFileName) {
    ofstream fout;
    fout.open(outputFileName, ios::trunc);
    fout << "Number of Ports,Packet Generation Prob,Queue Type,Buffer Size,Knockout,Drop Ratio,Average Packet Delay,Std. Dev. of Packet Delay,Average Link Utilisation,KOUQ Drop Probability" << endl;
    fout.close();
}

// function to run a given instance of the simulation with the desired value
void runSimulation(int n, int l, double k, double prob, int timeslot, string scheduleType, string outputFile) {
    
    /*  create a switch instance of appropriate type
        call the run function to run simulation
        calculate the results according to the metrics
        print the obtained results
    */
    
    
    if (scheduleType == "INQ") {
        INQ switchInstance(n, l, prob, timeslot, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.printMetrics();
    }
    else if (scheduleType == "KOUQ") {
        KOUQ switchInstance(n, l, prob, timeslot, k, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.calcDropprobability();
        switchInstance.printMetrics();
    }
    else if (scheduleType == "iSLIP") {
        SwitchISLIP switchInstance(n, l, prob, timeslot, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.printMetrics();
    }
    else {
        cout << "ERROR: Invalid scheduling type " << scheduleType << endl;
    }
}

// function to genrate values of graph used for generating report
void getGraphs() {    
    vector<string> queueType = {"INQ", "KOUQ", "iSLIP"};

    // 1. Change N
    string outputFile = "varyPortCount.csv";
    printTableHeaders(outputFile);
    for (int i = 0; i < 3; i++) {
        for (int n = 4; n <= 32; n += 2) {
            runSimulation(n, 4, 0.6, 0.5, 10000, queueType[i], outputFile);
        }    
    }

    // 2. Change L
    outputFile = "varyBufferSize.csv";
    printTableHeaders(outputFile);
    for (int i = 0; i < 3; i++) {
        for (int l = 1; l <= 4; l++) {
            runSimulation(8, l, 0.6, 0.5, 10000, queueType[i], outputFile);
        }
    }

    // 3. Change K in KOUQ only
    outputFile = "varyKnockout.csv";
    printTableHeaders(outputFile);
    for (double k = 0.2; k <= 1.0; k += 0.1) {
        runSimulation(8, 4, k, 0.5, 10000, "KOUQ", outputFile);
    }

    // 4. Change p
    outputFile = "varyGenProb.csv";
    printTableHeaders(outputFile);
    for (int i = 0; i < 3; i++) {
        for (double p = 0.1; p <= 1.0; p += 0.1) {
            runSimulation(8, 4, 0.6, p, 10000, queueType[i], outputFile);
        }
    }
    return;
}

int main(int argc, char *argv[]) {
    srand(time(0)); // Change random seed
    // Default values of switch parameters
    int switchPortCount = 8;
    int bufferSize = 4;
    double packetGenProb = 0.5;
    string queue = "INQ";
    int knockout = 0.6 * switchPortCount;
    string outputFile = "output.csv";
    int maxTimeSlots = 10000;
    bool getAllGraphs = 0;

    for (int i = 1; i < argc; i += 2) {
        string parameter(argv[i] + 1);
        if (parameter == "N") {
            switchPortCount = atoi(argv[i+1]);
        }
        else if (parameter == "B") {
            bufferSize = atoi(argv[i+1]);
        }
        else if (parameter == "p") {
            packetGenProb = atof(argv[i+1]);   
        }
        else if (parameter == "queue") {
            queue = argv[i+1];
        }
        else if (parameter == "K") {
            knockout = atoi(argv[i+1]);
        }
        else if (parameter == "out") {
            outputFile = argv[i+1];
        }
        else if (parameter == "T") {
            maxTimeSlots = atoi(argv[i+1]);
        }
        else if (parameter == "G") {
            getAllGraphs = atoi(argv[i+1]);
        }
        else {
            cout << "Unknown parameter " << parameter << " skipping this and next paramter value";
        }
    } 
    if (!getAllGraphs) {
        printTableHeaders(outputFile);
        runSimulation(switchPortCount, bufferSize, knockout, packetGenProb, maxTimeSlots, queue, outputFile);
    }
    else {
        getGraphs();
    }
    return 0;
};
