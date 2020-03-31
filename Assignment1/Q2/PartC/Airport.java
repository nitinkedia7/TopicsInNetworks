import java.util.*;

public class Airport implements Runnable {
    public PassengerQueue queues[];
    public int noOfQueues;
    public Officer officers[];
    public PassengerGenerator passengerGenerator;
    public double arrivalMeanRate;
    public double serviceMeanRate;
    public long currentTime;
    public long stopTime;
    public int bufferSize;

    class Statistics {
        public long noOfWaitingPassengersOfficer1;
        public long noOfWaitingPassengersOfficer2;
        public long noOfWaitingPassengersOfficer3;
        public long noOfPassengersInSystem;
    }

    public Statistics momentDataList[];

    public Airport(int noOfQueues, double arrivalMeanRate, double serviceMeanRate, long stopTime) {
        this.bufferSize = 10;
        this.noOfQueues = noOfQueues;
        this.arrivalMeanRate = arrivalMeanRate;
        this.serviceMeanRate = serviceMeanRate;
        this.stopTime = stopTime;
        this.currentTime = 0;
        // momentDataList = new Statistics[10000];
        momentDataList = new Statistics[(int) stopTime + 1];
        for (int i = 0; i <= stopTime; i++)
            momentDataList[i] = new Statistics();

        queues = new PassengerQueue[noOfQueues];
        for (int i = 0; i < noOfQueues; i++) {
            queues[i] = new PassengerQueue(10);
        }

        passengerGenerator = new PassengerGenerator(arrivalMeanRate, queues, noOfQueues);

        officers = new Officer[3];
        for (int i = 0; i < 3; i++) {
            int queueNo = noOfQueues == 1 ? 0 : i;
            officers[i] = new Officer(queues[queueNo], serviceMeanRate);
        }
    }

    public static int getPoissonRandom(double mean) {
        double y = Math.random();
        return (int) Math.round(Math.log(1 - y) * (-1.0 / mean));
        // mean = 1 / mean;
        // Random r = new Random();
        // double L = Math.exp(-mean);
        // int k = 0;
        // double p = 1.0;
        // do {
        // p = p * r.nextDouble();
        // k++;
        // } while (p > L);
        // return k - 1;
    }

    public static void main(String args[]) {
        Scanner sc = new Scanner(System.in);

        int noOfQueues = 3;

        System.out.print("Enter the mean arrival rate: ");
        double arrivalMeanRate = sc.nextDouble();

        System.out.print("Enter the mean service rate: ");
        double serviceMeanRate = sc.nextDouble();

        System.out.print("Enter simulation time: ");
        long stopTime = sc.nextLong();

        sc.close();

        Airport airport = new Airport(noOfQueues, arrivalMeanRate, serviceMeanRate, stopTime);
        Thread airportSimulator = new Thread(airport);
        airportSimulator.start();
    }

    public void captureMoment() {
        // System.out.println(currentTime);
        momentDataList[(int) currentTime].noOfWaitingPassengersOfficer1 = (officers[0].sourceQueue).returnSize();
        momentDataList[(int) currentTime].noOfWaitingPassengersOfficer2 = (officers[1].sourceQueue).returnSize();
        momentDataList[(int) currentTime].noOfWaitingPassengersOfficer3 = (officers[2].sourceQueue).returnSize();
        assert (momentDataList[(int) currentTime].noOfWaitingPassengersOfficer1 <= 10) : "hi";
        assert (momentDataList[(int) currentTime].noOfWaitingPassengersOfficer2 <= 10) : "hi";
        assert (momentDataList[(int) currentTime].noOfWaitingPassengersOfficer3 <= 10) : "hi";

        long noOfPassengersInSystem = 0;
        for (Officer officer : officers) {
            noOfPassengersInSystem += (officer.sourceQueue).returnSize();
            if (officer.passenger != null)
                noOfPassengersInSystem++;
        }
        momentDataList[(int) currentTime].noOfPassengersInSystem = noOfPassengersInSystem;
    }

