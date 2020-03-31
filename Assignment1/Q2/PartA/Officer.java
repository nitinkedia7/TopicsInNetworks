import java.util.*;

public class Officer implements Runnable {
    long selfTime;
    Queue<Passenger> inspectedPassengers;
    double serviceMean;
    PassengerQueue sourceQueue;
    Passenger passenger;

    Officer(PassengerQueue queue, double serviceMean) {
        this.selfTime = 0;
        this.sourceQueue = queue;
        this.serviceMean = serviceMean;
        this.inspectedPassengers = new LinkedList<>();
    }

    public void run() {
        if (passenger != null) {
            passenger.completionTime = selfTime;
            inspectedPassengers.add(passenger);
            passenger = null;
        }

        for (int i = 0; i < 10; i++) {
            passenger = sourceQueue.extractPassenger(selfTime);
            if (passenger != null)
                break;
        }
        if (passenger != null) {
            // double y = Math.random();
            // long serviceTime = (long) Math.round(Math.log(1-y)*(-serviceMean));
            long serviceTime = Airport.getPoissonRandom(serviceMean);
            assert (serviceTime >= 1) : "Service time of officer < 1";
            selfTime += serviceTime;
        } else {
            selfTime++;
        }
    }

}