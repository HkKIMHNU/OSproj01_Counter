#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <conio.h>
#include <windows.h>
#include <string>
using namespace std;

// 최대 스레드 수 제한
const int MAX_THREADS = 16;

// 전역 변수
int n, freq, maxVal;                           // 입력 인자: 스레드 개수, 빈도, 최대 카운트 값
atomic<bool> terminateFlag(false);             // 종료 신호
atomic<int> selectedCounter(0);                // 선택된 카운터 인덱스
atomic<int> counters[MAX_THREADS];             // 카운터 값 저장
atomic<bool> counterStates[MAX_THREADS];       // 카운터 동작 상태 (counting/paused)
mutex consoleMutex;                            // 콘솔 출력 보호

// 명령행 인자 파싱 (순서 무관)
bool parseArgs(int argc, char* argv[]) {
    if (argc != 4) return false;
    for (int i = 1; i < 4; ++i) {
        string token(argv[i]);
        auto pos = token.find('=');
        if (pos == string::npos) return false;
        string key = token.substr(0, pos);
        int value = stoi(token.substr(pos + 1));

        if (key == "n" && value >= 1 && value <= MAX_THREADS)         n = value;
        else if (key == "freq" && value > 0)                          freq = value;
        else if (key == "max" && value >= 0)                          maxVal = value;
        else                                                            return false;
    }
    return true;
}

// 카운터 스레드 함수
void counterThread(int id) {
    using clock = chrono::steady_clock;
    auto nextTick = clock::now();

    while (!terminateFlag.load()) {
        if (counterStates[id].load()) {
            int current = counters[id].load();
            counters[id].store((current + 1 > maxVal) ? 0 : current + 1);
            nextTick += chrono::milliseconds(1000) / freq;
            this_thread::sleep_until(nextTick);  // 정확한 시점까지 대기
        } else {
            this_thread::sleep_for(chrono::milliseconds(100));
            nextTick = clock::now(); // paused 이후 다시 기준점 맞춤
        }
    }
}

// UI 스레드 함수
void uiThread() {
    while (!terminateFlag.load()) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 'q') terminateFlag.store(true);
            else if (key == 'n') selectedCounter.store((selectedCounter.load() + 1) % n);
            else if (key == ' ') {
                int sel = selectedCounter.load();
                bool curr = counterStates[sel].load();
                counterStates[sel].store(!curr);
            }
        }

        {   // 화면 출력
            lock_guard<mutex> lock(consoleMutex);
			system("cls"); 
            for (int i = 0; i < n; ++i) {
                cout << "counter" << i << " : " << counters[i].load() << " (" << (counterStates[i].load() ? "counting" : "paused") << ")\n";
            }
            cout << "\ncurrent: counter" << selectedCounter.load() << " (" << (counterStates[selectedCounter.load()].load() ? "counting" : "paused") << ")\n";
        }

        Sleep(100);
    }
}

int main(int argc, char* argv[]) {

    if (!parseArgs(argc, argv)) {
        cout << "Usage: n=1~16 freq>0 max>=0\n";
        return 1;
    }

    // 카운터 초기화
    for (int i = 0; i < n; ++i) {
        counters[i].store(0);
        counterStates[i].store(false);
    }

    // 카운터 스레드 생성
    thread threads[MAX_THREADS];
    for (int i = 0; i < n; ++i) {
        threads[i] = thread(counterThread, i);
    }

    // UI 스레드 생성
    thread ui(uiThread);

    // 스레드 종료 대기
    for (int i = 0; i < n; ++i) threads[i].join();
    ui.join();

    return 0;
}
