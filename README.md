Distributed Transaction Management System
===================
Created by: Johan Henriksson and Sebastian Valdivia

This repo contains a distributed transaction management system using 2 phase commit. It is composed of 3 programs:

 1. Transaction manager: tmanager.c
 2. Worker: tworker.c
 3. A program to issue instructions to workers: cmd.c

----------

Transaction Manager
-------------

Responsible for coordinating simultaneous transactions (4 max.) between a maximum of 9 worker nodes.

 - Manager handles the situation where workers crash during a transaction.

**To Run**
`./tmanager <port_num>`


Worker
-------------

 - Communicates with transaction manager and listens from cmd commands.
 - It stores information about 2 integer objects A and B and a string
   identifier in a log files. This is to simulate the effects of a transaction; however, each node has its own log file for the simplicity of this assignment. In a real system, the transaction manager would handle the transactions of a shared resource.
 - Have vector clocks to keep track of other workers' transactions.
 - Workers that work on the same transaction vote to pass or abort the transaction when one of the workers commit.
 - Workers handle crashes by rolling back to their previous state. If they were in the middle of a transaction they ask the transaction manager for the result.

**To Run**
`./tworker <port_number>`

Cmd
-------------

The command provides an interface to simulate the interactions between workers and the transaction manager.

**To Run**
`./cmd`

**Commands**
For usages type the command after running the program.

*To start a transaction:*

 - begin
	 - Begin a transaction in a specific worker
 - join
	 - Join a worker to an existing transaction

*When transaction has started:*

 - newa
	 - New value for A
 - newb
	 - New value for B
 - newid
	 - New value for string ID
 - delay
	 - Delay a workers voting response ( to potentially introduce timeout problems)
 - commit
	 - Commit a transaction with local changes
 - commitcrash
	 - Commit transaction changes and crash
 - abort
	 - Abort a the current transaction
 - abortcrash
	 - Abort a transaction and crash
 - voteabort
	 - Vote abort when voting to commit a transaction between participating workers

*Use anytime*

 - crash 
	 - Crash a specified worker


Example
-------------
Run in separate terminals:

    tmanager 8080
    tworker 1040
    tworker 1030
    cmd

Run in cmd (our program):

    begin localhost 1040 localhost 8080 10
    join localhost 1030 localhost 8080 10
    newa localhost 1040 5
    newb localhost 1030 5
    voteabort localhost 1030
    commit localhost 1040

Result:

    Transaction aborts