    public void printTheoreticalValue() {
        double arrivalMeanRatefor1queue = arrivalMeanRate / 3;
        double rho = arrivalMeanRatefor1queue / serviceMeanRate;
        double p0 = (1 - rho )/( 1 - Math.pow(rho,bufferSize+1));
        double pB = p0 * Math.pow(rho,bufferSize);
        double numberOfPassengerInSystem = (rho)/(1-rho) - ((bufferSize+1)*(Math.pow(rho,bufferSize+1)))/(1-(Math.pow(rho,bufferSize+1)));
        double responseTime = numberOfPassengerInSystem/(arrivalMeanRatefor1queue *(1-pB));
        double waitTime = responseTime - (1.0 / serviceMeanRate);
        double numberOfPassengersWaiting = arrivalMeanRatefor1queue*(1-pB)*waitTime;
        double numberOfPassengerGetInspected = (numberOfPassengerInSystem - numberOfPassengersWaiting);

        System.out.println("\nTheortical Results:");
        System.out.println("Average number of Passengers getting Inspected : " + 3 * numberOfPassengerGetInspected);
        System.out.println("Average Response Time : " + responseTime);
        System.out.println("Average Waiting Time : " + waitTime);
        System.out.println("Average Number Of Passengers Waiting in queue  : " + 3 * numberOfPassengersWaiting);
    }

    public void printStatistics() {
        long totalNoOfInspectedPassenger = 0;
        long totalResponseTime = 0;
        long totalWaitingTime = 0;
        long gettingInspectedPassenger = 0;

        for (Officer officer : officers) {
            totalNoOfInspectedPassenger += officer.inspectedPassengers.size();
            for (Passenger passenger : officer.inspectedPassengers) {
                totalResponseTime += passenger.completionTime - passenger.arrivalTime;
                totalWaitingTime += passenger.queueLeavingTime - passenger.arrivalTime;
            }
        }

        long waitingPassenger = 0;
        for (int i = 0; i < stopTime; i++) {
            Statistics stat = momentDataList[i];
            long curWaitingPassenger = stat.noOfWaitingPassengersOfficer1 + stat.noOfWaitingPassengersOfficer2
                    + stat.noOfWaitingPassengersOfficer3;
            assert curWaitingPassenger <= 30 : "bye";
            waitingPassenger += curWaitingPassenger;
            gettingInspectedPassenger += stat.noOfPassengersInSystem - curWaitingPassenger;
        }

        System.out.println("\nSimulation Results:");
        System.out.println("Total Number Of generated Passengers : " + passengerGenerator.totalNoOfGeneratedPassengers);
        System.out.println("Total Number Of dropped Passengers : " + passengerGenerator.droppedPassengerCount); 
        System.out.println(
                "Average number of Passengers getting Inspected : " + (1.0 * gettingInspectedPassenger) / (stopTime));
        System.out.println("Average Response Time : " + (1.0 * totalResponseTime) / (totalNoOfInspectedPassenger));
        System.out.println("Average Waiting Time : " + (1.0 * totalWaitingTime) / (totalNoOfInspectedPassenger));
        System.out.println("Average Number Of Passengers Waiting in queue  : " + (1.0 * waitingPassenger) / (stopTime));

    }

    public void run() {
        try {
            while (currentTime <= stopTime) {
                List<Thread> threadList = new ArrayList<>();
                if (passengerGenerator.selfTime == currentTime) {
                    Thread generatorThread = new Thread(this.passengerGenerator);
                    // generatorThread.start();
                    threadList.add(generatorThread);
                }

                for (Officer officer : officers) {
                    if (officer.selfTime == currentTime) {
                        Thread officerThread = new Thread(officer);
                        // officerThread.start();
                        threadList.add(officerThread);
                    }
                }

                for (Thread thread : threadList) {
                    thread.start();
                }

                for (Thread thread : threadList) {
                    thread.join();
                }

                captureMoment();
                currentTime++;
                currentTime = Math.min(currentTime, passengerGenerator.selfTime);
                for (Officer officer : officers) {
                    currentTime = Math.min(currentTime, officer.selfTime);
                }

            }
            printStatistics();
            printTheoreticalValue();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}