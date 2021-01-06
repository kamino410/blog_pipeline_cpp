#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

constexpr size_t TASK_A_COST = 1000;
constexpr size_t TASK_B_COST = 100;
constexpr size_t TASK_C_COST = 100;
constexpr size_t TASK_D_COST = 500;

struct MyTimestamp {
  int id = -1;
  std::chrono::system_clock::time_point checkpoint1;
  std::chrono::system_clock::time_point checkpoint2;
  std::chrono::system_clock::time_point checkpoint3;
  std::chrono::system_clock::time_point checkpoint4;
};

int main() {
  MyTimestamp log_thread1[200];
  MyTimestamp log_thread2[200];

  int data_1to2;
  std::mutex mutex_1to2;

  std::atomic<bool> flag_exit = false;
  std::atomic<bool> flag_update1to2 = false;

  std::chrono::system_clock::time_point tm_start = std::chrono::system_clock::now();

  std::thread thread1([&log_thread1, &flag_exit, &flag_update1to2, &mutex_1to2, &data_1to2] {
    int id = 1;
    for (size_t i = 0; i < 30; i++) {
      log_thread1[id].id = id;

      // =============== 処理A ===============
      log_thread1[id].checkpoint1 = std::chrono::system_clock::now();
      std::this_thread::sleep_for(std::chrono::microseconds(TASK_A_COST));
      log_thread1[id].checkpoint2 = std::chrono::system_clock::now();

      {
        // mutexを確保できるまで待つ
        std::lock_guard<std::mutex> lock(mutex_1to2);

        // =============== 処理B ===============
        log_thread1[id].checkpoint3 = std::chrono::system_clock::now();
        // データを書き込む
        data_1to2 = id;
        std::this_thread::sleep_for(std::chrono::microseconds(TASK_B_COST));
      }
      flag_update1to2 = true;
      log_thread1[id].checkpoint4 = std::chrono::system_clock::now();

      id++;
    }
    flag_exit = true;
  });
  std::thread thread2([&log_thread2, &flag_exit, &flag_update1to2, &mutex_1to2, &data_1to2] {
    int log_id = 1;
    while (!flag_exit) {
      // スレッド1から新しいデータが来るまで待つ
      while (!flag_update1to2) continue;
      flag_update1to2 = false;

      {
        // mutexを確保できるまで待つ
        std::lock_guard<std::mutex> lock(mutex_1to2);

        // =============== 処理C ===============
        log_thread2[log_id].checkpoint1 = std::chrono::system_clock::now();
        // データを受け取る
        log_thread2[log_id].id = data_1to2;
        std::this_thread::sleep_for(std::chrono::microseconds(TASK_C_COST));
      }
      log_thread2[log_id].checkpoint2 = std::chrono::system_clock::now();

      // =============== 処理D ===============
      std::this_thread::sleep_for(std::chrono::microseconds(TASK_D_COST));
      log_thread2[log_id].checkpoint3 = std::chrono::system_clock::now();

      log_id++;
    }
  });

  // スレッドが終了するのを待つ
  thread1.join();
  thread2.join();

  // ログを出力
  std::cout << "<costs>" << std::endl;
  std::cout << TASK_A_COST << ", "
            << TASK_B_COST << ", "
            << TASK_C_COST << ", "
            << TASK_D_COST << std::endl;
  std::cout << "</costs>" << std::endl;

  std::cout << "<th1>" << std::endl;
  for (size_t i = 1; i < 20; i++) {
    using namespace std::chrono;
    std::cout << log_thread1[i].id << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint1 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint2 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint3 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint4 - tm_start).count()
              << std::endl;
  }
  std::cout << "</th1>" << std::endl;

  std::cout << "<th2>" << std::endl;
  for (size_t i = 1; i < 20; i++) {
    using namespace std::chrono;
    std::cout << log_thread2[i].id << ", ";
    std::cout << duration_cast<microseconds>(log_thread2[i].checkpoint1 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread2[i].checkpoint2 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread2[i].checkpoint3 - tm_start).count()
              << std::endl;
  }
  std::cout << "</th2>" << std::endl;

  return 0;
}

