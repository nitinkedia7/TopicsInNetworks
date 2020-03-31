import java.util.*;
import java.util.concurrent.locks.ReentrantLock;

public class PassengerQueue {
    ReentrantLock accessLock;
    Queue<Passenger> queue;

    PassengerQueue() {
        this.accessLock = new ReentrantLock();
        this.queue = new LinkedList<>();
    }

    void addPassenger(Passenger passenger) {
        accessLock.lock();
        queue.add(passenger);
        accessLock.unlock();
    }

    long returnSize() {
        return (long) (queue.size());
    }

    Passenger extractPassenger(long selfTime) {
        accessLock.lock();
        Passenger passenger = queue.poll();
        if (passenger != null) {
            passenger.queueLeavingTime = selfTime;
        }
        accessLock.unlock();
        return passenger;
    }
}