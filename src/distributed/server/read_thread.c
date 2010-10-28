#include "distributed/server/server.h"
#include "core/logging.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define LEARNER_KQUEUE

// ------------------------------------------
// includes
// ------------------------------------------
#ifdef LEARNER_EPOLL
  #include <sys/epoll.h>
#endif

#ifdef LEARNER_KQUEUE
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>
#endif

#ifdef LEARNER_POLL
  #include <poll.h>
#endif


// ------------------------------------------
// event system list additions
// ------------------------------------------
#ifdef LEARNER_KQUEUE
  void add_socket_to_queue(int queue, int socket, uint16_t extra_flags) {
    struct kevent change;
    memset(&change, 0, sizeof(struct kevent));
    EV_SET(&change, socket, EVFILT_READ, EV_ADD | extra_flags, 0, 0, NULL);
    if(kevent(queue, &change, 1, NULL, 0, NULL) == -1) {
      fatal_with_errno("Unable to add a socket to the kqueue");
    }
  }
#endif

#ifdef LEARNER_EPOLL
#endif

#ifdef LEARNER_POLL
#endif


// ------------------------------------------
// cleanup
// ------------------------------------------
// FIXME: we need to cleanup our open file descriptors here
void read_thread_cleanup(void *fdset) {
  #ifdef LEARNER_EPOLL
  #endif
  
  #ifdef LEARNER_KQUEUE
  #endif
  
  #ifdef LEARNER_POLL
  #endif  
}


// ------------------------------------------
// helpers
// ------------------------------------------
// add a client to the process queue
void add_client_to_process_queue(int socket) {
  int bytes = write(process_queue_writer, &socket, sizeof(int));
  if (bytes != sizeof(int) && shutting_down) {
    pthread_exit(NULL);
  } else if (bytes == 0) {
    note("Process queue has closed. Reader thread shutting down.");
    pthread_exit(NULL);
  } else if (bytes == -1) {
    fatal_with_errno("Unable to write client socket file descriptor to process queue");
  } else if (bytes != sizeof(int)) {
    fatal_with_format("Unable to write complete client socket file descriptor. Wrote: %i", bytes);
  }
}

// read & return a client from the read queue socket
int read_client_from_queue() {
  int bytes = 0, client_socket = 0;
  bytes = read(read_queue_reader, &client_socket, sizeof(int));
  
  if (bytes != sizeof(int) && shutting_down) {
    pthread_exit(NULL);
  } else if (bytes == 0) {
    note("Read queue has closed. Reader thread shutting down.");
    pthread_exit(NULL);
  } else if (bytes == -1) {
    fatal_with_errno("Read thread unable to read from read queue");
  } else if (bytes != sizeof(int)) {
    fatal("Unable to read full client socket file descriptor");
  }
  
  return client_socket;
}

// remove a client
void close_client_connection(int socket) {
  if (close(socket) == -1) {
    warn_with_errno("Unable to close client connection");
  }
}


// ------------------------------------------
// event loop
// ------------------------------------------
void *read_thread(void *param) {
  int error = 0;
  
  #ifdef LEARNER_KQUEUE
    int events = 0, client_socket = 0;
    int queue = kqueue();
    struct kevent change;
    struct kevent event;
    
    // initialise the kqueue
    if (queue == -1) {
      fatal_with_errno("Unable to create a new kqueue object");
    }
    
    // add the queue socket to the queue
    add_socket_to_queue(queue, read_queue_reader, 0);
    note("Read thread started");
    
    while(1) {
      // FIXME: for now, we only monitor one event change at a time
      memset(&event, 0, sizeof(struct kevent));
      
      // block in kevent until a socket is available for reading
      events = kevent(queue, NULL, 0, &event, 1, NULL);
      if (events == -1 || event.flags == EV_ERROR) {
        fatal_with_errno("kevent failed in read thread");
      }
      
      // the socket used for the queue is available to read
      if (event.ident == read_queue_reader) {
        if (event.flags == EV_EOF) {
          pthread_exit(NULL);
        } else if (event.data == sizeof(int)) {
          client_socket = read_client_from_queue();
          add_socket_to_queue(queue, client_socket, EV_ONESHOT);
        }
      } else {
        if (event.flags == EV_EOF) {
          close_client_connection(event.ident);
        } else {
          add_client_to_process_queue(event.ident);
        }
      }
    }
  #endif
  
  #ifdef LEARNER_EPOLL
  #endif
  
  #ifdef LEARNER_POLL
  #endif  
}
