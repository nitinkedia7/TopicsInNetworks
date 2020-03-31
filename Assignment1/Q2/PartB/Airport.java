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

    class Statistics {
        public long noOfWaitingPassengersOfficers;
        public long noOfPassengersInSystem;
    }
    public Statistics momentDataList[];

    public Airport(int noOfQueues, double arrivalMeanRate, double serviceMeanRate, long stopTime) {
        this.noOfQueues = noOfQueues;
        this.arrivalMeanRate = arrivalMeanRate;
        this.serviceMeanRate = serviceMeanRate;
        this.stopTime = stopTime;
        this.currentTime = 0;

        momentDataList = new Statistics[(int) stopTime + 1];
        for (int i = 0; i <= stopTime; i++)
            momentDataList[i] = new Statistics();

        queues = new PassengerQueue[noOfQueues];
        for (int i = 0; i < noOfQueues; i++) {
            queues[i] = new PassengerQueue();
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
        return (int) Math.round(Math.log(1-y)*(-1.0/mean));
        // return (int) (1 / mean);
        // mean = 1 / mean;
        // Random r = new Random();
        // double L = Math.exp(-mean);
        // int k = 0;
        // double p = 1.0;
        // do {
        //     p = p * r.nextDouble();
        //     k++;
        // } while (p > L);
        // return k - 1;
    }

    public static void main(String args[]) {
        Scanner sc = new Scanner(System.in);

        int noOfQueues = 1;

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
        momentDataList[(int) currentTime].noOfWaitingPassengersOfficers = (officers[0].sourceQueue).returnSize();
        long noOfPassengersInSystem = (officers[0].sourceQueue).returnSize();
        for (Officer officer : officers) {
            if (officer.passenger != null)
                noOfPassengersInSystem++;
        }
        momentDataList[(int) currentTime].noOfPassengersInSystem = noOfPassengersInSystem;
    }

    public void printTheoreticalValue() { // M/M/3
        double rho = arrivalMeanRate / (3 * serviceMeanRate);
        
        double jobsInSystemFactor = 1;
        double jobsInSystem = jobsInSystemFactor;
        for (int i = 1; i < 3; i++) {
            jobsInSystemFactor *= (3 * rho) / i;
            jobsInSystem += jobsInSystemFactor;
        }
        jobsInSystemFactor *= (3 * rho) / (3 * (1 - rho));
        jobsInSystem += jobsInSystemFactor;
        jobsInSystem = 1 / jobsInSystem;

        double probOfQueueing = jobsInSystemFactor * jobsInSystem;
        double meannq = (rho * probOfQueueing) / (1 - rho);
        double meanWaitTime = meannq / arrivalMeanRate;
        double meanResponseTime = meanWaitTime + (1 / serviceMeanRate);

        double numberOfPassengerInSystem = arrivalMeanRate * meanResponseTime;
        double numberOfPassengersWaiting = arrivalMeanRate * meanWaitTime;
        double numberOfPassengerGetInspected = (numberOfPassengerInSystem - numberOfPassengersWaiting);

        System.out.println("\nTheortical Results:");
        System.out.println("Average number of Passengers getting Inspected : " + numberOfPassengerGetInspected);
        System.out.println("Average Response Time : " + meanResponseTime);
        System.out.println("Average Waiting Time : " + meanWaitTime);
        System.out.println("Average Number Of Passengers Waiting in queue  : " + numberOfPassengersWaiting);
    }


    public void printStatistics() {
        long totalNoOfInspectedPassenger = 0;
        long totalResponseTime = 0;
        long totalWaitingTime = 0;
        long waitingPassenger = 0;
        long gettingInspectedPassenger = 0;
        for (Officer officer : officers) {
            totalNoOfInspectedPassenger += officer.inspectedPassengers.size();
            for (Passenger passenger : officer.inspectedPassengers) {
                totalResponseTime += passenger.completionTime - passenger.arrivalTime;
                totalWaitingTime += passenger.queueLeavingTime - passenger.arrivalTime;
            }
        }
        
        for (int i = 0; i < stopTime; i++) {
            Statistics stat = momentDataList[i];
            waitingPassenger += stat.noOfWaitingPassengersOfficers;
            gettingInspectedPassenger += stat.noOfPassengersInSystem  - stat.noOfWaitingPassengersOfficers;
        }
        
        System.out.println("\nSimulation Results:");
        System.out.println("Total Number Of generated Passengers : " + passengerGenerator.totalNoOfGeneratedPassengers);
        // System.out.println("Total Number Of inspected Passenge?rs : " + totalNoOfInspectedPassenger);
        System.out.println(
                "Average number of Passengers getting Inspected : " + (1.0 * gettingInspectedPassenger) / (stopTime)
        );
        System.out.println("Average Response Time : " + (1.0 * totalResponseTime) / (totalNoOfInspectedPassenger));
        System.out.println("Average Waiting Time : " + (1.0 * totalWaitingTime) / (totalNoOfInspectedPassenger));
        System.out.println("Average Number Of Passengers Waiting in queue  : "
                + (1.0 * waitingPassenger) / (stopTime));

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