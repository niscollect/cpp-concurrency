#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <bits/stdc++.h>
using namespace std;


struct lockBased_threadSafe_Stack
{
    //* keep the data protected; data can’t be accessed outside the protection of the mutex
private:
    stack<int> thread_safe_stack;

    mutex mtx; //* mutex should be private, because we don't want to allow external code to lock the mutex and access the stack without proper synchronization.

public:
    void threadSafePush(int element)
    {
        // raii
        // {
            lock_guard<mutex> lk(mtx);
            thread_safe_stack.push(element);

            //! AVOID writing to cout inside the lock (see rzns below)
            // cout << "Pushed: " << element << endl;
        // }
    }
    bool threadSafePop(int &element)
    {
        // raii //* but lock_guard already handles that inherently
        // {
            lock_guard<mutex> lk(mtx);
            if(thread_safe_stack.empty())
            {
                return false;
            }
            element = thread_safe_stack.top();
            thread_safe_stack.pop();

            //! AVOID writing to cout inside the lock, because it can cause contention and slow down the performance of the stack. (coz cout is actually slow and can block other threads from accessing the stack while it's being written to). Instead, we can write to cout outside the lock, after we've popped the element from the stack. This way, we can avoid contention and improve the performance of the stack.
            //* it'd be a problem when we have a lot of threads trying to access the stack concurrently, because they would all be blocked waiting for the cout to finish writing. This can lead to a bottleneck and slow down the performance of the stack. So, it's better to write to cout outside the lock, after we've popped the element from the stack.
            // cout << "Popped: " << element << endl;
            return true;
        // }
    }

    //* top returning a value is a snapshot: by the time you act on it, another thread may have popped it.
    // bool threadSafeTop(int &element)
    // {
    //     // raii
    //     {
    //     }
    // }

    //* empty shouldn't be a separate function
    //* it should be a part of the pop function, because if we check for empty and then pop, there is a chance that another thread can pop the element in between the empty check and the pop operation. So, we should combine the empty check and the pop operation into a single atomic operation to ensure thread safety.
    //* it's not wrong to have, though.
    // bool empty()
    // {
    // }



    // copy assignment operator and copy constructor are deleted by the mutex, so we can't copy the stack. We can only move it. Also, we don't want to allow copying of the stack, because it can lead to data races and undefined behavior. So, we delete the copy assignment operator and copy constructor to prevent copying of the stack.
    lockBased_threadSafe_Stack(const lockBased_threadSafe_Stack &) = delete;    // though mutex already does it
    lockBased_threadSafe_Stack &operator=(const lockBased_threadSafe_Stack &) = delete; // though mutex already does it
};






const int PUSHERS = 2, POPPERS = 2, PER_PUSHER = 10000;
const int TOTAL = PUSHERS * PER_PUSHER;

lockBased_threadSafe_Stack st;
atomic<int> poppedCount{0};
atomic<long long> poppedSum{0};

void pusher(int id)
{
    for (int i = 1; i <= PER_PUSHER; i++)
        st.threadSafePush(id * PER_PUSHER + i);
}

void popper()
{
    int x;
    while (poppedCount < TOTAL)
    {
        if (st.threadSafePop(x))
        {
            poppedSum += x;
            poppedCount++;
        }
    }
}

int main()
{
    long long expected = 0;
    for (int id = 0; id < PUSHERS; id++)
        for (int i = 1; i <= PER_PUSHER; i++)
            expected += id * PER_PUSHER + i;

    vector<thread> ts;
    for (int id = 0; id < PUSHERS; id++) ts.emplace_back(pusher, id);
    for (int i = 0; i < POPPERS; i++) ts.emplace_back(popper);
    for (auto &t : ts) t.join();

    cout << "count " << poppedCount << "/" << TOTAL
         << ", sum " << poppedSum << " expected " << expected << endl;
}