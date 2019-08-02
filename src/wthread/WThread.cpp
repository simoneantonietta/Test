/*
 *-----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE: see module WThread.h file
 *-----------------------------------------------------------------------------
 */

#include <string.h>
#include "WThread.h"


/**
 * ctor
 */
WThread::WThread():
started(false),noattr(true)
{
//cout << "WThread::WThread()" << endl;
}

/**
 * ctor
 * @param detach true: detached type
 * @param size stack size
 */
WThread::WThread(bool detach, int size):
noattr(false)
{
cout << "Thread::Thread()" << endl;
int ret;
if ((ret = pthread_attr_init(&_attr)) != 0)
	{
	cout << strerror(ret) << endl;
	throw "Error";
	}
if (detach)
	{
	if ((ret = pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_DETACHED)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
if (size >= PTHREAD_STACK_MIN)
	{
	if ((ret = pthread_attr_setstacksize(&_attr, size)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
}

/**
 * dtor
 */
WThread::~WThread()
{
int ret;
//cout << "Thread::~Thread()" << endl;
if(!noattr)
	{
	if((ret = pthread_attr_destroy(&_attr)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
}

/**
 * get the thread id
 * @return thread idl
 */
unsigned int WThread::tid() const
{
return _id;
}

/**
 * start the execution of a thread
 * @param arg (Uses default argument: arg = NULL)
 */
void WThread::start(void *arg)
{
int ret;
if (!started)
	{
	started = true;
	this->arg = arg;
	/*
	 * Since pthread_create is a C library function, the 3rd
	 * argument is a global function that will be executed by
	 * the thread. In C++, we emulate the global function using
	 * the static member function that is called exec. The 4th
	 * argument is the actual argument passed to the function
	 * exec. Here we use this pointer, which is an instance of
	 * the Thread class.
	 */
	if ((ret = pthread_create(&_id, NULL, &WThread::exec, this)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
}

/**
 * to break a thread before its real end
 */
void WThread::finish()
{
pthread_detach(_id);
pthread_exit(0);
}

/**
 * Allow the thread to wait for the termination status
 */
void WThread::join()
{
pthread_join(_id, NULL);
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */

void *WThread::exec(void *thr)
{
reinterpret_cast<WThread *>(thr)->run();
}


#if 0
/**
 * example_thread.c, copyright 2001 Steve Gribble
 *
 * This program spawns off two threads, which use a condition
 * variable to "ping pong" control back and forth between
 * the threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

extern int errno;

pthread_mutex_t   mutex;
pthread_cond_t    condvar;
int               threads_left = 0;

void *do_pingpong(void *arg) {
  char *myname = (char *) arg;
  int   count = 0;

  fprintf(stdout, "\nThread \"%s\" has been born!\n\n", myname);
  threads_left++;

  // grab the mutex
  if (pthread_mutex_lock(&mutex) != 0) {
    perror("mutex lock failed (!!):");
    exit(-1);
  }

  // spin 5 times, pingponging
  while(count++ < 5) {
    fprintf(stdout, "%s says hello!\n", myname);
    sleep(1);

    // If the other thread is waiting, wake it up.  Note
    // that conditional variables, unlike semaphores, do NOT
    // have history.  So, if there is no other thread waiting
    // on this condition variable, the signal will be lost.
    pthread_cond_signal(&condvar);

    // Go to sleep until the other thread wakes me up.  By
    // waiting on this condition variable, I automatically
    // yield the mutex "mutex" until I get woken up.
    pthread_cond_wait(&condvar, &mutex);
  }

  if (pthread_mutex_unlock(&mutex) != 0) {
    perror("mutex unlock failed (!!):");
    exit(-1);
  }

  fprintf(stdout, "Thread %s is dying!\n", myname);
  threads_left--;
  pthread_cond_signal(&condvar);
  return NULL;
}

int main(int argc, char **argv) {

  char      t1name[] = "thread1", t2name[] = "thread2";
  pthread_t t1, t2;

  // initialize the mutex and condition variable
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&condvar, NULL);

  // create two threads, have them both enter the same
  // function "do_pingpong", but with different arguments.
  if (pthread_create(&t1, NULL, do_pingpong, (void *) t1name) != 0) {
    perror("t1 creation failed:");
    exit(-1);
  }
  if (pthread_create(&t2, NULL, do_pingpong, (void *) t2name) != 0) {
    perror("t2 creation failed:");
    exit(-1);
  }

  // by calling "pthread_detach", when the specified thread returns from
  // its entry function, the thread will be destroyed.  If we
  // don't call detach, then the memory associated with the thread
  // won't be cleaned up until somebody "joins" with the thread
  // by calling pthread_wait().
  pthread_detach(t1);
  pthread_detach(t2);

  // There are now three threads in the system; the "main" thread,
  // plus the two threads that were created above.  Now, we'll make
  // the "main" thread sleep forever while the other two threads run
  // amok
  while(1) {
    sleep(2);
    if (threads_left == 0)
       exit(0);
  }

  // should never get here
  return(0);
}
#endif

