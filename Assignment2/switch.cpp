#include <bits/stdc++.h>
using namespace std;

/* Packet class implements the attributes of a packet*/
class Packet {
    public :
    int arrivalTime;            //arrival time of packet
    int transmissionTime;       //transmission completion Time
    int outputPort;             //destined output port 

    /* Constructor to initialize member variables*/
    Packet(int arrivalTime, int outputPort) {
        this->arrivalTime = arrivalTime;
        this->outputPort = outputPort;
    }
};

class BaseSwitch {
    public:
    int numberOfPorts;
    int inputBufferSize;
    int outputBuffferSize;
    double packetGenProb;
    int maxTimeSlots;
    long cumulativeLinkUtilisation; 
    double linkUtilization;
    double avgPacketDelay;  
    double variancePacketDelay;
    double stddevPacketDelay;
    string outputFileName;
    string queueType;

    vector<queue<Packet>> inputPorts;
    vector<queue<Packet>> outputPorts;
    vector<Packet> transmittedPackets; 

    BaseSwitch(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName) {
        this->numberOfPorts = n;
        this->inputBufferSize = bufferSize;
        this->outputBuffferSize = 1;
        this->packetGenProb = packetGenProb;
        this->maxTimeSlots = maxTimeSlots;
        this->cumulativeLinkUtilisation = 0;
        this->outputFileName = outputFileName;
    }
    
    int randomWithProb(double p) {
        double rndDouble = (double) rand() / RAND_MAX;
        return rndDouble <= p;
    }    

    void generateInputPackets(int slotNumber) {
        for (auto port : inputPorts) {
            if (port.size() == inputBufferSize)
                continue;
            
            int gen = randomWithProb(packetGenProb);
            if (gen) {
                int targetOutputPort = rand()% (int)(outputPorts.size());
                Packet newPacket = Packet(slotNumber,targetOutputPort);
                port.push(newPacket);
            }
        }
    }

    virtual void schedule() {
        // Dummy virtual function to be defined for each type of queue
    }

    void transmission(int slotNumber) {
        for (int i = 0; i < outputPorts.size(); i++) {
            if (outputPorts[i].empty()) continue;
            Packet packet = outputPorts[i].front();
            packet.transmissionTime = slotNumber;
            transmittedPackets.push_back(packet);
            outputPorts[i].pop();
        }
    }
    
    void metricsCalculation() {
        int noOfTransmittedPacket = transmittedPackets.size();
        
        for(int i = 0; i < noOfTransmittedPacket; i++) {
            avgPacketDelay += transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime;
        }
        avgPacketDelay /= noOfTransmittedPacket;

        for(int i = 0 ; i < noOfTransmittedPacket; i++) {
            variancePacketDelay += pow(transmittedPackets[i].transmissionTime - transmittedPackets[i].arrivalTime - avgPacketDelay, 2);
        }
        variancePacketDelay /= noOfTransmittedPacket;
        stddevPacketDelay = sqrt(variancePacketDelay);

        linkUtilization = cumulativeLinkUtilisation / (numberOfPorts * maxTimeSlots);
    }

    virtual void printMetrics() {
        ofstream fout;
        fout.open(outputFileName);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << linkUtilization << ",Not Applicable" << endl;
        fout.close();
    }

    void run() {
        for (int slotNumber = 0; slotNumber < maxTimeSlots; slotNumber++) {
            transmission(slotNumber);
            schedule();
            generateInputPackets(slotNumber);
        }
    }
};

class INQ : public BaseSwitch {
    public:
    
    INQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
        : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
            queueType = "INQ";
        }

    void schedule() {
        vector<vector<int>> options(numberOfPorts);
    
        // For each output port, find the input ports that target it
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPorts[i].empty()) continue;
            int outputPort = inputPorts[i].front().outputPort;
            options[outputPort].push_back(i); 
        }

        vector<int> inputOutputMapping(numberOfPorts, -1);

        // Now, each output port randomly selects an input port
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() == 0) continue;
            int j = rand() % options[i].size();
            inputOutputMapping[j] = i;    
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

class KOUQ : public BaseSwitch {
    public:
    int K;
    long dropOutputPort;
    double dropProbablity;

    KOUQ(int n, int bufferSize, double packetGenProb, int maxTimeSlots, int K, string outputFileName) 
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName) {
        this->K = K;
        this->dropOutputPort = 0;
        this->queueType = "KOUQ";
    }

    void schedule(){
        // vector<vector<int>> options(numberOfPorts);
        vector<vector<Packet>> options(numberOfPorts);
        
        // For each output port, find the input ports that target it
        for (auto inputPort : inputPorts) {
            if (inputPort.empty()) continue;
            Packet headOfLine = inputPort.front();
            inputPort.pop();
            int outputPort = headOfLine.outputPort;
            options[outputPort].push_back(headOfLine); 
        }
        
         // Now, each output port randomly selects atmost K packets
        for (int i = 0; i < numberOfPorts; i++) {
            if (options[i].size() > K){
                this->dropOutputPort++;
                random_shuffle(options[i].begin(),options[i].end());
            }
            for(int j = 0; j < min(K,(int) options[i].size());j++){
                if(outputPorts[i].size()== outputBuffferSize)
                    break;
                cumulativeLinkUtilisation++;
                outputPorts[i].push(options[i][j]);
            }   
        }
        options.clear();
    }

    void calcDropprobability() {
        dropProbablity = dropOutputPort / (numberOfPorts * maxTimeSlots);
    }

    void printMetrics() {
        ofstream fout;
        fout.open(outputFileName);
        fout << numberOfPorts << "," << packetGenProb << "," << queueType << ",";
        fout << avgPacketDelay << "," << stddevPacketDelay << "," << linkUtilization << "," << dropProbablity << endl;
        fout.close();
    }
};

