FTags Design
============

Process Types
-------------

There are four process types:
   * the master server
      - one per project
      - started by end-user
      - long running
      - dispatches work to indexers, maintains the database, responds to
      requests from client
   * the indexer worker
      - several per master server
      - started by master server
      - medium running; started on demand by master server
      - receives batches of files to be processed; returns index fragments to
      master server
   * the file monitor
      - one per master server
      - started by master server
      - long running
   * the client
      - one global instance; can connect to multiple projects
      - started by end-user
      - short-lived; at the end of the request the process completes


Interprocess Communication
--------------------------

The interprocess communication is implemented via [ZeroMQ](http://zeromq.org/).

The vocabulary understood by the server will be published, so additional
clients could be implemented as libraries in programming languages with
ZeroMQ support, avoiding the overhead of starting a client.


