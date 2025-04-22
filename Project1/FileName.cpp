#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <conio.h>
#include <windows.h>
#include <string>
using namespace std;

// �ִ� ������ �� ����
const int MAX_THREADS = 16;

// ���� ����
int n, freq, maxVal;                           // �Է� ����: ������ ����, ��, �ִ� ī��Ʈ ��
atomic<bool> terminateFlag(false);             // ���� ��ȣ
atomic<int> selectedCounter(0);                // ���õ� ī���� �ε���
atomic<int> counters[MAX_THREADS];             // ī���� �� ����
atomic<bool> counterStates[MAX_THREADS];       // ī���� ���� ���� (counting/paused)
mutex consoleMutex;                            // �ܼ� ��� ��ȣ

// ����� ���� �Ľ� (���� ����)
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

// ī���� ������ �Լ�
void counterThread(int id) {
    using clock = chrono::steady_clock;
    auto nextTick = clock::now();

    while (!terminateFlag.load()) {
        if (counterStates[id].load()) {
            int current = counters[id].load();
            counters[id].store((current + 1 > maxVal) ? 0 : current + 1);
            nextTick += chrono::milliseconds(1000) / freq;
            this_thread::sleep_until(nextTick);  // ��Ȯ�� �������� ���
        } else {
            this_thread::sleep_for(chrono::milliseconds(100));
            nextTick = clock::now(); // paused ���� �ٽ� ������ ����
        }
    }
}

void moveTo(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}


// UI ������ �Լ�
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

        {   // ȭ�� ���
            lock_guard<mutex> lock(consoleMutex);
            moveTo(0, 0); // 커서를 맨 위로 이동
            for (int i = 0; i < n; ++i) {
                cout << "counter" << i << " : " << counters[i].load() << " ("
                     << (counterStates[i].load() ? "counting" : "paused") << ")\n";
            }
            cout << "\ncurrent: counter" << selectedCounter.load()
                 << " (" << (counterStates[selectedCounter.load()].load() ? "counting" : "paused") << ")\n";
        }

        Sleep(100);
    }
}

int main(int argc, char* argv[]) {

    if (!parseArgs(argc, argv)) {
        cout << "Usage: n=1~16 freq>0 max>=0\n";
        return 1;
    }

    // ī���� �ʱ�ȭ
    for (int i = 0; i < n; ++i) {
        counters[i].store(0);
        counterStates[i].store(false);
    }

    // ī���� ������ ����
    thread threads[MAX_THREADS];
    for (int i = 0; i < n; ++i) {
        threads[i] = thread(counterThread, i);
    }

    // UI ������ ����
    thread ui(uiThread);

    // ������ ���� ���
    for (int i = 0; i < n; ++i) threads[i].join();
    ui.join();

    return 0;
}
