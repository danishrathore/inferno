#ifndef EVR_THREADPOOL_H
#define EVR_THREADPOOL_H

#include <thread>
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <tuple>

/**
 * @brief The ThreadPool class launches a pre-defined number of threads and
 *        keeps them ready to execute jobs.
 *
 * Each job is composed by two functions (std::function<void()>):
 * the two functions are executed one after another, and the second one
 *  usually sends some sort of notification that the job has been done.
 *
 */
class ThreadPool
{
public:

    /**
     * @brief Initializes the pool and launches the threads.
     *
     * @param numThreads number of threads to launch.
     * @param maxJobsInQueue
     */
    ThreadPool(size_t numThreads, size_t maxJobsInQueue);


    /**
     * @brief Destructors
     *
     * Sends a "terminate" signal to the threads and waits for
     *  their termination.
     *
     * The threads will complete the currently running job
     *  before checking for the "terminate" flag.
     */
    virtual ~ThreadPool();


    /**
     * @brief Schedule a job for execution.
     *
     * The first available thread will pick up the job and run it.
     *
     * @param job             a function that executes the job. It is called
     *                         in the thread that pickec it up
     * @param notificationJob a function that usually sends a notification
     *                         that the job has been executed. it is executed
     *                         immediately after the first function in the same
     *                         thread that ran the first function
     */
    void executeJob(std::function<void()> job, std::function<void()> notificationJob);


private:
    /**
     * @brief Function executed in each worker thread.
     *
     * Runs until the termination flag m_bTerminate is set to true
     *  in the class destructor
     */
    void loop();


    /**
     * @brief Returns the next job scheduled for execution.
     *
     * The function blocks if the list of scheduled jobs is empty until
     *  a new job is scheduled or until m_bTerminate is set to true
     *  by the class destructor, in which case it throws Terminated.
     *
     * When a valid job is found it is removed from the queue and
     *  returned to the caller.
     *
     * @return the next job to execute
     */
    std::pair<std::function<void()>, std::function<void()> > getNextJob();


    /**
     * @brief Contains the running working threads (workers).
     */
    std::vector<std::unique_ptr<std::thread> > m_workers;


    /**
     * @brief Queue of jobs scheduled for execution and not yet executed.
     */
    std::list<std::pair<std::function<void()>, std::function<void()> > > m_jobs;


    /**
     * @brief Mutex used to access the queue of scheduled jobs (m_jobs).
     */
    std::mutex m_lockJobsList;


    /**
     * @brief Condition variable used to notify that a new job has been
     *        inserted in the queue (m_jobs).
     */
    std::condition_variable m_notifyJob;


    /**
     * @brief This flag is set to true by the class destructor to signal
     *         the worker threads that they have to terminate.
     */
    std::atomic<bool> m_bTerminate;


    /**
     * @brief This exception is thrown by getNextJob() when the flag
     *         m_bTerminate has been set.
     */
    class Terminated: public std::runtime_error
    {
    public:
        Terminated(const std::string& what): std::runtime_error(what) {}
    };

};


/*
 * Constructor
 *************/
ThreadPool::ThreadPool(size_t numThreads, size_t maxJobsInQueue):
    m_workers(numThreads), m_bTerminate(false)
{
    for(std::unique_ptr<std::thread>& worker: m_workers)
    {
        worker.reset(new std::thread(&ThreadPool::loop, this));
    }
}


/*
 * Destructor
 ************/
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lockList(m_lockJobsList);
        m_bTerminate = true;
        m_notifyJob.notify_all();
    }

    for(std::unique_ptr<std::thread>& worker: m_workers)
    {
        worker->join();
    }
}


/*
 * Schedule a job
 ****************/
void ThreadPool::executeJob(std::function<void()> job, std::function<void()> notificationJob)
{
    std::unique_lock<std::mutex> lockList(m_lockJobsList);
    m_jobs.push_back(std::pair<std::function<void()>, std::function<void()> >(job, notificationJob));
    m_notifyJob.notify_one();
}


/*
 * Retrieve the next job to execute
 **********************************/
std::pair<std::function<void()>, std::function<void()> > ThreadPool::getNextJob()
{
    std::unique_lock<std::mutex> lockList(m_lockJobsList);

    while(!m_bTerminate)
    {
        if(!m_jobs.empty())
        {
            std::pair<std::function<void()>, std::function<void()> > job = m_jobs.front();
            m_jobs.pop_front();
            return job;
        }

        m_notifyJob.wait(lockList);
    }

    throw Terminated("Thread terminated");
}


/*
 * Function executed by each worker
 **********************************/
void ThreadPool::loop()
{
    try
    {
        for(;;)
        {
            std::pair<std::function<void()>, std::function<void()> > job = getNextJob();
            job.first();
            job.second();
        }
    }
    catch(Terminated& e)
    {
    }
}


#endif // EVR_THREADPOOL_H