class SwitchISLIP : public BaseSwitch {
    public:
    vector<vector<queue<Packet>>> inputPorts;
    vector<int> inputPortsBufferUsed;

    SwitchISLIP(int n, int bufferSize, double packetGenProb, int maxTimeSlots, string outputFileName)
    : BaseSwitch(n, bufferSize, packetGenProb, maxTimeSlots, outputFileName)
    {
        inputPorts.resize(n, vector<queue<Packet>> (n));
        inputPortsBufferUsed.resize(n, 0);
        queueType = "iSLIP";
    }

    void generatePackets(int slotNumber) {
        for (int i = 0; i < numberOfPorts; i++) {
            if (inputPortsBufferUsed[i] == inputBufferSize) continue;
            
            int gen = randomWithProb(packetGenProb);
            if (gen) {
                int targetOutputPort = rand() % outputPorts.size();
                Packet newPacket = Packet(slotNumber, targetOutputPort);
                inputPorts[i][targetOutputPort].push(newPacket);
                inputPortsBufferUsed[i]++;
            }
        }
    }

    void schedule() {
        vector<int> inputOutputMapping(numberOfPorts, -1); 
        vector<vector<int>> v(numberOfPorts, vector<int> (numberOfPorts, 0));
        for (int ip = 0; ip < numberOfPorts; ip++) {
            for (int op = 0; op < numberOfPorts; op++) {
                if (!inputPorts[ip][op].empty()) {
                    v[ip][op] = 1;
                }
            }
        }
        vector<int> ipAvailable(numberOfPorts, 1);
        vector<int> opAvailable(numberOfPorts, 1);
        
        bool changed = true;
        while (changed) {
            changed = false;
            vector<int> grantMap(numberOfPorts, -1);
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                for (int ip = 0; ip < numberOfPorts; ip++) {
                    if (!ipAvailable[op]) continue;
                    if (v[ip][op] == 1) {
                        grantMap[op] = ip;
                        break;
                    }
                }
            }
            
            vector<int> acceptMap(numberOfPorts, -1); // ip -> op
            for (int op = 0; op < numberOfPorts; op++) {
                if (!opAvailable[op]) continue;
                
                int ip = grantMap[op];
                if (ip != -1 && acceptMap[ip] == -1) {
                    changed = true;
                    acceptMap[ip] = op;
                }
            }

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

void printTableHeaders(string outputFileName) {
    ofstream fout;
    fout.open(outputFileName);
    fout << "N,p,Queue Type,Average Packet Delay, Std. Dev. of Packet Delay, Average Link Utilisation,KOUQ Drop Probability" << endl;
    fout.close();
}

void runSimulation(int n = 8, int l = 4, double k = 0.6, double prob = 0.5, int timeslot = 10000, string scheduleType, string outputFile) {
    if (scheduleType == "INQ") {
        INQ switchInstance(n, l, prob, timeslot ,outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
    }
    else if (scheduleType == "KOUQ") {
        KOUQ switchInstance(n, l, prob, timeslot, k,outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
        switchInstance.calcDropprobability();
    }
    else if (scheduleType == "iSLIP") {
        SwitchISLIP switchInstance(n, l, prob, timeslot, outputFile);
        switchInstance.run();
        switchInstance.metricsCalculation();
    }
    else {
        cout << "ERROR: Invalid scheduling type " << scheduleType << endl;
    }
}

void getSimulationResult(string outputfile){
    printTableHeaders(outputfile);
    for (int n = 4; n <= 16; n += 2) {
        for (int l = 2; l <= 4; l++) {
            runSimulation(n, l, 0.0, 0.5, 10000, "INQ", outputfile);       
        }
    }
    
    for (int n = 4; n <= 16; n += 2){
        for(int l = 2; l<=4 ; l++)
            for(double k = 0.6 ; k<=1.0 ;k+=0.2){
                runSimulation(n,l,0.0,0.5,10000,"KOUQ",outputfile);
            }
    }
    
    for (int n = 4; n <= 16; n += 2) {
        for (int l = 2; l <= 4; l++) {
            runSimulation(n, l, 0.0,0.5,10000,"iSLIP",outputfile);       
        }
    }
}

int main(int argc, char *argv[]) {
    srand(time(0));
    // Default values of switch parameters
    getSimulationResult("output.csv");
    return 0;
    int switchPortCount = 8;
    int bufferSize = 4;
    double packetGenProb = 0.5;
    string queue = "INQ";
    int knockout = 0.6 * switchPortCount;
    string outputFile = "output.csv";
    int maxTimeSlots = 10000;

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
        else {
            cout << "Unknown parameter " << parameter << " skipping this and next paramter value";
        }
    } 
    printTableHeaders(outputFile);
    runSimulation(switchPortCount, bufferSize, knockout, packetGenProb, maxTimeSlots, queue, outputFile);
};
