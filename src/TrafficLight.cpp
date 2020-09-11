#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <queue>
#include <future>
#include <condition_variable>
#include <algorithm>

/* Implementation of class "MessageQueue" */


template<typename T>
T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
  // to wait for and receive new messages and pull them from the queue using move semantics.
  // The received object should then be returned by the receive function.
  std::unique_lock<std::mutex> u_lock(_mutex);
  _cond.wait(u_lock, [this] { return !_queue.empty(); });

  // get the latest element and remove it from the queue
  T msg = std::move(_queue.back());
  _queue.pop_back();
  return msg;
}

template<typename T>
void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
  // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

  // prevent data race
  std::lock_guard<std::mutex> u_lock(_mutex);
  _queue.push_back(std::move(msg));
  _cond.notify_one();
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight() {
  _current_phase = TrafficLightPhase::RED;
  _msg_queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::WaitForGreen() {
  // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
  // runs and repeatedly calls the receive function on the message queue.
  // Once it receives TrafficLightPhase::green, the method returns.
  while (true) {
//      std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Wait until the traffic light is green, received from message queue
    TrafficLightPhase curr_phase = _msg_queue->receive();
    if (curr_phase == GREEN) {
      return;
    }
  }
}

TrafficLightPhase TrafficLight::GetCurrentPhase() {
  return _current_phase;
}

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::CycleThroughPhases, this));
}

// virtual function which is executed in a thread
[[noreturn]] void TrafficLight::CycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
  // and toggles the current phase of the traffic light between red and green and sends an update method
  // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
  // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

  std::random_device rd;
  std::mt19937 eng(rd());
  std::uniform_int_distribution<> distr(4, 6);

  std::chrono::time_point<std::chrono::system_clock> lastUpdate;
  std::vector<std::future<void>> futures;
  lastUpdate = std::chrono::system_clock::now();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    int cycleDuration = distr(eng);
    long time_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - lastUpdate).count();
    if (time_duration >= cycleDuration) {
      if (TrafficLight::GetCurrentPhase() == 0) {
        _current_phase = TrafficLightPhase::GREEN;
      } else {
        _current_phase = TrafficLightPhase::GREEN;
      }
      futures.emplace_back(
        std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send,
                   _msg_queue, _current_phase));
    }
    lastUpdate = std::chrono::system_clock::now();
  }

}

