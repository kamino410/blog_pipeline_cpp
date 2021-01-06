#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

constexpr size_t TASK_A_COST = 500;
constexpr size_t TASK_B_COST = 100;
constexpr size_t TASK_C_COST = 100;
constexpr size_t TASK_D_COST = 1000;

struct MyTimestamp {
  int id = -1;
  std::chrono::system_clock::time_point checkpoint1;
  std::chrono::system_clock::time_point checkpoint2;
  std::chrono::system_clock::time_point checkpoint3;
  std::chrono::system_clock::time_point checkpoint4;
};

int main() {
  MyTimestamp log_thread1[200];
  MyTimestamp log_thread2s[2][200];

  std::atomic<bool> flag_exit = false;

  // データ受け渡し用のバッファーを確保（2つで十分なはずだが念の為3つ）
  std::queue<int*> buffers_1to2_used;
  std::queue<int*> buffers_1to2_free;
  int data_1to2_buf[3];
  for (size_t i = 0; i < 3; i++) { buffers_1to2_free.push(data_1to2_buf + i); }
  std::mutex mutex_buffers_1to2_used;
  std::mutex mutex_buffers_1to2_free;
  std::condition_variable condition_1to2;

  std::chrono::system_clock::time_point tm_start = std::chrono::system_clock::now();

  std::thread thread1([&log_thread1, &flag_exit, &buffers_1to2_used, &buffers_1to2_free,
                       &mutex_buffers_1to2_used, &mutex_buffers_1to2_free, &condition_1to2] {
    int id = 1;
    for (size_t i = 0; i < 30; i++) {
      log_thread1[id].id = id;

      // =============== 処理A ===============
      log_thread1[id].checkpoint1 = std::chrono::system_clock::now();
      std::this_thread::sleep_for(std::chrono::microseconds(TASK_A_COST));
      log_thread1[id].checkpoint2 = std::chrono::system_clock::now();

      int* data;
      {
        // mutexを確保できるまで待つ
        std::lock_guard<std::mutex> lock(mutex_buffers_1to2_free);
        // 空いているバッファーを確保する
        data = buffers_1to2_free.front();
        buffers_1to2_free.pop();
      }

      // =============== 処理B ===============
      log_thread1[id].checkpoint3 = std::chrono::system_clock::now();
      // データを書き込む
      data[0] = id;
      std::this_thread::sleep_for(std::chrono::microseconds(TASK_B_COST));
      {
        // mutexを確保できるまで待つ
        std::lock_guard<std::mutex> lock(mutex_buffers_1to2_used);
        // バッファーを使用中キューに追加する
        buffers_1to2_used.push(data);
      }
      // 待機中のスレッド2の1つに通知する
      condition_1to2.notify_one();
      log_thread1[id].checkpoint4 = std::chrono::system_clock::now();

      id++;
    }

    // 終了フラグを立て、全てのスレッド2に通知する
    flag_exit = true;
    condition_1to2.notify_all();
  });

  std::vector<std::thread> thread2s;
  // スレッド2を2つ走らせる
  for (size_t th_id = 0; th_id < 2; th_id++) {
    thread2s.push_back(
        std::thread([th_id, &log_thread2s, &flag_exit, &buffers_1to2_used, &buffers_1to2_free,
                     &mutex_buffers_1to2_used, &mutex_buffers_1to2_free, &condition_1to2] {
          size_t log_id = 1;
          while (true) {
            int* data;
            {
              std::unique_lock<std::mutex> lock(mutex_buffers_1to2_used);
              // スレッド1から通知が来るまで待つ
              // 念の為、次のデータがある or 終了フラグが立っている を満たさないときは通知を無視
              condition_1to2.wait(lock, [&] { return !buffers_1to2_used.empty() || flag_exit; });
              if (flag_exit) break;

              // 使用中キューからバッファーを1つ取り出す
              data = buffers_1to2_used.front();
              buffers_1to2_used.pop();
            }

            // =============== 処理C ===============
            log_thread2s[th_id][log_id].checkpoint1 = std::chrono::system_clock::now();
            // データを受け取る
            log_thread2s[th_id][log_id].id = data[0];
            std::this_thread::sleep_for(std::chrono::microseconds(TASK_C_COST));
            {
              // mutexを確保できるまで待つ
              std::lock_guard<std::mutex> lock(mutex_buffers_1to2_free);
              // 使い終わったバッファーを未使用キューに追加
              buffers_1to2_free.push(data);
            }
            log_thread2s[th_id][log_id].checkpoint2 = std::chrono::system_clock::now();

            // =============== 処理D ===============
            std::this_thread::sleep_for(std::chrono::microseconds(TASK_D_COST));
            log_thread2s[th_id][log_id].checkpoint3 = std::chrono::system_clock::now();

            log_id++;
          }
        }));
  }

  // スレッドが終了するのを待つ
  thread1.join();
  for (size_t i = 0; i < 2; i++) { thread2s[i].join(); }

  // ログを出力
  std::cout << "<costs>" << std::endl;
  std::cout << TASK_A_COST << ", " << TASK_B_COST << ", " << TASK_C_COST << ", " << TASK_D_COST
            << std::endl;
  std::cout << "</costs>" << std::endl;

  std::cout << "<th1>" << std::endl;
  for (size_t i = 1; i < 15; i++) {
    using namespace std::chrono;
    std::cout << log_thread1[i].id << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint1 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint2 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint3 - tm_start).count() << ", ";
    std::cout << duration_cast<microseconds>(log_thread1[i].checkpoint4 - tm_start).count()
              << std::endl;
  }
  std::cout << "</th1>" << std::endl;

  for (size_t th_id = 0; th_id < 2; th_id++) {
    std::cout << "<th2-" << th_id + 1 << ">" << std::endl;
    for (size_t i = 1; i < 15; i++) {
      using namespace std::chrono;
      std::cout << log_thread2s[th_id][i].id << ", ";
      std::cout
          << duration_cast<microseconds>(log_thread2s[th_id][i].checkpoint1 - tm_start).count()
          << ", ";
      std::cout
          << duration_cast<microseconds>(log_thread2s[th_id][i].checkpoint2 - tm_start).count()
          << ", ";
      std::cout
          << duration_cast<microseconds>(log_thread2s[th_id][i].checkpoint3 - tm_start).count()
          << std::endl;
    }
    std::cout << "</th2-" << th_id + 1 << ">" << std::endl;
  }

  return 0;
}

